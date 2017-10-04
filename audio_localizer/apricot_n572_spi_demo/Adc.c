#include "Adc.h"

//
// Local Variables
//
#define ADC_BUF_LENGTH	100
static int16_t adc_circ_buf[ADC_BUF_LENGTH];
static volatile uint8_t adc_buf_head = 0;
static volatile uint8_t adc_buf_tail = 0;
static volatile int32_t adc_current_sound_level;
static uint16_t sound_threshold = 0x40;

//
// Local Functions
//
static int16_t abs(int16_t num) {
	if(num < 0) return -num;
	else return num;
}

// Wait for a while for ADC value to be stable.
static void ADC_skip_some_samples(uint16_t samples_to_skip) {
	int i;
	for(i=0;i<samples_to_skip;i++) {
		if (DrvADC_GetAdcIntFlag()) {
			DrvADC_ClearAdcIntFlag();
		}
	}
}

//
// Global Functions
//

void Sound_Detect_set_threshold(uint8_t threshold) {
	sound_threshold = threshold * 0x10;
}

void ADC_IRQHandler() {
	int16_t adc_sample;

	// Clear the ADC interrupt flag.
	DrvADC_ClearAdcIntFlag();

	adc_sample = DrvADC_GetConversionDataSigned(0) + 0x1fc;

	// Remove oldest sample from moving average.
	adc_current_sound_level -= adc_circ_buf[adc_buf_head];
	
	// Add new sample to adc_circ_buf.
	adc_circ_buf[adc_buf_head] = abs(adc_sample);
//	adc_circ_buf[adc_buf_head] = adc_sample;
	
	// Increment head.
	adc_buf_head++;
	if(adc_buf_head == ADC_BUF_LENGTH) adc_buf_head = 0;
	
	// Finish updating moving average.
	adc_current_sound_level += abs(adc_sample);
//	adc_current_sound_level += adc_sample;
	
	// Did the sound average go above sound_threshold?
	if(adc_current_sound_level/ADC_BUF_LENGTH > sound_threshold) {
		// Indicate sound above threshold.
		LED_turn_on();
		DrvGPIO_SetOutputBit(&GPIOB, DRVGPIO_PIN_12);
	} else {
		// Indicate sound below threshold.
		LED_set_low();
	}
}


//	Set up ADC regulator and preamps.
// 	Choose ADC channels to sample.
//	Calibrate ADC.
//	Set sound detection threshold.
void Sound_Detect_init(void) {
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
	DrvADC_SetConversionSequence(eDRVADC_CH0,eDRVADC_SCANEND,eDRVADC_SCANEND,eDRVADC_SCANEND,
									eDRVADC_SCANEND,eDRVADC_SCANEND, eDRVADC_SCANEND,eDRVADC_SCANEND);
	DrvADC_StartConvert();
	DrvADC_AnalysisAdcCalibration();
	DrvADC_StopConvert();
}

// Start ADC and enable interrupt.
 void Sound_Detect_start(void) {
	DrvADC_StartConvert();
	ADC_skip_some_samples(1280);
	DrvADC_EnableAdcInt();
	LED_set_low();
}

void Sound_Detect_stop(void) {
	DrvADC_StopConvert();
	LED_turn_off();
}

int16_t Sound_Detect_get_current_sound_level(void) {
	return adc_current_sound_level/ADC_BUF_LENGTH;
}

void Sound_Detect_reset_moving_average(void) {
	int i;
	DrvADC_StopConvert();
	for(i=0;i<ADC_BUF_LENGTH;i++) {
		adc_circ_buf[i] = 0;
	}
	adc_current_sound_level = 0;
	Sound_Detect_start();
}
