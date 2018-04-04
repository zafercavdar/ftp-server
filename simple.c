#include "simple.h"
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define BUFF_SIZE 256

int cwd(int fd, char *dir) {
  printf("CWD command is called\n");
  char *f;
  char cpdir[BUFF_SIZE];

  if (dir == NULL){
    send(fd, "550 Failed to change directory.\n", 33, 0);
  } else {
    strcpy(cpdir, dir);
    f = strtok(dir, "/\r\n");
    printf("f: %s\n", f);
    if (strcmp(f, "..") == 0 || strcmp(f, ".") == 0) {
      send(fd, "550 Directory cannot start with ../ or ./\n", 43, 0);
      return 0;
    }

    f = strtok(NULL, "/\r\n");

    while(f != NULL){
      printf("f: %s\n", f);
      if (strcmp(f, "..") == 0) {
        send(fd, "550 Directory cannot contain ../\n", 33, 0);
        return 0;
      }
      f = strtok(NULL, "/\n");
    }

    if (chdir(cpdir) == 0){
      send(fd, "250 Directory successfully changed.\n", 37, 0);
    } else {
      send(fd, "550 Failed to change directory.\n", 33, 0);
    }
  }
  return 0;
}

int cdup(int fd, char init_dir[]) {
  printf("CDUP command is called\n");
  char current_dir[BUFF_SIZE];

  getcwd(current_dir, BUFF_SIZE);
  if (strcmp(current_dir, init_dir) == 0){
    send(fd, "550 Not permitted to access parent of root directory.\n", 55, 0);
  } else {
    if (chdir("..") == 0){
      send(fd, "250 Directory successfully changed.\n", 37, 0);
    } else {
      send(fd, "550 Failed to change directory.\n", 33, 0);
    }
  }
  return 0;
}

int type(int fd, char *rept) {
  printf("TYPE command is called\n");
  if (rept == NULL) {
    send(fd, "500 Unrecognised TYPE command.\n", 32, 0);
  } else if (strcasecmp(rept, "I") == 0) {
    send(fd, "200 Switching to Binary mode.\n", 31, 0);
  } else if (strcasecmp(rept, "A") == 0){
    send(fd, "200 Switching to ASCII mode.\n", 30, 0);
  } else {
    send(fd, "500 Unrecognised TYPE command.\n", 32, 0);
  }
  return 0;
}

int mode(int fd, char *transm) {
  printf("MODE command is called\n");
  if (transm == NULL){
    send(fd, "504 Bad MODE command.\n", 23, 0);
  } else if (strcasecmp(transm, "S") == 0){
    send(fd, "200 Mode set to S.\n", 20, 0);
  } else if (strcasecmp(transm, "B") == 0){
    send(fd, "504 MODE Block is not supported.\n", 34, 0);
  } else if (strcasecmp(transm, "C") == 0){
    send(fd, "504 MODE Compressed is not supported.\n", 39, 0);
  } else {
    send(fd, "504 Bad MODE command.\n", 23, 0);
  }
  return 0;
}

int stru(int fd, char *filestrt) {
  printf("STRU command is called\n");
  if (filestrt == NULL){
    send(fd, "504 Bad STRU command.\n", 23, 0);
  } else if (strcasecmp(filestrt, "F") == 0){ // FILE
    send(fd, "200 Structure set to F.\n", 25, 0);
  } else if (strcasecmp(filestrt, "R") == 0){ // RECORD
    send(fd, "504 STRU Record is not supported.\n", 35, 0);
  } else if (strcasecmp(filestrt, "P") == 0){ // PAGE
    send(fd, "504 STRU Page is not supported.\n", 33, 0);
  } else {
    send(fd, "504 Bad STRU command.\n", 23, 0);
  }
  return 0;
}
