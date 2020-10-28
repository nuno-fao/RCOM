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
uint8_t getBCC2(uint8_t *packet, int length);
int infoPacket(unsigned char *packet, int length, unsigned char A, unsigned char C);

int flag = 1, conta = 1;
deviceType global_flag;

linkLayer linkNumber[512];
int linkCounter = 0;

int dataSize = 16;
int dataSizeCounter = 0;

void *getPointer(void *pointer)
{
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
        setupLinkLayer(&linkNumber[linkLayerNumber], porta, B38400, 0, 3, 3);
        linkNumber[linkLayerNumber].fd = open(linkNumber[linkLayerNumber].port, O_RDWR | O_NOCTTY);
        if (setTermIO(&newtio, &oldtio, &linkNumber[linkLayerNumber], 1, 0))
            return -1;
        if (send_receive(&linkNumber[linkLayerNumber], UA, SET))
        {
        }
    }
    else if (flag == RECEIVER)
    {
        setupLinkLayer(&linkNumber[linkLayerNumber], porta, B38400, 1, 3, 3);
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

int llwrite(int fd, unsigned char *buffer, int length)
{

    printf("1\n");
    fflush(stdout);
    uint8_t *packet = malloc(length + 1);
    printf("1.1\n");
    fflush(stdout);
    uint8_t *stuffedPacket = malloc((length + 1) * 2 + 5);
    printf("1.2\n");
    fflush(stdout);
    printf("2\n");
    fflush(stdout);

    memcpy(packet, buffer, length);
    packet[length] = getBCC2(packet,length);
    length = byteStuff(packet, length + 1, stuffedPacket);
    printf("3\n");
    fflush(stdout);

    if (infoPacket(stuffedPacket, length, SNDR_COMMAND, linkNumber[fd].sequenceNumber))
    {
    }

    printf("4\n\n");
    fflush(stdout);
    changeSeqNumber(&linkNumber[fd].sequenceNumber);

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

int llread(int fd, unsigned char *buffer)
{
    char cona[1];
    read(linkNumber[fd].fd, &cona, 1);
    printf("%x ", cona[0]);
    printf("a ler\n");
    return 1;
}

int infoPacket(unsigned char *packet, int length, unsigned char A, unsigned char C)
{
    packet[0] = FLAG;
    packet[1] = A;
    packet[2] = C;
    packet[3] = A ^ C;

    packet[length + 5] = FLAG;
    return 0;
}

void changeSeqNumber(unsigned int *seqNumber)
{
    *seqNumber ^= 0x01;
}

int byteStuff(unsigned char *data, int size, uint8_t *stuffedPacket)
{
    int sum = 0;
    for (int i = 0; i < size; i++)
    {
        if (data[i] == 0x7e)
        {
            sum++;
            stuffedPacket[i + 4] = 0x7d;
            stuffedPacket[++i + 4] = 0x5e;
        }
        else if (data[i] == ESCAPE_BYTE)
        {
            sum++;
            stuffedPacket[i + 4] = 0x7d;
            stuffedPacket[++i + 4] = 0x5d;
        }
        else
        {
            stuffedPacket[i + 4] = data[i];
        }
    }
    uint8_t *newstuffedPacket = realloc(stuffedPacket, size + sum + 6);
    stuffedPacket = newstuffedPacket;
    return size + sum + 5;
}
