#include <gif_lib.h> //https://sourceforge.net/projects/giflib/
#include <ws2811/ws2811.h> //https://github.com/jgarff/rpi_ws281x

#include <stdio.h>
#include <dirent.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>

#define OTHER_PATH              "./gifs/others/"
#define BASE_PATH               "./gifs/base.gif"
#define BLINK_PATH              "./gifs/blinks/" //TODO: point to folder with different blinks
#define MAX_FILENAME_LENGTH     256
#define MAX_PATH_LENGTH         4096
#define DEFAULT_DELAY_TIME      100
#define MAX_BLINKS              4                       //How many times does TASBot blink between animations
#define MIN_TIME_BETWEEN_BLINKS 4                       //Based on human numbers. We Blink about every 4 to 6 seconds
#define MAX_TIME_BETWEEN_BLINKS 6

#define LED_HEIGHT              8
#define LED_WIDTH               28

#define TARGET_FREQ             WS2811_TARGET_FREQ
#define GPIO_PIN                10
#define DMA                     10
#define LED_COUNT               (LED_WIDTH * LED_HEIGHT)
#define STRIP_TYPE              WS2811_STRIP_BRG
#define INVERTED                false
#define BRIGHTNESS              32

#define ARRAY_SIZE(arr)         (sizeof(arr) / sizeof(arr[0]))

typedef struct AnimationFrame {
    GifColorType *color[LED_WIDTH][LED_HEIGHT];
    u_int16_t delayTime;
} AnimationFrame;

typedef struct Animation {
    AnimationFrame **frames; //pointer to a pointer, that's an array of frames. pain.
    int frameCount;
    bool monochrome;
    GifFileType* image; //needed very dirty trick to get around weird behavior, where DGifCloseFile() manipulates the animation data for some reason
} Animation;

//Declarations
void setupHandler();
void finish(int _number);

int countFilesInDir(char* _path);
bool getFileList(const char* _path, char *_list[]);

char *getRandomAnimation(char *list[], int _count);
char *getFilePath(char *_path, char *_file);
bool checkIfImageHasRightSize(GifFileType *_image);
u_int16_t getDelayTime(SavedImage *_frame);
AnimationFrame *readFramePixels(const SavedImage *frame, ColorMapObject *_globalMap, bool *_monochrome);
Animation* readAnimation(char *_file);

ws2811_return_t initLEDs();
ws2811_return_t renderLEDs();
ws2811_return_t clearLEDs();
ws2811_led_t translateColor(GifColorType *_color);

void showBaseExpression();
void showBlinkExpression();
void showRandomExpression(char* _path, bool _useRandomColor);
void showExpressionFromFilepath(char* _filePath);
void playExpression(Animation *_animation, bool _useRandomColor);
void showFrame(AnimationFrame *_frame, ws2811_led_t _color); //color is only used, when picture is monochrome
int getBlinkDelay();
void freeAnimation(Animation* _animation);

unsigned int ledMatrixTranslation(int _x, int _y);
bool numberIsEven(int _number);

//Function toggles
bool verboseLogging = true;
bool consoleRenderer = true;
bool activateLEDModule = true;
bool realTASBot = true;

//Variables
bool running = true;
float playbackSpeed = 1; //doesnt affects the time between the blinks. just the playback speed of the aimation
char* specificAnimationToShow = NULL; //"./gifs/blink.gif"; //TODO: Blink is just for test purposes here
char* pathForAnimations = OTHER_PATH;

ws2811_led_t *pixel;
ws2811_t display = {
        .freq = TARGET_FREQ,
        .dmanum = DMA,
        .channel ={
                [0] ={
                        .gpionum = GPIO_PIN,
                        .count = LED_COUNT,
                        .invert = INVERTED,
                        .brightness = BRIGHTNESS,
                        .strip_type = STRIP_TYPE,
                }
        },
};

//default colors
ws2811_led_t colors[] = {
        0xFF0000,  // red
        0xFF8000,  // orange
        0xFFFF00,  // yellow
        0x00FF00,  // green
        0x00FFFF,  // cyan
        0x0000FF,  // blue
        0xFF00FF,  // magenta
        0xFF80FF,  // pink
};

