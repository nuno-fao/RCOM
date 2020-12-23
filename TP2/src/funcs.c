#include <stdio.h> 
#include <stdlib.h> 
#include <errno.h> 
#include <netdb.h> 
#include <sys/types.h>
#include <netinet/in.h> 
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <stdbool.h>
#include <sys/ioctl.h>
#include <fcntl.h>       
#include <libgen.h>

#include "funcs.h"

int getIP(char *hostName, char *IP){
    struct hostent *h;
    if ((h=gethostbyname(hostName)) == NULL) {  
        herror("gethostbyname");
        exit(1);
    }

    strcpy(IP, inet_ntoa(*((struct in_addr *)h->h_addr)));

    //printf("Host name  : %s\n", h->h_name);
    //printf("IP Address : %s\n",inet_ntoa(*((struct in_addr *)h->h_addr)));

    return 0;
}

int openSocket(char *address, int *fd, int port){
    int	sockfd;
	struct	sockaddr_in server_addr;
	//char	buf[] = "Mensagem de teste na travessia da pilha TCP/IP\n";  
	int	bytes;
	
	/*server address handling*/
	bzero((char*)&server_addr,sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = inet_addr(address);	/*32 bit Internet address network byte ordered*/
	server_addr.sin_port = htons(port);		/*server TCP port must be network byte ordered */
    
	/*open an TCP socket*/
	if ((sockfd = socket(AF_INET,SOCK_STREAM,0)) < 0) {
    		perror("socket()");
        	exit(1);
    	}
	/*connect to the server*/
    	if(connect(sockfd, 
	           (struct sockaddr *)&server_addr, 
		   sizeof(server_addr)) < 0){
        	perror("connect()");
		exit(1);
	}

    // 	/*send a string to the server*/
	// bytes = write(sockfd, buf, strlen(buf));
	// printf("Bytes escritos %d\n", bytes);

	// close(sockfd);
	// exit(0);

    *fd=sockfd;

    return 0;
    
    
}

//ftp://[<user>:<password>@]<host>/<url-path>
int getArgsFromUrl(char *url, struct urlArgs *args){

    char *substr;

    char *urlAux = malloc(strlen(url));
    strcpy(urlAux,url);

    char protocol[6];
    memcpy(protocol,&urlAux[0],6);
    if(strcmp(protocol,"ftp://")!=0){
        printf("Not using correct protocol. It should be ftp://\n");
        return -1;
    }

    urlAux+=6;
    
    char *ptr_at = strchr(urlAux,'@');
    char *ptr_tp = strchr(urlAux,':');
    if(ptr_at == NULL || ptr_tp == NULL){
    	bzero(args->user,256);
    	bzero(args->password,256);
    	
    	strcpy(args->user, "anonymous");
    	strcpy(args->password, "1234");
    }
    else if(urlAux[0]==':'){
        printf("No user defined! Use: ftp://[<user>:<password>@]<host>/<url-path>\n");
        return -1;
    }
    else{
        substr = strtok(urlAux,":");
        urlAux = strtok(NULL,":");

        memset(args->user, 0, sizeof(args->user));
        strcpy(args->user, substr);

        // parsing password
        substr = strtok(urlAux, "@");
        urlAux = strtok(NULL,"\0");
        //urlAux++;

        if (substr == NULL) {
            printf("Error reading password, should end with '@'. Like this [<user>:<password>@]\n");
            return -1;
        }

        memset(args->password, 0, sizeof(args->password));
        strcpy(args->password, substr);

    }
    if(urlAux[0]=='/'){
        printf("Host not set. Use ftp://[<user>:<password>@]<host>/<url-path>\n");
        return -1;
    }
    substr = strtok(urlAux,"/");
    urlAux = strtok(NULL,"\0");

    memset(args->host, 0, sizeof(args->host));
    strcpy(args->host, substr);

    if(strlen(urlAux)==0){
        printf("No path provided. Use ftp://[<user>:<password>@]<host>/<url-path>\n");
    }
    
    char auxcpy[256];
    strcpy(auxcpy,urlAux);
    
    strcpy(args->path,"/");
    strcpy(args->path,dirname(urlAux));
    strcpy(args->filename,basename(auxcpy));
    
    if(!strcmp(args->path,".")){
    	strcpy(args->path,"");
    }
    printf("%s %s\n",args->user,args->password);
    printf("%s%s\n",args->path,args->filename);

    return 0;

}

int writeToSocket(int sockfd,char *command,char *text){
    char buffer[STR_LEN];

    snprintf(buffer, sizeof(buffer) , "%s %s\n", command, text);


    int written = write(sockfd, buffer, strlen(buffer));

    //printf("TAMANHO: %d LENGTH: %d\n",written,strlen(buffer));
    if (written != strlen(buffer)){
        printf("Error writing to socket\n");
        return -1;
    }
    return 0;
}

int readCommandFromSocket(int sockfd, char *response, char* body){
    //memset(response,0,3);
    usleep(100000);
    read(sockfd,response,3);

    printf("%s",response);
    

    int len = 0;
    ioctl(sockfd, FIONREAD, &len);

    body[len]=0;

    read(sockfd,body,len);

    printf("%s \n",body);
    int r = atoi(response);
    if(r>=500 && r<600){
    	printf("ERROR, aborting connection\n");
    	exit(1);
    }
    
    return 0;
}

int getIPFromBody(char *body,char *IP,int *port){
    char *aux, *aux2;
    aux = strtok(body,"(");
    aux = strtok(NULL,")");

    char *lista[6];
    aux2 = strtok(aux,",");
    for(int i=0;i<6;i++){
        lista[i] = aux2;
        aux2 = strtok(NULL,",");
    }
    
    
    sprintf(IP,"%s.%s.%s.%s",lista[0],lista[1],lista[2],lista[3]);

    *port = atoi(lista[4]) * 256 + atoi(lista[5]);
    
    return 0;
}


int readFromSocketWriteToFile(int fd,char *filename){
	int fileFd = open(filename,O_RDWR | O_TRUNC | O_CREAT,0666);
	char aux[1024];
	int size = 0;
	long long unsigned int accum = 0;
	while((size = read(fd,aux,1024))){
		if(accum%256==0){
			printf("%f mibs transfered\n",(float)accum/1024);
			fflush(stdout);
			//accum = 0;
		}
		write(fileFd,aux,size);
		accum++;
	}
    return 0;
}
