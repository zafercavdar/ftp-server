#ifndef __SIMPLE_H__
#define __SIMPLE_H__

int cwd(int fd, char *dir);
int cdup(int fd, char init_dir[]);
int type(int fd, char *rept);
int mode(int fd, char *transm);
int stru(int fd, char *filestrt);

#endif
