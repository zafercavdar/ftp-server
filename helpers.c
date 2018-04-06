#include <stdio.h>
#include "helpers.h"
#include <netinet/in.h>
#include <string.h>

#define BUFF_SIZE 256

int fdsend(int fd, char msg[]){
  char clon[BUFF_SIZE];
  char *p;
  strcpy(clon, msg);
  p = clon;
  p[strlen(p)-1] = 0;
  printf("info | server: sending-> %s\n", p);

  if (send(fd, msg, strlen(msg), 0) == -1){
    printf("error | server: send\n");
  }
}

// Below code is from https://stackoverflow.com/questions/238603/how-can-i-get-a-files-size-in-c
int fsize(FILE *fp){
    int prev=ftell(fp);
    fseek(fp, 0L, SEEK_END);
    int sz=ftell(fp);
    fseek(fp,prev,SEEK_SET); //go back to where we were
    return sz;
}
