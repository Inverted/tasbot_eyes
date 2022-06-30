/*! \file */
#include <gif_lib.h> //https://sourceforge.net/projects/giflib/
#include <ws2811/ws2811.h> //https://github.com/jgarff/rpi_ws281x

#include <stdio.h>
#include <dirent.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <ctype.h>
#include <sysexits.h>
#include <sys/stat.h>

#define BASE_PATH               "./gifs/base.gif"
#define STARTUP_PATH            "./gifs/startup.gif"
#define OTHER_PATH              "./gifs/others/"
#define BLINK_PATH              "./gifs/blinks/"
#define IMMEDIATE_ANIM_PATH     "./gifs/PLAYMENOW/"
#define OVERRIDE_FILE_PATH      "./gifs/OVERRIDE/gif.gif"
#define MAX_FILENAME_LENGTH     256
#define MAX_PATH_LENGTH         4096
#define MIN_DELAY_TIME          35                      //Smallest delay time that is possible due to hardware limitations (1000ms/30fps=33.3'ms)
#define DEFAULT_DELAY_TIME      100
#define MAX_BLINKS              4                       //How many times does TASBot blink between animations
#define MIN_TIME_BETWEEN_BLINKS 4                       //Based on human numbers. We Blink about every 4 to 6 seconds
#define MAX_TIME_BETWEEN_BLINKS 6

#define LED_HEIGHT              8
#define LED_WIDTH               28

#define TARGET_FREQ             WS2811_TARGET_FREQ      //800000 Kbps
#define GPIO_PIN                10
#define DMA                     10
#define LED_COUNT               (LED_WIDTH * LED_HEIGHT)
#define STRIP_TYPE              SK6812_STRIP            //TASBot
#define INVERTED                false
#define BRIGHTNESS              4

typedef struct AnimationFrame {
    GifColorType* color[LED_WIDTH][LED_HEIGHT];
    u_int16_t delayTime;
} AnimationFrame;

typedef struct Animation {
    AnimationFrame** frames; //pointer to a pointer, that's an array of frames. pain.
    int frameCount;
    bool monochrome;
    GifFileType* image; //needed for very dirty trick to get around weird behavior, where DGifCloseFile() manipulates the animation data for some reason
} Animation;

//region Declarations
bool running = true;
ws2811_led_t* pixel;
ws2811_t display;
ws2811_led_t* palette;
unsigned int paletteCount;

//Argument variables
bool verboseLogging = false;
bool consoleRenderer = false;
bool useRandomColors = false;
bool useRandomColorsForAll = false;
bool playbackSpeedAffectBlinks = false;
bool useGammaCorrection = false;
bool skipStartupAnimation = false;
char* specificAnimationToShow = NULL;
char* pathForAnimations = OTHER_PATH;
char* pathForBlinks = BLINK_PATH;
char* pathForPalette = NULL;
int brightness = BRIGHTNESS;
int dataPin = GPIO_PIN;
int maxBlinks = MAX_BLINKS;
int minTimeBetweenBlinks = MIN_TIME_BETWEEN_BLINKS;
int maxTimeBetweenBlinks = MAX_TIME_BETWEEN_BLINKS;
int repetitions = 1;
float playbackSpeed = 1;
ws2811_led_t defaultColor = -1;

//Signals
void setupHandler();
void finish(int _number);

//Arguments
void parseArguments(int _argc, char** _argv);
void printHelp();

//GIF
char* getRandomAnimation(char* list[], int _count);
bool checkIfImageHasRightSize(GifFileType* _image);
bool isGrayScale(GifColorType* _color);
AnimationFrame* readFramePixels(const SavedImage* frame, ColorMapObject* _globalMap, bool* _monochrome);
Animation* readAnimation(char* _file);

//LEDs
ws2811_return_t initLEDs();
ws2811_return_t renderLEDs();
ws2811_return_t clearLEDs();
ws2811_led_t translateColor(GifColorType* _color, bool _useGammaCorrection);

//TASBot
void showBlinkExpression();
void showRandomExpression(char* _path, bool _useRandomColor, bool _repeatAnimations);
void showExpressionFromFilepath(char* _filePath, bool _useRandomColor, bool _repeatAnimations);
void playExpression(Animation* _animation, bool _useRandomColor, bool _repeatAnimations);
void showFrame(AnimationFrame* _frame, ws2811_led_t _color);
void freeAnimation(Animation* _animation);
unsigned int getBlinkDelay();
unsigned int getBlinkAmount();
float getLuminance(GifColorType* _color);

//Palette
int chtohex(char _ch);
ws2811_led_t strtocol(char* _color);
void readPalette(char* _path);

