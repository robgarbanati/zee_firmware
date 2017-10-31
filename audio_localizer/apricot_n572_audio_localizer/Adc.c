#include "Adc.h"

//
// Local Variables
//
#define MIC_BUF_LENGTH	750
#define i16 int16_t
#define u16 uint16_t

static i16 micA_buf[MIC_BUF_LENGTH];
static i16 micB_buf[MIC_BUF_LENGTH];
static i16 micC_buf[MIC_BUF_LENGTH];

static u16 x = 0;
static unsigned short collect_samples;

i16 Vol_Max, vol_max_A, vol_max_B, vol_max_C;
i16 Vol_Min, vol_min_A, vol_min_B, vol_min_C;

static i16 current_audio_direction;

//
// Local Functions
//
static i16 abs(i16 num) {
	if(num < 0) return -num;
	else return num;
}

// Wait for a while for ADC value to be stable.
static void ADC_skip_some_samples(u16 samples_to_skip) {
	i16 i;
	for(i=0;i<samples_to_skip;i++) {
		if (DrvADC_GetAdcIntFlag()) {
			DrvADC_ClearAdcIntFlag();
		}
	}
}

//
// Global Functions
//
void ADC_IRQHandler() {
	static u16 index = 0;
	static uint8_t counter = 0;

	// Clear the ADC interrupt flag.
	DrvADC_ClearAdcIntFlag();
	
	// Add most recent ADC samples to microphone buffers.
	micA_buf[index] = 	DrvADC_GetConversionDataSigned(0) + 0x1fc;
	micB_buf[index] = 	DrvADC_GetConversionDataSigned(1) + 0x1fc;
	micC_buf[index] = 	DrvADC_GetConversionDataSigned(2) + 0x1fc;
	
	// Record the max and min values for each buffer.
	if(micA_buf[x] > vol_max_A) {
		vol_max_A = micA_buf[x];
	}
	if(micB_buf[x] > vol_max_B) {
		vol_max_B = micB_buf[x];
	}
	if(micC_buf[x] > vol_max_C) {
		vol_max_C = micC_buf[x];
	}
	
	if(micA_buf[x] < vol_min_A) {
		vol_min_A = micA_buf[x];
	}
	if(micB_buf[x] < vol_min_B) {
		vol_min_B = micB_buf[x];
	}
	if(micC_buf[x] < vol_min_C) {
		vol_min_C = micC_buf[x];
	}
	
	index++;
			
	if (index >= (MIC_BUF_LENGTH)) {
		collect_samples = 0;
		index = 0;
	}
}


//	Set up ADC regulator and preamps.
// 	Choose ADC channels to sample.
//	Calibrate ADC.
void AT_init(void) {
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
 void AT_start(void) {
	DrvADC_StartConvert();
	ADC_skip_some_samples(1280);
	DrvADC_EnableAdcInt();
}

void AT_stop(void) {
	DrvADC_StopConvert();
	LED_turn_off();
}

i16 AT_get_current_audio_direction(void) {
	return current_audio_direction;
}
