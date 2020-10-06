#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <unistd.h>
#include <strings.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

#define BAUDRATE B38400
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1

enum state{
    START,
    FLAGRCV,
    ARCV,
    CRCV,
    BCC,
    STOP
};

int setTermIO(struct termios *newtio,struct termios *oldtio,int fd,int vtime,int vmin);
 
int resetTermIO(struct termios *oldtio,int fd);

void setHeader(char flag, char endereco, char controlo,char *str, char bcc);
