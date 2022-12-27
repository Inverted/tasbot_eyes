#ifndef TASBOT_EYES_NETWORK_H
#define TASBOT_EYES_NETWORK_H

#include <stdbool.h>
#include <pthread.h>
#include <netinet/in.h>

#include "utils.h"
#include "filesystem.h"

#define PORT_INJECTION                  8080
#define MAX_ANSWER_LENGTH               64
#define DATAGRAM_SIZE_LIMIT_INJECTION   (MAX_PATH_LENGTH+MAX_ANSWER_LENGTH)

#define PORT_REALTIME                   19446
#define DATAGRAM_SIZE_LIMIT_REALTIME    182
#define SLEEP_REALTIME                  35 //ms, to ease the hardware

//general
extern bool running;

int getSocketFD();
void setupServerInfo(struct sockaddr_in* _server, int _port);
void bindSocket(int _sockfd, struct sockaddr_in* _server);

//injection
extern pthread_t serverInject;

void receiveAnimationInjection(int sockfd);
void* runAnimationInjection(void* vargp);
void startAnimationInjectionServer();

//realtime
extern pthread_t serverRealtime;

void receiveRealtimeControl(int sockfd);
void* runRealtimeControl(void* vargp);
void startRealtimeControlServer();

#endif //TASBOT_EYES_NETWORK_H
