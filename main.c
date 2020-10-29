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
        llclose(linkLayerNumber);
    }
    else
    {
        int linkLayerNumber = llopen(11, RECEIVER);
        if (linkLayerNumber == -1)
            return -1;
        else
        {
            receiveFile(linkLayerNumber);
        }
        llclose(linkLayerNumber);
    }
}

int sendFile(int linkLayerNumber, char *file)
{

    FILE *fd = fopen(file, "r");
    struct stat st;
    int size;
    int packetSize = 13 + strlen(file);
    unsigned char CTRLPacket[packetSize];
    unsigned char dataPack[TRAMA_SIZE + 4];

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
        int leftSize = size;
        int localSize = size / TRAMA_SIZE;
        int readSize;
        if (size % TRAMA_SIZE > 0)
        {
            localSize++;
        }
        for (int sequenceNumber = 0; sequenceNumber < localSize; sequenceNumber++)
        {
            readSize = fread(&dataPack[4], 1, TRAMA_SIZE, fd);
            dataPacket(dataPack, sequenceNumber % 255, readSize);
            if (llwrite(linkLayerNumber, dataPack, readSize + 4) == -1)
                return -1;
        }
    }

    controlPacket(CTRL_END, CTRLPacket, &size, file);

    if (llwrite(linkLayerNumber, CTRLPacket, packetSize) == -1)
    {
        return -1;
    }
}

int receiveFile(int linkLayerNumber)
{
    unsigned char buffer[TRAMA_SIZE+4];
    int dataSize;
    int fd;
    packetType packetType = -1;
    packet_u packet;
    while (1)
    {   
        if ((dataSize = llread(linkLayerNumber, buffer)) < 0)
        {
            return -1;
        }
        readPacket(buffer, dataSize, &packetType, &packet);
        if (packetType == CONTROL && packet.c.end)
        {
            break;
        }
        if (packetType == DATA)
        {
            dataPacket_s *dataPacket = &(packet.d);
            write(fd,dataPacket->data,dataPacket->dataSize);
        }
        else if (packetType == CONTROL)
        {
            fd = open("coninhas", O_RDWR | O_NOCTTY | O_CREAT,0777);
        }
    }
}

int readPacket(unsigned char *data, int dataSize, packetType *packetType, packet_u *packet)
{
    int index = 0;

    //check wich type of packet it is
    if (data[index] == 2 || data[index] == 3)
    {
        index++;
        int state = 0;
        int length = 0;
        char *name;
        uint64_t *size;
        state = data[index++];
        length = data[index++];

        if (state == 0)
        {
            size = (uint64_t *)malloc(8);
            memcpy(size, &data[index], length);
            index += length;
        }
        else if (state == 1)
        {
            name = (char *)malloc(length);
            memcpy(name, &data[index], length);
            index += length;
        }
        else
        {
            /* code */
        }
        state = data[index++];
        length = data[index++];
        if (state == 0)
        {
            size = (uint64_t *)malloc(8);
            memcpy(size, &data[index], length);
            index += length;
        }
        else if (state == 1)
        {
            name = (char *)malloc(length);
            memcpy(name, &data[index], length);
            index += length;
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
        packet->c = *controlPacket_s;
        return 0;
    }
    else if (data[index] == 1)
    {
        index++;
        dataPacket_s *dataPacket_s = malloc(sizeof dataPacket_s);

        dataPacket_s->seqNumber = data[index++];
        dataPacket_s->dataSize = data[index++] * 256;
        dataPacket_s->dataSize += data[index++];
        dataPacket_s->data = malloc(dataPacket_s->dataSize);
        memcpy(dataPacket_s->data, &data[index++], dataPacket_s->dataSize);

        *packetType = DATA;

        packet->d = *dataPacket_s;
        return 0;
    }
    return -1;
}

void controlPacket(unsigned char controlByte, unsigned char *packet, int *length, char *file)
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

void dataPacket(unsigned char *packet, int sequenceNumber, int size)
{
    packet[0] = CTRL_DATA;
    packet[1] = sequenceNumber;
    packet[2] = (uint8_t)(size / 256);
    packet[3] = (uint8_t)(size % 256);
    return;
}
