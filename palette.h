#ifndef TASBOT_EYES_PALETTE_H
#define TASBOT_EYES_PALETTE_H

#include <ws2811/ws2811.h>

extern ws2811_led_t* palette;
extern unsigned int paletteCount;

int chtohex(char _ch);
ws2811_led_t strtocol(char* _color);
void readPalette(char* _path);

#endif //TASBOT_EYES_PALETTE_H
