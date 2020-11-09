#include "macro.h"
#include <termios.h>

#define TRAMA_SIZE 256
#define BAUDRATE B115200
//error probability in percentage
#define ERROR_PROB 0
//delay in microseconds
#define PROPAGATION_DELAY 0  

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
    int fd;                        /*Identificador da ligação */
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


/**
 * Function used to change the value used to set the port baudrate
 * @param rate Value to set baudrate
 * @return void
 */
void setDefaultBaudRate(int rate);

/**
 * Function used to change the value of the frame size
 * @param size Frame size to set
 * @return void
 */
void setDefaultTramaSize(int size);

/**
 * Function used to establish the connection between sender and receiver
 * @param porta value of the port to open
 * @param flag contains the value in which llopen should operate (TRANSMITTER / RECEIVER)
 * @return index of the linkLayer to use, -1 in case of error
 */
int llopen(int porta,deviceType flag);

/**
 * Function used to close the connection between sender and receiver
 * @param ll index of the linkLayer to use
 * @return 0 in case of success, -1 otherwise
 */
int llclose(int ll);

/**
 * Function used to create and write a frame to the port
 * @param fd index of the linkLayer to use
 * @param buffer array of chars to proccess
 * @param length size of buffer
 * @return number of chars written, -1 in case of error
 */
int llwrite(int fd, unsigned char * buffer, int length);

/**
 * Function used to read and process data from the port
 * @param fd index of the linkLayer to use
 * @param buffer array to write what is read after it is processed
 * @return number of chars read, -1 in case of error
 */
int llread(int fd, unsigned char *buffer);
