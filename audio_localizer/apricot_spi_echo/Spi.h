#ifndef __SPI_H
#define __SPI_H

#include "Platform.h"

//
// Global Defines and Declarations
//


#define SPI_BUF_LENGTH 8

// Buffer data structure for head.
typedef __packed struct
{
	UINT8 status;
	UINT8 buffer[SPI_BUF_LENGTH];
} spiHeadData;

// XXX Note: The Rev B circuit board incorrectly swaps the DO and DI pins 
// XXX so the cable to the head needs have the DO and DI pins swapped.
#define SPI_GPIO_PORT		GPIOB
#define SPI_CS_PIN			DRVGPIO_PIN_1
#define SPI_SCK_PIN			DRVGPIO_PIN_2
#define SPI_MISO_PIN		DRVGPIO_PIN_3
#define SPI_MOSI_PIN		DRVGPIO_PIN_4

typedef enum spi_state_enum_t {
	NORMAL,
	SOUND_VOL_2ND_BYTE,
	READ_THRESHOLD,
	READ_DETECTION_BOOL
} spi_state_t;

//
// Global Functions
//


//
// open spi slave driver to talk to Sway Host
//
void spiSlave_Init(void);
//
// close spi slave driver
//
void spiSlave_Close(void);
// open spi driver
void spiMaster_init(void);
//
// close spi driver
//
void spiMaster_close(void);
//
// return 8 bits read from spi.
//
static UINT8 spiMaster_Read8(void);
//
// write a value of 8 bits to an address
//
void spiMaster_Write8(UINT8 value);
//
// Init functions.
//

// Interrupt handler for SPI packets from the body.
void spiHeadHandler(void);

#endif // __SPI_H


