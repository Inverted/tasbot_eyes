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
#define BLINK_PATH              "./gifs/blink.gif"
#define MAX_FILENAME_LENGTH     256
#define MAX_PATH_LENGTH         4096
#define DEFAULT_DELAY_TIME      100
#define MAX_BLINKS              4                       //How many times does TASBot blink between animations
#define MIN_TIME_BETWEEN_BLINKS 4                       //Based on human numbers. We Blink about every 4 to 6 seconds
#define MAX_TIME_BETWEEN_BLINKS 6

#define LED_HEIGHT              8
#define LED_WIDTH               28

#define TARGET_FREQ             WS2811_TARGET_FREQ
#define GPIO_PIN                18
#define DMA                     10
#define LED_COUNT               (LED_WIDTH * LED_HEIGHT)
#define STRIP_TYPE              WS2811_STRIP_BRG
#define INVERTED                false
#define BRIGHTNESS              64

typedef struct AnimationFrame {
    GifColorType *color[LED_WIDTH][LED_HEIGHT];
    u_int16_t delayTime;
} AnimationFrame;

//Declarations
void setupHandler();
void finish(int _number);

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
void showBlinkExpression();
void showRandomExpression();
void showExpression(AnimationFrame** _frames, unsigned int _frameCount);
void showFrame(AnimationFrame* _frame);
ws2811_led_t translateColor(GifColorType* _color);
int getBlinkDelay();

void debugRenderer(GifColorType *_rgb);
unsigned int ledMatrixTranslation(int _x, int _y);
bool numberIsEven(int _number);

//Variables
bool verboseLogging = true;
bool useDebugRenderer = false;
bool activateLEDModule = true;
bool running = true;

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

//TODO: [Errors] mittels fprintf ausgeben

//TODO: Wie viele cyclen blink, bis nächste animation? ==> random
// zeit zwischen blinks auch random basierend auf variablen

//TODO: Extent Animation frame with bool. Check while parsing gif, if all pixels where white. If so,
// set the monochrom bool to true, which means select a random color while rendering.
// Have a predefined set of given colors (e.g. 6 colors), to choose from.
// Create bool* in playAnimation, which is given into every readFramePixels(), to determine if monochrom
// Encapsulate AnimationFrames in new struct Animation, containing the results of the bool and the frames

//TODO: Refine return values. No voids!

//=======TODO: FREE() UNUSED MEMORY, OMG :O=========
//=> For every malloc, there must be a free!

//TODO: Proper comments (doxygen and such)

//TODO: args:
//args: -s: playback speed; -I: specific image; -P: specific folder; -v: verbose logging; -h: help; -r: console renderer; -b: brightness [0-255]
//      -B: how many blinks between animation; -Bmin: min time between blinks; -Bmax: max time between blinks
int main() {
#if defined(__x86_64__)
    activateLEDModule = false;
#endif

    srand(time(NULL));
    setupHandler();

    if (activateLEDModule){
        ws2811_return_t r = initLEDs();
        if (r != WS2811_SUCCESS) {
            printf("[ERROR] Can't run program. Did you started it as root?\n");
            return r;
        }
    }

    bool firstIteration = true;
    while (running){
        //skip to base expression on first iteration, to not start on a random animation
        if (!firstIteration){
            showRandomExpression();
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
            if (verboseLogging){
                printf("[INFO] Blink #%d for %d seconds", blinks, blinkTime);
            }
            sleep(blinkTime);
        }
    }

    finish(0);
    return 0;
}

//determine how long to wait between blinks
int getBlinkDelay(){
    return MIN_TIME_BETWEEN_BLINKS + (rand() % (MAX_TIME_BETWEEN_BLINKS - MIN_TIME_BETWEEN_BLINKS));
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
            .sa_handler = finish,
    };
    sigemptyset(&sa.sa_mask);
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
    sigaction(SIGKILL, &sa, NULL);
}

/**
 * @brief Central handler for leaving the application
 * Handler for SIGINT, SIGTERM and SIGKILL
 * @param _number
 */
