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

typedef union packet_u{
    dataPacket_s d;
    controlPacket_s c;
}packet_u;

typedef enum packetType{
    CONTROL = 0,
    DATA = 1
}packetType;

/**
 * Function used to read a file, create all packets necessary and pass them to llwrite() one by one
 * @param linkLayerNumber Value to set baudrate
 * @param file pointer to a char array that contains the name of the target file
 * @return -1 if there is any error, 0 if it is successfull
 */
int sendFile(int linkLayerNumber, char *file);

/**
 * Function used to create a file with what it receives from llread()
 * @param linkLayerNumber Value to set baudrate
 * @param file pointer to a char array that contains the name of the target file
 * @return -1 if there is any error, 0 if it is successfull
 */
int receiveFile(int linkLayerNumber,char *file);

/**
 * Function used to fill a control packet with the information on the arguments
 * @param controlByte value of the control byte
 * @param packet pointer to the control packet
 * @param length pointer to the size of the file
 * @param file pointer to the name of the file
 * @return void
 */
void controlPacket(unsigned char controlByte, unsigned char *packet, int* length, char *file);

/**
 * Function used to fill a data packet with the information on the arguments
 * @param packet pointer to the array that cointains the chars read from the file
 * @param sequenceNumber value of the sequence number to use
 * @param size number of chars read from the file
 * @return void
 */
void dataPacket(unsigned char *packet, int sequenceNumber, int size);

/**
 * Function used to process what receiveFile() receives from llread(), indentifying what type of packet it is filling the union with the right information
 * @param data pointer to the data received
 * @param dataSize size of the data param
 * @param packetType enum that will be set when the type of packet is identified
 * @param packet union that will contain the packet
 * @return 
 */
int readPacket(unsigned char *data, int dataSize, packetType *packetType, packet_u *packet);
