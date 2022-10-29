#ifndef TASBOT_EYES_GIF_H
#define TASBOT_EYES_GIF_H

#include <gif_lib.h>
#include <stdbool.h>
#include "led.h"
#include "utils.h"

#define MIN_DELAY_TIME          35                      //Smallest delay time that is possible due to hardware limitations (1000ms/30fps=33.3'ms)
#define DEFAULT_DELAY_TIME      100

typedef struct AnimationFrame {
    GifColorType* color[LED_WIDTH][LED_HEIGHT];
    unsigned short int delayTime;
} AnimationFrame;

typedef struct Animation {
    AnimationFrame** frames; //pointer to a pointer, that's an array of frames.
    int frameCount;
    bool monochrome;
    GifFileType* image; //needed for very dirty trick to get around weird behavior, where DGifCloseFile() manipulates the animation data for some reason
} Animation;

bool checkIfImageHasRightSize(GifFileType* _image);
bool isGrayScale(GifColorType* _color);
AnimationFrame* readFramePixels(const SavedImage* frame, ColorMapObject* _globalMap, bool* _monochrome);
Animation* readAnimation(string_t * _filePath);

#endif //TASBOT_EYES_GIF_H
