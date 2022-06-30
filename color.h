//
// Created by mirco on 30.06.22.
//

#ifndef TASBOT_EYES_COLOR_H
#define TASBOT_EYES_COLOR_H

#include <stdio.h>
#include <unistd.h>
#include <sys/mman.h>
#include <math.h>

//By Karsten "postspectacular" Schmidt
//From https://gist.github.com/postspectacular/2a4a8db092011c6743a7#file-hsv2rgb-ino-L29
float fract(float x);// { return x - (int) x; }

float mix(float a, float b, float t);// { return a + (b - a) * t; }

float constrain(float x, float low, float high);// { return ((x) < (low) ? (low) : ((x) > (high) ? (high) : (x))); }

float* hsv2rgb(float h, float s, float b, float* rgb);

float hueToFloat(int _hue);

int valueToInt(float _value);


#endif //TASBOT_EYES_COLOR_H
