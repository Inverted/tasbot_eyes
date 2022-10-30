#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "led.h"
#include "color.h"
#include "arguments.h"

int brightness = BRIGHTNESS;
int dataPin = GPIO_PIN;

ws2811_led_t* pixel;
ws2811_t display;

/**
 * Initialize the LEDs and their data structure
 * @return Infos about, if initialization was successful
 */
void initLEDs() {
    if (activateLEDModule) {
        //Setup display
        memset(&display, 0, sizeof(ws2811_t));
        display.freq = TARGET_FREQ;
        display.dmanum = DMA;

        ws2811_channel_t* channel = calloc(1, sizeof(ws2811_channel_t));
        channel->gpionum = dataPin;
        channel->count = LED_COUNT;
        channel->invert = INVERTED;
        channel->brightness = brightness;
        channel->strip_type = STRIP_TYPE;
        display.channel[0] = *channel; //todo: free at end

        //Setup color array
        pixel = malloc(sizeof(ws2811_led_t) * LED_WIDTH * LED_HEIGHT);
        if (!pixel) {
            fprintf(stderr, "[ERROR] initLEDs: Failed to allocate memory for render buffer");
            exit(EXIT_FAILURE);
        }

        //Initialize hardware
        ws2811_return_t r;
        if ((r = ws2811_init(&display)) != WS2811_SUCCESS) {
            fprintf(stderr, "[ERROR] ws2811_init failed. Couldn't initialize LEDs: %s\n", ws2811_get_return_t_str(r));
        } else {
            if (verbose) {
                printf("[INFO] Initialized LEDs with code %d\n", r);
            }
        }
        clearLEDs();

        if (r != WS2811_SUCCESS) {
            fprintf(stderr, "[ERROR] Can't run program. Did you started with root privileges?\n");
            exit(EXIT_FAILURE);
        }
    } //else
    if (verbose) {
        printf("[INFO] Starting program without LED module\n");
    }
}

/**
 * Updates the display's hardware LEDs color to the local pixel variables array
 * @return Infos about, if the LEDs where rendered successful
 */
ws2811_return_t renderLEDs() {
    for (int x = 0; x < LED_WIDTH; x++) {
        for (int y = 0; y < LED_HEIGHT; y++) {
            display.channel[0].leds[(y * LED_WIDTH) + x] = pixel[y * LED_WIDTH + x];
        }
    }

    ws2811_return_t r;
    if ((r = ws2811_render(&display)) != WS2811_SUCCESS) {
        fprintf(stderr, "[ERROR] Failed to render: %s\n", ws2811_get_return_t_str(r));
    } else {
        if (verbose) {
            printf("[INFO] Rendered LEDs with code %d\n", r);
        }
    }

    return r;
}

/**
 * Clears all the LEDs by setting their color to black and renders it
 */
ws2811_return_t clearLEDs() {
    for (size_t i = 0; i < (size_t) LED_COUNT; i++) {
        pixel[i] = 0;
    }
    return renderLEDs();
}

/**
 * Translate the RGB (255,255,255) color structure into a hexadecimal value
 * @param _color The RGB color, that is to convert
 * @return The convert hexadecimal color
 */
ws2811_led_t translateColor(GifColorType* _color, bool _useGammaCorrection) {
    if (_useGammaCorrection) { //TODO: when used, breaks things
        _color->Red = gamma8[_color->Red];
        _color->Green = gamma8[_color->Green];
        _color->Blue = gamma8[_color->Blue];
    }
    return ((_color->Red & 0xff) << 16) + ((_color->Green & 0xff) << 8) + (_color->Blue & 0xff);
}