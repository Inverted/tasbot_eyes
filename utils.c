#include <string.h>
#include <time.h>
#include <stdio.h>
#include "utils.h"

/**
 * Initialize a given string with the capacity based on given content
 * @param _str The string pointer, that is to initialize
 * @param _content The content for this string
 */
void initstr(string_t* _str, char* _content) {
    size_t length = strlen(_content);
    _str->capacity = length;
    _str->buffer = _content;
}

/**
 * Initialize a given string with a given capacity
 * @param _str The string pointer, that is to initialize
 * @param _cap The maximum capacity this string can hold
 */
void initcstr(string_t* _str, size_t _cap){
    _str->capacity = _cap;
    char buffer[_cap];
    _str->buffer = buffer;
}

/**
 * Allocate memory and create a new string with capacity based on content
 * @param _content The content for this string
 * @return The pointer to this new allocated string
 */
string_t* allocstr(char* _content) {
    //Allocate memory for string structure
    string_t* s = (string_t*) malloc(sizeof(string_t));
    if (!s) {
        string_t message;
        initstr(&message, "[ERROR] allocstr: Failed to allocate memory for string");
        failExit(&message);
    }

    //Get length of content and set it
    size_t length = strlen(_content);
    s->capacity = length;

    //Allocate memory for string content based on determined length
    char* buffer = (char*) malloc(length * sizeof(char));
    if (!buffer) {
        string_t message;
        initstr(&message, "[ERROR] allocstr: Failed to allocate memory for string buffer");
        failExit(&message);
    }

    //Set content of string
    s->buffer = buffer;
    strcpy(s->buffer, _content);

    //Return pointer to finalized string
    return s;
}

void freestr(string_t* _str) {
    free(_str->buffer);
    free(_str);
}

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

void failExit(string_t* _message){
    fprintf(stderr, "%s", _message->buffer);
    //todo:clearLEDs();
    exit(EXIT_FAILURE);
}