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

void* UDPSocketServer(void* vargp) {
    int sockfd;
    char buffer[DATAGRAM_SIZE_LIMIT];
    struct sockaddr_in serverAddress;
    struct sockaddr_in clientAddress;

    // Creating socket file descriptor
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("[ERROR] Socket creation failed\n");
        exit(EXIT_FAILURE);
    } else {
        printf("[INFO] Created socket on domain %d and type %d\n", AF_INET, SOCK_DGRAM);
    }

    memset(&serverAddress, 0, sizeof(serverAddress));
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
        printf("[INFO] Bound socket to port %d\n", PORT);
    }

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

        printf("received %s\n", path);

        //Forming answer
        char answer[DATAGRAM_SIZE_LIMIT];
        if (immediately) {
            sprintf(answer, "[INFO] Playing [%s] now!\n", path);
            //todo: play now
        } else {
            if (addToStack(path)) {
                sprintf(answer, "[INFO] Successfully added (%s) to animation stack\n", path);
            } else {
                sprintf(answer, "[WARNING] Failed to add (%s) to animation stack. Stack most likely full\n", path);
            }
        }

        //Printing out and sending back answer
        printf("%s", answer);
        sendto(sockfd, answer, strlen(answer), MSG_CONFIRM, (const struct sockaddr*) &clientAddress, size);
    }

    printf("[INFO] Shutting down UDP socket server on port %d\n", PORT);
    return NULL;
}

void startServer() {
    pthread_t server;
    pthread_create(&server, NULL, UDPSocketServer, NULL);
    if (verbose){
        printf("[INFO] Started thread for UDP server with TID %lu\n", server);
    }
}