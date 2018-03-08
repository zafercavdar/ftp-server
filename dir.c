#include <dirent.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include "dir.h"
#include <sys/stat.h>

/* 
   Arguments: 
      fd - a valid open file descriptor. This is not checked for validity
           or for errors with it is used.
      directory - a pointer to a null terminated string that names a 
                  directory

   Returns
      -1 the named directory does not exist or you don't have permission
         to read it.
      -2 insufficient resources to perform request

 
   This function takes the name of a directory and lists all the regular
   files and directoies in the directory. 
 

 */

int listFiles(int fd, char * directory) {

  // Get resources to see if the directory can be opened for reading
  
  DIR * dir = NULL;
  
  dir = opendir(directory);
  if (!dir) return -1;
  
  // Setup to read the directory. When printing the directory
  // only print regular files and directories. 

  struct dirent *dirEntry;
  int entriesPrinted = 0;
  
  for (dirEntry = readdir(dir);
       dirEntry;
       dirEntry = readdir(dir)) {
    if (dirEntry->d_type == DT_REG) {  // Regular file
      struct stat buf;

      // This call really should check the return value
      stat(dirEntry->d_name, &buf);

	dprintf(fd, "F    %-20s     %d\n", dirEntry->d_name, buf.st_size);
    } else if (dirEntry->d_type == DT_DIR) { // Directory
      dprintf(fd, "D        %s\n", dirEntry->d_name);
    } else {
      dprintf(fd, "U        %s\n", dirEntry->d_name);
    }
    entriesPrinted++;
  }
  
  // Release resources
  closedir(dir);
  return entriesPrinted;
}

   
