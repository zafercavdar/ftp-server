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
#include <signal.h>
#include <pthread.h>
#include <netdb.h>

#define BUFF_SIZE 256

int parse_command(char *);
int quit();
int pasv();
int retr(char *);
int nlst(char *);

int sockfd, newsockfd;
int pasvsockfd = -1;
int pasvnewsockfd = -1;
int pasv_called = 0;
char init_dir[BUFF_SIZE];

void terminate(int signum){
  close(sockfd);
  close(newsockfd);
  close(pasvsockfd);
  close(pasvnewsockfd);
}

int main(int argc, char **argv) {

    // This is the main program for the thread version of nc

    int port;
    struct sockaddr_in serv_addr, cli_addr;
    int clilen;
    char buffer[BUFF_SIZE];

    struct sigaction action;
    memset(&action, 0, sizeof(struct sigaction));
    action.sa_handler = terminate;
    sigaction(SIGTERM, &action, NULL);

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

    printf("info | server: opening socket\n");
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
      printf("error | server: opening socket\n");
      exit(-1);
    }

    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    serv_addr.sin_addr.s_addr = INADDR_ANY;

    printf("info | server: binding to port %d\n", port);
    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
      printf("error | server: binding\n");
      exit(-1);
    }

    while(1) {
      // Go to initial directory for each client.
      chdir(init_dir);

      printf("info | server: started listening\n");
      listen(sockfd, 5);

      printf("info | server: accepting connections\n");
      clilen = sizeof(cli_addr);
      newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
      if (newsockfd < 0) {
        printf("error | server: accept\n");
        continue;
      }

      printf("info | server: sending 220 FTP server ready.\n");
      if(send(newsockfd, "220 Service ready for new user.\n", 32, 0) < 0) {
        printf("error | server: sending 220\n");
        continue;
      }

      while (1) {
        printf("info | server: reading client message\n");
        bzero(buffer, BUFF_SIZE);
        if (recv(newsockfd, buffer, BUFF_SIZE - 1, 0) < 0 || strcmp(buffer, "") == 0) {
          printf("error | server: reading from socket\n");
          close(newsockfd);
          logout();
          break;
        }
        if (parse_command(buffer) == -1) { // QUIT returns -1
          break;
        }
      }
    }

    // printf("Printed %d directory entries\n", listFiles(1, "."));
    return 0;
}

int parse_command(char *str) {
  char *cmd;
  char *arg;
  char msg[BUFF_SIZE];

  // Extract cmd and it's arg if exists
  if ((cmd = strtok(str, " \r\n")) != NULL){
    printf("info | server: cmd -> %s\n", cmd);
    if ((arg = strtok(NULL, " \r\n")) != NULL){
      printf("info | server: arg -> %s\n", arg);
    }
  }

  if (strcasecmp(cmd, "USER") == 0)
    return user(newsockfd, arg);

  else if (strcasecmp(cmd, "PASS") == 0)
    return pass(newsockfd, arg);

  else if (strcasecmp(cmd, "QUIT") == 0)
    return quit();

  else {
    if (is_logged_in() == 1) {
      if (strcasecmp(cmd, "CWD") == 0)
        return cwd(newsockfd, arg);

      else if (strcasecmp(cmd, "CDUP") == 0)
        return cdup(newsockfd, init_dir);

      else if (strcasecmp(cmd, "TYPE") == 0)
        return type(newsockfd, arg);

      else if (strcasecmp(cmd, "MODE") == 0)
        return mode(newsockfd, arg);

      else if (strcasecmp(cmd, "STRU") == 0)
        return stru(newsockfd, arg);

      else if (strcasecmp(cmd, "RETR") == 0)
        return retr(arg);

      else if (strcasecmp(cmd, "PASV") == 0)
        return pasv();

      else if (strcasecmp(cmd, "NLST") == 0 || strcasecmp(cmd, "LIST") == 0)
        return nlst(arg);

      else {
        strcpy(msg, "502 Command not implemented.\n");
        fdsend(newsockfd, msg);
        return 1;
      }
    } else {
      strcpy(msg, "503 Bad sequence of commands. Login with USER first.\n");
      fdsend(newsockfd, msg);
      return 0;
    }
  }
}

