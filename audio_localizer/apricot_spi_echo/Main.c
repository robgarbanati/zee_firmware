#include <stdio.h>
#include <string.h>
#include "Platform.h"
#include "Driver/DrvGPIO.h"
#include "Driver/DrvSYS.h"
#include "Driver/DrvCLK.h"
#include "Driver/DrvSPI.h"
#include "Driver/DrvTimer.h"
#include "SysClkConfig.h"
#include "Spi.h"
#include "Adc.h"

//void GPAB_IRQHandler(void)
//{
//	// Is this SPI CS?
//	if (DrvGPIO_GetIntFlag(&SPI_GPIO_PORT, SPI_CS_PIN))
//	{
//		// Handle send/receive of packets from the robot body.
//		spiHeadHandler();
//		
//		// Clear the SPI CS interrupt.
//		DrvGPIO_ClearIntFlag(&SPI_GPIO_PORT, SPI_CS_PIN);
//	}
//}

// Init ADC clock
void adcClockInit() {
	UINT32	u32HCLK = 0,u32ADCClk = 0;
	INT32 adc_clk_freq;
	
	DrvCLK_SetClkSrcAdc(eDRVCLK_ADCSRC_48M);
	
	DrvCLK_SetClkDividerAdc(430);//ADC_CLOCK_FREQUENCY);  // == sampling_frequency*25
	
	u32HCLK = DrvCLK_GetHclk();
	u32ADCClk = DrvCLK_GetClkAdc();

	
//	//The ADC engine clock must meet the constraint: ADCLK <=  HCKL/2.
//	if (u32ADCClk>(u32HCLK/2)) {
//		puts("ADCClk is greater than half the frequency of the HCLK. That's bad.\n");
//		puts("Check adcClockInit() in Main.c\n");
//	}
}

// Initialize system clock.
void clkInit(void) {
	// Unlock protected registers.
	DrvSYS_UnlockKeyReg();

	// Configure the system clock source.  See SysClkConfig.h.
	_SYSCLKCONFIG();

	// Enable LDO33.
	DrvCLK_EnableLDO30_P();

	// Disable low voltage reset
	DrvSYS_DisableLowVoltageReset_P();
	
	// Enable power on reset (assert reset when power first comes on).
	DrvSYS_ClearPORDisableCode_P();
	
	adcClockInit();

	// Lock protected registers.
	DrvSYS_LockKeyReg();
}

// Initialize system GPIO.
void gpioInit(void) {
	
	// Configure GPIO A special functions.
	DrvSYS_EnableMultifunctionGpioa(
									DRVSYS_GPIOA_MF1_SPI0_1ST_CHIP_SEL_OUT |	// Master SPI select output serial flash
									DRVSYS_GPIOA_MF2_SPI0_CLOCK_OUT |					// Master SPI clock output
									DRVSYS_GPIOA_MF3_SPI0_DATA_IN |						// Master SPI data input
									DRVSYS_GPIOA_MF4_SPI0_DATA_OUT |					// Master SPI data output
									
									DRVSYS_GPIOA_MF8_ADC_CHANNEL0_IN|  	// ADC input
									DRVSYS_GPIOA_MF9_ADC_CHANNEL1_IN|  	// ADC input
									DRVSYS_GPIOA_MF10_ADC_CHANNEL2_IN|  // ADC input
									DRVSYS_GPIOA_MF11_ADC_CHANNEL3_IN|  // ADC input
									DRVSYS_GPIOA_MF12_ADC_CHANNEL4_IN|  // ADC input
									DRVSYS_GPIOA_MF13_ADC_CHANNEL5_IN  	// ADC input
	);
	
	// Configure GPIO B special functions.
	DrvSYS_EnableMultifunctionGpiob(
		DRVSYS_GPIOB_MF1_SPI1_1ST_CHIP_SEL_OUT |	// Slave SPI select input
		DRVSYS_GPIOB_MF2_SPI1_CLOCK_OUT |					// Slave SPI clock input
		DRVSYS_GPIOB_MF3_SPI1_DATA_IN |						// Slave SPI data output
		DRVSYS_GPIOB_MF4_SPI1_DATA_OUT //|					// Slave SPI data input
	);
	
	// Configure GPIO port B pins.
	DrvGPIO_SetIOMode(&GPIOB,
		DRVGPIO_IOMODE_PIN8_OUT |
		DRVGPIO_IOMODE_PIN9_OUT |
		DRVGPIO_IOMODE_PIN10_OUT |
		DRVGPIO_IOMODE_PIN11_OUT
	); 
	
	DrvGPIO_ClearOutputBit(&GPIOB, DRVGPIO_PIN_8);
}


// Main thread.
int main (void) {
	int i =0;
	// Initialize clocks.
	clkInit();

	// Initialize GPIO.
	gpioInit();
	
	init_ADC();
	start_ADC();
//	puts("got past start_adc\r\n");
	
	spiSlave_Init();
	
	// Blink LEDs.
	for (;;) {
		DrvGPIO_ClearOutputBit(&GPIOB, DRVGPIO_PIN_8);
		DrvTimer_WaitMillisecondTmr2(100);
		DrvGPIO_SetOutputBit(&GPIOB, DRVGPIO_PIN_8);
		DrvTimer_WaitMillisecondTmr2(100);
	}	
}

