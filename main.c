#include <stdio.h>
#include <dirent.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#include <gif_lib.h> //https://sourceforge.net/projects/giflib/
#include <ws2811/ws2811.h> //https://github.com/jgarff/rpi_ws281x

#define PATH                    "./gifs/"
#define MAX_FILENAME_LENGTH     256
#define MAX_PATH_LENGTH         4096
#define DEFAULT_DELAY_TIME      100

#define LED_HEIGHT              8
#define LED_WIDTH               28

//TODO: Signal handlers
//TODO: [Errors] mittels fprintf ausgeben

typedef struct AnimationFrame {
    GifColorType *rgb[LED_WIDTH][LED_HEIGHT];
    u_int16_t delayTime;
} AnimationFrame;

//Declarations
bool readAnimation(const char *file);
int countFilesInDir(char _path[]);
bool getFileList(const char _path[], char *_list[]);
char *getRandomAnimation(char *list[], int _count);
bool checkIfImageHasRightSize(GifFileType *_image);
u_int16_t getDelayTime(SavedImage *_frame);
AnimationFrame *readFramePixels(const SavedImage *frame, ColorMapObject *_globalMap);

void debugRenderer(GifColorType *_rgb);
int ledStuff();

//Variables
bool verboseLogging = false;
bool useDebugRenderer = false;

//args: -s: playback speed; -I: specific image; -P: specific folder; -v: verbose logging; -h: help; -r: debug renderer; -b: brightness [0-255]
int main() {
    srand(time(NULL));

    int fileCount = countFilesInDir(PATH); //get file count
    char *list[fileCount];
    getFileList(PATH, list); //get list of files
    char *animation = getRandomAnimation(list, fileCount); //get random animation

    //allocate memory for path string and ditch together
    char *path = malloc(sizeof(char) * (MAX_PATH_LENGTH + MAX_FILENAME_LENGTH));
    strcpy(path, PATH);
    strcat(path, animation);

    //read animation from path
    readAnimation(path);

    ledStuff();

    return 0;
}


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
            if (_frame->ExtensionBlocks[e].Function ==
                0xF9) { //F9 is the identifier for "Graphics Control Extension"-blocks
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
                animationFrame->rgb[x][y] = rgb;

                if (useDebugRenderer) {
                    debugRenderer(rgb);
                }

                if (verboseLogging) {
                    printf("(%03i,%03i,%03i)", rgb->Red, rgb->Green, rgb->Blue);
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

bool readAnimation(const char *file) {
    //Open file
    int error;
    GifFileType *image = DGifOpenFileName(file, &error);
    if (!image) {
        printf("[ERROR] EGifOpenFileName() failed: %d\n", error);
        return false;
    }

    //"Slurp" infos into struct
    if (DGifSlurp(image) == GIF_ERROR) {
        printf("[ERROR] DGifSlurp() failed: %d\n", image->Error);
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
        AnimationFrame *animationFrames[image->ImageCount]; // = malloc(sizeof (AnimationFrame)*image->ImageCount);
        for (int i = 0; i < image->ImageCount; ++i) {
            const SavedImage *frame = &image->SavedImages[i]; //get access to frame data

            if (verboseLogging) {
                printf("[Frame %i] Size: %ix%i; Left: %i, Top: %i; Local color map: %s\n",
                       i, frame->ImageDesc.Width, frame->ImageDesc.Height, frame->ImageDesc.Left, frame->ImageDesc.Top,
                       (frame->ImageDesc.ColorMap ? "Yes" : "No"));
            }

            // Needs the getDelayTime() get actually tested
            /*
            u_int16_t delayTime = getDelayTime(frame);
            if (delayTime == 0) {
                delayTime = DEFAULT_DELAY_TIME;
            }
            */
            u_int16_t delayTime = DEFAULT_DELAY_TIME;

            if (useDebugRenderer) {
                printf("[Frame: %d]\n", i);
            }

            animationFrames[i] = readFramePixels(frame, globalColorMap);
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

//==============================

#define TARGET_FREQ             WS2811_TARGET_FREQ
#define GPIO_PIN                18
#define DMA                     10
#define LED_COUNT               (LED_WIDTH * LED_HEIGHT)
#define STRIP_TYPE              WS2811_STRIP_GBR
#define INVERTED                false
#define BRIGHTNESS              255

ws2811_t ledstring = {
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
                        [1] ={
                            .gpionum = 0,
                            .count = 0,
                            .invert = 0,
                            .brightness = 0,
                        },
                },
        };

int ledStuff() {
    ws2811_return_t ret;
    ws2811_led_t *matrix = malloc(sizeof(ws2811_led_t) * LED_WIDTH * LED_HEIGHT);

    if ((ret = ws2811_init(&ledstring)) != WS2811_SUCCESS) {
        fprintf(stderr, "ws2811_init failed: %s\n", ws2811_get_return_t_str(ret));
        return ret;
    }

}