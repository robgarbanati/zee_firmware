#include "Adc.h"

//
// Local Variables
//

static i16 micA_buf[MIC_BUF_LENGTH];
static i16 micB_buf[MIC_BUF_LENGTH];
static i16 micC_buf[MIC_BUF_LENGTH];

static u16 x = 0;
static unsigned short collect_samples;

i16 micA_maximum_volume, micB_maximum_volume, micC_maximum_volume;
i16 micA_minimum_volume, micB_minimum_volume, micC_minimum_volume;

static float current_audio_direction;

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
	static u8 counter = 0;
	
	if (collect_samples) {
		if (DrvADC_GetAdcIntFlag()) {
			
			// Add most recent ADC samples to microphone buffers. TODO check if biases are consistent amongst boards and microphones.
			micA_buf[index] = 	DrvADC_GetConversionDataSigned(0) + 0x1fc; // for some reason this is how the microphones are biased (should be biased at 2V which is a bit off from a natural 0 value of 1.65V)
			micB_buf[index] = 	DrvADC_GetConversionDataSigned(1) + 0x185; // for some reason this is how the microphones are biased (should be biased at 2V which is a bit off from a natural 0 value of 1.65V)
			micC_buf[index] = 	DrvADC_GetConversionDataSigned(2) + 0x185; // for some reason this is how the microphones are biased (should be biased at 2V which is a bit off from a natural 0 value of 1.65V)
			
			// Clear the ADC interrupt flag.
			DrvADC_ClearAdcIntFlag();
			
			// Record max and min for each microphone buffer.
			if(micA_buf[index] > micA_maximum_volume) {
				micA_maximum_volume = micA_buf[index];
			}
			if(micB_buf[index] > micB_maximum_volume) {
				micB_maximum_volume = micB_buf[index];
			}
			if(micC_buf[index] > micC_maximum_volume) {
				micC_maximum_volume = micC_buf[index];
			}
			
			if(micA_buf[index] < micA_minimum_volume) {
				micA_minimum_volume = micA_buf[index];
			}
			if(micB_buf[index] < micB_minimum_volume) {
				micB_minimum_volume = micB_buf[index];
			}
			if(micC_buf[index] < micC_minimum_volume) {
				micC_minimum_volume = micC_buf[index];
			}
			
			index++;
					
			if (index >= (MIC_BUF_LENGTH)) {
				collect_samples = 0;
				index = 0;
				AT_stop();
			}
		}
	}
	// Clear the ADC interrupt flag.
	DrvADC_ClearAdcIntFlag();
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
	DrvADC_SetConversionSequence(eDRVADC_CH0,eDRVADC_CH1,eDRVADC_CH2,eDRVADC_SCANEND,
								 eDRVADC_SCANEND,eDRVADC_SCANEND,eDRVADC_SCANEND,eDRVADC_SCANEND);
	DrvADC_StartConvert();
	DrvADC_AnalysisAdcCalibration();
	ADC_skip_some_samples(1280);
	DrvADC_StopConvert();
}

// Start ADC and enable interrupt.
 void AT_start(void) {
	DrvADC_StartConvert();
	DrvADC_EnableAdcInt();
	collect_samples = 1;
}

void AT_stop(void) {
	DrvADC_StopConvert();
	LED_turn_off();
}

float AT_get_current_audio_direction(void) {
	return current_audio_direction;
}

// Find the phase difference in terms of number of sample periods
// (determined by SAMPLE_RATE) between audio_array1 and audio_array2.
// If sound hit mic2 first, this function returns a positive integer.
// If sound hit mic1 first, this function returns a negative integer.
static i16 find_phase(i16* audio_array1, i16* audio_array2) {
	i16 phase;
	u16 i;
	i32 error = 0, cost = 0;
	i32 best_phase = 0, lowest_cost = INT32_MAX;
	
	// Calculate cost function for all possible phase shifts.
	for(phase = -PHASE_ESTIMATION_RESOLUTION; phase<PHASE_ESTIMATION_RESOLUTION; phase++) {
		cost = 0;
		// Calculate cost function (sum of all errors^2).
		for(i = PHASE_ESTIMATION_RESOLUTION; i<MIC_BUF_LENGTH - PHASE_ESTIMATION_RESOLUTION; i++) {
			error = (audio_array1[i + phase] - audio_array2[i]);
			cost = cost + error*error;
		}
		// Remember phase associated with lowest cost (that's the phase shift we think the sound has between the two mics).
		if (cost < lowest_cost) {
			lowest_cost = cost;
			best_phase = phase;
		}
	}

	return best_phase; 
}

