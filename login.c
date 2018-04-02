#include "login.h"
#include <stdio.h>
#include <string.h>
#include <netinet/in.h>

int logged_in = 0;

int user(int fd, char *username) {
  printf("USER command is called\n");
  if (logged_in == 1) {
    send(fd, "530 Can't change user when logged in.\n", 39, 0);
  } else {
    if (username == NULL || strcmp(username, "cs317") != 0){
      send(fd, "530 This server is cs317 only.\n", 31, 0);
    }
    else {
      logged_in = 1;
      send(fd, "230 Login successful.\n", 22, 0);
    }
  }
  return 0;
}

int pass(int fd, char * pw) {
  printf("PASS command is called\n");
  if (logged_in == 0) {
    send(fd, "503 Login with USER first.\n", 27, 0);
  } else {
    send(fd, "230 Already logged in.\n", 23, 0);
  }
  return 0;
}

int is_logged_in(){
  return logged_in;
}

void logout(){
  logged_in = 0;
}