void finish(int _number) {
    printf("\n"); //pretty uwu

    running = false;
    if (activateLEDModule){
        clearLEDs();
        ws2811_render(&display);
        ws2811_fini(&display);
    }
    exit(_number);
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

    AnimationFrame* animationFrame = malloc(sizeof(AnimationFrame));

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
                printf("[WARNING] No color map given. Can't process picture. Skip frame");
            }
        }

        if (useDebugRenderer) {
            printf("\n");
        }
    }
    return animationFrame;
}

bool playAnimation(const char *file) {

    if (verboseLogging){
        printf("[INFO] Load file %s\n", file);
    }

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
        AnimationFrame** animationFrames = malloc(sizeof (AnimationFrame) * image->ImageCount);

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

            // Needs getDelayTime() to get actually tested
            /*
            u_int16_t delayTime = getDelayTime(frame);
            if (delayTime == 0) {
                delayTime = DEFAULT_DELAY_TIME;
            }
             */

            u_int16_t delayTime = DEFAULT_DELAY_TIME;

            animationFrames[i] = readFramePixels(frame, globalColorMap);
            animationFrames[i]->delayTime = delayTime;
        }
        showExpression(animationFrames, image->ImageCount);
    } else {
        printf("[ERROR] Image has wrong size (%dx%d). Required is (%dx%d)", image->SWidth, image->SHeight, LED_WIDTH,
               LED_HEIGHT);
        return false;
    }

    DGifCloseFile(image, &error);
    printf("[INFO] Closed GIF file with code %d\n", error);
    return true;
}

//TODO: This could be tidied up into one method with some refactoring...somehow...I bet
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
    } else {
        printf("[INFO] Initialized LEDs with code %d\n", r);
    }
    return r;
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
    } else {
        printf("[INFO] Rendered LEDs with code %d\n", r);
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
    playAnimation(BASE_PATH);
}

void showBlinkExpression(){
    playAnimation(BLINK_PATH);
}

void showRandomExpression(){
    int fileCount = countFilesInDir(OTHER_PATH); //get file count
    char *list[fileCount];
    getFileList(OTHER_PATH, list); //get list of files
    char *animation = getRandomAnimation(list, fileCount); //get random animation
    char* filePath = getFilePath(OTHER_PATH, animation);

    playAnimation(filePath);
}

void showExpression(AnimationFrame** _frames, unsigned int _frameCount){
    for (int i = 0; i < _frameCount; ++i) {
        showFrame(_frames[i]);

        //usleep(1000000 / PLAYBACK_SPEED);
        usleep(_frames[i]->delayTime * 1000);
    }

    //free _frames
}

void showFrame(AnimationFrame* _frame){
    if (verboseLogging){
        printf("[INFO] Render frame: \n");
    }

    for (int y = 0; y < LED_HEIGHT; ++y) {
        for (int x = 0; x < LED_WIDTH; ++x) {
            GifColorType* color = _frame->color[x][y];

            if (activateLEDModule){
                leds[ledMatrixTranslation(x, y)] = translateColor(color);
            }

            //Debug renderer
            if (color->Red != 0 || color->Green != 0 || color->Blue != 0) {
                if (verboseLogging){
                    printf("x");
                }
            } else {
                if (verboseLogging){
                    printf(" ");
                }
            }
        }
        if (verboseLogging){
            printf("\n");
        }
    }

    if (activateLEDModule){
        renderLEDs();
    }
}

ws2811_led_t translateColor(GifColorType* _color){
    return ((_color->Red & 0xff) << 16) + ((_color->Green & 0xff) << 8) + (_color->Blue & 0xff);
}
//endregion

//region Debug and Development
//Wenn x eine ungerade Zahl ist, dann []
//Wenn x eine   gerade Zahl ist, dann [x * LED_HEIGHT + y]
unsigned int ledMatrixTranslation(int _x, int _y){
    if (numberIsEven(_x)){
        return (_x * LED_HEIGHT + _y);
    } else {
        return (_x * LED_HEIGHT + LED_HEIGHT - 1 - _y);
    }
}

bool numberIsEven(int _number){
    return (_number % 2 == 0);
}
//endregion