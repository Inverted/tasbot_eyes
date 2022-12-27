#ifndef TASBOT_EYES_LED_H
#define TASBOT_EYES_LED_H

#include <ws2811/ws2811.h>
#include <gif_lib.h>
#include <pthread.h>

#define GPIO_PIN                10
#define TARGET_FREQ             WS2811_TARGET_FREQ      //800000 Kbps
#define DMA                     10
#define STRIP_TYPE              SK6812_STRIP            //TASBot
#define INVERTED                false
#define BRIGHTNESS              4

#define LED_HEIGHT              8
#define LED_WIDTH               28
#define LED_COUNT               (LED_WIDTH * LED_HEIGHT)

extern int brightness;
extern int dataPin;

extern ws2811_led_t* buffer;
extern ws2811_t display;

void initLEDs();
ws2811_return_t renderLEDs();
ws2811_return_t clearLEDs();
ws2811_led_t translateColor(GifColorType* _color, bool _useGammaCorrection);

//todo: discard
void setSpecificPixel(unsigned int _index, ws2811_led_t _color);

//todo
void startRenderThread();
void* runRenderThread(void* vargp);

#endif //TASBOT_EYES_LED_H