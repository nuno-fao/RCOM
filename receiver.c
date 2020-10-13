#include "utils.h"

int main(int argc, char** argv)
{
    int fd,c, res;
    struct termios oldtio,newtio;
    char buf[1];
    enum state estado=START;

    if ( (argc < 2) || 
  	     ((strcmp("/dev/ttyS10", argv[1])!=0) && 
  	      (strcmp("/dev/ttyS11", argv[1])!=0) )) {
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
      char set[5];
      char A, C;
      bool error=false;
    	//ler caracter a caracter
    	while (TRUE) {     
    	  	res = read(fd,buf,1);
          if(*buf==0x7e ){
            if(estado==BCC){
              estado=START;
              break;
            }
            estado=FLAGRCV;
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
            if(!error){
              setHeader(0x7e,0x03,0x07,set,0x7e);
            }
            else{
              setHeader(0x7e,0x03,0x00,set,0x7e);
            }
            write(fd,set,5);
            
          }
          printf("%x\n",estado);	
    	}
    	//printf("\nRECEIVER:\t%s\n",string);
    	
    }

    resetTermIO(&oldtio,fd);
    close(fd);
    return 0;
}
