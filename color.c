#include "color.h"

//By Karsten "postspectacular" Schmidt
//From https://gist.github.com/postspectacular/2a4a8db092011c6743a7#file-hsv2rgb-ino-L29
float fract(float x) { return x - (int) x; }

float mix(float a, float b, float t) { return a + (b - a) * t; }

float constrain(float x, float low, float high) { return ((x) < (low) ? (low) : ((x) > (high) ? (high) : (x))); }

float* hsv2rgb(float h, float s, float b, float* rgb) {
    rgb[0] = b * mix(1.0F, constrain(fabsf(fract(h + 1.0F) * 6.0F - 3.0F) - 1.0F, 0.0F, 1.0F), s);
    rgb[1] = b * mix(1.0F, constrain(fabsf(fract(h + 0.6666666F) * 6.0F - 3.0F) - 1.0F, 0.0F, 1.0F), s);
    rgb[2] = b * mix(1.0F, constrain(fabsf(fract(h + 0.3333333F) * 6.0F - 3.0F) - 1.0F, 0.0F, 1.0F), s);
}

float hueToFloat(int _hue) {
    return (float) _hue / 255;
}

int valueToInt(float _value) {
    return (int) (_value * 255);
}