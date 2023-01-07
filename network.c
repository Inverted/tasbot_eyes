#include "network.h"
#include "utils.h"
#include "stack.h"
#include "tasbot.h"
#include "arguments.h"

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <pthread.h>


pthread_t serverInject;
pthread_t serverRealtime;

//region general

/**
 * Creating socket file descriptor
 * @return The file descriptor for the new socket
 */
int getSocketFD() {
    int sockfd;
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("[ERROR] Socket creation failed\n");
        exit(EXIT_FAILURE);
    } else {
        if (verbose) {
            printf("[INFO] Created socket with file descriptor (FD) %d on domain %d with type %d\n", sockfd, AF_INET,
                   SOCK_DGRAM);
        }
    }

    return sockfd;
}

/**
 * Setup server information
 * @param _server The server socket in address, that is to set up
 * @param _port The port the socket should use
 */
void setupServerInfo(struct sockaddr_in* _server, int _port) {
    _server->sin_family = AF_INET; // IPv4
    _server->sin_addr.s_addr = INADDR_ANY;
    _server->sin_port = htons(_port);
}

/**
 * Bind the socket with the server address
 * @param _sockfd The file descriptor for the socket
 * @param _server The server address that should be bound to the file descriptor
 */
void bindSocket(int _sockfd, struct sockaddr_in* _server) {
    if (bind(_sockfd, (const struct sockaddr*) _server, sizeof(*_server)) < 0) {
        perror("[ERROR] Binding socket port failed. ");
        printf("FD was %d\n", _sockfd);
        exit(EXIT_FAILURE);
    } else {
        if (verbose) {
            printf("[INFO] Bound socket with FD %d to port %d\n", _sockfd, ntohs(_server->sin_port));
        }
    }
}
//endregion

//region injection
void receiveAnimationInjection(int sockfd) {
    struct sockaddr_in cliaddr;
    socklen_t clilen = sizeof(cliaddr);

    char buffer[DATAGRAM_SIZE_LIMIT_INJECTION];
    while (running) {
        //Receiving string via UDP socket
        long n = recvfrom(sockfd, (char*) buffer, DATAGRAM_SIZE_LIMIT_INJECTION, MSG_WAITALL,
                          (struct sockaddr*) &cliaddr, &clilen);
        buffer[n] = '\0';

        //Determine playback style
        bool immediately = buffer[0] == 'I';

        //Cut off playback style from path
        unsigned long length = strlen(buffer);
        char* path = malloc(sizeof(char) * length - 2);
        memcpy(path, &buffer[2], length - 2);
        path[length - 2] = '\0';

        //Forming answer
        char answer[DATAGRAM_SIZE_LIMIT_INJECTION];
        if (immediately) {
            //sprintf(answer, "[INFO] Playing [%s] now!\n", path);
            sprintf(answer, "[INFO] Immediate playback is not yet supported!\n");
            if (verbose) {
                printf("%s", answer);
            }

            //todo: play now
        } else {
            //todo: mutex for stack
            if (addToStack(path)) {
                sprintf(answer, "[INFO] Successfully added (%s) to animation stack\n", path);
            } else {
                sprintf(answer, "[WARNING] Failed to add (%s) to animation stack. Stack most likely full\n", path);
            }
        }

        //Sending answer back
        sendto(sockfd, answer, strlen(answer), MSG_CONFIRM, (const struct sockaddr*) &cliaddr, clilen);
    }
}

void* runAnimationInjection(void* vargp) {
    // Create a socket
    int sockfd = getSocketFD();

    // Set up the server address
    struct sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(servaddr));
    setupServerInfo(&servaddr, PORT_INJECTION);

    // Bind the socket to the address
    bindSocket(sockfd, &servaddr);

    receiveAnimationInjection(sockfd);

    if (verbose) {
        printf("[INFO] Shutting down UDP socket serverInject on port %d\n", PORT_INJECTION);
    }
    return NULL;
}

void startAnimationInjectionServer() {
    pthread_create(&serverInject, NULL, runAnimationInjection, NULL);
    if (verbose) {
        printf("[INFO] Started thread for animation injection with TID %lu\n", serverInject);
    }
}
//endregion

//region realtime
void receiveRealtimeControl(int sockfd) {
    // Set up the client address
    struct sockaddr_in cliaddr;
    socklen_t clilen = sizeof(cliaddr);

    // Receive data from the client
    unsigned char recvBuffer[DATAGRAM_SIZE_LIMIT_REALTIME];

    while (running){

        printf("before work \n");

        long n;
        long lastN;
        while ((n = recvfrom(sockfd, recvBuffer, DATAGRAM_SIZE_LIMIT_REALTIME, MSG_DONTWAIT, (struct sockaddr*) &cliaddr, &clilen)) > 0) {
            lastN = n;
            recvBuffer[n] = '\0';
        }

        printf("work N=%ld \n", lastN);

        if (recvBuffer[0] == 2) { //ensure right mode
            for (int i = 2; i < lastN; i += 3) {
                GifColorType color;
                color.Red = recvBuffer[i];
                color.Blue = recvBuffer[i + 1];
                color.Green = recvBuffer[i + 2];
                setNoseLED(i / 3, color);
            }

            //todo: do something with timeout
        }
    }
}

/**
 * Run the WLED UDP realtime control
 * @param sockfd The socket file descriptor, the realtime control is listening to
 */
void* runRealtimeControl(void* vargp) {
    int sockfd;
    struct sockaddr_in servaddr;

    // Create a socket
    sockfd = getSocketFD();

    // Set up the server address
    memset(&servaddr, 0, sizeof(servaddr));
    setupServerInfo(&servaddr, PORT_REALTIME);

    // Bind the socket to the address
    bindSocket(sockfd, &servaddr);

    //Start running
    receiveRealtimeControl(sockfd);

    return NULL;
}


void startRealtimeControlServer() {
    pthread_create(&serverRealtime, NULL, runRealtimeControl, NULL);
    if (verbose) {
        printf("[INFO] Started thread for realtime control with TID %lu\n", serverRealtime);
    }
}
//endregion