#include <stdio.h>
#include "funcs.h"

int main(int argc, char *argv[]){
    if (argc != 2) {
        printf("Usage: %s %s\n", argv[0], "ftp://[<user>:<password>@]<host>/<url-path>");
        return -1;
    }
    
    return 0;
}