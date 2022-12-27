#include <stdbool.h>
#include <ws2811/ws2811.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include "tasbot.h"
#include "gif.h"
#include "arguments.h"
#include "color.h"
#include "palette.h"
#include "led.h"
#include "filesystem.h"
#include "stack.h"

//TASBot display conversation table
//Based on https://github.com/jakobrs/tasbot-display/blob/master/src/tasbot.rs
int TASBotIndex[8][28] = {{-1, -1, 0,  1,  2,  3,  -1, -1, -1, -1, 101, 100, 99, 98, 97, 96, 95, 94, -1, -1, -1,  -1,  105, 104, 103, 102, -1,  -1},
                          {-1, 4,  5,  6,  7,  8,  9,  -1, -1, 84, 85,  86,  87, 88, 89, 90, 91, 92, 93, -1, -1,  111, 110, 109, 108, 107, 106, -1},
                          {10, 11, 12, 13, 14, 15, 16, 17, -1, -1, -1,  -1,  -1, -1, -1, -1, -1, -1, -1, -1, 119, 118, 117, 116, 115, 114, 113, 112},
                          {18, 19, 20, 21, 22, 23, 24, 25, -1, -1, -1,  83,  82, 81, 80, 79, 78, -1, -1, -1, 127, 126, 125, 124, 123, 122, 121, 120},
                          {26, 27, 28, 29, 30, 31, 32, 33, -1, -1, 70,  71,  72, 73, 74, 75, 76, 77, -1, -1, 135, 134, 133, 132, 131, 130, 129, 128},
                          {34, 35, 36, 37, 38, 39, 40, 41, -1, -1, -1,  -1,  -1, -1, -1, -1, -1, -1, -1, -1, 143, 142, 141, 140, 139, 138, 137, 136},
                          {-1, 42, 43, 44, 45, 46, 47, -1, -1, -1, 68,  67,  66, 65, 64, 63, 62, 61, -1, -1, -1,  149, 148, 147, 146, 145, 144, -1},
                          {-1, -1, 48, 49, 50, 51, -1, -1, -1, 69, 52,  53,  54, 55, 56, 57, 58, 59, 60, -1, -1,  -1,  153, 152, 151, 150, -1,  -1}};


int maxBlinks = MAX_BLINKS;
int minTimeBetweenBlinks = MIN_TIME_BETWEEN_BLINKS;
int maxTimeBetweenBlinks = MAX_TIME_BETWEEN_BLINKS;
int repetitions = 1;
int* hue;
float playbackSpeed = 1;
ws2811_led_t defaultColor = -1;
bool playbackSpeedAffectBlinks = false;
bool useGammaCorrection = false;
bool useRandomColors = false;
bool useRandomColorsForAll = false;
bool rainbowMode = false;


void fillStack(char* _sourceFolder) {
    int fileCount = countFilesInDir(_sourceFolder); //get file count

    //if (fileCount != -1) {
    if (fileCount > 0) {

        //Fill array with all possible indices
        int index[fileCount];
        for (int i = 0; i < fileCount; ++i) {
            index[i] = i;
        }

        //Shuffle said array
        shuffle(index, fileCount);

        //get list of files
        char* list[fileCount];
        getFileList(_sourceFolder, list);

        for (int i = 0; i < fileCount; ++i) {
            char* filePath = getFilePath(pathForAnimations, list[index[i]]);
            addToStack(filePath);
            free(list[index[i]]);
        }
    }
}

/**
 * Add a given path to the animation stack
 * @param _path Path to an animation that's allocated in memory already!
 */
bool addToStack(char* _path) {
    bool result;
    if (push(_path)) {
        char* speek = (char*) peek();
        if (verbose) {
            printf("[INFO] Successfully added (%s) to animation stack\n",
                   speek); //Using peeked value to reinsure it got added properly
        }
        result = true;
    } else {
        printf("[WARNING] Failed to add (%s) to animation stack. Stack most likely full\n", _path);
        result = false;
    }

    return result;
}

/**
 * Show a random animation in TASBots display
 * @param _path Path, from where a random animation should be chosen from
 * @param _useRandomColor If the animation can be played with an randomly chosen color, if it's monochrome
 */
