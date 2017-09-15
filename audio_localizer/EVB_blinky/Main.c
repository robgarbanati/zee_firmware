#include <stdio.h>
#include <string.h>
#include "Platform.h"
#include "Driver/DrvGPIO.h"
#include "Driver/DrvSYS.h"
#include "Driver/DrvCLK.h"
#include "Driver/DrvSPI.h"
#include "SysClkConfig.h"

// Initialize system clock.
void clkInit(void)
{
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

	// Lock protected registers.
	DrvSYS_LockKeyReg();
}

// Initialize system GPIO.
void gpioInit(void)
{
	// Configure GPIO port B pins.
	DrvGPIO_SetIOMode(&GPIOB,
		DRVGPIO_IOMODE_PIN8_OUT |
		DRVGPIO_IOMODE_PIN9_OUT
	); 
}


// Main thread.
int main (void)
{
	int i =0;
	// Initialize clocks.
	clkInit();

	// Initialize GPIO.
	gpioInit();
	
	// Blink LEDs.
	for (;;) {
		DrvGPIO_ClearOutputBit(&GPIOB, DRVGPIO_PIN_8);
		DrvGPIO_SetOutputBit(&GPIOB, DRVGPIO_PIN_9);
		for(i=0;i<600000;i++);
		DrvGPIO_SetOutputBit(&GPIOB, DRVGPIO_PIN_8);
		DrvGPIO_ClearOutputBit(&GPIOB, DRVGPIO_PIN_9);
		for(i=0;i<600000;i++);
	}	
}