//File system operations
bool getFileList(const char* _path, char* _list[]);
bool checkIfFileExist(char* _file);
bool checkIfDirectoryExist(char* _path);
int countFilesInDir(char* _path);
int countLines(const char* _path);
char* getFilePath(char* _path, char* _file);
void readFile(const char* _path, int _count, char** _out);

//Development
unsigned int ledMatrixTranslation(int _x, int _y);
bool numberIsEven(int _number);

//delay(): Behaves like usleep(), but polls OVERRIDE_FILE_PATH in the meantime.
//If that GIF exists, immediately show it.
void delay(useconds_t usec) {
		for(useconds_t elapsed=0; elapsed<usec; elapsed+=10 * 1000) {
			usleep(10 * 1000);
			struct stat buf;
			//TODO: Properly check that this is indeed a GIF. Or does showExpressionFromFilepath handle that safely?
			while(!stat(OVERRIDE_FILE_PATH, &buf)) {
				showExpressionFromFilepath(OVERRIDE_FILE_PATH, false, false);
			}
		}
}

//Development function toggles
bool activateLEDModule = true;
bool realTASBot = true;
//endregion

//TASBot display conversation table
//Based on https://github.com/jakobrs/tasbot-display/blob/master/src/tasbot.rs
int TASBotIndex[8][28] = {
        {-1, -1, 0,  1,  2,  3,  -1, -1, -1, -1, 101, 100, 99, 98, 97, 96, 95, 94, -1, -1, -1,  -1,  105, 104, 103, 102, -1,  -1},
        {-1, 4,  5,  6,  7,  8,  9,  -1, -1, 84, 85,  86,  87, 88, 89, 90, 91, 92, 93, -1, -1,  111, 110, 109, 108, 107, 106, -1},
        {10, 11, 12, 13, 14, 15, 16, 17, -1, -1, -1,  -1,  -1, -1, -1, -1, -1, -1, -1, -1, 119, 118, 117, 116, 115, 114, 113, 112},
        {18, 19, 20, 21, 22, 23, 24, 25, -1, -1, -1,  83,  82, 81, 80, 79, 78, -1, -1, -1, 127, 126, 125, 124, 123, 122, 121, 120},
        {26, 27, 28, 29, 30, 31, 32, 33, -1, -1, 70,  71,  72, 73, 74, 75, 76, 77, -1, -1, 135, 134, 133, 132, 131, 130, 129, 128},
        {34, 35, 36, 37, 38, 39, 40, 41, -1, -1, -1,  -1,  -1, -1, -1, -1, -1, -1, -1, -1, 143, 142, 141, 140, 139, 138, 137, 136},
        {-1, 42, 43, 44, 45, 46, 47, -1, -1, -1, 68,  67,  66, 65, 64, 63, 62, 61, -1, -1, -1,  149, 148, 147, 146, 145, 144, -1},
        {-1, -1, 48, 49, 50, 51, -1, -1, -1, 69, 52,  53,  54, 55, 56, 57, 58, 59, 60, -1, -1,  -1,  153, 152, 151, 150, -1,  -1}
};

//Gamma correction table
//From https://learn.adafruit.com/led-tricks-gamma-correction/the-quick-fix
//Article written by Phillip Burgess
const uint8_t gamma8[256] = {
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  1,  1,  1,
    1,  1,  1,  1,  1,  1,  1,  1,  1,  2,  2,  2,  2,  2,  2,  2,
    2,  3,  3,  3,  3,  3,  3,  3,  4,  4,  4,  4,  4,  5,  5,  5,
    5,  6,  6,  6,  6,  7,  7,  7,  7,  8,  8,  8,  9,  9,  9, 10,
    10, 10, 11, 11, 11, 12, 12, 13, 13, 13, 14, 14, 15, 15, 16, 16,
    17, 17, 18, 18, 19, 19, 20, 20, 21, 21, 22, 22, 23, 24, 24, 25,
    25, 26, 27, 27, 28, 29, 29, 30, 31, 32, 32, 33, 34, 35, 35, 36,
    37, 38, 39, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 50,
    51, 52, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 66, 67, 68,
    69, 70, 72, 73, 74, 75, 77, 78, 79, 81, 82, 83, 85, 86, 87, 89,
    90, 92, 93, 95, 96, 98, 99,101,102,104,105,107,109,110,112,114,
    115,117,119,120,122,124,126,127,129,131,133,135,137,138,140,142,
    144,146,148,150,152,154,156,158,160,162,164,167,169,171,173,175,
    177,180,182,184,186,189,191,193,196,198,200,203,205,208,210,213,
    215,218,220,223,225,228,231,233,236,239,241,244,247,249,252,255
};

