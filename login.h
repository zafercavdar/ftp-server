#ifndef __LOGIN_H__
#define __LOGIN_H__

int user(int fd, char *username);
int pass(int fd, char *pw);
void logout();
int is_logged_in();

#endif
