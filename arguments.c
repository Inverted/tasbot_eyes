#include "arguments.h"
#include "tasbot.h"
#include "led.h"
#include "palette.h"
#include "filesystem.h"

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <ctype.h>

bool verboseLogging = false;
bool consoleRenderer = false;
bool skipStartupAnimation = false;
char* specificAnimationToShow = NULL;

//Debug
bool activateLEDModule = true;
bool realTASBot = true;

/**
 * Parse the program arguments from the console line
 */
void parseArguments(int _argc, char** _argv) {
    int c;
    while ((c = getopt(_argc, _argv, "XhvgwruacDd:b:s:B:i:p:z:P:C:R:")) != -1) {
        switch (c) {
            case 'h':
                printHelp();
                exit(EXIT_SUCCESS);
                break;
            case 'v':
                verboseLogging = true;
                printf("[INFO] Use verbose logging\n");
                break;
            case 'r':
                consoleRenderer = true;
                printf("[INFO] Use console renderer\n");
                break;
            case 'g':
                useGammaCorrection = true;
                printf("[INFO] Use gamma correction\n");
                break;
            case 'D':
                playbackSpeedAffectBlinks = true;
                printf("[INFO] Playback speed will affect blink delay\n");
                break;
            case 'X':
                realTASBot = false;
                printf("[INFO] SECRET NOT TASBot MODE. FOR DEBUGGING ONLY!\n");
                break;
            case 'u':
                skipStartupAnimation = true;
                printf("[INFO] Skip startup animation\n");
                break;

            case 'd': {
                int pin = (int) strtol(optarg, NULL, 10);

                if (pin > 27 || pin < 2) {
                    fprintf(stderr, "[ERROR] GPIO pin %d doesnt exist. Pin ID must be between 2 and 27\n", pin);
                    abort();
                }

                dataPin = pin;
                printf("[INFO] Set data pin to GPIO pin %d\n", pin);
                break;
            }

            case 'c':
                useRandomColors = true;
                printf("[INFO] Use random color\n");
                break;

            case 'a':
                useRandomColorsForAll = true;
                printf("[INFO] Use random color, also for blinks and the base\n");
                break;

            case 'w':
                rainbowMode = true;
                printf("[INFO] Rainbow mode activated!\n");
                break;

            case 'C': {
                defaultColor = strtocol(optarg);

                printf("[INFO] Set color to #%06x\n", defaultColor);
                break;
            }

            case 'b': {
                int bright = (int) strtol(optarg, NULL, 10);

                if (bright > 255) {
                    printf("[WARNING] Brightness given (%d) higher than 255. Gonna use 255\n", bright);
                    bright = 255;
                } else if (bright < 0) {
                    printf("[WARNING] Brightness given (%d) smaller than 0. Gonna use 0\n", bright);
                    bright = 0;
                }

                brightness = bright;
                printf("[INFO] Set bright to %d\n", brightness);
                break;
            }

            case 'R': {
                int reps = (int) strtol(optarg, NULL, 10);

                if (reps < 1) {
                    printf("[WARNING] Repetitions given (%d) smaller then 1. Gonna use 1\n", reps);
                    reps = 1;
                }

                repetitions = reps;
                printf("[INFO] Set repetitions to %d\n", repetitions);
                break;
            }

            case 's': {
                float speed = strtof(optarg, NULL);

                if (speed > 0) {
                    playbackSpeed = speed;
                    printf("[INFO] Set playback speed to %f\n", speed);
                } else {
                    fprintf(stderr,
                            "[ERROR] Can't use %f as playback speed. Please use a playback speed bigger than 0\n",
                            speed);
                    abort();
                }
                break;
            }

            case 'B': {
                int blinks = optarg[0] - '0';
                int min = optarg[2] - '0';
                int max = optarg[4] - '0';

                if (blinks < 0 || blinks > 9 || min < 0 || min > 9 || max < 0 || max > 9) {
                    fprintf(stderr, "[ERROR] Invalid pattern (%s). Please check pattern\n", optarg);
                    abort();
                } else if (min > max) {
                    printf("[WARNING] Faulty pattern (%s). The minimum seconds between blinks can't be bigger than then the maximum seconds. Going to flip them!\n",
                           optarg);
                    minTimeBetweenBlinks = max;
                    maxTimeBetweenBlinks = min;
                } else {
                    minTimeBetweenBlinks = min;
                    maxTimeBetweenBlinks = max;
                }
                maxBlinks = blinks;
                printf("[INFO] Set blink pattern to %d-%d-%d\n", maxBlinks, minTimeBetweenBlinks, maxTimeBetweenBlinks);
                break;
            }

            case 'p': {
                if (checkIfDirectoryExist(optarg)) {
                    pathForAnimations = optarg;
                    printf("[INFO] Use animations from \"%s\"\n", optarg);
                } else {
                    fprintf(stderr, "[ERROR] Can't open directory \"%s\"\n", optarg);
                    abort();
                }
                break;
            }

            case 'z': {
                if (checkIfDirectoryExist(optarg)) {
                    pathForBlinks = optarg;
                    printf("[INFO] Use blink animations from \"%s\"\n", optarg);
                } else {
                    fprintf(stderr, "[ERROR] Can't open directory \"%s\"\n", optarg);
                    abort();
                }
                break;
            }

            case 'i': {
                if (checkIfFileExist(optarg)) {
                    specificAnimationToShow = optarg;
                    printf("[INFO] Use specific animation \"%s\"\n", optarg);
                } else {
                    fprintf(stderr, "[ERROR] Can't open file \"%s\"\n", optarg);
                    abort();
                }
                break;
            }

            case 'P': {
                if (checkIfFileExist(optarg)) {
                    pathForPalette = optarg;
                    printf("[INFO] Set color palette to \"%s\"\n", optarg);
                } else {
                    fprintf(stderr, "[ERROR] Can't open file \"%s\"\n", optarg);
                    abort();
                }
                break;
            }

            case '?':
                if (optopt == 'b' || optopt == 's' || optopt == 'B' || optopt == 'i' || optopt == 'p' ||
                    optopt == 'z' || optopt == 'P') {
                    fprintf(stderr, "Argument -%c requires an argument.\n", optopt);
                } else if (isprint (optopt)) {
                    fprintf(stderr, "Unknown argument `-%c'. Use -h for more information\n", optopt);
                } else {
                    fprintf(stderr, "Unknown argument character `\\x%x'.\n", optopt);
                }

            default:
                abort();
        }
    }

    for (int index = optind; index < _argc; index++) {
        printf("Non-option argument %s\n", _argv[index]);
    }
}