int main(int _argc, char** _argv) {
    //can't use LED hardware on desktops
#if defined(__x86_64__)
    activateLEDModule = false;
#endif

    //Init program
    srand(time(NULL));
    setupHandler();
    parseArguments(_argc, _argv);

    //Init palette
    if (pathForPalette != NULL) {
        readPalette(pathForPalette);
    } else {
        //Default palette
        paletteCount = 8;
        palette = malloc(sizeof(ws2811_led_t) * paletteCount);
				if (!palette) {
					fprintf(stderr, "[ERROR] Failed to allocate palette memory");
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

    //Init blink times
    minTimeBetweenBlinks *= 1000;
    maxTimeBetweenBlinks *= 1000;

    //Init LEDS
    if (activateLEDModule) {
        ws2811_return_t r = initLEDs();
        if (r != WS2811_SUCCESS) {
            fprintf(stderr, "[ERROR] Can't run program. Did you started it with root privileges?\n");
            return r;
        }
    }

    //Option for playing give specific animation
    if (specificAnimationToShow != NULL) {
        while (running) {
            showExpressionFromFilepath(specificAnimationToShow, false, false);
        }
        return 0;
    }

    //Main loop
    bool firstIteration = true;
    while (running) {
        //skip to base expression on first iteration, to not start on a random animation
        if (!firstIteration) {
            showRandomExpression(pathForAnimations, useRandomColors, true);
        } else {
            if (!skipStartupAnimation){
                showExpressionFromFilepath(STARTUP_PATH, false, false);
            }
            firstIteration = false;
        }

        //skip base expression, when no blinks at all
        if (maxBlinks != 0 && minTimeBetweenBlinks != 0) {
            showExpressionFromFilepath(BASE_PATH, false, false);
        }

        delay(getBlinkDelay() * 1000);
        //blink for a random amount of times
        for (unsigned int blinks = getBlinkAmount(); blinks > 0; --blinks) {
            showBlinkExpression();
            showExpressionFromFilepath(BASE_PATH, false, false);

            unsigned int blinkTime = getBlinkDelay();
            if (verboseLogging) {
                printf("[INFO] Blink #%d for %d milliseconds \n", blinks, blinkTime);
            }
            delay(blinkTime * 1000);

						//Check for immediate animation to play back
					  DIR *immediate_anim_dir = opendir(IMMEDIATE_ANIM_PATH);
						struct dirent *dir;
						if(immediate_anim_dir) {
							struct dirent *dir;
							do {
								dir = readdir(immediate_anim_dir);
								if(dir && strlen(dir->d_name) > 4 && !strcmp(dir->d_name + strlen(dir->d_name) - 4, ".gif")) {
									char pathinter[PATH_MAX+1];
									sprintf(pathinter, "%s/%s", IMMEDIATE_ANIM_PATH, dir->d_name);
									char path[PATH_MAX+1];
									realpath(pathinter, path);
									Animation* animation = readAnimation(path);
							    playExpression(animation, false, false);
									remove(path);
									blinks++;
									break;
								}
							} while(dir);
						}
        }

    }

    //Clean up
    finish(0);
    return 0;
}

//region Arguments
/**
 * Parse the program arguments from the console line
 */
void parseArguments(int _argc, char** _argv) {
    int c;
    while ((c = getopt(_argc, _argv, "XhvgruacDd:b:s:B:i:p:z:P:C:R:")) != -1) {
        switch (c) {
            case 'h':
                printHelp();
                exit(EX_OK);
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

                if (blinks < 0 || blinks > 9 ||
                    min < 0 || min > 9 ||
                    max < 0 || max > 9) {
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
    printf("- To bring TASBot in a state, where he is only blinking, execute with argument \"-p ./gifs/blinks/\". This will narrow all possible options for animations down to blinking ones, while keeping the support for blink patterns and the usual appearance. To further improve appearance, don't use with -c option.\n");
    printf("\n- Use different working directories as \"profiles\". This way, you could feature different sets of animations with different base frames if needed.\n");
}
//endregion

//region Signal
/**
 * Register the handler for SIGINT, SIGTERM and SIGKILL
 */
void setupHandler() {
    struct sigaction sa = {
            .sa_handler = finish,
    };
    sigemptyset(&sa.sa_mask);
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
    sigaction(SIGKILL, &sa, NULL);
}

/**
 * Central handler for leaving the application. Handler for SIGINT, SIGTERM and SIGKILL. Gets also called after the endless loop
 * @param _number uwu
 */
void finish(int _number) {
    printf("\n"); //pretty uwu

    running = false;
    if (activateLEDModule) {
        clearLEDs();
        ws2811_render(&display);
        ws2811_fini(&display);
    }
    free(pixel); //did this for good measurement, but I guess since next command is exit, this is unnecessary, since complete process memory get freed anyway?
    exit(EX_OK);
}
//endregion

//region GIF
/**
 * check if GIF has the exact right size of 28x8 pixels
 * @param _image The GIF that is to check
 * @return If image has right size or not
 */
bool checkIfImageHasRightSize(GifFileType* _image) {
    return (_image->SWidth == LED_WIDTH && _image->SHeight == LED_HEIGHT);
}

/**
 * Opens and reads the GIF file in a data structure. Container for AnimationFrames and various other needed infos
 * @param _file The GIF animation, that is to read
 * @return Data structure, that contains all needed information to display it on TASBots display
 * TODO: This is pretty blob like. Can probably be shorten.
 */

Animation* readAnimation(char* _file) {
    if (verboseLogging) {
        printf("[INFO] Load file %s\n", _file);
    }

    //Open _file
    int e;
    GifFileType* image = DGifOpenFileName(_file, &e);
    if (!image) {
        fprintf(stderr, "[ERROR] EGifOpenFileName() failed. Couldn't find or open _file: %d\n", e);
        return false;
    }

    //"Slurp" infos into struct
    if (DGifSlurp(image) == GIF_ERROR) {
        fprintf(stderr, "[ERROR] DGifSlurp() failed. Couldn't load infos about GIF: %d\n", image->Error);
        DGifCloseFile(image, &e);
        return false;
    }

    Animation* animation = NULL;
    //Before we do anything, lets check if image has right size
    if (checkIfImageHasRightSize(image)) {

        //Obtain global color map if available
        ColorMapObject* globalColorMap = image->SColorMap;
        if (verboseLogging) {
            printf("[INFO] (Image info): Size: %ix%i; Frames: %i; Path: \"%s\"\n",
                   image->SWidth, image->SHeight, image->ImageCount, _file);
        }

        //Process frames
        animation = malloc(sizeof(Animation));
				if (!animation) {
					fprintf(stderr, "[ERROR] readAnimation: Failed to allocate Animation");
					clearLEDs();
					exit(EXIT_FAILURE);
				}
        AnimationFrame** animationFrames = malloc(sizeof(AnimationFrame*) * image->ImageCount);
				if (!animationFrames) {
					fprintf(stderr, "[ERROR] readAnimation: Failed to allocate AnimationFrames");
					clearLEDs();
					exit(EXIT_FAILURE);
				}
        animation->frames = animationFrames;
        animation->frameCount = image->ImageCount;
        animation->monochrome = true;
        animation->image = image;

        GraphicsControlBlock gcb;
        for (int i = 0; i < image->ImageCount; ++i) {
            const SavedImage* frame = &image->SavedImages[i]; //get access to frame data

            u_int16_t delayTime = DEFAULT_DELAY_TIME;
            if (DGifSavedExtensionToGCB(image, i, &gcb) == GIF_ERROR){
                printf("[WARNING] Can't read frame delay. Using default delay time");
            } else {
                delayTime = gcb.DelayTime * 10;
                if (delayTime < MIN_DELAY_TIME){
                    printf("[WARNING] Delay time is smaller than allowed smallest delay time (%d ms). Using that.\n", MIN_DELAY_TIME);
                    delayTime = MIN_DELAY_TIME;
                }
            }

            if (verboseLogging) {
                printf("[INFO] (Frame %i info): Size: %ix%i; Delay time: %i ms; Left: %i, Top: %i; Local color map: %s\n",
                       i, frame->ImageDesc.Width, frame->ImageDesc.Height, delayTime, frame->ImageDesc.Left, frame->ImageDesc.Top,
                       (frame->ImageDesc.ColorMap ? "Yes" : "No"));
            }

            animationFrames[i] = readFramePixels(frame, globalColorMap, &animation->monochrome);
            animationFrames[i]->delayTime = delayTime;
        }

        if (verboseLogging) {
            printf("[INFO] Animation is monochrome: %s\n", animation->monochrome ? "true" : "false");
        }

    } else {
        fprintf(stderr, "[ERROR] Image has wrong size (%dx%d). Required is (%dx%d)", image->SWidth, image->SHeight,
                LED_WIDTH,
                LED_HEIGHT);
        return false;
    }

    return animation;
}

/**
 * Read the pixel of an animation frame and parse it into a data structure.
 * @param frame
 * @param _globalMap
 * @param _monochrome
 * @return
 */
AnimationFrame* readFramePixels(const SavedImage* frame, ColorMapObject* _globalMap, bool* _monochrome) {
    const GifImageDesc desc = frame->ImageDesc; //get description of current frame
    const ColorMapObject* colorMap = desc.ColorMap ? desc.ColorMap
                                                   : _globalMap; //choose either global or local color map

    AnimationFrame* animationFrame = malloc(sizeof(AnimationFrame));
		if (!animationFrame) {
			fprintf(stderr, "[ERROR] readFramePixels: Failed to allocate animationFrame");
			clearLEDs();
			exit(EXIT_FAILURE);
		}

    bool keepColor = false;
    for (int y = 0; y < desc.Height; ++y) {
        for (int x = 0; x < desc.Width; ++x) {
            int c = frame->RasterBits[y * desc.Width + x];

            if (colorMap) {
                GifColorType* color = &colorMap->Colors[c];
                animationFrame->color[x][y] = color;

                //Check if animation is monochrome.
                //When a single frame contains color, then preserve the animations color later while rendering.
                if (!isGrayScale(color)) {
                    keepColor = true;
                }

            } else {
                printf("[WARNING] No color map given. Can't process picture. Skip frame");
            }
        }
    }

    *_monochrome &= !keepColor;
    return animationFrame;
}

bool isGrayScale(GifColorType* _color){
    return (_color->Red == _color->Green && _color->Red == _color->Blue);
}

/**
 * Get a random entry from a list
 * @param list List, from which the random item should be chosen
 * @param _count Length of list
 * @return A random item from the list
 */
char* getRandomAnimation(char* list[], int _count) {
    int randomGif = rand() % _count;
    return list[randomGif];
}
//endregion

//region LED
/**
 * Initialize the LEDs and their data structure
 * @return Infos about, if initialization was successful
 */
ws2811_return_t initLEDs() {
    //Setup display
    memset(&display, 0, sizeof(ws2811_t));
    display.freq = TARGET_FREQ;
    display.dmanum = DMA;

    ws2811_channel_t* channel = calloc(1, sizeof(ws2811_channel_t));
    channel->gpionum = dataPin;
    channel->count = LED_COUNT;
    channel->invert = INVERTED;
    channel->brightness = brightness;
    if (realTASBot){
        channel->strip_type = STRIP_TYPE;
    } else {
        channel->strip_type = WS2812_STRIP;
    }
    display.channel[0] = *channel;

    //Setup color array
    pixel = malloc(sizeof(ws2811_led_t) * LED_WIDTH * LED_HEIGHT);
		if (!pixel) {
			fprintf(stderr, "[ERROR] initLEDs: Failed to allocate color array");
			clearLEDs();
			exit(EXIT_FAILURE);
		}

    //Initialize hardware
    ws2811_return_t r;
    if ((r = ws2811_init(&display)) != WS2811_SUCCESS) {
        fprintf(stderr, "[ERROR] ws2811_init failed. Couldn't initialize LEDs: %s\n", ws2811_get_return_t_str(r));
    } else {
        if (verboseLogging) {
            printf("[INFO] Initialized LEDs with code %d\n", r);
        }
    }
    clearLEDs();
    return r;
}

/**
 * Updates the display's hardware LEDs color to the local pixel variables array
 * @return Infos about, if the LEDs where rendered successful
 */
ws2811_return_t renderLEDs() {
    for (int x = 0; x < LED_WIDTH; x++) {
        for (int y = 0; y < LED_HEIGHT; y++) {
            display.channel[0].leds[(y * LED_WIDTH) + x] = pixel[y * LED_WIDTH + x];
        }
    }

    ws2811_return_t r;
    if ((r = ws2811_render(&display)) != WS2811_SUCCESS) {
        fprintf(stderr, "[ERROR] Failed to render: %s\n", ws2811_get_return_t_str(r));
    } else {
        if (verboseLogging) {
            printf("[INFO] Rendered LEDs with code %d\n", r);
        }
    }

    return r;
}

/**
 * Clears all the LEDs by setting their color to black and renders it
 */
ws2811_return_t clearLEDs() {
    for (size_t i = 0; i < LED_COUNT; i++) {
        pixel[i] = 0;
    }
    return renderLEDs();
}
//endregion

//region TASBot
/**
 * Show a random blink expression from BLINK_PATH
 */
void showBlinkExpression() {
    showRandomExpression(pathForBlinks, false, false);
}

/**
 * Show a random animation in TASBots display
 * @param _path Path, from where a random animation should be chosen from
 * @param _useRandomColor If the animation can be played with an randomly chosen color, if it's monochrome
 */
void showRandomExpression(char* _path, bool _useRandomColor, bool _repeatAnimations) {
    int fileCount = countFilesInDir(_path); //get file count
    if (fileCount != -1) {
        char* list[fileCount];
        getFileList(_path, list); //get list of files
        char* file = getRandomAnimation(list, fileCount); //get random animation
        char* filePath = getFilePath(_path, file);

        showExpressionFromFilepath(filePath, _useRandomColor, _repeatAnimations);
    } else {
        fprintf(stderr, "[ERROR] No files in %s. Please check directory\n", _path);
    }
}

/**
 * Play one specific animation from given file
 * @param _filePath That should be played
 */
void showExpressionFromFilepath(char* _filePath, bool _useRandomColor, bool _repeatAnimations) {
    Animation* animation = readAnimation(_filePath);
    playExpression(animation, _useRandomColor, _repeatAnimations);
}

/**
 * Play a given animation on TASBots display
 *
 * Priorities for colors are
 * -1. Given colors within the animation if given (other than white)
 * -2. If applies, random color
 * -3. If applies, default color
 * -4. Given colors within the animation (just white)
 *
 * @param _animation The animation structure, that is to play
 * @param _useRandomColor If the animation should overwrite the animations palette with a random one, if its monochrome
 */
void playExpression(Animation* _animation, bool _useRandomColor, bool _repeatAnimations) {
    bool randColor;
    if (useRandomColorsForAll){
        //When animation is monochrome, use a random color, also for blinks and the base
        randColor = _animation->monochrome;
    } else {
        //When random color should be selected, make it depended on monochrome.
        //variable = (condition) ? expressionTrue : expressionFalse;
        randColor = _useRandomColor ? _animation->monochrome : false;
    }

    //When the default color is set and image is monochrome, then use the default color
    bool defColor = false;
    if (defaultColor != -1) {
        if (_animation->monochrome){
            defColor = true;
        }
    }

    if (verboseLogging){
        printf("[INFO] Use a random color: %s\n", randColor ? "true" : "false");
        printf("[INFO] Use the default color: %s\n", randColor ? "true" : "false");
    }

    //If applies, choose color to overwrite
    ws2811_led_t color = 0;
    if (randColor) {
        int r = rand() % paletteCount;
        color = palette[r];
    } else if (defColor) {
        color = defaultColor;
    }

    //Set correct amount of repetitions
    int reps = repetitions;
    if (!_repeatAnimations){
        reps = 1;
    }

    //Show frame
    for (int r = 0; r < reps; ++r) {
        for (int i = 0; i < _animation->frameCount; ++i) {
            if (verboseLogging) {
                printf("[INFO] Render frame #%d \n", i);
            }
            showFrame(_animation->frames[i], color);
            usleep((int) ((float) (_animation->frames[i]->delayTime * 1000) / playbackSpeed));
        }
    }

    //Remove entire animation from memory
    freeAnimation(_animation);
}

/**
 * Output the pixel data to the LED data structure, which then gets rendered.
 * @param _frame The frame, that is to render
 * @param _color The color, which should overwrite the actual color data from the frame and only used, when the animation is monochrome. Otherwise, it's NULL and used to indicate, that animation has its own color.
 */
void showFrame(AnimationFrame* _frame, ws2811_led_t _color) {
    for (int y = 0; y < LED_HEIGHT; ++y) {
        for (int x = 0; x < LED_WIDTH; ++x) {
            GifColorType* gifColor = _frame->color[x][y];
            ws2811_led_t color;

            if (activateLEDModule) {
                if (_color == 0) {
                    color = translateColor(gifColor, useGammaCorrection);
                } else {
                    if (gifColor->Red != 0 || gifColor->Green != 0 || gifColor->Blue != 0) {
                        color = _color;
                        //TODO: Adjust to brightness of gifColor given in GIF
                        // Right now it's flat the same gifColor to all pixels, that just _aren't_ black
                        // Use function getLuminance() for that
                    } else {
                        color = 0;
                    }
                }
            }

            //Map color to pixel based on given render device
            if (activateLEDModule) {
                if (realTASBot) {
                    int index = TASBotIndex[y][x];
                    if (index >= 0) {
                        pixel[index] = color;
                    }
                } else {
                    pixel[ledMatrixTranslation(x, y)] = color;
                }
            }


            //Debug renderer
            if (consoleRenderer) {
                if (gifColor->Red != 0 || gifColor->Green != 0 || gifColor->Blue != 0) {
                    printf("x");
                } else {
                    printf(" ");
                }
            }
        }
        if (consoleRenderer) {
            printf("\n");
        }
    }

    if (activateLEDModule) {
        renderLEDs();
    }
}

/**
 * Free up the allocated memory space for an animation
 * @param _animation The animation, that is to free from memory
 */
//TODO: Can someone check please, if I got it right?
void freeAnimation(Animation* _animation) {
    //dirty trick, close file here, after animation. That way DGifCloseFile() can't destroy the animation data
    int e = 0;
    DGifCloseFile(_animation->image, &e);
    if (verboseLogging) {
        printf("[INFO] Closed GIF with code %d\n", e);
    }

    for (int i = 0; i < _animation->frameCount; ++i) {
        free(_animation->frames[i]); //frame
    }
    free(_animation->frames); //pointer to frames
    free(_animation); //animation
}

/**
 * Determine How long to wait between blinks
 * @return Seconds, that are to wait between blink animation
 */
unsigned int getBlinkDelay() {
    if (minTimeBetweenBlinks == maxTimeBetweenBlinks) {
        //Cast to float to multiply with payback speed. Then cast back, as we need an integer.
        if (playbackSpeedAffectBlinks) {
            return (int) ((float) minTimeBetweenBlinks * (1 / playbackSpeed));
        }
        return minTimeBetweenBlinks;
    }

    if (playbackSpeedAffectBlinks) {
        return (int) ((float) (minTimeBetweenBlinks + (rand() % (maxTimeBetweenBlinks - minTimeBetweenBlinks))) *
                      (1 / playbackSpeed));
    }

    //default case
    return (int) ((float) (minTimeBetweenBlinks + (rand() % (maxTimeBetweenBlinks - minTimeBetweenBlinks))));
}

/**
 * Get an random amount of how many times TASBot should blink
 * @return The amount of blinks
 */
unsigned int getBlinkAmount() {
    if (maxBlinks == 0) {
        return 0;
    }
    return (rand() % maxBlinks) + 1;
}

/**
 * Get the luminance of a given color
 * @param _color The color the luminance should be calculated of
 * @return The luminance of the given color
 */
float getLuminance(GifColorType* _color) {
    return (0.2126F * (float) _color->Red + 0.7152F * (float) _color->Green + 0.0722F * (float) _color->Blue);
}

/**
 * Translate the RGB (255,255,255) color structure into a hexadecimal value
 * @param _color The RGB color, that is to convert
 * @return The convert hexadecimal color
 */
ws2811_led_t translateColor(GifColorType* _color, bool _useGammaCorrection) {
    if (_useGammaCorrection){ //TODO: when used, breaks things
        _color->Red = gamma8[_color->Red];
        _color->Green = gamma8[_color->Green];
        _color->Blue = gamma8[_color->Blue];
    }
    return ((_color->Red & 0xff) << 16) + ((_color->Green & 0xff) << 8) + (_color->Blue & 0xff);
}
//endregion

//region Palette
/**
 * Main function to read a palette file, convert it into numerical values and providing the final color array
 * @param _path The color palette file, that is to read
 */
void readPalette(char* _path) {
    //Read raw palette from file into the rawPal variable
    int colorCount = countLines(_path);
    char* rawPal[colorCount];
    readFile(_path, colorCount, rawPal);

    //Read palette into final palette array
    ws2811_led_t* pal = malloc(sizeof(ws2811_led_t) * colorCount);
		if (!pal) {
			fprintf(stderr, "[ERROR] readPalette: Failed to allocate palette memory");
			clearLEDs();
			exit(EXIT_FAILURE);
		}
    for (int i = 0; i < colorCount; ++i) {
        int color = strtocol(rawPal[i]);
        if (color != -1) {
            pal[i] = color;
            if (verboseLogging) {
                printf("[INFO] Convert string \"%s\" to integer in hex 0x%x\n", rawPal[i], pal[i]);
            }
        } else {
            printf("[WARNING] Skip color %s because of parsing error", rawPal[i]);
        }
    }

    paletteCount = colorCount;
    palette = pal;
}

/**
 * Converts a sequenz of hexadecimal characters into an actual integer. Meant to be used with hexadecimal color strings
 * @param _color The color string, that should be converted
 * @return An integer, that contains the actual numeric value of the given _color string
 */
ws2811_led_t strtocol(char* _color) {
    ws2811_led_t result = 0;
    int len = (int) strlen(_color);

    //Convert string character by character into an actual integer
    for (int i = 0; i < len; i++) {
        int hex = chtohex(_color[i]);
        if (hex != -1) {
            result = result | hex << (len - i - 1) *
                                     4; //Bit-shift by 4, because hex numbers use only lower 4 bits. OR all of it together.
        } else {
            fprintf(stderr,
                    "[ERROR] Can't read character '%c' color \"%s\". Please check formatting!\n",
                    _color[i],
                    _color
                    );
            return -1;
        }
    }
    return result;
}

/**
 * Convert a given char into it's actually numerical value. Supports 0-9, a-f and A-F
 * @param _ch A character, that contains the sign of an hexadecimal number
 * @return The numerical value of that character
 */
int chtohex(char _ch) {
    int result = -1;

    //For "normal" decimal number character (0-9)
    if (_ch - '0' >= 0 && _ch - '0' <= 9) {
        result = _ch - '0';

        //For hexadecimal numbers (a-f), lower case
    } else if (_ch - 'a' >= 0 && _ch - 'a' <= 'f' - 'a') {
        result = _ch - 'a' + 10;

        //For hexadecimal numbers (A-F), upper case
    } else if (_ch - 'A' >= 0 && _ch - 'A' <= 'F' - 'A') {
        result = _ch - 'A' + 10;
    }
    return result;
}
//endregion

//region File system operations
/**
 * Allocate memory for path complete file path and ditch path and filename together
 * @param _path relativ or absolut path, where the file is laying
 * @param _file filename
 * @return relative or absolut path to file
 */
char* getFilePath(char* _path, char* _file) {
    char* path = malloc(sizeof(char) * (MAX_PATH_LENGTH + MAX_FILENAME_LENGTH));
    strcpy(path, _path);
    strcat(path, _file);

    return path;
}

/**
 * Check if a directory exists
 * @param _path Path of the directory, that's supposed to be checked
 * @return If the directory of the given path exists
 */
bool checkIfDirectoryExist(char* _path) {
    DIR* dir = opendir(_path);
    if (dir) {
        closedir(dir);
        return true;
    }
    return false;
}

/**
 * Check if a given file exists
 * @param _file Path to the file, that should be checked
 * @return If the file for the given path exists
 */
bool checkIfFileExist(char* _file) {
    if (access(_file, F_OK) == 0) {
        return true;
    }
    return false;
}

/**
 * Read lines of a given file into a given array
 * @param _path The file, that should be parsed into the array
 * @param _count The count of lines, the file has
 * @param _out The array, the lines should be read into
 */
void readFile(const char* _path, int _count, char** _out) {
    //Check if file exists
    FILE* ptr = fopen(_path, "r");
    if (ptr == NULL) {
        fprintf(stderr, "[ERROR] File can't be opened \n");
    }

    //Read line after line into give array
    for (int i = 0; i < _count; i++) {
        _out[i] = malloc(sizeof(unsigned int));
				if (!_out[i]) {
					fprintf(stderr, "[ERROR] readFile: Failed to allocate memory");
					clearLEDs();
					exit(EXIT_FAILURE);
				}
        if (fscanf(ptr, "%s\n", _out[i]) != 1) fprintf(stderr, "[ERROR] readFile: Failed to read line %d\n", i);
    }

    //Closing the file
    fclose(ptr);
}

/**
 * Count the files in a given directory
 * @param _path The path of the directory, it's files should be counted
 * @return The number of files in that directory
 */
int countFilesInDir(char* _path) {
    DIR* d = opendir(_path);
    if (d) {
        int counter = 0;
        struct dirent* dir;
        while ((dir = readdir(d)) != NULL) {
            if (dir->d_type == DT_REG) {
                counter++;
            }
        }
        closedir(d);
        return counter;
    }
    return -1;
}

/**
 * Writes all file names of an given directory into the given _list-Array
 * @param _path The path of the directory, it file names should be written into _list
 * @param _list Pointer to output array. Results get writen into here.
 * @return If the directory was successfully open
 */
bool getFileList(const char* _path, char* _list[]) {
    DIR* d = opendir(_path);
    if (d) {
        int counter = 0;
        struct dirent* dir;
        while ((dir = readdir(d)) != NULL) {
            if (dir->d_type == DT_REG) {
                _list[counter] = dir->d_name;
                counter++;
            }
        }
        closedir(d);
        return true;
    }
    return false;
}

/**
 * Count the lines of a file
 * @param _path The file whose lines should be count
 * @return The number of lines
 */
int countLines(const char* _path) {
    //Check if file exists
    FILE* ptr = fopen(_path, "r");
    if (ptr == NULL) {
        fprintf(stderr, "[ERROR] File can't be opened \n");
    }

    //Read every character of the file and count new lines/line feeds
    int newLines = 0;
    int ch;
    do {
        ch = fgetc(ptr);
        if (ch == '\n') {
            newLines++;
        }
    } while (ch != EOF);

    //Closing the file
    fclose(ptr);

    //Last line doesn't have a line feed, so add one
    return newLines + 1;
}
//endregion

//region Debug and Development
/**
 * Converts pixel coordinate of a frame into the the actual coordinate for the 32x8 LED matrix R3tr0BoiDX aka Mirbro used during development
 * @param _x x-coordinate of the frame
 * @param _y y-coordinate of the frame
 * @return index of the LED for a 32x8 LED matrix
 */
unsigned int ledMatrixTranslation(int _x, int _y) {
    if (numberIsEven(_x)) {
        return (_x * LED_HEIGHT + _y);
    } else {
        return (_x * LED_HEIGHT + LED_HEIGHT - 1 - _y);
    }
}

/**
 * Checks if a number is even
 * @param _number The number, that should be checked
 * @return If the number is even
 */
bool numberIsEven(int _number) {
    return (_number % 2 == 0);
}
//endregion
