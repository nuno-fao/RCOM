#include "utils.h"

int main(int argc, char** argv)
{
    int fd,c, res;
    struct termios oldtio,newtio;
    char buf[1];
    enum state estado=START;

    if ( (argc < 2) || 
  	     ((strcmp("/dev/ttyS0", argv[1])!=0) && 
  	      (strcmp("/dev/ttyS1", argv[1])!=0) )) {
      printf("Usage:\tnserial SerialPort\n\tex: nserial /dev/ttyS1\n");
      exit(1);
    }


  /*
    Open serial port device for reading and writing and not as controlling tty
    because we don't want to get killed if linenoise sends CTRL-C.
  */
  
    
    fd = open(argv[1], O_RDWR | O_NOCTTY );
    if (fd <0) {perror(argv[1]); exit(-1); }

    if(setTermIO(&newtio,&oldtio,fd,0,1)) exit(-1);  /* blocking read until 5 chars received */

    while(TRUE){
    	int i=0;
    	char string[255];
      char A, C;
      bool error=false;
    	//ler caracter a caracter
    	while (TRUE) {     
    	  	res = read(fd,buf,1);
          if(estado==START && *buf==0x7d){
            estado=FLAGRCV;
          }
          else if(estado!=START && *buf==0x7d){
            estado=START;
          }
          else if(estado==FLAGRCV){
            if(*buf!=0x03){
              error=true;
            }
            A=*buf;
            estado=ARCV;
          }
          else if(estado==ARCV){
            if(*buf!=0x03){
              error=true;
            }
            C=*buf;
            estado=CRCV;
          }
          else if(estado==CRCV){
            if(*buf != (A ^ C)){
              error=true;
            }
            estado=BCC;
          }
          printf("%x\n",estado);
    	  	if(buf[0]==0){
    	  		string[i]=0;
    	  		break;
    	  	}
    	  	string[i++]=buf[0];    	  	
    	}
    	//printf("\nRECEIVER:\t%s\n",string);
    	write(fd,string,i);
    }

    resetTermIO(&oldtio,fd);
    close(fd);
    return 0;
}
