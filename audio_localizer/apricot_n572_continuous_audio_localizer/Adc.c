#include "Adc.h"

//
// Local Variables
//

static i16 micA_buf[MIC_BUF_LENGTH];
static i16 micB_buf[MIC_BUF_LENGTH];
static i16 micC_buf[MIC_BUF_LENGTH];

static unsigned short collect_samples;

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



#define SAMPLE_TRIPS_DETECTION_THRESHOLD ((temp1 > DETECTION_THRESHOLD) || (temp1 < -DETECTION_THRESHOLD) || \
											(temp2 > DETECTION_THRESHOLD) || (temp2 < -DETECTION_THRESHOLD) || \
											(temp3 > DETECTION_THRESHOLD) || (temp3 < -DETECTION_THRESHOLD) \
										 )
// TODO this is imperfect because it could miss a valid sound if there's a not-quite-energetic enough prologue. Not sure I care though.

//~~~~~~~~~~~~~~~~~~~~~~SLIGHTLY SIMPLIFIED STATE MACHINE FOR ADC_IRQHandler:~~~~~~~~~~~~~~~~//
// Have we started recording?
	// Has it been too long since the sound has been loud enough to record? And/or have we sampled enough?	
		// Have we tripped the threshold enough times to be a valid sound (and perform direction detection on this sound?)
			// Trigger direction detection.
		// Stop recording.
		// Reset variables.	

	// Else we should keep recording.
		// Record samples.
		//Is this sample also loud enough to make note of?
			// Record another trip of the threshold.
			// Update most recent index the threshold was exceeded.
			
// Else, we have not started recording.
	//Is current sample loud enough to start recording?
		// Start recording.
void ADC_IRQHandler() {
	static u16 index = 0, detection_counter = 0;
	static i16 possible_head = 0, most_recent_threshold_trip = 0;
	u16 head, tail;
	i16 temp1, temp2, temp3;
	DrvGPIO_SetOutputBit(&GPIOB, DRVGPIO_PIN_3);
	
	// Is this an erroneously called ISR?
	if(!collect_samples || !DrvADC_GetAdcIntFlag()) {
		// Clear the ADC interrupt flag.
		DrvADC_ClearAdcIntFlag();
		DrvGPIO_ClearOutputBit(&GPIOB, DRVGPIO_PIN_3);	
		return;
	}
		
	// Have we started recording?
	if(index != 0) {
		// Has it been too long since the sound has been loud enough to record? And/or have we sampled enough?
		if( (index - most_recent_threshold_trip > 40) || (index > MIC_BUF_LENGTH) ) {	
			// Have we tripped the threshold enough times to be a valid sound (and perform direction detection on this sound?)
			if(detection_counter > TRIPS_TO_TRIGGER_DIRECTION_DETECTION) {
				// Trigger direction detection.
				collect_samples = 0; // TODO rename?
				AT_stop(); // TODO maybe take out of interrupt.
			}
			
			// Stop recording.
			index = 0;
			// Reset variables.
			detection_counter = 0;
			most_recent_threshold_trip = 0;
		}	
		// Else we should keep recording.
		else {
			// Record all microphone samples to arrays.
			temp1 = DrvADC_GetConversionDataSigned(0) + 0x1fc;
			temp2 = DrvADC_GetConversionDataSigned(1) + 0x185;
			temp3 = DrvADC_GetConversionDataSigned(2) + 0x185;
			micA_buf[index] = 	temp1;
			micB_buf[index] = 	temp2;
			micC_buf[index] = 	temp3;
			
			//Is this sample also loud enough to make note of?
			if(SAMPLE_TRIPS_DETECTION_THRESHOLD) {
				// Record another trip of the threshold.
				detection_counter++;
				// Update most recent index the threshold was exceeded.
				most_recent_threshold_trip = index;
			}
			
			index++;
		}
	}	
	// Else, we have not started recording.
	else {
		// Collect micA sample.
		temp1 = DrvADC_GetConversionDataSigned(0) + 0x1fc;
		temp2 = DrvADC_GetConversionDataSigned(1) + 0x185;
		temp3 = DrvADC_GetConversionDataSigned(2) + 0x185;
		
		// Is this sample loud enough to start recording?
		if(SAMPLE_TRIPS_DETECTION_THRESHOLD) {
			// Record current sample to arrays. TODO check if biases are consistent amongst boards and microphones. Should be biased at 2V which is a bit off from a natural 0 value of 1.65V)
			micA_buf[index] = 	temp1;
			micB_buf[index] = 	temp2;
			micC_buf[index] = 	temp3;
			
			// Increment index to start recording next samples.
			index++;
		}
	}
			
	// Clear the ADC interrupt flag.
	DrvADC_ClearAdcIntFlag();
	DrvGPIO_ClearOutputBit(&GPIOB, DRVGPIO_PIN_3);	
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
}

