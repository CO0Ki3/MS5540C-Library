#ifndef _MS5540C_H_
#define _MS5540C_H_

#include <SPI.h>
#include <inttypes.h>

#define FRESH      1000.0
#define SEA        1030.0

void ms5540c_reset();
void ms5540c_setup();
void ms5540c_loop(float water_type);

extern long pressure;
extern float temperature;
extern float waterdepth;

#endif
