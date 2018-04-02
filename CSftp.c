#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>

#include "dir.h"
#include "login.h"
#include "usage.h"
#include "simple.h"
#include "helpers.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#define BUFF_SIZE 256

int parse_command(char *);
int quit();
int pasv();
int retr(char *);
int nlst(char *);

int newsockfd;
int pasvnewsockfd = -1;
int pasv_called = 0;
char init_dir[BUFF_SIZE];

int main(int argc, char **argv) {

    // This is the main program for the thread version of nc

    int port;
    struct sockaddr_in serv_addr, cli_addr;
    int sockfd, clilen;
    char buffer[BUFF_SIZE];

    // Check the command line arguments
    if (argc != 2) {
      usage(argv[0]);
      return -1;
    }

    port = atoi(argv[1]);

    if (port < 1024 || port > 65535) {
      usage(argv[0]);
      return -1;
    }

    // Save the initial working directory
    getcwd(init_dir, BUFF_SIZE);

    printf("Opening socket ...\n");
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
      printf("Error on opening socket.\n");
      exit(-1);
    }

    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    serv_addr.sin_addr.s_addr = INADDR_ANY;

    printf("Binding to port %d\n", port);
    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
      printf("Error on binding.\n");
      exit(-1);
    }

    while(1) {
      // Go to initial directory for each client.
      chdir(init_dir);

      printf("Started listening\n");
      listen(sockfd, 5);

      printf("Accepting connections\n");
      clilen = sizeof(cli_addr);
      newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
      if (newsockfd < 0) {
        printf("Error on accept\n");
        continue;
      }

      printf("Sending 220 Message\n");
      if(send(newsockfd, "220 FTP server ready.\n", 22, 0) < 0) {
        printf("Error on sending 220\n");
        continue;
      }

      while (1) {
        printf("Reading from socket\n");
        bzero(buffer, BUFF_SIZE);
        if (recv(newsockfd, buffer, BUFF_SIZE - 1, 0) < 0 || strcmp(buffer, "") == 0) {
          printf("Error while reading from socket\n");
          close(newsockfd);
          logout();
          break;
        }
        printf("Message: %s\n", buffer);
        if (parse_command(buffer) == -1) { // QUIT returns -1
          break;
        }
      }
    }

    // This is how to call the function in dir.c to get a listing of a directory.
    // It requires a file descriptor, so in your code you would pass in the file descriptor
    // returned for the ftp server's data connection
    // printf("Printed %d directory entries\n", listFiles(1, "."));
    // return 0;
}

int parse_command(char *str) {
  char *command;
  char *argument;

  // Extract command and it's argument if exists
  command = strtok(str, " \r\n");
  if (command != NULL) {
    argument = strtok(NULL, " \r\n");
  }

  if (strcasecmp(command, "USER") == 0)
    return user(newsockfd, argument);

  else if (strcasecmp(command, "PASS") == 0)
    return pass(newsockfd, argument);

  else if (strcasecmp(command, "QUIT") == 0)
    return quit();

  else {
    if (is_logged_in() == 1) {
      if (strcasecmp(command, "CWD") == 0)
        return cwd(newsockfd, argument);

      else if (strcasecmp(command, "CDUP") == 0)
        return cdup(newsockfd, init_dir);

      else if (strcasecmp(command, "TYPE") == 0)
        return type(newsockfd, argument);

      else if (strcasecmp(command, "MODE") == 0)
        return mode(newsockfd, argument);

      else if (strcasecmp(command, "STRU") == 0)
        return stru(newsockfd, argument);

      else if (strcasecmp(command, "RETR") == 0)
        return retr(argument);

      else if (strcasecmp(command, "PASV") == 0)
        return pasv();

      else if (strcasecmp(command, "NLST") == 0)
        return nlst(argument);

      else {
        send(newsockfd, "502 Command unrecognized and the requested action did not take place.\n", 70, 0);
        return 1;
      }
    } else {
      send(newsockfd, "503 Login with USER first.\n", 27, 0);
      return 0;
    }
  }
}

int quit() {
  printf("QUIT command is called\n");
  logout();
  send(newsockfd, "221 Goodbye.\n", 14, 0);
  pasv_called = 0;
  pasvnewsockfd = -1;
  close(newsockfd);
  return -1;
}

