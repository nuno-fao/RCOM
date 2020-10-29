#include "headers/link.h"
#include "headers/macro.h"
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <stdbool.h>
#include <string.h>
#include <signal.h>
#include <stdint.h>

int receive(linkLayer *linkLayer, char expected);
void changeSeqNumber(unsigned int *seqNumber);
int byteStuff(unsigned char *data, int size, uint8_t *stuffedPacket);
int byteDeStuff(unsigned char* data, int size);
uint8_t getBCC2(uint8_t *packet, int length);
int infoPacket(unsigned char *packet, int length, unsigned char A, unsigned char C);
int infoDePack(u_int8_t *packet,int *length,uint8_t *A,uint8_t *C);

int flag = 1, conta = 1;
deviceType global_flag;

linkLayer linkNumber[512];
int linkCounter = 0;

int dataSize = 0;
int dataSizeCounter = 0;

void *getPointer(void *pointer)
{
    dataSize;
    dataSizeCounter;
    if(dataSize == 0){
        dataSize = 16;
        return malloc(16);
    }
    if (dataSizeCounter == dataSize - 1)
    {
        return realloc(pointer, dataSize * 2);
    }
    return pointer;
}

void setHeader(char flag, char endereco, char controlo, char *str)
{
    str[0] = flag;
    str[1] = endereco;
    str[2] = controlo;
    str[3] = endereco ^ controlo;
    str[4] = flag;
}

void atende() // atende alarme
{
    flag = 1;
    conta++;
}

int send_receive(linkLayer *linkLayer, char expected, char send)
{
    flag = 1;
    conta = 1;
    char sendChar[5];
    (void)signal(SIGALRM, atende);
    setHeader(FLAG, ADDRESS, send, sendChar);
    write(linkLayer->fd, sendChar, 5);
    while (conta <= linkLayer->numTransmissions)
    {
        int revc = receive(linkLayer, expected);
        if (revc == 0)
        {
            break;
        }
        if (flag && conta <= linkLayer->numTransmissions)
        {
            flag = 0;
            //printf("Flag %d %d\n", flag, conta);
            write(linkLayer->fd, sendChar, 5);
            alarm(linkLayer->timeout);
        }
    }
    return 0;
}

int receive(linkLayer *linkLayer, char expected)
{
    int res;
    char rcv_str[1];
    char A, C;
    state estado = START;
    while (true)
    {
        res = read(linkLayer->fd, rcv_str, 1);
        if (res == 0)
        {
            return -1;
        }
        //printf("Receber %x\n", rcv_str[0]);
        if (*rcv_str == FLAG && estado != BCC)
        {
            estado = FLAGRCV;
        }
        else if (estado == BCC)
        {
            if (*rcv_str != FLAG)
            {
                return -1;
            }
            return 0;
        }
        else if (estado == FLAGRCV)
        {
            if (*rcv_str != ADDRESS)
            {
                return -1;
            }
            A = *rcv_str;
            estado = ARCV;
        }
        else if (estado == ARCV)
        {
            if (*rcv_str != expected)
            {
                return -1;
            }
            C = *rcv_str;
            estado = CRCV;
        }
        else if (estado == CRCV)
        {
            if (*rcv_str != (A ^ C))
            {
                return -1;
            }
            estado = BCC;
        }
    }
}

int setupLinkLayer(linkLayer *linkLayer, int porta, int baudRate, int sequenceNumber, int timeout, int numTransmissions)
{
    sprintf(linkLayer->port, "/dev/ttyS%d", porta);
    linkLayer->baudRate = baudRate;
    linkLayer->sequenceNumber = sequenceNumber;
    linkLayer->timeout = timeout;
    linkLayer->numTransmissions = numTransmissions;
    return 0;
}

