#ifndef BUBBLES_H_INCLUDED
#define BUBBLES_H_INCLUDED

#include <stdint.h>

//Test Functions
void testFunction();
void isReady();

uint8_t crc4(uint16_t n_prom[]);
void depth_calculate();
void depth_read();
float pressure(float conversion = 1.0); //If a .h file exists, presets are included and removed from .cpp files.
float temperature();
float depth();
float altitude();
void autoCalibration();
bool depth_init();

#endif // BUBBLES_H_INCLUDED