void *pasv_connection(int pasvsockfd){
  struct sockaddr_in pasv_cli_addr;
  int pasvclilen;
  printf("(PASV) Started listening\n");
  listen(pasvsockfd, 5);

  printf("(PASV) Accepting connections\n");
  pasvclilen = sizeof(pasv_cli_addr);
  pasvnewsockfd = accept(pasvsockfd, (struct sockaddr *) &pasv_cli_addr, &pasvclilen);
  if (pasvnewsockfd < 0) {
    printf("(PASV) Error on accept\n");
  }
}

int pasv() {
  printf("PASV command is called\n");
  struct sockaddr_in pasv_serv_addr;
  int pasvsockfd;
  int pasv_port;
  struct sockaddr_in sa;
  int sa_len;
  char msg[BUFF_SIZE];
  pthread_t pasv_thread;

  printf("(PASV) Opening socket ...\n");
  pasvsockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (pasvsockfd < 0) {
    printf("(PASV) Error on opening socket.\n");
    exit(-1);
  }

  bzero((char *) &pasv_serv_addr, sizeof(pasv_serv_addr));
  pasv_serv_addr.sin_family = AF_INET;
  pasv_serv_addr.sin_port = 0;
  pasv_serv_addr.sin_addr.s_addr = INADDR_ANY;

  printf("(PASV) Binding to random port.\n");
  if (bind(pasvsockfd, (struct sockaddr *) &pasv_serv_addr, sizeof(pasv_serv_addr)) < 0) {
    printf("(PASV) Error on binding.\n");
    exit(-1);
  }

  sa_len = sizeof(sa);
  getsockname(pasvsockfd, &sa, &sa_len);
  pasv_port = (int) ntohs(sa.sin_port);
  printf("(PASV) Bound to port %d\n", pasv_port);
  // printf("Addr: %s\n", inet_ntoa(sa.sin_addr));

  sprintf(msg, "227 Entering Passive Mode (0,0,0,0,%d,%d).\n", pasv_port / 256, pasv_port % 256);
  send(newsockfd, msg, strlen(msg), 0);

  pasv_called = 1;

  if(pthread_create(&pasv_thread, NULL, pasv_connection, pasvsockfd)) {
    printf("(PASV) Error on creating thread\n");
    exit(-1);
  }

  return 0;
}

int nlst(char *path) {
  printf("NLST command is called\n");
  char dir[BUFF_SIZE];

  if (pasv_called == 0){
    send(newsockfd, "425 Use PASV first.\n", 21, 0);
  } else {
    while(pasvnewsockfd == -1); // wait until client is connected to pasv port.
    printf("PASV connection is established.\n");

    if (path == NULL){
      getcwd(dir, BUFF_SIZE);
    } else {
      strcpy(dir, path);
    }

    send(newsockfd, "150 Here comes the directory listing.\n", 39, 0);
    listFiles(pasvnewsockfd, dir);
    send(newsockfd, "226 Directory send OK.\n", 24, 0);
    pasv_called = 0;
    close(pasvnewsockfd);
    pasvnewsockfd = -1;
  }

  return 0;
}


int retr(char *fname) {
  printf("RETR command is called\n");
  char msg[BUFF_SIZE];
  int bytes_read;
  FILE *fs;
  int fs_block_size;
  char sdbuf[BUFF_SIZE * 2];
  int fs_total;

  if (pasv_called == 0){
    send(newsockfd, "425 Use PASV first.\n", 21, 0);
  } else{
    while(pasvnewsockfd == -1); // wait until client is connected to pasv port.
    printf("PASV connection is established.\n");
    if (fname == NULL){
      send(newsockfd, "550 Failed to open file.\n", 26, 0);
    } else{
      if (access(fname, R_OK) != -1){
        // Read and send data
        fs = fopen(fname, "r");
        fs_total = fsize(fs);
        sprintf(msg, "150 Opening BINARY mode data connection for %s (%d bytes).\n", fname, fs_total);
        send(newsockfd, msg, strlen(msg), 0);

        bzero(sdbuf, BUFF_SIZE * 2);
        while((fs_block_size = fread(sdbuf, sizeof(char), BUFF_SIZE * 2, fs)) > 0){
          printf("fs_block_size: %d\n", fs_block_size);
          if (send(pasvnewsockfd, sdbuf, fs_block_size, 0) < 0){
            printf("(PASV) Error while sending data.\n");
          }
          bzero(sdbuf, BUFF_SIZE * 2);
        }
        close(fs);

        send(newsockfd, "226 Transfer complete.\n", 24, 0);
        pasv_called = 0;
        close(pasvnewsockfd);
        pasvnewsockfd = -1;
      } else{
        send(newsockfd, "550 Failed to open file.\n", 26, 0);
      }
    }
  }
  return 0;
}
