#include <stdint.h>
typedef struct controlPacket_s{
    char *fileName;
    uint64_t *fileSize;
    uint8_t end;
}controlPacket_s;

typedef struct dataPacket_s{
    uint8_t seqNumber;
    uint16_t dataSize;
    char *data;
}dataPacket_s;


typedef enum packetType{
    CONTROL = 0,
    DATA = 1
}packetType;


int sendFile(int linkLayerNumber, char *file);
void controlPacket(unsigned char controlByte, unsigned char *packet, int* length, char *file);
void dataPacket(unsigned char *packet, int sequenceNumber,unsigned char* data, int size);
int receiveFile(int linkLayerNumber);
int readPacket(unsigned char *data, int dataSize, packetType *packetType, void *packet);