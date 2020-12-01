#define SERVER_PORT 6000
#define SERVER_ADDR "192.168.28.96"
#define STR_LEN 256

struct urlArgs{
    char user[STR_LEN];
    char password[STR_LEN];
    char host[STR_LEN];
    char path[STR_LEN];
    char filename[STR_LEN];
};

int getIP(char *hostName,char *IP);
int openSocket();
int getArgsFromUrl(char *url, struct urlArgs *args);