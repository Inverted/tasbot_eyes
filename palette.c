#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "palette.h"
#include "arguments.h"
#include "filesystem.h"
#include "led.h"

ws2811_led_t* palette;
unsigned int paletteCount;

/**
 * Main function to read a palette file, convert it into numerical values and providing the final color array
 * @param _path The color palette file, that is to read
 */
void readPalette(char* _path) {
    //Read raw palette from file into the rawPal variable
    int colorCount = countLines(_path);
    char* rawPal[colorCount];
    readFile(_path, colorCount, rawPal);

    //Read palette into final palette array
    ws2811_led_t* pal = malloc(sizeof(ws2811_led_t) * colorCount); //todo: free?
    if (!pal) {
        fprintf(stderr, "[ERROR] readPalette: Failed to allocate palette memory");
        clearLEDs();
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < colorCount; ++i) {
        ws2811_led_t color = strtocol(rawPal[i]);
        if (color != -1) {
            pal[i] = color;
            if (verbose) {
                printf("[INFO] Converted string \"%s\" to integer in hex 0x%x\n", rawPal[i], pal[i]);
            }
        } else {
            printf("[WARNING] Skip color %s because of parsing error", rawPal[i]);
        }
        free(rawPal[i]);
    }

    paletteCount = colorCount;
    palette = pal;
}

/**
 * Converts a sequenz of hexadecimal characters into an actual integer. Meant to be used with hexadecimal color strings
 * @param _color The color string, that should be converted
 * @return An integer, that contains the actual numeric value of the given _color string
 */
ws2811_led_t strtocol(char* _color) {
    ws2811_led_t result = 0;
    int len = (int) strlen(_color);

    //Convert string character by character into an actual integer
    for (int i = 0; i < len; i++) {
        int hex = chtohex(_color[i]);
        if (hex != -1) {
            //Bit-shift by 4, because hex numbers use only lower 4 bits OR all of it together.
            result = result | hex << (len - i - 1) * 4;
        } else {
            fprintf(stderr, "[ERROR] Can't read character '%c' color \"%s\". Please check formatting!\n", _color[i],
                    _color);
            return -1;
        }
    }
    return result;
}

/**
 * Convert a given char into it's actually numerical value. Supports 0-9, a-f and A-F
 * @param _ch A character, that contains the sign of an hexadecimal number
 * @return The numerical value of that character
 */
int chtohex(char _ch) {
    int result = -1;

    //For "normal" decimal number character (0-9)
    if (_ch - '0' >= 0 && _ch - '0' <= 9) {
        result = _ch - '0';

        //For hexadecimal numbers (a-f), lower case
    } else if (_ch - 'a' >= 0 && _ch - 'a' <= 'f' - 'a') {
        result = _ch - 'a' + 10;

        //For hexadecimal numbers (A-F), upper case
    } else if (_ch - 'A' >= 0 && _ch - 'A' <= 'F' - 'A') {
        result = _ch - 'A' + 10;
    }
    return result;
}