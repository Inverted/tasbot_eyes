#include <stdbool.h>
#include <ws2811/ws2811.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include "tasbot.h"
#include "gif.h"
#include "arguments.h"
#include "color.h"
#include "palette.h"
#include "led.h"
#include "filesystem.h"

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
float playbackSpeed = 1;
ws2811_led_t defaultColor = -1;
bool playbackSpeedAffectBlinks = false;
bool useGammaCorrection = false;
bool useRandomColors = false;
bool useRandomColorsForAll = false;

//Rainbow mode
int* hue;
int huePid = -1;
bool rainbowMode = false;
//AnimationFrame currentFrame;


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

    if (verboseLogging) {
        printf("[INFO] Use a random color: %s\n", randColor ? "true" : "false");
        printf("[INFO] Use the default color: %s\n", randColor ? "true" : "false");
    }

    //If applies, choose color to overwrite
    ws2811_led_t color = 0;
    if (!rainMode) {
        if (randColor) {
            int r = rand() % paletteCount;
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
            if (verboseLogging) {
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
            //memcpy(&currentFrame, _animation->frames[i], sizeof (AnimationFrame));
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