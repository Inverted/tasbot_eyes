#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include "led.h"
#include "color.h"
#include "arguments.h"
#include "tasbot.h"

int brightness = BRIGHTNESS;
int dataPin = GPIO_PIN;

ws2811_led_t* buffer;
ws2811_t display;

pthread_t renderThread;

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
        buffer = malloc(sizeof(ws2811_led_t) * LED_WIDTH * LED_HEIGHT);
        if (!buffer) {
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
 * Updates the display's hardware LEDs color to the local buffer variables array
 * @return Infos about, if the LEDs where rendered successful
 */
ws2811_return_t renderLEDs(){
    for (int x = 0; x < LED_WIDTH; x++) {
        for (int y = 0; y < LED_HEIGHT; y++) {

            //rainbow mode color fade
            if (rainbowMode && buffer[(y * LED_WIDTH) + x] == 0xffffff){

                float rgb[3];
                hsv2rgb(hueToFloat(hue), 1, 1, rgb);

                GifColorType rgbColor;
                rgbColor.Red = valueToInt(rgb[0]);
                rgbColor.Green = valueToInt(rgb[1]);
                rgbColor.Blue = valueToInt(rgb[2]);

                ws2811_led_t color = translateColor(&rgbColor, useGammaCorrection);

                //set buffer to output
                display.channel[0].leds[(y * LED_WIDTH) + x] = color;

            } else {
                display.channel[0].leds[(y * LED_WIDTH) + x] = buffer[(y * LED_WIDTH) + x];
            }
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

void* runRenderThread(void* vargp){
    while (running){
        renderLEDs();
        usleep(RENDER_DELAY * 1000);
    }
}

void startRenderThread() {
    if (activateLEDModule){
        pthread_create(&renderThread, NULL, runRenderThread, NULL);
        if (verbose) {
            printf("[INFO] Started thread for rendering with TID %lu\n", renderThread);
        }
    }
}

/**
 * Clears all the LEDs by setting their color to black and renders it
 */
ws2811_return_t clearLEDs() {
    for (size_t i = 0; i < (size_t) LED_COUNT; i++) {
        buffer[i] = 0;
    }
    return renderLEDs();
}

/**
 * Translate the RGB (255,255,255) color structure into a hexadecimal value
 * @param _color The RGB color, that is to convert
 * @return The convert hexadecimal color
 */
 ws2811_led_t translateColor(GifColorType* _color, bool _useGammaCorrection) {
    uint8_t red = _color->Red;
    uint8_t green = _color->Green;
    uint8_t blue = _color->Blue;

    if (_useGammaCorrection) {
        red = gamma8[red & 0xff];
        green = gamma8[green & 0xff];
        blue = gamma8[blue & 0xff];
    }

    return ((red & 0xff) << 16) + ((green & 0xff) << 8) + (blue & 0xff);
}
