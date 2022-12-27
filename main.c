/*! \file */
#include <gif_lib.h> //https://sourceforge.net/projects/giflib/
#include <ws2811/ws2811.h> //https://github.com/jgarff/rpi_ws281x

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <signal.h>
#include <unistd.h>
#include <pthread.h>

#include "led.h"
#include "arguments.h"
#include "tasbot.h"
#include "palette.h"
#include "filesystem.h"
#include "network.h"
#include "stack.h"

void finish() {
    printf("\n"); //pretty uwu

    runningInject = false;
    if (activateLEDModule) {
        clearLEDs();
        ws2811_render(&display);
        ws2811_fini(&display);
    }

    while (!isEmpty()) {
        char* file = (char*) pop();
        free(file);
    }

    free(buffer);
    free(palette);

    pthread_kill(serverInject, SIGKILL);
    pthread_exit(NULL);

    exit(EXIT_SUCCESS);
}

/**
 * Central handler for leaving the application. Handler for SIGINT, SIGTERM and SIGKILL. Gets also called after the endless loop
 * @param _number uwu
 */
void exitHandler(int _number) {
    finish();
}

/**
 * Register the handler for SIGINT, SIGTERM and SIGKILL
 */
void setupHandler() {
    struct sigaction sa = {.sa_handler = exitHandler,};
    sigemptyset(&sa.sa_mask);
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
    sigaction(SIGKILL, &sa, NULL);
}

void initPalette() {
    if (pathForPalette != NULL) {
        readPalette(pathForPalette);
    } else {
        //Default palette
        paletteCount = 8;
        palette = malloc(sizeof(ws2811_led_t) * paletteCount);
        if (!palette) {
            fprintf(stderr, "[ERROR] Failed to allocate memory for palette");
            clearLEDs();
            exit(EXIT_FAILURE);
        }
        palette[0] = 0xFF0000; // red
        palette[1] = 0xFF8000; // orange
        palette[2] = 0xFFFF00; // yellow
        palette[3] = 0x00FF00; // green
        palette[4] = 0x0000FF; // cyan
        palette[5] = 0xFF0000; // blue
        palette[6] = 0xFF00FF; // magenta
        palette[7] = 0xFF80FF; // pink
    }
}

void initBlinking() {
    minTimeBetweenBlinks *= 1000;
    maxTimeBetweenBlinks *= 1000;
}

void specificAnimation() {
    if (specificAnimationToShow != NULL) {
        while (runningInject) {
            playAnimationFromFilepath(specificAnimationToShow, false, false);
        }
    }
}

void tasbotsEyes() {
    bool firstIteration = true;
    while (runningInject) {

        //play startup animation in first iteration
        if (!firstIteration) {
            if (isEmpty()) {
                fillStack(pathForAnimations);
            }

            //Get animation from stack
            char* file = (char*) pop();

            //Play animation
            playAnimationFromFilepath(file, useRandomColors, true);

            free(file);

        } else {
            if (!skipStartupAnimation) {
                playAnimationFromFilepath(STARTUP_PATH, false, false);
            }
            firstIteration = false;
        }

        //show base
        if (maxBlinks != 0 && minTimeBetweenBlinks != 0) { //skip base, when no blinks at all
            playAnimationFromFilepath(BASE_PATH, false, false);
        }

        //blink for a random amount of times
        usleep(getBlinkDelay() * 1000);
        for (unsigned int blinks = getBlinkAmount(); blinks > 0; --blinks) {
            playRandomAnimationFromDirectory(pathForBlinks, false, false);
            playAnimationFromFilepath(BASE_PATH, false, false);

            unsigned int blinkTime = getBlinkDelay();
            if (verbose) {
                printf("[INFO] Blink #%d for %d milliseconds \n", blinks, blinkTime);
            }
            usleep(blinkTime * 1000);
        }
    }
}

int main(int _argc, char** _argv) {
    srand(time(NULL));

    //can't use LED hardware on desktops
#if defined(__x86_64__)
    activateLEDModule = false;
#endif

    //Init things
    setupHandler();
    parseArguments(_argc, _argv);
    initPalette();
    initBlinking();
    initLEDs();
    startAnimationInjectionServer();

    if (useRealtimeControl){
        startRealtimeControlServer();
    }

    //Option for playing a given specific animation
    specificAnimation();

    //Main loop
    tasbotsEyes();

    //Clean up
    finish();
    return 0;
}
