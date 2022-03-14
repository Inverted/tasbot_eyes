#include <gif_lib.h> //https://sourceforge.net/projects/giflib/
#include <ws2811/ws2811.h> //https://github.com/jgarff/rpi_ws281x

#include <stdio.h>
#include <dirent.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <signal.h>

#define GIF_PATH                "./gifs/"
#define BASE_EXPRESSION         "base.gif"
#define MAX_FILENAME_LENGTH     256
#define MAX_PATH_LENGTH         4096
#define DEFAULT_DELAY_TIME      100

#define LED_HEIGHT              8
#define LED_WIDTH               28

#define TARGET_FREQ             WS2811_TARGET_FREQ
#define GPIO_PIN                18
#define DMA                     10
#define LED_COUNT               (LED_WIDTH * LED_HEIGHT)
#define STRIP_TYPE              WS2811_STRIP_BRG
#define INVERTED                false
#define BRIGHTNESS              255
#define PLAYBACK_SPEED          15                  // frames per second

typedef struct AnimationFrame {
    GifColorType *color[LED_WIDTH][LED_HEIGHT];
    u_int16_t delayTime;
} AnimationFrame;

//Declarations
void setupHandler();
void exitHandler(int signalNumber);

int countFilesInDir(char _path[]);
bool getFileList(const char _path[], char *_list[]);
char *getRandomAnimation(char *list[], int _count);
char *getFilePath(char* _path, char* _file);

bool checkIfImageHasRightSize(GifFileType *_image);
u_int16_t getDelayTime(SavedImage *_frame);
AnimationFrame *readFramePixels(const SavedImage *frame, ColorMapObject *_globalMap);
bool playAnimation(const char *file);

ws2811_return_t initLEDs();
ws2811_return_t renderLEDs();
ws2811_return_t clearLEDs();

void showBaseExpression();
void showExpression(AnimationFrame* _frames, unsigned int _frameCount);
void showFrame(AnimationFrame* _frame);

void debugRenderer(GifColorType *_rgb);

//Variables
bool verboseLogging = true;
bool useDebugRenderer = false;
bool running = false;
ws2811_led_t* leds;
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
                },
                [1] ={ //TODO: do we need that?
                        .gpionum = 0,
                        .count = 0,
                        .invert = 0,
                        .brightness = 0,
                },
        },
};

//TODO: Signal handlers
//TODO: [Errors] mittels fprintf ausgeben

//args: -s: playback speed; -I: specific image; -P: specific folder; -v: verbose logging; -h: help; -r: debug renderer; -b: brightness [0-255]
int main() {
    srand(time(NULL));
    setupHandler();

    /*
    int fileCount = countFilesInDir(GIF_PATH); //get file count
    char *list[fileCount];
    getFileList(GIF_PATH, list); //get list of files
    char *animation = getRandomAnimation(list, fileCount); //get random animation
     */

    initLEDs();
    showBaseExpression();

    for (;/*ever*/;){}

    exitHandler(0);
    return 0;
}

