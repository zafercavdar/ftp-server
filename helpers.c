#include <stdio.h>
#include "helpers.h"

// Below code is from https://stackoverflow.com/questions/238603/how-can-i-get-a-files-size-in-c

int fsize(FILE *fp){
    int prev=ftell(fp);
    fseek(fp, 0L, SEEK_END);
    int sz=ftell(fp);
    fseek(fp,prev,SEEK_SET); //go back to where we were
    return sz;
}
