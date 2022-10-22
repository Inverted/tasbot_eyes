#include <stdlib.h>
#include <stdio.h>
#include "gif.h"
#include "arguments.h"
#include "led.h"

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
            printf("[INFO] (Image info): Size: %ix%i; Frames: %i; Path: \"%s\"\n", image->SWidth, image->SHeight,
                   image->ImageCount, _file);
        }

        //Process frames
        animation = malloc(sizeof(Animation));
        AnimationFrame** animationFrames = malloc(sizeof(AnimationFrame*) * image->ImageCount);
        animation->frames = animationFrames;
        animation->frameCount = image->ImageCount;
        animation->monochrome = true;
        animation->image = image;

        GraphicsControlBlock gcb;
        for (int i = 0; i < image->ImageCount; ++i) {
            const SavedImage* frame = &image->SavedImages[i]; //get access to frame data

            u_int16_t delayTime = DEFAULT_DELAY_TIME;
            if (DGifSavedExtensionToGCB(image, i, &gcb) == GIF_ERROR) {
                printf("[WARNING] Can't read frame delay. Using default delay time");
            } else {
                delayTime = gcb.DelayTime * 10;
                if (delayTime < MIN_DELAY_TIME) {
                    printf("[WARNING] Delay time is smaller than allowed smallest delay time (%d ms). Using that.\n",
                           MIN_DELAY_TIME);
                    delayTime = MIN_DELAY_TIME;
                }
            }

            if (verboseLogging) {
                printf("[INFO] (Frame %i info): Size: %ix%i; Delay time: %i ms; Left: %i, Top: %i; Local color map: %s\n",
                       i, frame->ImageDesc.Width, frame->ImageDesc.Height, delayTime, frame->ImageDesc.Left,
                       frame->ImageDesc.Top, (frame->ImageDesc.ColorMap ? "Yes" : "No"));
            }

            animationFrames[i] = readFramePixels(frame, globalColorMap, &animation->monochrome);
            animationFrames[i]->delayTime = delayTime;
        }

        if (verboseLogging) {
            printf("[INFO] Animation is monochrome: %s\n", animation->monochrome ? "true" : "false");
        }

    } else {
        fprintf(stderr, "[ERROR] Image has wrong size (%dx%d). Required is (%dx%d)", image->SWidth, image->SHeight,
                LED_WIDTH, LED_HEIGHT);
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

bool isGrayScale(GifColorType* _color) {
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