//allocate memory for path string and ditch together
char *getFilePath(char* _path, char* _file){
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
void setupHandler(){
    struct sigaction sa = {
            .sa_handler = exitHandler,
    };
    sigemptyset(&sa.sa_mask);
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
    sigaction(SIGKILL, &sa, NULL);
}

/**
 * @brief Central handler for leaving the application
 * Handler for SIGINT, SIGTERM and SIGKILL
 * @param signalNumber
 */
void exitHandler(int signalNumber) {
    running = false;
    clearLEDs();
    ws2811_render(&display);
    ws2811_fini(&display);
    exit(0);
}

//region GIF
bool checkIfImageHasRightSize(GifFileType *_image) {
    return (_image->SWidth == LED_WIDTH && _image->SHeight == LED_HEIGHT);
}

//Obtain delay time
//INFO: In theory, we could encode more information the comment block, if we needed to
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

void debugRenderer(GifColorType *_rgb) {
    if (_rgb->Red != 0 || _rgb->Green != 0 || _rgb->Blue != 0) {
        printf("x");
    } else {
        printf(" ");
    }
}

//Read pixel color
AnimationFrame *readFramePixels(const SavedImage *frame, ColorMapObject *_globalMap) {
    const GifImageDesc desc = frame->ImageDesc; //get description of current frame
    const ColorMapObject *colorMap = desc.ColorMap ? desc.ColorMap
                                                   : _globalMap; //choose either global or local color map

    AnimationFrame *animationFrame = (AnimationFrame *) malloc(sizeof(AnimationFrame));

    for (int y = 0; y < desc.Height; ++y) {
        for (int x = 0; x < desc.Width; ++x) {
            int c = frame->RasterBits[y * desc.Width + x];

            if (colorMap) {
                GifColorType *rgb = &colorMap->Colors[c];
                animationFrame->color[x][y] = rgb;

                if (useDebugRenderer) {
                    debugRenderer(rgb);
                }
            } else {
                printf("[WARNING] Can't process picture. Skip frame");
            }
        }

        if (useDebugRenderer) {
            printf("\n");
        }
    }
    return animationFrame;
}

bool playAnimation(const char *file) {
    //Open file
    int error;
    GifFileType *image = DGifOpenFileName(file, &error);
    if (!image) {
        printf("[ERROR] EGifOpenFileName() failed. Could't find or open file: %d\n", error);
        return false;
    }

    //"Slurp" infos into struct
    if (DGifSlurp(image) == GIF_ERROR) {
        printf("[ERROR] DGifSlurp() failed. Couldt load infos about GIF: %d\n", image->Error);
        DGifCloseFile(image, &error);
        return false;
    }

    //Before we do anything, lets check if image has right size
    if (checkIfImageHasRightSize(image)) {

        //Obtain global color map if available
        ColorMapObject *globalColorMap = image->SColorMap;
        if (verboseLogging) {
            printf("[Image] Size: %ix%i; Frames: %i; Path: \"%s\"\n",
                   image->SWidth, image->SHeight, image->ImageCount, file);
        }

        //Process frames
        //AnimationFrame *animationFrames[image->ImageCount]; // = malloc(sizeof (AnimationFrame)*image->ImageCount);
        AnimationFrame **animationFrames = malloc(sizeof (AnimationFrame) * image->ImageCount);

        for (int i = 0; i < image->ImageCount; ++i) {
            const SavedImage *frame = &image->SavedImages[i]; //get access to frame data

            if (verboseLogging) {
                printf("[Frame %i] Size: %ix%i; Left: %i, Top: %i; Local color map: %s\n",
                       i, frame->ImageDesc.Width, frame->ImageDesc.Height, frame->ImageDesc.Left, frame->ImageDesc.Top,
                       (frame->ImageDesc.ColorMap ? "Yes" : "No"));
            }
            if (useDebugRenderer) {
                printf("[Frame: %d]\n", i);
            }

            // Needs the getDelayTime() get actually tested
            /*
            u_int16_t delayTime = getDelayTime(frame);
            if (delayTime == 0) {
                delayTime = DEFAULT_DELAY_TIME;
            }
            */
            u_int16_t delayTime = DEFAULT_DELAY_TIME;

            animationFrames[i] = readFramePixels(frame, globalColorMap);
            showExpression(*animationFrames, image->ImageCount);
        }
    } else {
        printf("[ERROR] Image has wrong size (%dx%d). Required is (%dx%d)", image->SWidth, image->SHeight, LED_WIDTH,
               LED_HEIGHT);
        return false;
    }

    DGifCloseFile(image, &error);
    printf("[INFO] Closed GIF file with code %d\n", error);
    return true;
}

//TODO: This could be tidied up into one method with some refactoring
int countFilesInDir(char _path[]) {
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

bool getFileList(const char _path[], char *_list[]) {
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

char *getRandomAnimation(char *list[], int _count) {
    int randomGif = rand() % _count;
    return list[randomGif];
}
//endregion

//region LED
ws2811_return_t initLEDs() {
    ws2811_return_t r;
    leds = malloc(sizeof(ws2811_led_t) * LED_WIDTH * LED_HEIGHT);

    if ((r = ws2811_init(&display)) != WS2811_SUCCESS) {
        fprintf(stderr, "ws2811_init failed. Couldt initialize LEDs: %s\n", ws2811_get_return_t_str(r));
    }
    return r;

    /*
    //funzt
    for (size_t i = 0; i < LED_COUNT; i++) {
        leds[i] = 0x00FF0000;
        renderLEDs();
        usleep(1000000 / PLAYBACK_SPEED);
    }
     ws2811_fini(&display);
     */
}

/**
 * @brief Update LEDs to new color
 * Updates the display's hardware LEDs color to the local leds variables array
 */
ws2811_return_t renderLEDs(){
    for (int x = 0; x < LED_WIDTH; x++) {
        for (int y = 0; y < LED_HEIGHT; y++) {
            // ==> pop conversation table in here (replace .leds[0] with .leds[convertToTASBot[0]]) or like that
            display.channel[0].leds[(y * LED_WIDTH) + x] = leds[y * LED_WIDTH + x];
        }
    }

    ws2811_return_t r;
    if ((r = ws2811_render(&display)) != WS2811_SUCCESS) {
        fprintf(stderr, "Failed to render: %s\n", ws2811_get_return_t_str(r));
    }
    return r;
}

/**
 * @brief Turns of all the LEDs
 * Clears all the LEDs by setting their color to black and renders it
 */
ws2811_return_t clearLEDs(){
    for (size_t i = 0; i < LED_COUNT; i++) {
        // ==> here to
        leds[i] = 0;
    }
    return renderLEDs();
}
//endregion

//region TASBot
void showBaseExpression(){
    char* baseExpr = getFilePath(GIF_PATH, BASE_EXPRESSION);
    playAnimation(baseExpr);
}

void showExpression(AnimationFrame* _frames, unsigned int _frameCount){
    for (int i = 0; i < _frameCount; ++i) {
        showFrame(&_frames[i]);
    }
}

void showFrame(AnimationFrame* _frame){

    leds[1 * LED_WIDTH + 0] = 0x00FF0000;

    leds[2 * LED_WIDTH + 0] = 0x0000FF00;

    leds[3 * LED_WIDTH + 0] = 0x000000FF;

    leds[4 * LED_WIDTH + 0] = 0x00FFFF00;

    leds[5 * LED_WIDTH + 0] = 0x0000FFFF;

    leds[6 * LED_WIDTH + 0] = 0x00FF00FF;

    /*
    for (int y = 0; y < LED_HEIGHT; ++y) {
        for (int x = 0; x < LED_WIDTH; ++x) {

            GifColorType* color = _frame->color[x][y];

            if (color->Red != 0 || color->Green != 0 || color->Blue != 0) {
                leds[y * LED_WIDTH + x] = 0x00FF0000;
                printf("x");

            } else {
                leds[y * LED_WIDTH + x] = 0x00000000;
                printf(" ");

            }
        }
        printf("\n");
    }
    printf("\n");
     */

    renderLEDs();
}

//endregion