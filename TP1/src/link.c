#include "headers/link.h"
#include "headers/macro.h"
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>
#include <signal.h>
#include <stdint.h>
#include <sys/time.h>

/**
 * Function used to setup the linkLayer struct used along the program
 * @param linkLayer pointer to the struct that will be setup
 * @param porta 
 * @param baudrate value of the baudrate to use
 * @param sequenceNumber starting sequence number
 * @param timeout amout of time to wait everytime the alarm is used
 * @param numTransmissions number of tries when sending the same frame and no answer is obtained
 * @return 0 in case of success
 */
int setupLinkLayer(linkLayer *linkLayer, int porta, int baudRate, int sequenceNumber, int timeout, int numTransmissions);

/**
 * Function used to configure the terminal
 * @param newtio pointer to the new terminal setup
 * @param oldtio pointer to the old terminal setup
 * @param linkLayer pointer to the struct used to configure the new terminal
 * @param vtime maximum reading time without an answer
 * @param vmin minimum amount of chars read before read function returns
 * @return 0 in case of success, -1 otherwise
 */
int setTermIO(struct termios *newtio, struct termios *oldtio, linkLayer *linkLayer, int vtime, int vmin);

/**
 * Function that implements a state machine to read a supervision frame and checks if it contains the expected value
 * @param linkLayer contains the file descriptor from which to read
 * @param expected expected value on the frame
 * @return 0 in case of success, -1 otherwise
 */
int receive(linkLayer *linkLayer, char expected);

/**
 * Function that implements a state machine to read a supervision frame and checks if it contains one of the expected values
 * @param linkLayer contains the file descriptor from which to read
 * @param expected array with all the expected values
 * @param expectedSize size of the expected array
 * @return index of the value found if it is in the expected array in case of success, -1 otherwise
 */
int receive2(linkLayer *linkLayer, char expected[], int expectedSize);

/**
 * Function writes a supervision frame with a certain value and then implements a state machine to read an answer and checks if it contains the expected value
 * @param linkLayer contains the file descriptor from which to read
 * @param expected expected value of the answer
 * @param send value to send on the supervision frame
 * @return 0 in case of success, -1 otherwise
 */
int send_receive(linkLayer *linkLayer, char expected, char send);

/**
 * Function that updates the value of the sequence number to be used while creating/reading frames (0 or 1)
 * @param seqNumber pointer to the sequence to update
 * @return void
 */
void changeSeqNumber(unsigned int *seqNumber);

/**
 * Function that aplies byte stuffing mechanism to the date param
 * @param data pointer to the data that is to be stuffed
 * @param size size of data param
 * @param stuffedPacket pointer to write the resulting array
 * @return number of chars written on stuffedPacket
 */
int byteStuff(unsigned char *data, int size, uint8_t *stuffedPacket);

/**
 * Function that aplies the inverse of the byte stuffing mechanism to the date param
 * @param data pointer to the data that is to be destuffed
 * @param size size of data param
 * @return new size of the data param
 */
int byteDeStuff(unsigned char *data, int size);

/**
 * Function that iterates through the packet param and calculate the value of BCC2
 * @param packet pointer to the data that is to be destuffed
 * @param length size of packet param
 * @return value of BCC2
 */
uint8_t getBCC2(uint8_t *packet, int length);

/**
 * Function that fills the headers of a data frame with the parameters passed
 * @param packet pointer to the data frame
 * @param length size of data frame
 * @param A Address field
 * @param C Control field
 * @return 0 in case of success
 */
int infoPacket(unsigned char *packet, int length, unsigned char A, unsigned char C);

/**
 * Function that fills an array to be a supervision frame
 * @param flag value of the flags (start and end)
 * @param endereco address field
 * @param controlo control field
 * @param str array to fill
 * @return void
 */
void setHeader(char flag, char endereco, char controlo, char *str);

int flag = 1, conta = 1;
deviceType global_flag;

linkLayer linkNumber[512];
int linkCounter = 0;

int dataSize = 0;
int dataSizeCounter = 0;

int defaultBaudRate = 38400;
int defaultTramaSize = 256;

void setDefaultBaudRate(int rate)
{
    defaultBaudRate = rate;
}

void setDefaultTramaSize(int size)
{
    defaultTramaSize = size;
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
    conta = 1;
    char sendChar[5];
    (void)signal(SIGALRM, atende);
    setHeader(FLAG, ADDRESS, send, sendChar);
    write(linkLayer->fd, sendChar, 5);
    flag = 0;
    alarm(linkLayer->timeout);

    struct timeval start, end;
    gettimeofday(&start, NULL);

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
            if (conta > 1)
                printf("Resending Info\n");
        }
    }

    gettimeofday(&end, NULL);
    /*printf("Time taken to count to 10^5 is : %ld micro seconds\n",
           ((end.tv_sec * 1000000 + end.tv_usec) -
            (start.tv_sec * 1000000 + start.tv_usec)));*/

    if (conta > linkLayer->numTransmissions)
        return -1;
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

