#ifndef TASBOT_EYES_ARGUMENTS_H
#define TASBOT_EYES_ARGUMENTS_H

#include <stdbool.h>

extern bool verboseLogging;
extern bool consoleRenderer;
extern bool skipStartupAnimation;
extern char* specificAnimationToShow;

//Debug
extern bool activateLEDModule;
extern bool realTASBot;

void parseArguments(int _argc, char** _argv);
void printHelp();

#endif //TASBOT_EYES_ARGUMENTS_H