int quit() {
  printf("info | server: called QUIT func\n");
  logout();
  char msg[] = "221 Good bye.Closing control connection.\n";
  fdsend(newsockfd, msg);
  pasv_called = 0;
  pasvnewsockfd = -1;
  close(newsockfd);
  return -1;
}

void *pasv_connection(void *pasvsockfd){
  struct sockaddr_in pasv_cli_addr;
  int pasvclilen;
  int *psfd = ((int *) pasvsockfd);
  char msg[BUFF_SIZE];

  listen(*psfd, 5);
  printf("info | server: (PASV) started listening\n");

  fd_set rfds;
  struct timeval tv;
  int retval;
  FD_ZERO(&rfds);
  FD_SET(*psfd, &rfds);

  /* Wait up to 20 seconds. */
  tv.tv_sec = 20;
  tv.tv_usec = 0;
  retval = select(*psfd + 1, &rfds, NULL, NULL, &tv);
  if (retval == -1){
    printf("error | server: (PASV) error on select\n");
  } else if (retval){
      if (FD_ISSET(*psfd, &rfds)){
        printf("info | server: (PASV) accepting connections\n");
        pasvclilen = sizeof(pasv_cli_addr);
        pasvnewsockfd = accept(*psfd, (struct sockaddr *) &pasv_cli_addr, &pasvclilen);
        if (pasvnewsockfd < 0) {
          printf("error | server: (PASV) accept\n");
        }
        printf("info | server: (PASV) connection is established\n");
      }
  } else {
    printf("error | server: (PASV) timeout on dataconnection\n");
    pasv_called = 0;
    close(pasvnewsockfd);
    close(*psfd);
    pasvnewsockfd = -500; // for timeout
    *psfd = -1;
    strcpy(msg, "500 Timeout on data conection\n");
    fdsend(newsockfd, msg);
  }
}

int pasv() {
  printf("info | server: called PASV func\n");
  struct sockaddr_in pasv_serv_addr;
  int pasv_port;
  struct sockaddr_in sa;
  int sa_len;
  char msg[BUFF_SIZE];
  char addr[BUFF_SIZE];
  int ip1, ip2, ip3, ip4;
  char *token;
  pthread_t pasv_thread;

  int i;
  struct hostent *he;
  struct in_addr **addr_list;
  char hostname[BUFF_SIZE];


  printf("info | server: (PASV) opening socket\n");
  pasvsockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (pasvsockfd < 0) {
    printf("error | server: (PASV) opening socket\n");
    exit(-1);
  }

  bzero((char *) &pasv_serv_addr, sizeof(pasv_serv_addr));
  pasv_serv_addr.sin_family = AF_INET;
  pasv_serv_addr.sin_port = 0;
  pasv_serv_addr.sin_addr.s_addr = INADDR_ANY;

  printf("info | server: (PASV) binding to random port\n");
  if (bind(pasvsockfd, (struct sockaddr *) &pasv_serv_addr, sizeof(pasv_serv_addr)) < 0) {
    printf("error | server: (PASV) binding\n");
    exit(-1);
  }

  sa_len = sizeof(sa);
  getsockname(pasvsockfd, (struct sockaddr *) &sa, &sa_len);
  pasv_port = (int) ntohs(sa.sin_port);

  gethostname(hostname, sizeof hostname);
  if ((he = gethostbyname(hostname)) == NULL) {  // get the host info
        herror("gethostbyname");
        return 2;
  }

  // print information about this host:
  printf("info | server: (PASV) official server name is: %s\n", he->h_name);
  addr_list = (struct in_addr **)he->h_addr_list;
  for(i = 0; addr_list[i] != NULL; i++) {
      strcpy(addr, inet_ntoa(*addr_list[i]));
  }

  printf("info | server: (PASV) bound to port %d\n", pasv_port);
  //strcpy(addr, inet_ntoa(sa.sin_addr));
  printf("info | server: (PASV) bound to address: %s\n", addr);

  token = strtok(addr, ".\r\n");
  if (token != NULL) {
    ip1 = atoi(token);
    token = strtok(NULL, ".\r\n");
  }
  if (token != NULL) {
    ip2 = atoi(token);
    token = strtok(NULL, ".\r\n");
  }
  if (token != NULL) {
    ip3 = atoi(token);
    token = strtok(NULL, ".\r\n");
  }
  if (token != NULL) {
    ip4 = atoi(token);
  }

  sprintf(msg, "227 Entering Passive Mode (%d,%d,%d,%d,%d,%d).\n", ip1, ip2, ip3, ip4, pasv_port / 256, pasv_port % 256);
  fdsend(newsockfd, msg);

  pasv_called = 1;

  if(pthread_create(&pasv_thread, NULL, pasv_connection, (void *) &pasvsockfd)) {
    printf("error | server: (PASV) creating thread\n");
    exit(-1);
  }

  return 0;
}

