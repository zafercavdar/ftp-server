#include <sys/types.h>
#include <stdio.h>
#include "usage.h"

// This file is provided to show how link/add in another file with C code in it)

// Given the name of the program print out usage instructions. */
void usage(char *progName) {

  fprintf(stderr, "Usage: %s <port>\n", progName);
  fprintf(stderr, "     <port>   Specifies the port the server will accept connections on.\n");
  fprintf(stderr, "              The port value must >= 1024 and <= 65535.\n");
}
