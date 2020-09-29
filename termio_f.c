/*Non-Canonical Input Processing*/

#include "termio_f.h"


int setTermIO(struct termios *newtio,struct termios *oldtio,int fd,int vtime,int vmin){

	if ( tcgetattr(fd,oldtio) == -1) { /* save current port settings */
      		perror("1\n");
      		exit(-1);
    	}

	bzero(newtio, sizeof(*newtio));
    	newtio->c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
    	newtio->c_iflag = IGNPAR;
    	newtio->c_oflag = 0;

    	/* set input mode (non-canonical, no echo,...) */
    	newtio->c_lflag = 0;
  
  	/* 
    		VTIME e VMIN devem ser alterados de forma a proteger com um temporizador a 
    		leitura do(s) próximo(s) caracter(es)
  	*/

    	newtio->c_cc[VTIME]    = vtime;   /* inter-character timer unused */
    	newtio->c_cc[VMIN]     = vmin;
    	
    	tcflush(fd, TCIOFLUSH);

    	if (tcsetattr(fd,TCSANOW,newtio) == -1) {
      		perror("2\n");
      		exit -1;
    	}
    	return 0;
}
 
int resetTermIO(struct termios *oldtio,int fd){
    tcsetattr(fd,TCSANOW,oldtio);
}
