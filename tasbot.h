#ifndef TASBOT_EYES_TASBOT_H
#define TASBOT_EYES_TASBOT_H

#include "gif.h"
#include "utils.h"

#define MAX_BLINKS              4                       //How many times does TASBot blink between animations
#define MIN_TIME_BETWEEN_BLINKS 4                       //Based on human numbers. We Blink about every 4 to 6 seconds
#define MAX_TIME_BETWEEN_BLINKS 6

extern int TASBotIndex[8][28];

extern int maxBlinks;
extern int minTimeBetweenBlinks;
extern int maxTimeBetweenBlinks;
extern int repetitions;
extern float playbackSpeed;
extern ws2811_led_t defaultColor;
extern bool playbackSpeedAffectBlinks;
extern bool useGammaCorrection;
extern bool useRandomColors;
extern bool useRandomColorsForAll;

//Rainbow mode
extern int* hue;
extern int huePid;
extern bool rainbowMode;
//extern AnimationFrame currentFrame;

void showBlinkExpression();
void showRandomExpression(string_t* _path, bool _useRandomColor, bool _repeatAnimations);
void showExpressionFromFilepath(string_t* _filePath, bool _useRandomColor, bool _repeatAnimations);
void playExpression(Animation* _animation, bool _useRandomColor, bool _repeatAnimations);
void showFrame(AnimationFrame* _frame, ws2811_led_t _color);
void freeAnimation(Animation* _animation);
unsigned int getBlinkDelay();
unsigned int getBlinkAmount();
string_t* getRandomAnimation(string_t* list[], int _count);

float getLuminance(GifColorType* _color);

//Debug
unsigned int ledMatrixTranslation(int _x, int _y);
bool numberIsEven(int _number);

#endif //TASBOT_EYES_TASBOT_H