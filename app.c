#include "headers/link.h"
#include "headers/app.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <fcntl.h>
#include <sys/stat.h>

int main(int argc, char *argv[])
{
    int arg = atoi(argv[1]);
    char *file = "./img/pinguim.gif";
    if (arg == 0)
    {
        int linkLayerNumber = llopen(10, TRANSMITTER);
        if (linkLayerNumber == -1)
            return -1;
        else
        {
            sendFile(linkLayerNumber, file);
        }
        sleep(3);
        llclose(linkLayerNumber);
    }
    else
    {
        int linkLayerNumber = llopen(11, RECEIVER);
        sleep(3);
        llclose(linkLayerNumber);
    }
}

int sendFile(int linkLayerNumber, char *file)
{

    FILE *fd = fopen(file, "r");
    struct stat st;
    int size;
    int packetSize = 13 + strlen(file);
    char CTRLPacket[packetSize];
    char data[TRAMA_SIZE];
    char dataPack[TRAMA_SIZE + 4];

    if (fd == NULL)
    {
        printf("FILE NOT FOUND\n");
        return -1;
    }

    stat(file, &st);
    size = st.st_size;

    controlPacket(CTRL_START, CTRLPacket, &size, file);

    if (llwrite(linkLayerNumber, CTRLPacket, packetSize) == -1)
    {
        return -1;
    }
    else
    {

        int localSize = size / TRAMA_SIZE;
        int readSize;
        if (size % TRAMA_SIZE > 0)
        {
            localSize++;
        }
        for (int sequenceNumber = 0; sequenceNumber < localSize; sequenceNumber++)
        {
            readSize = fread(data, TRAMA_SIZE, 1, fd);
            dataPacket(dataPack, sequenceNumber % 255, data, readSize);
            if (llwrite(linkLayerNumber, dataPack, size + 4) == -1)
                return -1;
        }
    }
}
int receiveFile(int linkLayerNumber)
{
    int fd;
    char *data;
    int dataSize;
    if ((dataSize = llread(linkLayerNumber, data)) < 0)
    {
    }

    packetType packetType = -1;
    void *packet;
    while (1)
    {
        readPacket(data, dataSize, &packetType, packet);
        if (packetType == CONTROL && ((controlPacket_s *)packet)->end)
        {
            break;
        }
        if (packetType == DATA)
        {
            dataPacket_s *dataPacket = ((dataPacket_s*)packet);
            write(fd,dataPacket->data,dataPacket->dataSize);
        }
        else if (packetType == CONTROL)
        {
            fd = open(((controlPacket_s *)packet)->fileName, O_RDWR | O_NOCTTY | O_CREAT);
        }
    }
}

// takes the data receive from llread,its size, and return a packet type(DATA or CONTROL) 
//and a pointer to a struct with the packet(controllPacket_s and dataPacket_s)
int readPacket(char *data, int dataSize, packetType *packetType, void *packet)
{
    int index = 0;

    //check wich type of packet it is
    if (data[index++] == 2 || data[index++] == 3)
    {
        int state = 0;
        int lenght = 0;
        char *name;
        uint64_t *size;
        state = data[index++];
        lenght = data[index++];
        if (state == 0)
        {
            size = (uint64_t *)malloc(8);
            memcpy(size, &data[index++], lenght);
            index += lenght;
        }
        else if (state == 1)
        {
            name = (char *)malloc(lenght);
            memcpy(name, &data[index++], lenght);
            index += lenght;
        }
        else
        {
            /* code */
        }
        state = data[index++];
        lenght = data[index++];
        if (state == 0)
        {
            size = (uint64_t *)malloc(8);
            memcpy(size, &data[index++], lenght);
            index += lenght;
        }
        else if (state == 1)
        {
            name = (char *)malloc(lenght);
            memcpy(name, &data[index++], lenght);
            index += lenght;
        }
        else
        {
            /* code */
        }
        *packetType = CONTROL;
        controlPacket_s *controlPacket_s = malloc(sizeof controlPacket_s);
        controlPacket_s->fileName = name;
        controlPacket_s->fileSize = size;
        controlPacket_s->end = data[0] - 2;
        packet = controlPacket_s;
        return 0;
    }
    else if (data[index++] == 1)
    {
        int state = 0;
        int lenght = 0;
        char *name;
        uint64_t *size;
        dataPacket_s *dataPacket_s = malloc(sizeof dataPacket_s);

        dataPacket_s->seqNumber = data[index++];
        dataPacket_s->dataSize = data[index++] * 256;
        dataPacket_s->dataSize += data[index++];
        dataPacket_s->data = malloc(dataPacket_s->dataSize);
        memcpy(dataPacket_s->data, &data[index++], dataPacket_s->dataSize);

        *packetType = DATA;

        packet = dataPacket_s;
        return 0;
    }
    return -1;
}

void controlPacket(char controlByte, char *packet, int *length, char *file)
{

    packet[0] = controlByte;
    packet[1] = TYPE_FILESIZE;
    packet[2] = 8;
    memcpy((void *)&packet[3], (void *)length, 8); //length to bits

    packet[11] = TYPE_FILENAME;
    packet[12] = strlen(file);
    memcpy((void *)&packet[13], (void *)file, packet[12]);

    return;
}

void dataPacket(char *packet, int sequenceNumber, char *data, int size)
{

    packet[0] = CTRL_DATA;
    packet[1] = sequenceNumber;
    packet[2] = (uint8_t)(size / 256);
    packet[3] = (uint8_t)(size % 256);
    memcpy((void *)&packet[4], (void *)data, size);
    return;
}
