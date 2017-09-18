#include <stdio.h>
#include <string.h>
//#include "cmsis_os.h"
#include "Driver/DrvADC.h"
#include "Driver/DrvCLK.h"
#include "Driver/DrvSYS.h"
#include "Driver/DrvGPIO.h"
#include "Driver/DrvTimer.h"
#include "Adc.h"
#include "LED.h"

//
// Global Variables
//
extern volatile uint8_t brighten_LED;

//
// Local Variables
//
#define ADC_BUF_LENGTH	100
static int16_t adc_circ_buf[ADC_BUF_LENGTH];
static volatile uint8_t adc_buf_head = 0;
static volatile uint8_t adc_buf_tail = 0;
static volatile int32_t adc_current_sound_level;
static uint8_t sound_threshold;


//
// Local Functions
//
static int16_t abs(int16_t num) {
	if(num < 0) return -num;
	else return num;
}

// wait for a while for ADC value to be stable
static void SkipAdcUnstableInput(UINT16 u16SkipCount)
{
	UINT16 i;
	for(i = 0 ; i < u16SkipCount; )
	{
		if (DrvADC_GetAdcIntFlag())
		{
			DrvADC_ClearAdcIntFlag();
			i++;
		}
	}
}

void turn_on_sound_detection(void) {
}

void turn_off_sound_detection(void) {
}

void adc_set_sound_threshold(uint8_t threshold) {
	sound_threshold = threshold;
}

void adc_toggle_sound_detection(uint8_t command) {
	if(command == 1) {
		start_ADC();
		LED_set_low();
	} else {
		DrvADC_StopConvert();
		LED_turn_off();
	}
}


void ADC_IRQHandler()
{
	int16_t adc_sample;

	// Clear the ADC interrupt flag.
	DrvADC_ClearAdcIntFlag();

	adc_sample = DrvADC_GetConversionDataSigned(0);

	// Adjust moving average.
	adc_current_sound_level -= adc_circ_buf[adc_buf_head];
	adc_circ_buf[adc_buf_head] = abs(adc_sample);
	
	// Increment head.
	adc_buf_head++;
	if(adc_buf_head == ADC_BUF_LENGTH) adc_buf_head = 0;
	
	// Finish updating moving average.
	adc_current_sound_level += abs(adc_sample);
	
	// Did the sound average go above sound_threshold?
	if(adc_current_sound_level/ADC_BUF_LENGTH > sound_threshold) {
		LED_turn_on();
	}
	
//	if(adcSampleMicrophone > 40 || adcSampleMicrophone < -40) {
//		DrvGPIO_ClearOutputBit(&GPIOB, DRVGPIO_PIN_10);
//		DrvGPIO_ClearOutputBit(&GPIOB, DRVGPIO_PIN_11);
//	} else { 
//		DrvGPIO_SetOutputBit(&GPIOB, DRVGPIO_PIN_10);
//		DrvGPIO_SetOutputBit(&GPIOB, DRVGPIO_PIN_11);
//	}
//	if(adcSampleMicrophone < 0) {
//		adcSampleMicrophone = -adcSampleMicrophone;
//		printf("-%x\r\n", adcSampleMicrophone);
//	} else {
//		printf("%d\r\n", adc_current_sound_level/ADC_BUF_LENGTH);
//	}
}

//
// Global Functions
//

//	Set up regulator.
//	Set up preamps.
//	Calibrate.
void init_ADC(void) {
	DrvADC_Open();
	DrvADC_EnableAdc();

	DrvADC_EnableRegulator();
	// Lower regulator resistance to fill caps more quickly.
	DrvADC_SetRegulatorRC(eDRVADC_CTRS_R10K,eDRVADC_FWU_R8K);
	DrvTimer_WaitMillisecondTmr2(100);
	// Now that that's done, increase resistance for normal operation.
	DrvADC_SetRegulatorRC(eDRVADC_CTRS_R600K,eDRVADC_FWU_R400K);

	DrvADC_StopConvert();

	// Set internal gain.
	DrvADC_PreAmpGainControl(DRVADC_PAG1_20DB, 16, DRVADC_PAG2_0DB, TRUE);

	// Calibrate ADC channel.
	DrvADC_SetAdcOperationMode(eDRVADC_CONTINUOUS_SCAN);
	DrvADC_SetConversionDataFormat(eDRVADC_2COMPLIMENT);
//	DrvADC_SetConversionSequence(eDRVADC_CH4CH5,eDRVADC_SCANEND,eDRVADC_SCANEND,eDRVADC_SCANEND,
//									eDRVADC_SCANEND,eDRVADC_SCANEND, eDRVADC_SCANEND,eDRVADC_SCANEND);
	DrvADC_SetConversionSequence(eDRVADC_CH0,eDRVADC_SCANEND,eDRVADC_SCANEND,eDRVADC_SCANEND,
									eDRVADC_SCANEND,eDRVADC_SCANEND, eDRVADC_SCANEND,eDRVADC_SCANEND);
	DrvADC_StartConvert();
	DrvADC_AnalysisAdcCalibration();
	DrvADC_StopConvert();
	
	adc_set_sound_threshold(0x80);
}

// Start to record.
 void start_ADC(void)
{
	 DrvADC_StartConvert();				// start convert
	 SkipAdcUnstableInput(1280);		// skip 128 * 8 samples
	 DrvADC_EnableAdcInt();				//enable ADC interrupt
}


int16_t adc_get_current_sound_level(void)
{ 
	return adc_current_sound_level/ADC_BUF_LENGTH;
}

