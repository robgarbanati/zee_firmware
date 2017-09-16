#ifndef __ADC_H
#define __ADC_H

#include "Platform.h"

//
// Global Defines and Declarations
//

#define ADC_CLK_SOURCE				eDRVCLK_ADCSRC_48M
#define ADC_PAG_GAIN1					DRVADC_PAG1_30DB
#define ADC_PAG_GAIN2					DRVADC_PAG2_20DB

// We want to sample one channel every 10 milliseonds.  Therefore the
// sample rate is 100 samples per second.
#define ADC_SAMPLE_RATE				100
#define ADC_CLK_DIVIDER				(DrvCLK_GetSrcClkAdc() / 25 / (ADC_SAMPLE_RATE - 1))

//
// Global Functions
//

// Init functions.
void init_ADC(void);
void start_ADC(void);
int16_t adc_get_current_sound_level(void);

#endif // __ADC_H