int receive2(linkLayer *linkLayer, char expected[], int expectedSize)
{
    int res;
    char rcv_str[1];
    char A, C;
    state estado = START;
    int i = 0;
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
            return i;
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
            for (i = 0; i < expectedSize; i++)
            {
                if (*rcv_str == expected[i])
                {
                    //printf("okkkkk");
                    C = *rcv_str;
                    estado = CRCV;
                    break;
                }
            }
            if (i == expectedSize)
                return -1;
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
        setupLinkLayer(&linkNumber[linkLayerNumber], porta, BAUDRATE, 0, 3, 3);
        linkNumber[linkLayerNumber].fd = open(linkNumber[linkLayerNumber].port, O_RDWR | O_NOCTTY);
        if (setTermIO(&newtio, &oldtio, &linkNumber[linkLayerNumber], 1, 0))
            return -1;

        if (send_receive(&linkNumber[linkLayerNumber], UA, SET))
        {
            return -1;
        }
    }
    else if (flag == RECEIVER)
    {
        setupLinkLayer(&linkNumber[linkLayerNumber], porta, BAUDRATE, 0, 3, 3);
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
    linkNumber[linkLayerNumber].oltio = oldtio;
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
    conta = 1;
    flag = 0;
    (void)signal(SIGALRM, atende);

    memcpy(packet, buffer, length);

    packet[length] = getBCC2(packet, length);

    length++;
    //printf("%d\n",length);
    length = byteStuff(packet, length, &stuffedPacket[4]);

    if (infoPacket(stuffedPacket, length, SNDR_COMMAND, linkNumber[fd].sequenceNumber))
    {
    }
    changeSeqNumber(&linkNumber[fd].sequenceNumber);

    // conta<=linkNumber[fd].numTransmissions
    write(linkNumber[fd].fd, stuffedPacket, length + 5);
    alarm(linkNumber[fd].timeout);
    unsigned char answer[5];
    while (conta <= linkNumber[fd].numTransmissions)
    {
        unsigned char out[2] = {RR, REJ};
        int r = receive2(&linkNumber[fd], out, 2);
        if (r == 0)
        {
            free(packet);
            free(stuffedPacket);
            return length + 5;
        }
        if (r == 1)
        {
            write(linkNumber[fd].fd, stuffedPacket, length + 5);
            alarm(linkNumber[fd].timeout);
            printf("Sent another time\n");
            conta = 1;
        }
        else if (flag && conta <= linkNumber[fd].numTransmissions)
        {
            flag = 0;
            //printf("Flag %d %d\n", flag, conta);
            write(linkNumber[fd].fd, stuffedPacket, length + 5);
            alarm(linkNumber[fd].timeout);
            printf("Waiting for answer\n");
        }
    }
    free(packet);
    free(stuffedPacket);
    return -1;
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
    int space = defaultTramaSize;
    if (space < 30)
        space = 30;
    unsigned char data[space * 2 + 5];
    unsigned char answer[5];
    bool erro = false;
    bool duplicado = false;
    int state = 0;
    int i = 0;
    int size;
    int tries = 0;
    bool flagReached = false;
    if(PROPAGATION_DELAY)
    	usleep(PROPAGATION_DELAY);
    while (tries <= linkNumber[fd].numTransmissions)
    {
        while (!flagReached)
        {
            int r = read(linkNumber[fd].fd, &aux, 1);

            if (r == 0)
            {
                continue;
            }
            switch (state)
            {
            case 0:
                if (aux == FLAG)
                {
                    state = 1;
                }
                break;
            case 1:
                if (aux == SNDR_COMMAND)
                {
                    A = aux;
                    state = 2;
                }
                else
                {
                    erro = true;
                    if (aux == FLAG)
                        state = 1;
                    else
                        state = 0;
                }
                break;
            case 2:
                if (linkNumber[fd].sequenceNumber == aux)
                {
                    C = aux;
                    changeSeqNumber(&linkNumber[fd].sequenceNumber);
                }
                else
                {
                    duplicado = true;
                }
                state = 3;
                break;
            case 3:
                if ((A ^ C) == aux)
                {
                    state = 4;
                }
                else
                {
                    erro = true;
                    if (aux == FLAG)
                        state = 1;
                    else
                        state = 0;
                }
                break;
            case 4:
                if (aux != 0x7e)
                    data[i++] = aux;
                else
                {
                    flagReached = true;
                    state = 0;
                }
                break;
            }
        }

        size = byteDeStuff(data, i);

        bcc2 = getBCC2(data, size - 1);
        //change this to increase the error probability
        if (rand() % 100 < ERROR_PROB)
        {
            bcc2 = 0x01;
            perror("alright alright\n");
        }

        if (bcc2 != data[size - 1])
        {
            erro = true;
        }

        if (erro == false)
        {
            setHeader(FLAG, SNDR_COMMAND, RR, answer);
            write(linkNumber[fd].fd, answer, 5);
            if (!duplicado)
            {
                memcpy(buffer, data, size - 1);
                return size - 1;
            }
            return 0;
        }
        else
        {
            setHeader(FLAG, SNDR_COMMAND, REJ, answer);
            write(linkNumber[fd].fd, answer, 5);
            erro = false;
            i = 0;
            changeSeqNumber(&linkNumber[fd].sequenceNumber);
            flagReached = false;
        }
    }

    return -1;
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

int byteDeStuff(unsigned char *data, int size)
{

    char aux[size];

    memcpy(aux, data, size);

    int finalSize = 0;
    int sum = 0;

    for (int i = 0; i < size; i++)
    {

        if (aux[i] == 0x7d && i + 1 < size)
        {
            if (aux[i + 1] == 0x5d)
            {
                data[finalSize] = 0x7d;
            }
            else if (aux[i + 1] == 0x5e)
            {
                data[finalSize] = 0x7e;
            }
            finalSize++;
            i++;
            sum++;
        }
        else
        {
            data[finalSize] = aux[i];
            finalSize++;
        }
    }
    //printf("DESTUFF %d\n",sum);
    return finalSize;
}