float AT_get_current_audio_direction(void) {
	return current_audio_direction;
}

#define NUM 7
static void remember_top_7_phases(i16 phase, i32 cost, i8 *t7phases, u32 *costs) {
	u8 i, most_i;
	u32 most_cost = 0;
	// for size of phases and costs
	for(i=0;i<NUM;i++) {
		// Is this cost more than most_cost?
		if(costs[i] > most_cost) {
			// Update most_cost and most_i
			most_cost = costs[i];
			most_i = i;
		}
	}
	// Now that we found most_cost and most_i, is our new cost less than it?
	if(cost < most_cost) {
		// Update that index in costs and phases.
		costs[most_i] = cost;
		t7phases[most_i] = phase;
	}
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
	i8 top_7_phases_array[NUM];
	u32 their_costs_array[NUM] = {UINT32_MAX, UINT32_MAX, UINT32_MAX, UINT32_MAX, UINT32_MAX, UINT32_MAX, UINT32_MAX};
	
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
		remember_top_7_phases(phase, cost, top_7_phases_array, their_costs_array);
	}
	
	// print top 7 phases and costs.
	for(i=0; i<NUM; i++) {
		printf("%6d ", top_7_phases_array[i]);
	}
	printf("\nCosts: ");
	for(i=0; i<NUM; i++) {
		printf("%6d ", their_costs_array[i]);
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
	current_audio_direction = audio_direction;
	
	// only change audio_direction if we are very confident in that direction
//	if ( (abs(audio_direction - prev_audio_direction) <= 3) || (abs(audio_direction - prev_audio_direction) >= 10) ) {
//		consistency_counter++;
//	} else {
//		consistency_counter = 0;
//	}
//	
//	if (consistency_counter >=2) {
//		if((micA_maximum_volume - micA_minimum_volume) > 110) {
//			current_audio_direction = (float) (((float) (prev_audio_direction + audio_direction))/2);
//			if (abs(audio_direction - prev_audio_direction) >= 11) {
//				current_audio_direction += 6;
//			}
//		} else {
//			current_audio_direction = 13;
//		}
//	} else {
//		current_audio_direction = 13;
//	}
}

void audio_trilaterate(void) {	
	static int k=0;
	i16 phaseAB=0, phaseAC=0, phaseBC=0;

	//Have we collected an array's worth of data?
	if (!collect_samples) {	
		printf("AB:     ");
		phaseAB = find_phase(micA_buf, micB_buf);
		printf("\nAC:    ");
		phaseAC = find_phase(micA_buf, micC_buf);
		printf("\nBC:    ");
		phaseBC = find_phase(micB_buf, micC_buf);
		
		determine_audio_direction(phaseAB, phaseAC, phaseBC);
		puts("");
		
#if 1
		// print direction.
		if(AT_get_current_audio_direction() != 13) {
//			printf("%1.1f\n", AT_get_current_audio_direction());
//			printf("%1.1f %d %d %d\n", AT_get_current_audio_direction(), phaseAB, phaseAC, phaseBC);
			printf("\n% 03d % 03d % 03d\n", phaseAB, phaseAC, phaseBC);
		}
//		for(;;);
#endif
		
#if 1
		// print arrays.
		if(abs(phaseBC) > 28)
		for(k = 0; k<MIC_BUF_LENGTH; k++) {
			printf("%d, %d, %d, %d\n", k, micA_buf[k], micB_buf[k], micC_buf[k]);
		}
//		for(;;);
#endif
		
		AT_start();
	}
}
