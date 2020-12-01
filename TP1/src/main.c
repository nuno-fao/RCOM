#include "headers/link.h"
#include "headers/app.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <time.h>

int trama_size;

int main(int argc, char *argv[])
{
    int arg = atoi(argv[1]);
    srand((unsigned) time(NULL));
    trama_size = TRAMA_SIZE;
    	
    if (arg == 0)
    {
    	if(argc>4){
    		setDefaultTramaSize(atoi(argv[4]));
    		trama_size = atoi(argv[4]);
    	}
        int linkLayerNumber = llopen(atoi(argv[3]), TRANSMITTER);
        if (linkLayerNumber == -1)
            return -1;
        else
        {
            if(sendFile(linkLayerNumber, argv[2]) == -1){
                return -1;
            }
        }
	printf("Closing Connectin\n");
        llclose(linkLayerNumber);
    }
    else
    {
    	if(argc>3){
    		setDefaultTramaSize(atoi(argv[3]));
    		trama_size = atoi(argv[3]);
    	}
        int linkLayerNumber = llopen(atoi(argv[2]), RECEIVER);
        if (linkLayerNumber == -1)
            return -1;
        else
        {
            if(receiveFile(linkLayerNumber) == -1){
                return -1;
            }
        }
	printf("Closing Connection\n");
        llclose(linkLayerNumber);
    }
}

int sendFile(int linkLayerNumber, char *file)
{

    FILE *fd = fopen(file, "r");
    if(fd==NULL){
        perror("ERRO ao abrir o ficheiro");
    }
    struct stat st;
    int size;
    int packetSize = 13 + strlen(file);
    unsigned char CTRLPacket[packetSize];
    unsigned char dataPack[trama_size + 4];

    if (fd == NULL)
    {
        printf("FILE NOT FOUND\n");
        return -1;
    }

    stat(file, &st);
    size = st.st_size;

    controlPacket(CTRL_START, CTRLPacket, &size, file);
    printf("FileName: %s\n",file);
    printf("FileSize: %d\n",size);

    if (llwrite(linkLayerNumber, CTRLPacket, packetSize) == -1)
    {
        return -1;
    }
    else
    {
        int leftSize = size;
        int localSize = size / trama_size;
        int readSize;
        if (size % trama_size > 0)
        {
            localSize++;
        }
        for (int sequenceNumber = 0; sequenceNumber < localSize; sequenceNumber++)
        {
            readSize = fread(&dataPack[4], 1, trama_size, fd);
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
    return 0;
}

int receiveFile(int linkLayerNumber)
{
    int space = trama_size;
    if(space<30) space = 30;
    unsigned char buffer[space + 4];
    int dataSize;
    int fd;

    packetType packetType = -1;
    packet_u packet;
    float acumSize = 0;
    float totalSize = 0;
    float lastP = 0;
    unsigned long iniTime = time(NULL);
    while (1)
    {
        if ((dataSize = llread(linkLayerNumber, buffer)) < 0)
        {
            return -1;
        }
        if (dataSize == 0)
        {
            continue;
        }
        readPacket(buffer, dataSize, &packetType, &packet);
        if (packetType == CONTROL && packet.c.end)
        {
            break;
        }
        else if (packetType == DATA)
        {
            acumSize += (dataSize - 4);
            if (acumSize / totalSize * 100 > lastP)
            {
                unsigned long curTime = time(NULL);
                float rest = (totalSize - acumSize) * (curTime - iniTime) / acumSize;
                unsigned long acumTime = curTime - iniTime;
                printf("Tempo Restante: %fs\n", rest);
                printf("Tempo Decorrido: %lds\n", acumTime);
                printf("Percentagem: %f%%\n\n", lastP);
		 fflush(stdout);
                lastP = (int)(acumSize / totalSize * 100) + 1;
            }
            dataPacket_s *dataPacket = &(packet.d);
            write(fd, dataPacket->data, dataPacket->dataSize);
        }
        else if (packetType == CONTROL)
        {
            totalSize = *packet.c.fileSize;
            fd = open(packet.c.fileName, O_RDWR | O_NOCTTY | O_CREAT, 0777);
        }
    }
    return 0;
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
	printf("FileName %s\n",(char*)name);
	fflush(stdout);
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

