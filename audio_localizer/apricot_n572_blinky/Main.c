#include <stdio.h>
#include <string.h>
#include "Platform.h"
#include "Driver/DrvGPIO.h"
#include "Driver/DrvSYS.h"
#include "Driver/DrvCLK.h"
#include "Driver/DrvSPI.h"
#include "Driver/DrvTimer.h"
#include "SysClkConfig.h"

// Initialize system clock.
void init_clock(void)
{
	DrvSYS_UnlockKeyReg();
	_SYSCLKCONFIG();
	DrvCLK_EnableLDO30_P();
	DrvSYS_DisableLowVoltageReset_P();
	DrvSYS_ClearPORDisableCode_P();
	DrvSYS_LockKeyReg();
}

// Initialize system GPIO.
void init_gpio(void)
{
	DrvGPIO_SetIOMode(&GPIOB,
		DRVGPIO_IOMODE_PIN8_OUT
	); 
}


// Main thread.
int main (void)
{
	int i =0;
	// Initialize clocks.
	init_clock();

	// Initialize GPIO.
	init_gpio();
	
	// Blink LEDs.
	for (;;) {
		DrvGPIO_ClearOutputBit(&GPIOB, DRVGPIO_PIN_8);
		DrvTimer_WaitMillisecondTmr2(100);
		DrvGPIO_SetOutputBit(&GPIOB, DRVGPIO_PIN_8);
		DrvTimer_WaitMillisecondTmr2(100);
	}	
}