int setTermIO(struct termios *newtio, struct termios *oldtio, linkLayer *linkLayer, int vtime, int vmin)
{

    if (tcgetattr(linkLayer->fd, oldtio) == -1)
    { /* save current port settings */
        perror("1\n");
        exit(-1);
    }

    bzero(newtio, sizeof(*newtio));
    newtio->c_cflag = linkLayer->baudRate | CS8 | CLOCAL | CREAD;
    newtio->c_iflag = IGNPAR;
    newtio->c_oflag = 0;

    /* set input mode (non-canonical, no echo,...) */
    newtio->c_lflag = 0;

    /*
            VTIME e VMIN devem ser alterados de forma a proteger com um temporizador a
            leitura do(s) pr�ximo(s) caracter(es)
    */

    newtio->c_cc[VTIME] = vtime; /* inter-character timer unused */
    newtio->c_cc[VMIN] = vmin;

    tcflush(linkLayer->fd, TCIOFLUSH);

    if (tcsetattr(linkLayer->fd, TCSANOW, newtio) == -1)
    {
        perror("2\n");
        return -1;
    }
    return 0;
}

int resetTermIO(struct termios *oldtio, int fd)
{
    tcsetattr(fd, TCSANOW, oldtio);
    return 0;
}

int llopen(int porta, deviceType flag)
{
    int linkLayerNumber = linkCounter++;
    struct termios oldtio, newtio;
    if (linkNumber[linkCounter].fd < 0)
    {
        perror(&linkNumber[linkLayerNumber].port[0]);
        exit(-1);
    }

    global_flag = flag;

    if (flag == TRANSMITTER)
    {
        setupLinkLayer(&linkNumber[linkLayerNumber], porta, B115200, 0, 3, 3);
        linkNumber[linkLayerNumber].fd = open(linkNumber[linkLayerNumber].port, O_RDWR | O_NOCTTY);
        if (setTermIO(&newtio, &oldtio, &linkNumber[linkLayerNumber], 1, 0))
            return -1;
        if (send_receive(&linkNumber[linkLayerNumber], UA, SET))
        {
        }
    }
    else if (flag == RECEIVER)
    {
        setupLinkLayer(&linkNumber[linkLayerNumber], porta, B115200, 0, 3, 3);
        linkNumber[linkLayerNumber].fd = open(linkNumber[linkLayerNumber].port, O_RDWR | O_NOCTTY);
        if (setTermIO(&newtio, &oldtio, &linkNumber[linkLayerNumber], 1, 0))
            return -1;

        char set[5];
        setHeader(FLAG, ADDRESS, UA, set);
        while (receive(&linkNumber[linkLayerNumber], SET))
        {
        }
        write(linkNumber[linkLayerNumber].fd, set, 5);
    }
    else
    {
        return -1;
    }
    /*resetTermIO(&oldtio,linkNumber[linkLayerNumber].fd);
    close(linkNumber[linkLayerNumber].fd);*/
    return linkLayerNumber;
}

int llclose(int linkLayerNumber)
{
    if (global_flag == TRANSMITTER)
    {
        int a = send_receive(&linkNumber[linkLayerNumber], DISC, DISC);
        char set[5];
        setHeader(FLAG, ADDRESS, UA, set);
        write(linkNumber[linkLayerNumber].fd, set, 5);
    }
    else if (global_flag == RECEIVER)
    {
        send_receive(&linkNumber[linkLayerNumber], UA, DISC);
    }
    else
    {
        return -1;
    }
    return 0;
}

int s = 0;

int llwrite(int fd, unsigned char *buffer, int length)
{
    uint8_t *packet = malloc(length + 1);
    uint8_t *stuffedPacket = malloc((length + 1) * 2 + 5);
    int u = buffer[0];

    memcpy(packet, buffer, length);
    packet[length] = getBCC2(packet,length);
    length++;
    //printf("%d\n",length);
    length = byteStuff(packet, length , &stuffedPacket[4]);
    
    if (infoPacket(stuffedPacket, length, SNDR_COMMAND, linkNumber[fd].sequenceNumber))
    {
    }
    changeSeqNumber(&linkNumber[fd].sequenceNumber);

    write(linkNumber[fd].fd,stuffedPacket,length+5);
    
    free(packet);
    free(stuffedPacket);
    return 1;
}

