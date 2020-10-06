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
    

    while(TRUE){
    	char str[255];
    	//printf("SENDER:\t");
    	
    	//pedir input e for�ar a input n�o vazio
    	/*while((gets(str)==0 && 0) || str[0]==0){
    		printf("\nSENDER:\t");    		
    	}
    	int i;
    	
    	//verificar posi��o do "\0"
    	for(i=0;i<255;i++){
    		if(str[i]==0)
    			break;
    	}
    	
    	//enviar a string
    	write(fd,str,i+1);
    	
    	char rcv_str[255];
    	
    	//ler a resposta do recetor
    	read(fd,rcv_str,255);
    	
    	//verificar que a resposta � igual � msg enviada
    	if(strcmp(rcv_str,str)){
    		printf("SENT:\t%s\n",str);
    		printf("RECEIVED:\t%s\n",rcv_str);
    	}    */
    	
    	char set[5];
    	setHeader(0x7d,0x03,0x03,set,0x7d);
		write(fd,set,5);
    	
    	char rcv_str[1];
    	int recvd_bytes = read(fd,rcv_str,1);
    	
    	
    	//printf("%d\n",recvd_bytes);
    	bzero(&rcv_str, sizeof(rcv_str));
    }
    resetTermIO(&oldtio,fd);
    close(fd);
    return 0;
}


