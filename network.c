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

bool running = true;
pthread_t server;

void* UDPSocketServer(void* vargp) {

    // Creating socket file descriptor
    int sockfd;
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("[ERROR] Socket creation failed\n");
        exit(EXIT_FAILURE);
    } else {
        if (verbose){
            printf("[INFO] Created socket on domain %d and type %d\n", AF_INET, SOCK_DGRAM);
        }
    }

    //Setting up address structures
    struct sockaddr_in serverAddress;
    memset(&serverAddress, 0, sizeof(serverAddress));

    struct sockaddr_in clientAddress;
    memset(&clientAddress, 0, sizeof(clientAddress));

    // Filling server information
    serverAddress.sin_family = AF_INET; // IPv4
    serverAddress.sin_addr.s_addr = INADDR_ANY;
    serverAddress.sin_port = htons(PORT);

    // Bind the socket with the server address
    if (bind(sockfd, (const struct sockaddr*) &serverAddress, sizeof(serverAddress)) < 0) {
        perror("[ERROR] Binding port failed\n");
        exit(EXIT_FAILURE);
    } else {
        if (verbose){
            printf("[INFO] Bound socket to port %d\n", PORT);
        }
    }

    char buffer[DATAGRAM_SIZE_LIMIT];
    while (running) {
        //Receiving string via UDP socket
        unsigned int size = sizeof(clientAddress);
        long n = recvfrom(sockfd, (char*) buffer, DATAGRAM_SIZE_LIMIT, MSG_WAITALL, (struct sockaddr*) &clientAddress,
                          &size);
        buffer[n] = '\0';

        //Determine playback style
        bool immediately = buffer[0] == 'I';

        //Cut off playback style from path
        unsigned long length = strlen(buffer);
        char* path = malloc(sizeof(char) * length - 2);
        memcpy(path, &buffer[2], length - 2);
        path[length - 2] = '\0';

        //Forming answer
        char answer[DATAGRAM_SIZE_LIMIT];
        if (immediately) {
            //sprintf(answer, "[INFO] Playing [%s] now!\n", path);
            sprintf(answer, "[INFO] Immediate playback is not yet supported!\n");
            if (verbose){
                printf("%s", answer);
            }

            //todo: play now
        } else {
            if (addToStack(path)) {
                sprintf(answer, "[INFO] Successfully added (%s) to animation stack\n", path);
            } else {
                sprintf(answer, "[WARNING] Failed to add (%s) to animation stack. Stack most likely full\n", path);
            }
        }

        //Sending answer back
        sendto(sockfd, answer, strlen(answer), MSG_CONFIRM, (const struct sockaddr*) &clientAddress, size);
    }

    if (verbose){
        printf("[INFO] Shutting down UDP socket server on port %d\n", PORT);
    }
    return NULL;
}

void startServer() {
    pthread_create(&server, NULL, UDPSocketServer, NULL);
    if (verbose){
        printf("[INFO] Started thread for UDP server with TID %lu\n", server);
    }
}
