enum flag
{
    TRANSMITTER = 0,
    RECEIVER = 1
};

struct linkLayer
{
    char port[20];                 /*Dispositivo /dev/ttySx, x = 0, 1*/
    int baudRate;                  /*Velocidade de transmissão*/
    unsigned int sequenceNumber;   /*Número de sequência da trama: 0, 1*/
    unsigned int timeout;          /*Valor do temporizador: 1 s*/
    unsigned int numTransmissions; /*Número de tentativas em caso def alha*/
    char frame[4096];               /*Trama*/
    int fd;
};
enum state{
    START = 0,
    FLAGRCV = 1,
    ARCV = 2,
    CRCV = 3,
    BCC = 4,
    STOP = 5
};

int llopen(int porta,enum flag flag);
int llclose(int ll);