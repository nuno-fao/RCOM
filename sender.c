/*Non-Canonical Input Processing*/
#include "utils.h"
#include <stdio.h>
#include <string.h>

int main(int argc, char** argv)
{
    int fd,c, res;
    struct termios oldtio,newtio;
    char buf[1];

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
    printf("teste\n");

    if(setTermIO(&newtio,&oldtio,fd,30,0)) exit(-1);  /* blocking read until 5 chars received */
    
    	char str[255];
    	
    	char set[5];
    	
    	char rcv_str[1];
    	
    	setHeader(0x7d,0x03,0x03,set,0x7d);
	write(fd,set,5);
	bool error = false;
	enum state estado;
	char A,C;
	
    	while (TRUE) {     
    	  int recvd_bytes = read(fd,rcv_str,1);
    	  
    	  if(!recvd_bytes || error){
            	estado=START;
    	  	write(fd,set,5);
    	  	printf("ERRO\n");    
    	  	continue;	  	
    	  }
    	  
          if(*buf==0x7d && estado!=BCC){
            estado=FLAGRCV;
          }
          else if(*buf==0x7d){
            break;
          }
          else if(estado==FLAGRCV){
            if(*buf!=0x03){
              error=true;
            }
            A=*buf;
            estado=ARCV;
          }
          else if(estado==ARCV){
            if(*buf!=0x07){
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
    	}
    	  printf("Tudo Bem\n");   
    	
    	
    	
    	
    	//printf("%d\n",recvd_bytes);
    	bzero(&rcv_str, sizeof(rcv_str));
    resetTermIO(&oldtio,fd);
    close(fd);
    return 0;
}


