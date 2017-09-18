#ifndef __ADC_H
#define __ADC_H

#include <stdio.h>
#include <string.h>
#include "Driver/DrvADC.h"
#include "Driver/DrvCLK.h"
#include "Driver/DrvSYS.h"
#include "Driver/DrvGPIO.h"
#include "Driver/DrvTimer.h"
#include "Platform.h"
#include "LED.h"

void Sound_Detect_set_threshold(uint8_t threshold);
void Sound_Detect_init(void);
void Sound_Detect_start(void);
void Sound_Detect_stop(void);
int16_t Sound_Detect_get_current_sound_level(void);
void Sound_Detect_reset_moving_average(void);

#endif // __ADC_H