void playRandomAnimationFromDirectory(char* _path, bool _useRandomColor, bool _repeatAnimations) {
    //get count of files in directory
    int fileCount = countFilesInDir(_path);

    if (fileCount != -1) {
        //get list of files
        char* list[fileCount];
        getFileList(_path, list);

        //get random animation;
        char* file = getRandomAnimation(list, fileCount);
        char* filePath = getFilePath(_path, file);

        playAnimationFromFilepath(filePath, _useRandomColor, _repeatAnimations);

        for (int i = 0; i < fileCount; ++i) {
            free(list[i]);
        }
        free(filePath);
    } else {
        fprintf(stderr, "[ERROR] No files in %s. Please check directory\n", _path);
    }
}

/**
 * Play one specific animation from given file
 * @param _filePath That should be played
 */
void playAnimationFromFilepath(char* _filePath, bool _useRandomColor, bool _repeatAnimations) {
    Animation* animation = readAnimation(_filePath);

    if (!animation) {
        fprintf(stderr, "[WARNING] playAnimationFromFilepath: animation is NULL, skipping it\n");
    } else {
        playAnimation(animation, _useRandomColor, _repeatAnimations);
    }
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
void playAnimation(Animation* _animation, bool _useRandomColor, bool _repeatAnimations) {
    bool randColor;
    if (useRandomColorsForAll) {
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
        if (_animation->monochrome) {
            defColor = true;
        }
    }

    bool rainMode = rainbowMode ? _animation->monochrome : false;

    if (verbose) {
        printf("[INFO] Use a random color: %s\n", randColor ? "true" : "false");
        printf("[INFO] Use the default color: %s\n", randColor ? "true" : "false");
    }

    //If applies, choose color to overwrite
    ws2811_led_t color = 0;
    if (!rainMode) {
        if (randColor) {
            unsigned int r = rand() % paletteCount;
            color = palette[r];
        } else if (defColor) {
            color = defaultColor;
        }
    }

    //Set correct amount of repetitions
    int reps = repetitions;
    if (!_repeatAnimations) {
        reps = 1;
    }

    //Show frame
    for (int r = 0; r < reps; ++r) {
        for (int i = 0; i < _animation->frameCount; ++i) {
            if (verbose) {
                printf("[INFO] Render frame #%d \n", i);
            }

            if (rainMode && randColor) {
                float rgb[3];
                hsv2rgb(hueToFloat(*hue), 1, 1, rgb);

                GifColorType rgbColor;
                rgbColor.Red = valueToInt(rgb[0]);
                rgbColor.Green = valueToInt(rgb[1]);
                rgbColor.Blue = valueToInt(rgb[2]);

                color = translateColor(&rgbColor, useGammaCorrection);
            }

            showFrame(_animation->frames[i], color);
            usleep((int) ((float) (_animation->frames[i]->delayTime * 1000) / playbackSpeed));
        }
    }

    //Remove entire animation from memory
    freeAnimation(_animation);
}

/**
 * Map color to buffer based on given render device and put into buffer
 * @param _x X-coordinate for pixel
 * @param _y Y-coordinate for pixel
 * @param _color Color, the LED should be set to
 */
void setBufferAtIndex(unsigned int _x, unsigned int _y, ws2811_led_t _color) {
    if (activateLEDModule) {
        if (realTASBot) {
            int index = TASBotIndex[_y][_x];
            if (index >= 0) {
                setSpecificPixel(index, _color);
                //buffer[index] = color;
            }
        } else {
            setSpecificPixel(ledMatrixTranslation(_x, _y), _color);
            //buffer[ledMatrixTranslation(x, y)] = color;
        }
    }
}

/**
 * Output the buffer data to the LED data structure, which then gets rendered.
 * @param _frame The frame, that is to render
 * @param _color The color, which should overwrite the actual color data from the frame and only used, when the
 * animation is monochrome. Otherwise, it's NULL and used to indicate, that animation has its own color.
 */
void showFrame(AnimationFrame* _frame, ws2811_led_t _color) {

//    lockBuffer();
    for (int y = 0; y < LED_HEIGHT; ++y) {
        for (int x = 0; x < LED_WIDTH; ++x) {

            //make sure, when we ain't using the real time control, and it's not a center buffer
            bool centerPixel = x >= NOSE_RANGE_MIN && x <= NOSE_RANGE_MAX;
            if (!(useRealtimeControl && centerPixel)) {

                GifColorType* gifColor = _frame->color[x][y];
                ws2811_led_t color;

                if (activateLEDModule) {
                    if (_color == 0) {
                        color = translateColor(gifColor, useGammaCorrection);
                    } else {
                        if (gifColor->Red != 0 || gifColor->Green != 0 || gifColor->Blue != 0) {
                            color = _color;
                            //todo: Adjust to brightness of gifColor given in GIF
                            // Right now it's flat the same gifColor to all pixels, that just _aren't_ black
                            // Use function getLuminance() for that
                        } else {
                            color = 0;
                        }
                    }

                    setBufferAtIndex(x, y, color);
                }

                //Debug renderer
                if (consoleRenderer) {
                    if (gifColor->Red != 0 || gifColor->Green != 0 || gifColor->Blue != 0) {
                        printf("█");
                    } else {
                        printf(" ");
                    }
                }
            } else if (consoleRenderer) {
                printf("░");
            }
        }
        if (consoleRenderer) {
            printf("\n");
        }
    }
    //unlockBuffer();

    if (activateLEDModule) {
        renderLEDs();
    }
}

void setNoseLED(unsigned int _index, GifColorType _color) {
    if (activateLEDModule) {
        /*
         * y=
         * 0 -> 0
         * 1 -> 1
         *
         * 2 -> 3
         * 3 -> 4
         *
         * 4 -> 6
         * 5 -> 7
         */

        unsigned int y = _index / FIELD_WIDTH;
        switch (y) {
            case 2:
            case 3:
                y++;
                break;
            case 4:
            case 5:
                y += 2;
                break;
            default:
                //default for shutting Clang-Tidy
                break;
        }

        //x += MIN+1
        unsigned int x = (NOSE_RANGE_MIN + 1) + (_index % FIELD_WIDTH);

        ws2811_led_t color = translateColor(&_color, false);
        setBufferAtIndex(x, y, color);

        //edge cases
        if (x == 10){
            if (y == 1 || y == 7){
                setBufferAtIndex(x-1, y, color);
            }
        }

        if (x == 17 ){
            if (y == 1 || y == 7){
                setBufferAtIndex(x+1, y, color);
            }
        }
    }
}

/**
 * Free up the allocated memory space for an animation
 * @param _animation The animation, that is to free from memory
 */
void freeAnimation(Animation* _animation) {
    //dirty trick, close file here, after animation. That way DGifCloseFile() can't mess with the animation data
    int e = 0;
    if (DGifCloseFile(_animation->image, &e) != GIF_OK) {
        fprintf(stderr, "[WARNING] freeAnimation: Closing GIF failed. DGifCloseFile returned %d\n", e);
    }

    if (verbose) {
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
 * Get a random entry from a list
 * @param list List, from which the random item should be chosen
 * @param _count Length of list
 * @return A random item from the list
 */
char* getRandomAnimation(char* list[], int _count) {
    int r = rand() % _count;
    return list[r];
}

/**
 * Get the luminance of a given color
 * @param _color The color the luminance should be calculated of
 * @return The luminance of the given color
 */
float getLuminance(GifColorType* _color) {
    return (0.2126F * (float) _color->Red + 0.7152F * (float) _color->Green + 0.0722F * (float) _color->Blue);
}

//region Debug and Development
/**
 * Converts buffer coordinate of a frame into the the actual coordinate for the 32x8 LED matrix R3tr0BoiDX aka Mirbro used during development
 * @param _x x-coordinate of the frame
 * @param _y y-coordinate of the frame
 * @return index of the LED for a 32x8 LED matrix
 */
unsigned int ledMatrixTranslation(unsigned int _x, unsigned int _y) {
    if (numberIsEven(_x)) {
        return (_x * LED_HEIGHT + _y);
    }

    return (_x * LED_HEIGHT + LED_HEIGHT - 1 - _y);
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
