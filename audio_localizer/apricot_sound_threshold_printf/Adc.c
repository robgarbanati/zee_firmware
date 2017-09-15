#include <stdio.h>
#include <string.h>
//#include "cmsis_os.h"
#include "Driver/DrvADC.h"
#include "Driver/DrvCLK.h"
#include "Driver/DrvSYS.h"
#include "Driver/DrvGPIO.h"
#include "Driver/DrvTimer.h"
#include "Adc.h"

// Contains the most recent microphone sample.
static INT16 adcSampleMicrophone;

//
// Local Functions
//

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


void ADC_IRQHandler()
{
	// Clear the ADC interrupt flag.
	DrvADC_ClearAdcIntFlag();

	// Update each of the samples.
	adcSampleMicrophone = DrvADC_GetConversionDataSigned(0);
//	if(adcSampleMicrophone > 40 || adcSampleMicrophone < -40) {
//		DrvGPIO_ClearOutputBit(&GPIOB, DRVGPIO_PIN_10);
//		DrvGPIO_ClearOutputBit(&GPIOB, DRVGPIO_PIN_11);
//	} else { 
//		DrvGPIO_SetOutputBit(&GPIOB, DRVGPIO_PIN_10);
//		DrvGPIO_SetOutputBit(&GPIOB, DRVGPIO_PIN_11);
//	}
	if(adcSampleMicrophone < 0) {
		adcSampleMicrophone = -adcSampleMicrophone;
		printf("-%x\r\n", adcSampleMicrophone);
	} else {
		printf("%x\r\n", adcSampleMicrophone);
	}
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
}

// Start to record.
 void start_ADC(void)
{
	 DrvADC_StartConvert();				// start convert
	 SkipAdcUnstableInput(1280);			// skip 128 * 8 samples
	 DrvADC_EnableAdcInt();				//enable ADC interrupt
}


INT16 adcGetMicrophone(void)
{
	INT16 result;

	// Disable interrupts.
	__disable_irq(); 

	// Get the sample.
	result = adcSampleMicrophone;

	// Enable interrupts.
	__enable_irq(); 

	return result;
}

