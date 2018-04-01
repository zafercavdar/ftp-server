#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

#include "dir.h"
#include "usage.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BUFF_SIZE 256

int parse_command(char *);
int user(char *);
int pass(char *);
int quit();
int cwd(char *);
int cdup(char *);
int type(char *);
int mode(char *);
int stru(char *);
int retr(char *);
int pasv();
int nlst(char *);


void val_check(int val, char *msg) {
  if (val < 0) {
      printf("Error on %s\n", msg);
      exit(-1);
  }
}

int newsockfd;
int logged_in = 0;

// Here is an example of how to use the above function. It also shows
// one how to get the arguments passed on the command line.
int main(int argc, char **argv) {

    // This is some sample code feel free to delete it
    // This is the main program for the thread version of nc

    int val;
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

    printf("Opening socket ...\n");
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    val_check(sockfd, "opening socket");

    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    serv_addr.sin_addr.s_addr = INADDR_ANY;

    printf("Binding to port %d\n", port);
    val = bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr));
    val_check(val, "on binding");

    while(1) {
      printf("Started listening\n");
      listen(sockfd, 5);

      printf("Accepting connections\n");
      clilen = sizeof(cli_addr);
      newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
      if (newsockfd < 0) {
        printf("Error on accept\n");
        continue;
      }

      logged_in = 0;

      printf("Sending 220 Message\n");
      if(send(newsockfd, "220 FTP server ready.\n", 22, 0) < 0) {
        printf("Error on sending 220\n");
        continue;
      }

      while (1) {
        printf("Reading from socket\n");
        bzero(buffer, BUFF_SIZE);
        val = recv(newsockfd, buffer, BUFF_SIZE - 1, 0);
        if (val < 0 || strcmp(buffer, "") == 0) {
          printf("Error while reading from socket\n");
          break;
        }
        printf("Message: %s\n", buffer);
        val = parse_command(buffer);
        if (val == -1) { // QUIT returns -1
          close(newsockfd);
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
    return user(argument);

  else if (strcasecmp(command, "PASS") == 0)
    return pass(argument);

  else if (strcasecmp(command, "QUIT") == 0)
    return quit();

  else if (strcasecmp(command, "CWD") == 0)
    return cwd(argument);

  else if (strcasecmp(command, "CDUP") == 0)
    return cdup(argument);

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

}

// TODO: FUNCTION DEFINITION
//
int user(char *username) {
  printf("USER command is called\n");
  if (logged_in == 1) {
    send(newsockfd, "530 Can't change user when logged in.\n", 39, 0);
  } else {
    if (strcmp(username, "cs317") == 0) {
      logged_in = 1;
      send(newsockfd, "230 Login successful.\n", 22, 0);
    } else {
      send(newsockfd, "530 This server is cs317 only.\n", 31, 0);
    }
  }
  return 0;
}

// TODO: FUNCTION DEFINITION
//
int pass(char * pw) {
  printf("PASS command is called\n");
  if (logged_in == 0) {
    send(newsockfd, "503 Login with USER first.\n", 27, 0);
  } else {
    send(newsockfd, "230 Already logged in.\n", 23, 0);
  }
  return 0;
}

// TODO: FUNCTION DEFINITION
//
int quit() {
  printf("QUIT command is called\n");
  send(newsockfd, "221 Goodbye.\n", 14, 0);
  return -1;
}

// TODO: FUNCTION DEFINITION
//
int cwd(char *dir) {
  printf("CWD command is called\n");


}

// TODO: FUNCTION DEFINITION
//
int cdup(char *dir) {
  printf("CDUP command is called\n");


}

// TODO: FUNCTION DEFINITION
//
int type(char *type) {
  printf("TYPE command is called\n");


}

// TODO: FUNCTION DEFINITION
//
int mode(char *mode) {
  printf("MODE command is called\n");


}

// TODO: FUNCTION DEFINITION
//
int stru(char *fst) {
  printf("STRU command is called\n");


}

// TODO: FUNCTION DEFINITION
//
int retr(char *x) {
  printf("RETR command is called\n");


}

// TODO: FUNCTION DEFINITION
//
int pasv() {
  printf("PASV command is called\n");


}

// TODO: FUNCTION DEFINITION
//
int nlst(char *x) {
  printf("NLST command is called\n");


}