/**
 * Print a help dialog to the console
 */
void printHelp() {
    printf("===[Debug options]===\n");
    printf("-h               Print this help screen\n");
    printf("-v               Enable verbose logging\n");
    printf("-r               Enable console renderer for frames\n");
    printf("-d [GPIO]        Change GPIO data pin. Possible options are between 2 to 27. Default is 10\n");
    printf("-g               Use gamma correction. DONT USE, IT'S BROKEN!\n");

    printf("\n===[Tune animation playback]===\n");
    printf("-c               Use random color from palette for monochrome animations\n");
    printf("-a               Use random color from palette for monochrome animations as well as blinks and the base\n");
    printf("-w               Activate rainbow mode. Needs to be used with -c or -a to define scope\n");
    printf("-C [xxxxxx]      Default color that should be used for not colored animations\n");
    printf("-b [0-255]       Set maximum possible brightness. Default is 24\n");
    printf("-s [MULTIPLIER]  Sets Playback speed. Needs to be bigger than 0\n");
    printf("-D               Let the playback speed affect blink delay\n");
    printf("-u               Skip the startup animation\n");
    printf("-R               Set how many times a animation should be repeated. Default is 1\n");
    printf("-B [PATTERN]     Controls the blinks. Highest number that can be used within the pattern is 9\n");
    printf("                 -1st: Maximum number of blinks between animations\n");
    printf("                 -2nd: Minimum seconds between blinks\n");
    printf("                 -3rd: Maximum seconds between blinks\n");
    printf("                 Example: \"4-4-6\" (default)\n");
    printf("                          -Maximum of 4 blinks between animations\n");
    printf("                          -4 to 6 seconds between each blink\n");

    printf("\n===[File arguments]===\n");
    printf("-p [FOLDER PATH] Play animations from a specific folder.\n");
    printf("-z [FOLDER PATH] Play blink animation from specific folder.\n");
    printf("-i [FILE PATH]   Play specific animation as endless loop. \"-p\" and \"-z\" become useless with this.\n");
    printf("-P [FILE PATH]   Use color palette from text file. For formatting of palette file use tool or see example.\n");

    printf("\n===[Hints]===\n");
    printf("- To bring TASBot in a state, where he is only blinking, execute with argument \"-p ./gifs/blinks/\". This will narrow all possible options for animations down to blinking ones, while keeping the support for blink patterns and the usual appearance. To further improve appearance, don't use with -c option.\n\n");
    printf("- Use different working directories as \"profiles\". This way, you could feature different sets of animations with different base frames if needed.\n");
}
