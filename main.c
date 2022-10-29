/*! \file */
#include <gif_lib.h> //https://sourceforge.net/projects/giflib/
#include <ws2811/ws2811.h> //https://github.com/jgarff/rpi_ws281x

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <signal.h>
#include <unistd.h>
#include <sysexits.h>
#include <sys/mman.h>

#include "led.h"
#include "arguments.h"
#include "tasbot.h"
#include "palette.h"
#include "filesystem.h"
#include "network.h"

#define COLOR_FADE_SPEED        100

void finish() {
    if (huePid != 0) { //exclude child
        printf("\n"); //pretty uwu

        running = false;
        if (activateLEDModule) {
            clearLEDs();
            ws2811_render(&display);
            ws2811_fini(&display);
        }
        free(pixel); //did this for good measurement, but I guess since next command is exit, this is unnecessary, since complete process memory get freed anyway?

        if (huePid > 0) { //catch failed forks
            kill(huePid, SIGTERM);
        }
    }
    exit(EX_OK);
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

/**
 * Setup child for calculating current hue
 * todo: screw this
 */
void initRainbowMode(){
    if (rainbowMode && useRandomColors || rainbowMode && useRandomColorsForAll) {
        hue = mmap(NULL, sizeof(*hue), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
        *hue = 0;

        huePid = fork();
        if (huePid == 0) { // Child process
            while (true) {
                if (*hue < 256) {
                    *hue = *hue + 1;
                } else {
                    *hue = 0;
                }
                usleep(1000 * COLOR_FADE_SPEED);
            }
        } else if (huePid < 0) {
            printf("[ERROR] Can't fork to start child for hue calculation! Revert back to deactivating rainbow mode!\n");
            rainbowMode = false;
        }
        //Parent process continues
    } else if (rainbowMode) {
        printf("[WARNING] Rainbow mode can only be used with -c or -a. Turning it off again!\n");
        rainbowMode = false;
    }
}

void initPalette(){
    if (pathForPalette != NULL) {
        string_t path;
        initstr(&path, pathForPalette);
        readPalette(&path);
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

void initBlinking(){
    minTimeBetweenBlinks *= 1000;
    maxTimeBetweenBlinks *= 1000;
}

void specificAnimation(){
    if (specificAnimationToShow != NULL) {
        string_t path;
        initstr(&path, specificAnimationToShow);
        while (running) {
            showExpressionFromFilepath(&path, false, false);
        }
        //finish(); //should not be mandatory, because running should be false at this point, so main loop gets skippe completely
    }
}

void tasbotsEyes(){
    bool firstIteration = true;
    while (running) {
        //skip to base expression on first iteration, to not start on a random animation
        if (!firstIteration) {
            string_t path;
            initstr(&path, pathForAnimations);
            showRandomExpression(&path, useRandomColors, true);
        } else {
            if (!skipStartupAnimation) {
                string_t path;
                initstr(&path, STARTUP_PATH);
                showExpressionFromFilepath(&path, false, false);
            }
            firstIteration = false;
        }

        //skip base expression, when no blinks at all
        if (maxBlinks != 0 && minTimeBetweenBlinks != 0) {
            string_t path;
            initstr(&path, BASE_PATH);
            showExpressionFromFilepath(&path, false, false);
        }

        usleep(getBlinkDelay() * 1000);
        //blink for a random amount of times
        for (unsigned int blinks = getBlinkAmount(); blinks > 0; --blinks) {
            showBlinkExpression();
            string_t path;
            initstr(&path, BASE_PATH);
            showExpressionFromFilepath(&path, false, false);

            unsigned int blinkTime = getBlinkDelay();
            if (verboseLogging) {
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
    initRainbowMode();
    initPalette();
    initBlinking();
    initLEDs();

    //Option for playing give specific animation
    //specificAnimation();

    //Main loop
    //tasbotsEyes();


    //this works
    string_t path;
    initstr(&path, STARTUP_PATH);
    showExpressionFromFilepath(&path, false, false);


    //this ain't working
    string_t path2;
    initstr(&path2, pathForAnimations);
    showRandomExpression(&path2, false, true);

    //Clean up
    finish();
    return 0;
}