int nlst(char *path) {
  printf("info | server: called NLST func\n");
  char dir[BUFF_SIZE];
  char msg[BUFF_SIZE];

  if (path != NULL){
    strcpy(msg, "501 NLST doesn't support argument.\n");
    fdsend(newsockfd, msg);
    return 0;
  }

  if (pasv_called == 0){
    strcpy(msg, "425 Data connection is not open. Use PASV first.\n");
    fdsend(newsockfd, msg);
  } else {
    while(pasvnewsockfd == -1); // wait until client is connected to pasv port.

    if (pasvnewsockfd < 0){
      printf("error | server: NLST received timeout on data connection. returning ..\n");
      pasvnewsockfd = -1;
      return 0;
    }

    bzero(msg, sizeof msg);
    strcpy(msg, "150 File status okay; about to open data connection. Here comes the directory listing.\n");
    fdsend(newsockfd, msg);

    getcwd(dir, BUFF_SIZE);
    listFiles(pasvnewsockfd, dir);

    pasv_called = 0;
    close(pasvnewsockfd);
    close(pasvsockfd);
    pasvnewsockfd = -1;
    pasvsockfd = -1;

    bzero(msg, sizeof msg);
    strcpy(msg, "226 Directory send OK. Closing data connection. Requested file action successful.\n");
    fdsend(newsockfd, msg);
  }

  return 0;
}


int retr(char *fname) {
  printf("info | server: called RETR func\n");
  char msg[BUFF_SIZE];
  int bytes_read;
  FILE *fs;
  int fs_block_size;
  char sdbuf[BUFF_SIZE * 2];
  int fs_total;

  if (pasv_called == 0){
    strcpy(msg, "425 Data connection is not open. Use PASV first.\n");
    fdsend(newsockfd, msg);
  } else{
    while(pasvnewsockfd == -1); // wait until client is connected to pasv port.

    if (pasvnewsockfd < 0){
      printf("error | server: RETR received timeout on data connection. returning ..\n");
      pasvnewsockfd = -1;
      return 0;
    }

    printf("info | server: (PASV) connection is established.\n");
    if (fname == NULL){
      bzero(msg, sizeof msg);
      strcpy(msg, "550 Requested action not taken. Failed to open file.\n");
      fdsend(newsockfd, msg);
    } else{
      if (access(fname, R_OK) != -1){
        // Read and send data
        fs = fopen(fname, "r");
        fs_total = fsize(fs);

        bzero(msg, sizeof msg);
        sprintf(msg, "150 File status okay; about to open data connection. Opening BINARY mode data connection for %s (%d bytes).\n", fname, fs_total);
        fdsend(newsockfd, msg);

        bzero(sdbuf, BUFF_SIZE * 2);
        while((fs_block_size = fread(sdbuf, sizeof(char), BUFF_SIZE * 2, fs)) > 0){
          printf("info | server: (PASV) fs_block_size: %d\n", fs_block_size);
          if (write(pasvnewsockfd, sdbuf, fs_block_size) < 0){
            printf("error | server: (PASV) sending data\n");
          }
          bzero(sdbuf, BUFF_SIZE * 2);
        }
        fclose(fs);

        pasv_called = 0;
        close(pasvnewsockfd);
        close(pasvsockfd);
        pasvnewsockfd = -1;
        pasvsockfd = -1;

        bzero(msg, sizeof msg);
        strcpy(msg, "226 Closing data connection. Transfer complete.\n");
        fdsend(newsockfd, msg);
      } else{
        bzero(msg, sizeof msg);
        strcpy(msg, "550 Requested action not taken. Failed to open file.\n");
        fdsend(newsockfd, msg);
      }
    }
  }
  return 0;
}
