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
    unsigned char data[TRAMA_SIZE];
    unsigned char dataPack[TRAMA_SIZE + 4];

    if (fd == NULL)
    {
        printf("FILE NOT FOUND\n");
        return -1;
    }

    stat(file, &st);
    size = st.st_size;

    controlPacket(CTRL_START, CTRLPacket, &size, file);

    /*if (llwrite(linkLayerNumber, CTRLPacket, packetSize) == -1)
    {
        return -1;
    }*/
    if(1)
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
            readSize = fread(data, 1, TRAMA_SIZE, fd);
            dataPacket(dataPack, sequenceNumber % 255, data, readSize);
            if (llwrite(linkLayerNumber, dataPack, readSize + 4) == -1)
                return -1;
        }
    }

    controlPacket(CTRL_END, CTRLPacket, &size, file);

    /*if (llwrite(linkLayerNumber, CTRLPacket, packetSize) == -1)
    {
        return -1;
    }*/
}

int receiveFile(int linkLayerNumber)
{

}

int readPacket(unsigned char *data, int dataSize, packetType *packetType, packet_u *packet)
{
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

void dataPacket(unsigned char *packet, int sequenceNumber, unsigned char *data, int size)
{
    packet[0] = CTRL_DATA;
    packet[1] = sequenceNumber;
    packet[2] = (uint8_t)(size / 256);
    packet[3] = (uint8_t)(size % 256);
    memcpy((void *)&packet[4], (void *)data, size);
    return;
}
