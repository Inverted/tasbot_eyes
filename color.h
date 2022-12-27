#ifndef TASBOT_EYES_COLOR_H
#define TASBOT_EYES_COLOR_H

#define HUE_MAX 16384

extern const unsigned char gamma8[256];

float fract(float x);

float mix(float a, float b, float t);

float constrain(float x, float low, float high);

float hueToFloat(int _hue);

int valueToInt(float _value);

float* hsv2rgb(float h, float s, float b, float* rgb);

#endif //TASBOT_EYES_COLOR_H
