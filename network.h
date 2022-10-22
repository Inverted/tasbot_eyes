#ifndef TASBOT_EYES_NETWORK_H
#define TASBOT_EYES_NETWORK_H

#include <stdbool.h>
#include "utils.h"
#include "filesystem.h"

#define PORT        8080
#define MAX_ANSWER_LENGTH   64
#define DATAGRAM_SIZE_LIMIT   (MAX_PATH_LENGTH+MAX_ANSWER_LENGTH)

extern bool running;

struct reqAni_t{
    bool immediately;
    string_t path;
} reqAni;

void* UDPSocketServer(void* vargp);

void processRequest(string_t* _message);

void startServer();

#endif //TASBOT_EYES_NETWORK_H