uint8_t getBCC2(uint8_t *packet, int length)
{
    unsigned char bcc2 = 0x00;
    for (int i = 0; i < length; i++)
    {
        bcc2 = bcc2 ^ packet[i];
    }
    return bcc2;
}

int llread(int fd, uint8_t *buffer)
{
    unsigned char A;
    unsigned char C;
    unsigned char aux;
    unsigned char bcc2;
    unsigned char data[TRAMA_SIZE*2+5];
    int state = 0;
    int i=0;
    int size;
    bool flagReached=false;

    while(!flagReached){
        int r = read(linkNumber[fd].fd, &aux, 1);
        if(r == 0){
        	perror("MERDA");
        	continue;
        	}
        switch(state){
        case 0:
            if(aux==FLAG){
                state=1;
            }
            break;
        case 1:
            if(aux==SNDR_COMMAND){
                A=aux;
                state=2;
            }
            else{
                state=0;
                //manda erro
            }
            break;
        case 2:
            if(linkNumber[fd].sequenceNumber==aux){
                C=aux;
                changeSeqNumber(&linkNumber[fd].sequenceNumber);
                state=3;
            }
            else{
                state=0;
                //manda erro
            }
            break;
        case 3:
            if((A^C)==aux){
                state=4;
            }
            else{
                state=0;
                //manda erro
            }
            break;
        case 4:
            if(aux!=0x7e)
                data[i++]=aux;
            else flagReached=true;
            break;
        }
    }

    size = byteDeStuff(data,i);
    bcc2=getBCC2(data,size-1);

    if(bcc2==data[size-1]){
    }
    else{
        //erro
    }
    memcpy(buffer,data,size-1);

    return size-1;

}

int infoPacket(unsigned char *packet, int length, unsigned char A, unsigned char C)
{
    packet[0] = FLAG;
    packet[1] = A;
    packet[2] = C;
    packet[3] = A ^ C;

    packet[length + 4] = FLAG;
    return 0;
}

int infoDePack(u_int8_t *packet,int *length,uint8_t *A,uint8_t *C){
    *A =  packet[1];
    *C =  packet[2];
}

void changeSeqNumber(unsigned int *seqNumber)
{
    *seqNumber ^= 0x01;
}

int byteStuff(unsigned char *data, int size, uint8_t *stuffedPacket)
{
    int sum = 0;
    int l = 0;

    for (int i = 0; i < size; i++)
    {
        if (data[i] == 0x7e)
        {
            sum++;
            stuffedPacket[l++] = 0x7d;
            stuffedPacket[l++] = 0x5e;
        }
        else if (data[i] == ESCAPE_BYTE)
        {
            sum++;
            stuffedPacket[l++] = 0x7d;
            stuffedPacket[l++] = 0x5d;
        }
        else
        {
            stuffedPacket[l++] = data[i];
        }
    }
    //printf("STUFF %d\n",sum);
    return l;
}


int byteDeStuff(unsigned char *data, int size) {

    char aux[size];

    memcpy(aux,data,size);

    int finalSize=0;
    int sum = 0;


    for(int i = 0; i < size; i++){

        if(aux[i] == 0x7d && i+1<size) {
            if(aux[i+1] == 0x5d){
                data[finalSize]=0x7d;
            }
            else if(aux[i+1] == 0x5e){
                data[finalSize]=0x7e;
            }
            finalSize ++;
            i++;
            sum++;
        }
        else{
            data[finalSize] = aux[i];
            finalSize++;
        }
    }
    //printf("DESTUFF %d\n",sum);
    return finalSize;
}
