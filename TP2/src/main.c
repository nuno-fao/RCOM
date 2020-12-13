#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "funcs.h"

int main(int argc, char *argv[]){
    if (argc != 2) {
        printf("Usage: %s %s\n", argv[0], "ftp://[<user>:<password>@]<host>/<url-path>");
        return -1;
    }

    struct urlArgs args;
    if(getArgsFromUrl(argv[1],&args) != 0){
        exit(1);
    }
    //printf("User: %s\n", args.user);
    //printf("Password: %s\n", args.password);
    //printf("Host name: %s\n", args.host);
    //printf("Path name: %s\n", args.path);
    //printf("File name: %s\n", args.filename);

    char IP[STR_LEN];
    if(getIP(args.host,IP)!=0){
       printf("failed retrieving IP\n");
       return -1;
    }
    printf("IP: %s\n",IP);

    int connectionSocket;
    if(openSocket(IP,&connectionSocket,SERVER_PORT) != 0){
        printf("failed opening socket\n");
       return -1;
    }

    char response[4];
    char body[1024];
    response[3]=0;
    readCommandFromSocket(connectionSocket,response,body);
    if(response[0]!='2'){
        printf("Error estabilishing connection\n");
        return -1;
    }


    writeToSocket(connectionSocket,"user",args.user);
    readCommandFromSocket(connectionSocket,response,body);

    writeToSocket(connectionSocket,"pass",args.password);
    readCommandFromSocket(connectionSocket,response,body);
    
    //writeToSocket(connectionSocket,"binary","");
    //readCommandFromSocket(connectionSocket,response,body);
    
    if(strcmp(args.path,"")){
    	printf("%s\n",args.path);
    	writeToSocket(connectionSocket,"cwd",args.path);
    	readCommandFromSocket(connectionSocket,response,body);
    }
    
    char pasvIP[15];
    int port;
    writeToSocket(connectionSocket,"pasv","");
    readCommandFromSocket(connectionSocket,response,body);
    getIPFromBody(body,pasvIP,&port);

    printf("IP: %s \n",pasvIP);
    printf("Port: %d \n",port);

    int dataSocket;
    if(openSocket(pasvIP,&dataSocket,port) != 0){
        printf("failed opening data socket\n");
        return -1;
    }
 
 
    //strcat(args.host,args.path);

    writeToSocket(connectionSocket,"retr",args.filename);
    readCommandFromSocket(connectionSocket,response,body);
    readFromSocketWriteToFile(dataSocket,args.filename);

    return 0;
}



