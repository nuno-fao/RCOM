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

enum flag global_flag;

struct linkLayer linkNumber[512];
int linkCounter = 0;

int receive(struct linkLayer *linkLayer, char expected);

void setHeader(char flag, char endereco, char controlo, char *str)
{
    str[0] = flag;
    str[1] = endereco;
    str[2] = controlo;
    str[3] = endereco ^ controlo;
    str[4] = flag;
}

int flag = 1, conta = 1;
void atende() // atende alarme
{
    flag = 1;
    conta++;
}

int send_receive(struct linkLayer *linkLayer, char expected, char send)
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
            printf("Flag %d %d\n", flag, conta);
            int wri = write(linkLayer->fd, sendChar, 5);
            alarm(linkLayer->timeout);
        }
    }
}

int receive(struct linkLayer *linkLayer, char expected)
{
    int res;
    char rcv_str[1];
    char set[5];
    char A, C;
    bool error = false;
    enum state estado = START;
    while (true)
    {
        res = read(linkLayer->fd, rcv_str, 1);
        if (res == 0)
        {
            return -1;
        }
        //printf("State %d %d\n", estado, linkLayer->fd);
        printf("Receber %x\n", rcv_str[0]);
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

int setupLinkLayer(struct linkLayer *linkLayer, int porta, int baudRate, int sequenceNumber, int timeout, int numTransmissions)
{
    sprintf(linkLayer->port, "/dev/ttyS%d", porta);
    linkLayer->baudRate = baudRate;
    linkLayer->sequenceNumber = sequenceNumber;
    linkLayer->timeout = timeout;
    linkLayer->numTransmissions = numTransmissions;
}

int setTermIO(struct termios *newtio, struct termios *oldtio, struct linkLayer *linkLayer, int vtime, int vmin)
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
        exit - 1;
    }
    return 0;
}

int resetTermIO(struct termios *oldtio, int fd)
{
    tcsetattr(fd, TCSANOW, oldtio);
}

int llopen(int porta, enum flag flag)
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
        write(linkNumber[linkLayerNumber].fd,set,5);
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
        write(linkNumber[linkLayerNumber].fd,set,5);
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