#define WIDTH 1
void determine_audio_direction(i16 phaseAB, i16 phaseAC, i16 phaseBC) {
	static u8 audio_direction, prev_audio_direction = 0;
	static u16 consistency_counter = 0;
	
	prev_audio_direction = audio_direction;

	if (abs(phaseAB) <= WIDTH) {
		if ((phaseAC > 0) && (phaseBC > 0))
			audio_direction = 9;
		else
			audio_direction = 3;
	}
	else if (abs(phaseAC) <= WIDTH) {
		if ((phaseAB > 0) && (phaseBC < 0))
			audio_direction = 5;
		else
			audio_direction = 11;
	}
	else if (abs(phaseBC) <= WIDTH) {
		if ((phaseAC < 0) && (phaseAB < 0))
			audio_direction = 1;
		else
			audio_direction = 7;
	}
	else if ((phaseAB < 0) && (phaseAC < 0)) {  // sound hits A first
		if (phaseBC > 0)
			audio_direction = 0;
		else
			audio_direction = 2;
	}
	else if ((phaseAC > 0) && (phaseBC > 0)) {  // sound hits C first
		if (phaseAB > 0)
			audio_direction = 8;
		else
			audio_direction = 10;
	}
	else if ((phaseAB > 0) && (phaseBC < 0)) {  // sound hits B first
		if (phaseAC > 0)
			audio_direction = 6;
		else
			audio_direction = 4;
	}
	else {
		current_audio_direction = 13;
		consistency_counter = 0;
		return;
	}
	
	// only change audio_direction if we are very confident in that direction
	if ( (abs(audio_direction - prev_audio_direction) <= 3) || (abs(audio_direction - prev_audio_direction) >= 10) ) {
		consistency_counter++;
	} else {
		consistency_counter = 0;
	}
	
	if (consistency_counter >=2) {
		if((micA_maximum_volume - micA_minimum_volume) > 110) {
			current_audio_direction = (float) (((float) (prev_audio_direction + audio_direction))/2);
			if (abs(audio_direction - prev_audio_direction) >= 11) {
				current_audio_direction += 6;
			}
		} else {
			current_audio_direction = 13;
		}
	} else {
		current_audio_direction = 13;
	}
}

void audio_trilaterate(void) {	
	static int k=0;
	i16 phaseAB=0, phaseAC=0, phaseBC=0;

	//Have we collected an array's worth of data?
	if (!collect_samples) {	
		phaseAB = find_phase(micA_buf, micB_buf);
		phaseAC = find_phase(micA_buf, micC_buf);
		phaseBC = find_phase(micB_buf, micC_buf);
		
		determine_audio_direction(phaseAB, phaseAC, phaseBC);

		// print maybe
#if 0
		for(k = PHASE_ESTIMATION_RESOLUTION; k<MIC_BUF_LENGTH - PHASE_ESTIMATION_RESOLUTION; k++) {
			printf("%d, %d, %d, %d\n", k, micA_buf[k], micB_buf[k], micC_buf[k]);
		}
//		for(;;);
#endif
#if 1
		if(AT_get_current_audio_direction() != 13) {
			printf("%1.1f\n", AT_get_current_audio_direction());
		}
//		for(;;);
#endif
		AT_start();
		
		micA_maximum_volume = INT16_MIN;
		micB_maximum_volume = INT16_MIN;
		micC_maximum_volume = INT16_MIN;
		
		micA_minimum_volume = INT16_MAX;
		micB_minimum_volume = INT16_MAX;
		micC_minimum_volume = INT16_MAX;
	}
}

