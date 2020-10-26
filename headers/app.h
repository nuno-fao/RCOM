int sendFile(int linkLayerNumber, char *file);
void controlPacket(char controlByte, char *packet, int* length, char *file);
void dataPacket(char *packet, int sequenceNumber,char* data, int size);