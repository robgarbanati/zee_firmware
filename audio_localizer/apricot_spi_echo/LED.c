#include "LED.h"

typedef enum LED_state_enum_t {
	OFF,
	LOW,
	ON
} LED_state_t;

LED_state_t LED_state = LOW;

void LED_turn_off(void) {
	LED_state = OFF;
}

void LED_set_low(void) {
	LED_state = LOW;
}

void LED_turn_on(void) {
	LED_state = ON;
}

void LED_blink(void) {
	// Blink LEDs.
	for (;;) {
		if(LED_state == ON) {
			DrvGPIO_ClearOutputBit(&GPIOB, DRVGPIO_PIN_8);
		} else if(LED_state == LOW) {
			DrvGPIO_ClearOutputBit(&GPIOB, DRVGPIO_PIN_8);
			DrvTimer_WaitMillisecondTmr2(1);
			DrvGPIO_SetOutputBit(&GPIOB, DRVGPIO_PIN_8);
			DrvTimer_WaitMillisecondTmr2(20);
		} else {
			DrvGPIO_SetOutputBit(&GPIOB, DRVGPIO_PIN_8);
		}
	}
}

void LED_blink_for_half_second(void) {
	int i;
	// Blink LEDs.
	for (i=0;i<5;i++) {
			DrvGPIO_ClearOutputBit(&GPIOB, DRVGPIO_PIN_8);
			DrvTimer_WaitMillisecondTmr2(50);
			DrvGPIO_SetOutputBit(&GPIOB, DRVGPIO_PIN_8);
			DrvTimer_WaitMillisecondTmr2(50);
	}
}