//TASBot display conversation table
//Based on https://github.com/jakobrs/tasbot-display/blob/master/src/tasbot.rs
int TASBotIndex[8][28] = {
        {-1,-1,0,1,2,3,-1,-1,-1,-1,101,100,99,98,97,96,95,94,-1,-1,-1,-1,105,104,103,102,-1,-1},
        {-1,4,5,6,7,8,9,-1,-1,84,85,86,87,88,89,90,91,92,93,-1,-1,111,110,109,108,107,106,-1},
        {10,11,12,13,14,15,16,17,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,119,118,117,116,115,114,113,112},
        {18,19,20,21,22,23,24,25,-1,-1,-1,83,82,81,80,79,78,-1,-1,-1,127,126,125,124,123,122,121,120},
        {26,27,28,29,30,31,32,33,-1,-1,70,71,72,73,74,75,76,77,-1,-1,135,134,133,132,131,130,129,128},
        {34,35,36,37,38,39,40,41,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,143,142,141,140,139,138,137,136},
        {-1,42,43,44,45,46,47,-1,-1,-1,68,67,66,65,64,63,62,61,-1,-1,-1,149,148,147,146,145,144,-1},
        {-1,-1,48,49,50,51,-1,-1,-1,69,52,53,54,55,56,57,58,59,60,-1,-1,-1,153,152,151,150,-1,-1}
};

//TODO: Proper comments (doxygen and such)

//TODO: single frame animations shown a random duration. multiple frame animation based of gif

//TODO: args:
//args: -s: playback speed; -I: specific image; -P: specific folder; -v: verbose logging; -h: help; -r: console renderer; -b: brightness [0-255]
//      -B: how many blinks between animation; -Bmin: min time between blinks; -Bmax: max time between blinks
//      -p: path to color palette text file
int main() {
    //can't use LED hardware on desktops
#if defined(__x86_64__)
    activateLEDModule = false;
#endif

    srand(time(NULL));
    setupHandler();

    if (activateLEDModule) {
        ws2811_return_t r = initLEDs();
        if (r != WS2811_SUCCESS) {
            fprintf(stderr, "[ERROR] Can't run program. Did you started it as root?\n");
            return r;
        }
    }

    int test;

    /*
    for (int i = 0; i < LED_COUNT; ++i) {
        pixel[i] = colors[0];

        scanf("%d", &test);
        //usleep(1000 * 1000);
        printf("Renderer LED index (%d:) is \n", i);
        renderLEDs();
    }
     */


    for (int y = 0; y < LED_HEIGHT; ++y) {
        for (int x = 0; x < LED_WIDTH; ++x) {

            int index = TASBotIndex[y][x];
            if(index >= 0){
                pixel[index] = colors[0];
            }

            scanf("%d", &test);
            //usleep(1000 * 1000);
            printf("Renderer LED index (%d:%d) is %d\n",y, x, TASBotIndex[y][x]);
            renderLEDs();
        }
    }


    /*
    //option for playing give specific animation
    if (specificAnimationToShow != NULL){
        while (running){
            showExpressionFromFilepath(specificAnimationToShow);
        }
        return 0;
    }

    //TODO: Set pathForAnimations, when given on console

    bool firstIteration = true;
    while (running) {
        //skip to base expression on first iteration, to not start on a random animation
        if (!firstIteration) {
            showRandomExpression(pathForAnimations, true);
        } else {
            firstIteration = false;
        }
        showBaseExpression();
        sleep(getBlinkDelay());

        //blink for a random amount of times
        for (int blinks = (rand() % MAX_BLINKS) + 1; blinks > 0; --blinks) {
            showBlinkExpression();
            showBaseExpression();

            int blinkTime = getBlinkDelay();
            if (verboseLogging) {
                printf("[INFO] Blink #%d for %d seconds \n", blinks, blinkTime);
            }
            sleep(blinkTime);
        }
    }
     */

    finish(0);
    return 0;
}

/**
 * determine how long to wait between blinks
 * @return seconds, that are to wait between blink animation
 */
int getBlinkDelay() {
    return MIN_TIME_BETWEEN_BLINKS + (rand() % (MAX_TIME_BETWEEN_BLINKS - MIN_TIME_BETWEEN_BLINKS));
}

/**
 * allocate memory for path complete file path and ditch path and filename together
 * @param _path relativ or absolut path, where the file is laying
 * @param _file filename
 * @return relative or absolut path to file
 */
char *getFilePath(char *_path, char *_file) {
    char *path = malloc(sizeof(char) * (MAX_PATH_LENGTH + MAX_FILENAME_LENGTH));
    strcpy(path, _path);
    strcat(path, _file);

    return path;
}

/**
 * @brief Registers the handler
 * Register the handler for SIGINT, SIGTERM and SIGKILL
 * All other Signals are other signals excluded
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
 * @brief Central handler for leaving the application
 * Handler for SIGINT, SIGTERM and SIGKILL. Gets also called after the endless loop
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
    exit(_number);
}

//region GIF
/**
 * check if GIF has the exact right size of 28x8 pixels
 * @param _image The GIF that is to check
 * @return If image has right size or not
 */
