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
#include "LED.h"

extern uint8_t spi_read_buf[20];
extern volatile uint8_t spi_buf_head;
extern volatile uint8_t spi_buf_tail;
#define SAMPLE_RATE	20000
#define ADC_CLK_DIVIDER				(DrvCLK_GetSrcClkAdc()/25/SAMPLE_RATE-1)

// Init ADC clock
static void init_adc_clock() {
	DrvCLK_SetClkSrcAdc(eDRVCLK_ADCSRC_48M);
	DrvCLK_SetClkDividerAdc(ADC_CLK_DIVIDER);
}

// Set interrupt priorities.
void set_interrupt_priorities(void) {
	// Set the SPI slave interrupt priority high.
	NVIC_SetPriority(SPI1_IRQn, 0);

	// Set the ADC interrupt medium-high.
	NVIC_SetPriority(ADC_IRQn, 1);
	
	// Set the GPIO interrupt priority low.
	NVIC_SetPriority(GPAB_IRQn, 2);
}

// Initialize system clock.
static void init_clock(void) {
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
	
	init_adc_clock();

	// Lock protected registers.
	DrvSYS_LockKeyReg();
}

// Initialize system GPIO.
static void init_gpio(void) {
	
	// Configure ADC pin.
	DrvSYS_EnableMultifunctionGpioa(DRVSYS_GPIOA_MF8_ADC_CHANNEL0_IN);
	
	// Configure SPI1 pins.
	DrvSYS_EnableMultifunctionGpiob(
									DRVSYS_GPIOB_MF1_SPI1_1ST_CHIP_SEL_OUT |
									DRVSYS_GPIOB_MF2_SPI1_CLOCK_OUT |
									DRVSYS_GPIOB_MF3_SPI1_DATA_IN |
									DRVSYS_GPIOB_MF4_SPI1_DATA_OUT
	);
	
	// Configure GPIO B pins.
	DrvGPIO_SetIOMode(&GPIOB, 
						DRVGPIO_IOMODE_PIN8_OUT |	// LED
						DRVGPIO_IOMODE_PIN12_OUT	// Sound Threshold Pin
	); 
	
	// Enable the GPIO interrupt
	NVIC_EnableIRQ(GPAB_IRQn);    
	NVIC_SetPriority(GPAB_IRQn, 2);
}


int main (void) {
	int i =0;

	init_clock();
	init_gpio();
	
	set_interrupt_priorities();
	
	LED_blink_for_half_second();
	
	Sound_Detect_init();
	Sound_Detect_start();
	
	SPI_init();
	
	LED_loop();
}
