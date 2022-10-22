#ifndef TASBOT_EYES_UTILS_H
#define TASBOT_EYES_UTILS_H

#include <stdlib.h>

typedef struct string_t {
    char* buffer;
    size_t capacity; //total capacity, rather then length
} string_t;

void initstr(string_t* _str, char* _content);
void initcstr(string_t* _str, size_t _cap);
string_t* allocstr(char* _content);
void freestr(string_t* _str);

void shuffle(int* array, size_t n);
void failExit(string_t* _message);

#endif //TASBOT_EYES_UTILS_H
