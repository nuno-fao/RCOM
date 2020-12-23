#define SERVER_PORT 21
#define STR_LEN 256

struct urlArgs{
    char user[STR_LEN];
    char password[STR_LEN];
    char host[STR_LEN];
    char path[STR_LEN];
    char filename[STR_LEN];
};

/**
 * Function that retrieves the IP address of a given host
 * 
 * @param hostName The host's name
 * @param IP pointer to the variable that will contain the IP
 * @return 0 if success
 */
int getIP(char *hostName,char *IP);

/**
 * Function that creates a new TCP socket, and connects it to the given address and port 
 * 
 * @param address Server IP address
 * @param fd Variable that will contain the file descriptor
 * @param port Port number to connect
 * @return 0 if success
 */
int openSocket(char *address, int *fd, int port);


/**
 * Function that parses the url given as console argument
 * 
 * @param url URL to parse
 * @param args struct where the relevant parsed information will be saved
 * @return 0 if success, -1 otherwise
 */
int getArgsFromUrl(char *url, struct urlArgs *args);

/**
 * Function that sends a command to a socket
 * 
 * @param sockfd file descriptor of the socket will be sent the command
 * @param command header of the command
 * @param text body of the command
 * @return 0 if success, -1 otherwise
 */
int writeToSocket(int sockfd,char *command,char *text);

/**
 * Function that receives the ftp answer's return code and text of from a socket
 * 
 * @param sockfd file descriptor of the socket will be sent the command
 * @param response ftp answer return code
 * @param body ftp answer text
 * @return 0 if success, 1 otherwise
 */
int readCommandFromSocket(int sockfd, char *response, char* body);

/**
 * Function reads the IP and port from the ftp answer to pasv command
 * 
 * @param body text of the ftp answer
 * @param IP IP obtained from the ftp answer
 * @param port port obtained from the ftp answer
 * @return 0 if success
 */
int getIPFromBody(char *body,char *IP, int *port);

/**
 * Function reads the data of the desired file from a socket (must be the socket opened after the pasv command -> data socket)
 * 
 * @param fd file descriptor of the data socket
 * @param filename name of the file that is going to be downloaded
 * @return 0 if success
 */
int readFromSocketWriteToFile(int fd,char *filename);
