#include "macro.h"
#include <termios.h>

#define errorTest 0

typedef enum flags
{
    TRANSMITTER = 0,
    RECEIVER = 1
}deviceType;

typedef struct linkLayers
{
    char port[20];                 /*Dispositivo /dev/ttySx, x = 0, 1*/
    int baudRate;                  /*Velocidade de transmissão*/
    unsigned int sequenceNumber;   /*Número de sequência da trama: 0, 1*/
    unsigned int timeout;          /*Valor do temporizador: 1 s*/
    unsigned int numTransmissions; /*Número de tentativas em caso def alha*/
    int fd;
    struct termios oltio;
}linkLayer;
typedef enum states{
    START = 0,
    FLAGRCV = 1,
    ARCV = 2,
    CRCV = 3,
    BCC = 4,
    STOP = 5
}state;

void setDefaultBaudRate(int rate);
void setDefaultTramaSize(int size);
int llopen(int porta,deviceType flag);
int llclose(int ll);
int llwrite(int fd, unsigned char * buffer, int length);
int llread(int fd, unsigned char *buffer);
