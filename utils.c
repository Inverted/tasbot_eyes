#include <string.h>
#include <stdio.h>
#include "utils.h"

#include "led.h"

void shuffle(int* array, size_t n) {
    if (n > 1) {
        size_t i;
        for (i = 0; i < n - 1; i++) {
            size_t j = i + rand() / (RAND_MAX / (n - i) + 1);
            int t = array[j];
            array[j] = array[i];
            array[i] = t;
        }
    }
}

void failExit(char* _message){
    fprintf(stderr, "%s", _message);
    clearLEDs();
    exit(EXIT_FAILURE);
}