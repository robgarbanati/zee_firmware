#include "Adc.h"

//
// Local Variables
//

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

// Find the number of indices buf1 is shifted from buf2. Return as best_phase.
// best_phase * SAMPLE_FREQUENCY = time delay in audio from mic2 to mic1.
// (positive best_phase means sound hits mic2 first).
static i16 find_phase(i16* buf1, i16* buf2, int vol_min_1, int vol_min_2)
{
	int phase,i;
	int sub = 0, cost = 0;
	int best_phase = 0, bestcost = 0x7FFFFFFF;
	int height_difference = vol_min_1 - vol_min_2;
	int64_t sumcost = 0;
	
	// Calculate squared sum of errors for all phase shifts from -PHASE_ESTIMATION_RESOLUTION to +PHASE_ESTIMATION_RESOLUTION.
	for(phase = -PHASE_ESTIMATION_RESOLUTION; phase<PHASE_ESTIMATION_RESOLUTION; phase++) {
		cost = 0;
		// Calculate squared sum of errors.
		for(i = PHASE_ESTIMATION_RESOLUTION; i<MIC_BUF_LENGTH - PHASE_ESTIMATION_RESOLUTION; i++) {
			sub = (buf1[i + phase] - buf2[i] - height_difference);
			cost = cost + sub*sub;
		}
//		printf("%d ", cost);
		if (bestcost > cost) {
			bestcost = cost;
			best_phase = phase;
			sumcost += cost;
		}
	}
//	printf("%d %lld %d\n", bestcost, sumcost/P_E_RES/2, best_phase);

	return best_phase; 
}

void determine_direction(i16 phaseAB, i16 phaseAC, i16 phaseBC) {
	static u8 direction, prev_direction = 0;
	static u8 consistency_counter = 0;
	
	prev_direction = direction;

	if (phaseAB == 0) {
		if ((phaseAC > 0) && (phaseBC > 0))
			direction = 9;
		else
			direction = 3;
	}
	else if (phaseAC == 0) {
		if ((phaseAB > 0) && (phaseBC < 0))
			direction = 5;
		else
			direction = 11;
	}
	else if (phaseBC == 0) {
		if ((phaseAC < 0) && (phaseAB < 0))
			direction = 1;
		else
			direction = 7;
	}
	else if ((phaseAB < 0) && (phaseAC < 0)) {  // sound hits A first
		if (phaseBC > 0)
			direction = 0;
		else
			direction = 2;
	}
	else if ((phaseAC > 0) && (phaseBC > 0)) {  // sound hits C first
		if (phaseAB > 0)
			direction = 8;
		else
			direction = 10;
	}
	else if ((phaseAB > 0) && (phaseBC < 0)) {  // sound hits B first
		if (phaseAC > 0)
			direction = 6;
		else
			direction = 4;
	}
	else {
		consistency_counter = 0;
		return;
	}
	
	// only change direction if we are very confident in that direction
	if (direction == prev_direction)
		consistency_counter++;
	else
		consistency_counter = 0;
	
	if (consistency_counter >=3) {
		current_audio_direction = direction;
	}
}

void audio_trilaterate(void) {	
	static int k=0;
	i16 phaseAB=0, phaseAC=0, phaseBC=0;

	//Have we collected an array's worth of data?
	if (!collect_samples) {	
		phaseAB = find_phase(micA_buf, micB_buf, vol_min_A, vol_min_B);
		phaseAC = find_phase(micA_buf, micC_buf, vol_min_A, vol_min_C);
		phaseBC = find_phase(micB_buf, micC_buf, vol_min_B, vol_min_C);
		
		determine_direction(phaseAB, phaseAC, phaseBC);

		// print maybe
#if 0
		for(k = P_E_RES; k<ADC_BUFFER_SIZE - P_E_RES; k++)
		{
			printf("%d, %d, %d, %d\n", k, micA_buf[k], micB_buf[k], micC_buf[k]);
		}
		for(;;);
#endif
		
		collect_samples = 1; // let the ADC_ISR take data again
		vol_max_A = INT16_MIN;
		vol_max_B = INT16_MIN;
		vol_max_C = INT16_MIN;
		
		vol_min_A = INT16_MAX;
		vol_min_B = INT16_MAX;
		vol_min_C = INT16_MAX;
	}
}

