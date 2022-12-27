#ifndef TASBOT_EYES_TASBOT_H
#define TASBOT_EYES_TASBOT_H

#include "gif.h"
#include "utils.h"

#define NOSE_RANGE_MIN          9
#define NOSE_RANGE_MAX          18
#define FIELD_WIDTH             8
#define FIELD_HEIGHT            6

#define MAX_BLINKS              4 //How many times does TASBot blink between animations
#define MIN_TIME_BETWEEN_BLINKS 4 //Based on human numbers. We Blink about every 4 to 6 seconds
#define MAX_TIME_BETWEEN_BLINKS 6

#define HUE_THREAD_SLEEP        10 //ms

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

extern bool rainbowMode;
extern int hue; //max 255
extern pthread_t hueThread;

//stack
void fillStack(char* _sourceFolder);
bool addToStack(char* _path);

//animations
void playRandomAnimationFromDirectory(char* _path, bool _useRandomColor, bool _repeatAnimations);
void playAnimationFromFilepath(char* _filePath, bool _useRandomColor, bool _repeatAnimations);
void playAnimation(Animation* _animation, bool _useRandomColor, bool _repeatAnimations);
void showFrame(AnimationFrame* _frame, ws2811_led_t _color);
void freeAnimation(Animation* _animation);
char* getRandomAnimation(char* list[], int _count);

//blink
unsigned int getBlinkDelay();
unsigned int getBlinkAmount();

//led
void setBufferAtIndex(unsigned int _x, unsigned int _y, ws2811_led_t _color);
void setNoseLED(unsigned int _index, GifColorType _color);

//misc
float getLuminance(GifColorType* _color);
void startHueThread();
void* fadeHue(void* vargp);

//Debug
unsigned int ledMatrixTranslation(unsigned int _x, unsigned int _y);
bool numberIsEven(unsigned int _number);

#endif //TASBOT_EYES_TASBOT_H