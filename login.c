#include "login.h"
#include <stdio.h>
#include <string.h>
#include "helpers.h"

#define BUFF_SIZE 256


int logged_in = 0;

int user(int fd, char *username) {
  printf("info | server: called USER func\n");
  char msg[BUFF_SIZE];

  if (logged_in == 1) {
    strcpy(msg, "530 Not logged in. Can't change user when logged in.\n");
  } else {
    if (username == NULL || strcmp(username, "cs317") != 0){
      strcpy(msg, "530 Not logged in. This server is cs317 only.\n");
    }
    else {
      logged_in = 1;
      strcpy(msg, "230 User logged in, proceed.\n");
    }
  }

  fdsend(fd, msg);
  return 0;
}

int pass(int fd, char * pw) {
  printf("info | server: called PASS func\n");
  char msg[BUFF_SIZE];

  if (logged_in == 0) {
    strcpy(msg, "503 Bad sequence of commands. Login with USER first.\n");
  } else {
    strcpy(msg, "230 User logged in, proceed.\n");
  }

  fdsend(fd, msg);
  return 0;
}

int is_logged_in(){
  return logged_in;
}

void logout(){
  logged_in = 0;
}
