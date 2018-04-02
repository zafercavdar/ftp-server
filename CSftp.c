#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>

#include "dir.h"
#include "login.h"
#include "usage.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#define BUFF_SIZE 256

int parse_command(char *);
int quit();
int cwd(char *);
int cdup();
int type(char *);
int mode(char *);
int stru(char *);
int retr(char *);
int pasv();
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
  command = strtok(str, " \n");
  if (command != NULL) {
    argument = strtok(NULL, " \n");
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
        return cwd(argument);

      else if (strcasecmp(command, "CDUP") == 0)
        return cdup();

      else if (strcasecmp(command, "TYPE") == 0)
        return type(argument);

      else if (strcasecmp(command, "MODE") == 0)
        return mode(argument);

      else if (strcasecmp(command, "STRU") == 0)
        return stru(argument);

      else if (strcasecmp(command, "RETR") == 0)
        return retr(argument);

      else if (strcasecmp(command, "PASV") == 0)
        return pasv();

      else if (strcasecmp(command, "NLST") == 0)
        return nlst(argument);

      else {
        send(newsockfd, "500 Syntax error, command unrecognized and the requested action did not take place.\n", 85, 0);
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

int cwd(char *dir) {
  printf("CWD command is called\n");
  char *f;
  char cpdir[BUFF_SIZE];

  if (dir == NULL){
    send(newsockfd, "550 Failed to change directory.\n", 33, 0);
  } else {
    strcpy(cpdir, dir);
    f = strtok(dir, "/\n");
    printf("f: %s\n", f);
    if (strcmp(f, "..") == 0 || strcmp(f, ".") == 0) {
      send(newsockfd, "550 Directory cannot start with ../ or ./\n", 43, 0);
      return 0;
    }

    f = strtok(NULL, "/\n");

    while(f != NULL){
      printf("f: %s\n", f);
      if (strcmp(f, "..") == 0) {
        send(newsockfd, "550 Directory cannot contain ../\n", 33, 0);
        return 0;
      }
      f = strtok(NULL, "/\n");
    }

    if (chdir(cpdir) == 0){
      send(newsockfd, "250 Directory successfully changed.\n", 37, 0);
    } else {
      send(newsockfd, "550 Failed to change directory.\n", 33, 0);
    }
  }
  return 0;
}

int cdup() {
  printf("CDUP command is called\n");
  char current_dir[BUFF_SIZE];

  getcwd(current_dir, BUFF_SIZE);
  if (strcmp(current_dir, init_dir) == 0){
    send(newsockfd, "550 Not permitted to access parent of root directory.\n", 55, 0);
  } else {
    if (chdir("..") == 0){
      send(newsockfd, "250 Directory successfully changed.\n", 37, 0);
    } else {
      send(newsockfd, "550 Failed to change directory.\n", 33, 0);
    }
  }
  return 0;
}

int type(char *rept) {
  printf("TYPE command is called\n");
  if (rept == NULL) {
    send(newsockfd, "500 Unrecognised TYPE command.\n", 32, 0);
  }
  else if (strcmp(rept, "I") == 0) {
    send(newsockfd, "200 Switching to Binary mode.\n", 31, 0);
  } else if (strcmp(rept, "A") == 0){
    send(newsockfd, "200 Switching to ASCII mode.\n", 30, 0);
  } else {
    send(newsockfd, "500 Unrecognised TYPE command.\n", 32, 0);
  }
  return 0;
}

int mode(char *transm) {
  printf("MODE command is called\n");
  if (transm == NULL){
    send(newsockfd, "504 Bad MODE command.\n", 23, 0);
  } else if (strcmp(transm, "S") == 0){
    send(newsockfd, "200 Mode set to S.\n", 20, 0);
  } else if (strcmp(transm, "B") == 0){
    send(newsockfd, "504 MODE Block is not supported.\n", 34, 0);
  } else if (strcmp(transm, "C") == 0){
    send(newsockfd, "504 MODE Compressed is not supported.\n", 39, 0);
  } else {
    send(newsockfd, "504 Bad MODE command.\n", 23, 0);
  }
  return 0;
}

int stru(char *filestrt) {
  printf("STRU command is called\n");
  if (filestrt == NULL){
    send(newsockfd, "504 Bad STRU command.\n", 23, 0);
  } else if (strcmp(filestrt, "F") == 0){ // FILE
    send(newsockfd, "200 Structure set to F.\n", 25, 0);
  } else if (strcmp(filestrt, "R") == 0){ // RECORD
    send(newsockfd, "504 STRU Record is not supported.\n", 35, 0);
  } else if (strcmp(filestrt, "P") == 0){ // PAGE
    send(newsockfd, "504 STRU Page is not supported.\n", 33, 0);
  } else {
    send(newsockfd, "504 Bad STRU command.\n", 23, 0);
  }
  return 0;
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

int nlst(char *x) {
  printf("NLST command is called\n");
  char dir[BUFF_SIZE];

  if (pasv_called == 0){
    send(newsockfd, "425 Use PASV first.\n", 21, 0);
  } else {
    while(pasvnewsockfd == -1); // wait until client is connected to pasv port.
    printf("PASV connection is established.\n");

    if (x == NULL){
      getcwd(dir, BUFF_SIZE);
    } else {
      strcpy(dir, x);
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


int retr(char *x) {
  printf("RETR command is called\n");

  return 0;
}