bool checkIfImageHasRightSize(GifFileType *_image) {
    return (_image->SWidth == LED_WIDTH && _image->SHeight == LED_HEIGHT);
}

/**
 * @brief [DONT USE, NOT TESTED, DEAD CODE] Obtain the delay time between frames
 * @param _frame Frame the delay shout get from
 * @return The delay between frames
 */
u_int16_t getDelayTime(SavedImage *_frame) {
    //Read all the extension blocks - e as in extension
    for (int e = 0; e < _frame->ExtensionBlockCount; ++e) {
        if (verboseLogging) {
            printf("[Extension Block] Bytes: %d; Function: %02X: ",
                   _frame->ExtensionBlocks[e].ByteCount, _frame->ExtensionBlocks[e].Function);
        }

        //Let's read all the bytes in this block - b as in bytes
        for (int b = 0; b < _frame->ExtensionBlocks[e].ByteCount; ++b) {
            if (verboseLogging) {
                printf("%02X ", _frame->ExtensionBlocks[e].Bytes[b]);
            }

            // this works just in theory and is not tested!
            // wait for first "Graphics Control Extension"-block and assume all frames have the same delay
            // TODO: Actually use the delay time of every _frame for the animation, rather than assuming all have the same
            // http://giflib.sourceforge.net/whatsinagif/bits_and_bytes.html
            // F9 is the identifier for "Graphics Control Extension"-blocks
            if (_frame->ExtensionBlocks[e].Function == 0xF9) {
                u_int8_t highByte = _frame->ExtensionBlocks[e].Bytes[2];
                u_int8_t lowByte = _frame->ExtensionBlocks[e].Bytes[3];
                return (highByte << 8) | lowByte;
            }
        }

        if (verboseLogging) {
            printf("\n");
        }
    }
    return 0;
}

/**
 * Opens and reads the GIF file in a data structure. Container for AnimationFrames and various other needed infos
 * @param _file The GIF animation, that is to read
 * @return Data structure, that contains all needed information to display it on TASBots display
 * TODO: This is pretty blob like. Can probably be shorten.
 */
