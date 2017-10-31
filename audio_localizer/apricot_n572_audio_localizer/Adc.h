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

#define MIC_BUF_LENGTH	750
#define i16 int16_t
#define u16 uint16_t
#define u8 uint8_t

void AT_init(void);
void AT_start(void);
void AT_stop(void);
i16 AT_get_current_audio_direction(void);

#define PHASE_ESTIMATION_RESOLUTION 7

#endif // __ADC_H
