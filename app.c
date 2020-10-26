#include "headers/link.h"
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc,char *argv[]){
    int arg = atoi(argv[1]);
    if(arg==0){
        int linkLayerNumber = llopen(10,TRANSMITTER);
        sleep(3);
        llclose(linkLayerNumber);
    }
    else
    {
        int linkLayerNumber = llopen(11,RECEIVER);
        sleep(3);
        llclose(linkLayerNumber);
    }
}