Animation* readAnimation(char *_file) {
    if (verboseLogging) {
        printf("[INFO] Load _file %s\n", _file);
    }

    //Open _file
    int e;
    GifFileType *image = DGifOpenFileName(_file, &e);
    if (!image) {
        fprintf(stderr, "[ERROR] EGifOpenFileName() failed. Could't find or open _file: %d\n", e);
        return false;
    }

    //"Slurp" infos into struct
    if (DGifSlurp(image) == GIF_ERROR) {
        fprintf(stderr, "[ERROR] DGifSlurp() failed. Couldt load infos about GIF: %d\n", image->Error);
        DGifCloseFile(image, &e);
        return false;
    }

    Animation* animation = NULL;
    //Before we do anything, lets check if image has right size
    if (checkIfImageHasRightSize(image)) {

        //Obtain global color map if available
        ColorMapObject *globalColorMap = image->SColorMap;
        if (verboseLogging) {
            printf("[Image] Size: %ix%i; Frames: %i; Path: \"%s\"\n",
                   image->SWidth, image->SHeight, image->ImageCount, _file);
        }

        //Process frames
        animation = malloc(sizeof(Animation));
        AnimationFrame **animationFrames = malloc(sizeof(AnimationFrame*) * image->ImageCount);
        animation->frames = animationFrames;
        animation->frameCount = image->ImageCount;
        animation->monochrome = true;
        animation->image = image;

        for (int i = 0; i < image->ImageCount; ++i) {
            const SavedImage *frame = &image->SavedImages[i]; //get access to frame data

            if (verboseLogging) {
                printf("[Frame %i] Size: %ix%i; Left: %i, Top: %i; Local color map: %s\n",
                       i, frame->ImageDesc.Width, frame->ImageDesc.Height, frame->ImageDesc.Left, frame->ImageDesc.Top,
                       (frame->ImageDesc.ColorMap ? "Yes" : "No"));
            }

            // Needs getDelayTime() to get actually tested
            /*
            u_int16_t delayTime = getDelayTime(frame);
            if (delayTime == 0) {
                delayTime = DEFAULT_DELAY_TIME;
            }
             */
            u_int16_t delayTime = DEFAULT_DELAY_TIME;

            animationFrames[i] = readFramePixels(frame, globalColorMap, &animation->monochrome);
            animationFrames[i]->delayTime = delayTime;
        }
        if (verboseLogging){
            printf("[INFO] Animation is monochrome: %d\n", animation->monochrome);
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
AnimationFrame *readFramePixels(const SavedImage *frame, ColorMapObject *_globalMap, bool* _monochrome) {
    const GifImageDesc desc = frame->ImageDesc; //get description of current frame
    const ColorMapObject *colorMap = desc.ColorMap ? desc.ColorMap
                                                   : _globalMap; //choose either global or local color map

    AnimationFrame *animationFrame = malloc(sizeof(AnimationFrame));

    for (int y = 0; y < desc.Height; ++y) {
        for (int x = 0; x < desc.Width; ++x) {
            int c = frame->RasterBits[y * desc.Width + x];

            if (colorMap) {
                GifColorType *color = &colorMap->Colors[c];
                animationFrame->color[x][y] = color;

                //check if animation is monochrome. When a single frame contain colors,
                //then preserve the animations color later while rendering.
                if (!(color->Red == color->Green && color->Red == color->Blue)) {
                    *_monochrome = false;
                }

            } else {
                printf("[WARNING] No color map given. Can't process picture. Skip frame");
            }
        }
    }
    return animationFrame;
}

//TODO: Both methods could probably tidied up into one method with some refactoring...somehow...I bet...I swear...I'm sure
/**
 * Count the files in a given directory
 * @param _path The path of the directory, it's files should be counted
 * @return The number of files in that directory
 */
int countFilesInDir(char* _path) {
    DIR *d = opendir(_path);
    if (d) {
        int counter = 0;
        struct dirent *dir;
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
 * @param _list Pointer to output array. Results get writen into here. //TODO: restructure with malloc and return array actually
 * @return If the directory was successfully open //TODO: could be improved to something better
 */
bool getFileList(const char* _path, char* _list[]) {
    DIR *d = opendir(_path);
    if (d) {
        int counter = 0;
        struct dirent *dir;
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
 * Get a random entry from a list
 * @param list List, from which the random item should be chosen
 * @param _count Length of list
 * @return A random item from the list
 */
char *getRandomAnimation(char *list[], int _count) {
    int randomGif = rand() % _count;
    return list[randomGif];
}
//endregion

//region LED
/**
 * Initialize the LEDs anf their data structure
 * @return Infos about, if initialization was successful
 */
ws2811_return_t initLEDs() {
    ws2811_return_t r;
    pixel = malloc(sizeof(ws2811_led_t) * LED_WIDTH * LED_HEIGHT);

    if ((r = ws2811_init(&display)) != WS2811_SUCCESS) {
        fprintf(stderr, "[ERROR] ws2811_init failed. Couldt initialize LEDs: %s\n", ws2811_get_return_t_str(r));
    } else {
        printf("[INFO] Initialized LEDs with code %d\n", r);
    }
    //clearLEDs(); TODO: try this
    return r;
}

/**
 * @brief Update LEDs to new color
 * Updates the display's hardware LEDs color to the local pixel variables array
 * @return Infos about, if the LEDs where rendered successful
 */
ws2811_return_t renderLEDs() {
    for (int x = 0; x < LED_WIDTH; x++) {
        for (int y = 0; y < LED_HEIGHT; y++) {

            //TASBot translation
            if (realTASBot){
                /*
                int id;
                if ((id = TASBotIndex[y][x]) != -1){
                    //if LED ia existing on TASBot, assign TASBot led to corresponding pixel from graphic
                    display.channel[0].leds[id] = pixel[y * LED_WIDTH + x];
                    //printf("Render LED index (%d;%d) at TASBot index %d\n", x, y, id); //produces high console output
                }
                 */
                display.channel[0].leds[(y * LED_WIDTH) + x] = pixel[y * LED_WIDTH + x];
            } else {
                display.channel[0].leds[(y * LED_WIDTH) + x] = pixel[y * LED_WIDTH + x];
            }
        }
    }

    ws2811_return_t r;
    if ((r = ws2811_render(&display)) != WS2811_SUCCESS) {
        fprintf(stderr, "[ERROR] Failed to render: %s\n", ws2811_get_return_t_str(r));
    } else {
        //TODO: printf"[INFO] Rendered LEDs with code %d\n", r);
    }
    return r;
}

/**
 * @brief Turns of all the LEDs
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
 * Show the base, resp. idle expression of TASBot
 */
void showBaseExpression() {
    Animation* animation = readAnimation(BASE_PATH);
    playExpression(animation, false);
}

/**
 * Show a random blink expression from BLINK_PATH
 */
void showBlinkExpression() {
    showRandomExpression(BLINK_PATH, false);
}

/**
 * Show a random animation in TASBots display
 * @param _path Path, from where a random animation should be chosen from
 * @param _useRandomColor If the animation can be played with an randomly chosen color, if it's monochrome
 */
void showRandomExpression(char* _path, bool _useRandomColor) {
    int fileCount = countFilesInDir(_path); //get file count
    char* list[fileCount];
    getFileList(_path, list); //get list of files
    char *file = getRandomAnimation(list, fileCount); //get random animation
    char *filePath = getFilePath(_path, file);

    Animation* animation = readAnimation(filePath);
    playExpression(animation, _useRandomColor);
}

/**
 * Play one specific animation from given file
 * @param _filePath That should be played
 */
void showExpressionFromFilepath(char* _filePath){
    Animation* animation = readAnimation(_filePath);
    playExpression(animation, false);
}

/**
 * Play a given animation on TASBots display
 * @param _animation The animation structure, that is to play
 * @param _useRandomColor If the animation should overwrite the animations colors with a random one, if its monochrome
 */
void playExpression(Animation *_animation, bool _useRandomColor) {

    //when random color should be selected, make it depended on monochrome
    bool randColor = _useRandomColor ? _animation->monochrome : false;

    ws2811_led_t color = 0;
    if (randColor){
        int r = rand() % ARRAY_SIZE(colors);
        color = colors[r];
    }

    for (int i = 0; i < _animation->frameCount; ++i) {
        if (verboseLogging) {
            printf("[INFO] Render frame #%d \n", i);
        }
        showFrame(_animation->frames[i], color);
        usleep((int)(_animation->frames[i]->delayTime * 1000 / playbackSpeed));
    }

    freeAnimation(_animation);
}

//TODO: Can someone check please, if I got it right?
/**
 * Free up the allocated memory space for an animation
 * @param _animation The animation, that is to free from memory
 */
void freeAnimation(Animation* _animation){
    //dirty trick, close file here, after animation. That way DGifCloseFile() can't destroy the animation data
    int e = 0;
    DGifCloseFile(_animation->image, &e);
    if(verboseLogging){
        printf("[INFO] Closed GIF with code %d\n", e);
    }

    for (int i = 0; i < _animation->frameCount; ++i) {
        free(_animation->frames[i]); //frame
    }
    free(_animation->frames); //pointer to frames
    free(_animation); //animation
}

/**
 * Output the pixel data to the LED data structure, which then gets rendered.
 * @param _frame The frame, that is to render
 * @param _color The color, which should overwrite the actual color data from the frame. If equal 0, the color of the frame is actually used.
 */
void showFrame(AnimationFrame *_frame, ws2811_led_t _color) {

    for (int y = 0; y < LED_HEIGHT; ++y) {
        for (int x = 0; x < LED_WIDTH; ++x) {
            GifColorType *color = _frame->color[x][y];

            if (activateLEDModule) {
                if (_color == 0){
                    //pixel[ledMatrixTranslation(x, y)] = translateColor(color);
                    pixel[x * LED_WIDTH + y] = translateColor(color);
                } else {
                    if (color->Red != 0 || color->Green != 0 || color->Blue != 0) {
                        //pixel[ledMatrixTranslation(x, y)] = _color;
                        pixel[x * LED_WIDTH + y] = _color;
                        //TODO: Adjust to brightness of color given in GIF
                        // Right now it's flat the same color to all pixels, that just _aren't_ black
                    } else{
                        //pixel[ledMatrixTranslation(x, y)] = 0; //set other pixels black
                        pixel[x * LED_WIDTH + y] = 0;
                    }
                }
            }

            //Debug renderer
            if (consoleRenderer){
                if (color->Red != 0 || color->Green != 0 || color->Blue != 0) {
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
 * Translate the RGB (255,255,255) color structure into a hexadecimal value
 * @param _color The RGB color, that is to convert
 * @return The convert hexadecimal color
 */
ws2811_led_t translateColor(GifColorType *_color) {
    return ((_color->Red & 0xff) << 16) + ((_color->Green & 0xff) << 8) + (_color->Blue & 0xff);
}
//endregion

//region Debug and Development

//For usage with an actual 32x8 LED matrix
unsigned int ledMatrixTranslation(int _x, int _y) {
    if (numberIsEven(_x)) {
        return (_x * LED_HEIGHT + _y);
    } else {
        return (_x * LED_HEIGHT + LED_HEIGHT - 1 - _y);
    }
}

bool numberIsEven(int _number) {
    return (_number % 2 == 0);
}
//endregion