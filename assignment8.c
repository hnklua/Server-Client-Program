#include <sys/types.h> 
#include <sys/socket.h> 
#include <sys/wait.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <time.h> 
#include <unistd.h>

#define PORT_NUMBER 49999
#define BUFFER 100 
#define TRUE 1

// Server Interface 
void serverInterface() {
    // Variables 
    int listenFD; 
    int connectFD; 
    int bytesRead; 
    int err;  
    char timeBuffer[19] = {0}; 
    char hostName[NI_MAXHOST]; 
    struct sockaddr_in serveaddr; 
    struct sockaddr_in clientaddr; 
    socklen_t length = sizeof(struct sockaddr_in); 
    int connectionCount = 0; 

    // Set Address 
    memset(&serveaddr, 0, sizeof(serveaddr)); 
    serveaddr.sin_family = AF_INET; 
    serveaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serveaddr.sin_port = htons(PORT_NUMBER); 

    // Create socket 
    if ((listenFD = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("Error"); 
        fflush(stderr); 
        exit(EXIT_FAILURE); 
    }

    // Set socket option
    if(setsockopt(listenFD, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) == -1) {
        perror("Error"); 
        fflush(stderr); 
        close(listenFD); 
        exit(EXIT_FAILURE); 
    } 

    // Bind socket to the port 
    if (bind(listenFD, (struct sockaddr*)&serveaddr, sizeof(serveaddr)) == -1) { // Double check this
        perror("Error"); 
        fflush(stderr); 
        close(listenFD); 
        exit(EXIT_FAILURE); 
    }

    // Listen and accept conditions 
    if (listen(listenFD, 5) < 0) {
        perror("Error"); 
        fflush(stderr); 
        close(listenFD); 
        exit(EXIT_FAILURE); 
    } 

    while (TRUE) {
    // Accept connection
    if ((connectFD = accept(listenFD, (struct sockaddr*)&clientaddr, &length)) < 0) {
        perror("Error");
        continue;
    }

    // Increment count
    connectionCount++;
    
    // Fork to handle multiple clients
    if (fork() == 0) { 
        close(listenFD); 

        // Get the hostname from the client address
        if ((err = getnameinfo((struct sockaddr*)&clientaddr, sizeof(clientaddr), hostName, sizeof(hostName), NULL, 0, 0)) != 0) {
            fprintf(stderr, "Error: %s\n", gai_strerror(err));
            strncpy(hostName, "Unknown", sizeof(hostName) - 1);
            hostName[sizeof(hostName) - 1] = '\0';
        }

        
        // Log hostname and connection count
        printf("%s %d\n", hostName, connectionCount);
        fflush(stdout); 

        //Parse date and time in correct format
        time_t now = time(NULL);
        char* ctimeStr = ctime(&now);
        strncpy(timeBuffer, ctimeStr, 18);
        strcat(timeBuffer, "\n");  

        // Send date and time to stdout (client)
        if (send(connectFD, timeBuffer, sizeof(timeBuffer), 0) < 0) {
            perror("Error");
        } 

        close(connectFD);
        exit(0);
     } else {  
        close(connectFD);  
        while (waitpid(-1, NULL, WNOHANG) > 0); 
        }
    }
}

// client interface 
void clientInterface(const char* serverAddress) {
    // Variables 
    int socketFD; 
    int bytesRead; 
    char buffer[BUFFER] = {0}; 
    struct addrinfo hints; 
    struct addrinfo *actualdata; 
    int err; 

    // Set Address
    memset(&hints, 0, sizeof(hints)); 
    hints.ai_socktype = SOCK_STREAM; 
    hints.ai_family = AF_INET; 

    // Get error messages 
    if((err = getaddrinfo(serverAddress, "49999", &hints, &actualdata)) != 0) {
        fprintf(stderr, "Error: %s\n", gai_strerror(err)); 
        fflush(stderr);  
        exit(EXIT_FAILURE); 
    }

    // Create socket 
    if ((socketFD = socket(actualdata->ai_family, actualdata->ai_socktype, 0)) == -1) {
        perror("Error"); 
        fflush(stderr); 
        freeaddrinfo(actualdata); 
        exit(EXIT_FAILURE); 
    }

    // Connect to Server 
    if(connect(socketFD, actualdata->ai_addr, actualdata->ai_addrlen) < 0) {
        perror("Error"); 
        fflush(stderr); 
        close(socketFD); 
        freeaddrinfo(actualdata); 
        exit(EXIT_FAILURE); 
    }

    // Free Adress info 
    freeaddrinfo(actualdata); 

    // Obtain message from server and print to terminal
    while((bytesRead = read(socketFD, buffer, BUFFER - 1)) > 0) {
        buffer[bytesRead] = '\0'; 
        printf("%s", buffer); 
        fflush(stdout); 
    }

    if (bytesRead < 0) {
        perror("Error"); 
        fflush(stderr); 
    }
    // close socket 
    close(socketFD); 
}

int main(int argc, char const *argv[]){
    // Argument checking
    if (argc != 2 && argc != 3) {
        write(2, strerror(errno), strlen(strerror(errno))); 
        return errno; 
    }
    // Check for server argument
    if (argc == 2 && strcmp(argv[1], "server") == 0) {
        serverInterface();
    } else if (argc == 3 && strcmp(argv[1], "client") == 0) { // Check for client argument
        clientInterface(argv[2]); 
    } else {
        write(2, strerror(errno), strlen(strerror(errno))); 
        return errno; 
    }
    return 0;
}