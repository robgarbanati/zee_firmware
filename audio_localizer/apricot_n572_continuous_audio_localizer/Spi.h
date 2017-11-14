#ifndef __SPI_H
#define __SPI_H

#include <string.h>
#include "Driver/DrvGPIO.h"
#include "Driver/DrvSPI.h"
#include "Driver/DrvSYS.h"
#include "Adc.h"
#include "LED.h"
#include "Platform.h"

#define SPI_GPIO_PORT		GPIOB
#define SPI_CS_PIN			DRVGPIO_PIN_1
#define SPI_SCK_PIN			DRVGPIO_PIN_2
#define SPI_MISO_PIN		DRVGPIO_PIN_3
#define SPI_MOSI_PIN		DRVGPIO_PIN_4

typedef enum spi_state_enum_t {
	NORMAL,
	SOUND_VOL_2ND_BYTE,
	READ_THRESHOLD,
	READ_TRILATERATION_BOOL
} spi_state_t;

#define GET_DIRECTION_CMD			0x01
#define TOGGLE_TRILATERATION_CMD	0x02
#define CLEAR_INTERRUPT_CMD			0x04

void SPI_init(void);
void SPI_write_byte(UINT8 value);
void SPI_clear_interrupt_line(void);

#endif // __SPI_H


