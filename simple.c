#include "simple.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "helpers.h"

#define BUFF_SIZE 256

int cwd(int fd, char *dir) {
  printf("info | server: called CWD func\n");
  char *f;
  char cpdir[BUFF_SIZE];
  char msg[BUFF_SIZE];

  if (dir == NULL){
    strcpy(msg, "550 Failed to change directory.\n");
  } else {
    strcpy(cpdir, dir);
    f = strtok(dir, "/\r\n");
    if (strcmp(f, "..") == 0 || strcmp(f, ".") == 0) {
      strcpy(msg, "550 Directory cannot start with ../ or ./\n");
      return 0;
    }

    f = strtok(NULL, "/\r\n");

    while(f != NULL){
      if (strcmp(f, "..") == 0) {
        strcpy(msg, "550 Directory cannot contain ../\n");
        return 0;
      }
      f = strtok(NULL, "/\n");
    }

    if (chdir(cpdir) == 0){
      strcpy(msg, "250 Directory successfully changed.\n");
    } else {
      strcpy(msg, "550 Failed to change directory.\n");
    }
  }

  fdsend(fd, msg);
  return 0;
}

int cdup(int fd, char init_dir[]) {
  printf("info | server: called CDUP func\n");
  char current_dir[BUFF_SIZE];
  char msg[BUFF_SIZE];

  getcwd(current_dir, BUFF_SIZE);
  if (strcmp(current_dir, init_dir) == 0){
    strcpy(msg, "550 Not permitted to access parent of root directory.\n");
  } else {
    if (chdir("..") == 0){
      strcpy(msg, "250 Directory successfully changed.\n");
    } else {
      strcpy(msg, "550 Failed to change directory.\n");
    }
  }

  fdsend(fd, msg);
  return 0;
}

int type(int fd, char *rept) {
  printf("info | server: called TYPE func\n");
  char msg[BUFF_SIZE];

  if (rept == NULL) {
    strcpy(msg, "500 Unrecognised TYPE command.\n");
  } else if (strcasecmp(rept, "I") == 0) {
    strcpy(msg, "200 Switching to Binary mode.\n");
  } else if (strcasecmp(rept, "A") == 0){
    strcpy(msg, "200 Switching to ASCII mode.\n");
  } else {
    strcpy(msg, "500 Unrecognised TYPE command.\n");
  }

  fdsend(fd, msg);
  return 0;
}

int mode(int fd, char *transm) {
  printf("info | server: called MODE func\n");
  char msg[BUFF_SIZE];

  if (transm == NULL){
    strcpy(msg, "504 Bad MODE command.\n");
  } else if (strcasecmp(transm, "S") == 0){
    strcpy(msg, "200 Mode set to S.\n");
  } else if (strcasecmp(transm, "B") == 0){
    strcpy(msg, "504 MODE Block is not supported.\n");
  } else if (strcasecmp(transm, "C") == 0){
    strcpy(msg, "504 MODE Compressed is not supported.\n");
  } else {
    strcpy(msg, "504 Bad MODE command.\n");
  }

  fdsend(fd, msg);
  return 0;
}

int stru(int fd, char *filestrt) {
  printf("info | server: called STRU func\n");
  char msg[BUFF_SIZE];

  if (filestrt == NULL){
    strcpy(msg, "504 Bad STRU command.\n");
  } else if (strcasecmp(filestrt, "F") == 0){ // FILE
    strcpy(msg, "200 Structure set to F.\n");
  } else if (strcasecmp(filestrt, "R") == 0){ // RECORD
    strcpy(msg, "504 STRU Record is not supported.\n");
  } else if (strcasecmp(filestrt, "P") == 0){ // PAGE
    strcpy(msg, "504 STRU Page is not supported.\n");
  } else {
    strcpy(msg, "504 Bad STRU command.\n");
  }

  fdsend(fd, msg);
  return 0;
}
