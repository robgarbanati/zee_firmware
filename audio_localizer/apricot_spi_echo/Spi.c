#include <string.h>
#include "Driver/DrvGPIO.h"
#include "Driver/DrvSPI.h"
#include "Driver/DrvSYS.h"
#include "Spi.h"
#include "Adc.h"
#include "LED.h"

//
// Global Variables
//
uint8_t spi_read_buf[20];
volatile uint8_t spi_buf_head = 0;
volatile uint8_t spi_buf_tail = 0;
extern volatile int adc_current_sound_level;

//
// Local Defines
//

// SPI slave mode for communication with robot body.
#define SPI_SLAVE_HANDLER					DRVSPI_SPI1_HANDLER
#define SPI_SLAVE_DIVIDER     			(_DRVSPI_DIVIDER(DrvCLK_GetHclk(), 8000000))
#define SPI_SLAVE_OPEN_FLAGS 			(DRVSPI_ENDIAN_BIG | \
										 DRVSPI_MSB_FIRST | \
										 DRVSPI_TX_1DATA | \
										 DRVSPI_TX_NEGEDGE | \
										 DRVSPI_RX_POSEDGE | \
										 _DRVSPI_DATA_BITS(8))

#define SET_THRESHOLD_CMD		0x01
#define TOGGLE_DETECTION_CMD	0x02
#define GET_SOUND_CMD			0x03
#define CLEAR_INTERRUPT_CMD		0x04




//
// Local Functions
//

//
// open spi slave driver to talk to Sway Host
//
void spiSlave_Init(void)
{
	// Open the SPI driver.
	DrvSPI_Open(SPI_SLAVE_HANDLER, SPI_SLAVE_OPEN_FLAGS, SPI_SLAVE_DIVIDER);

	// Configure for slave mode.
	DrvSPI_SPI1_SetSlaveMode(TRUE);

	// Level trigger for slave mode.
	DrvSPI_SPI1_LevelTriggerInSlave(TRUE);

	// Set the zero status byte to shift out.
	DrvSPI_SingleWriteData0(SPI_SLAVE_HANDLER, (UINT32) 0xa6);

	// Initiate the SPI transaction.
	DrvSPI_SetGo(SPI_SLAVE_HANDLER);
	
	// Enable the SPI interrupt
	DrvSPI_EnableInt(SPI_SLAVE_HANDLER);
}
//
// close spi slave driver
//
void spiSlave_Close(void) {
	// Close the SPI driver.
	DrvSPI_Close(SPI_SLAVE_HANDLER);
}

// 
// write a value of 8 bits to Sway Hoste
//
void spiSlave_Write8(UINT8 value)
{
	// Set the data to shift out.
	DrvSPI_SingleWriteData0(SPI_SLAVE_HANDLER, (UINT32) value);

	// Initiate the next SPI transaction.
	DrvSPI_SetGo(SPI_SLAVE_HANDLER);
}

uint8_t first_byte(int16_t num) {
	return (uint8_t) ((num>>8) & 0xFF);
}

uint8_t second_byte(int16_t num) {
	return (uint8_t) (num & 0xFF);
}

//uint8_t spi_get_sound_threshold(void) {
//	return sound_threshold;
//}

void spi_clear_interrupt_line(void) {
	LED_set_low();  // TODO slight bug if detection is off. 
}

void SPI1_IRQHandler() {
	uint8_t spi_read_byte;
	static spi_state_t spi_state = NORMAL;
	static int16_t current_sound_level;
	
	if(DrvSPI_GetIntFlag(DRVSPI_SPI1_HANDLER)) {
		// Clear interrupt flag.
		DrvSPI_ClearIntFlag(DRVSPI_SPI1_HANDLER);
		
		// Read incoming byte.
		spi_read_byte = DrvSPI_SingleReadData0(SPI_SLAVE_HANDLER);
		
		// Write a response based on state and incoming message.
		switch(spi_state) {
			case SOUND_VOL_2ND_BYTE:
				spiSlave_Write8(second_byte(current_sound_level));
				spi_state = NORMAL;
				break;
			case READ_THRESHOLD:
				adc_set_sound_threshold(spi_read_byte);
				spi_state = NORMAL;
				spiSlave_Write8(spi_read_byte);
				break;
			case READ_DETECTION_BOOL:
				adc_toggle_sound_detection(spi_read_byte);
				spi_state = NORMAL;
				spiSlave_Write8(spi_read_byte);
				break;
			case NORMAL:
			default:
				switch(spi_read_byte) {
					case GET_SOUND_CMD:
						current_sound_level = adc_get_current_sound_level();
						spiSlave_Write8(first_byte(current_sound_level));
						spi_state = SOUND_VOL_2ND_BYTE;
						break;
					case SET_THRESHOLD_CMD:
						current_sound_level = adc_get_current_sound_level();
						spi_state = READ_THRESHOLD;
						spiSlave_Write8(spi_read_byte);
						break;
					case TOGGLE_DETECTION_CMD:
						current_sound_level = adc_get_current_sound_level();
						spi_state = READ_DETECTION_BOOL;
						spiSlave_Write8(spi_read_byte);
						break;
					case CLEAR_INTERRUPT_CMD:
						spi_clear_interrupt_line();
						spiSlave_Write8(spi_read_byte);
						break;
					default:
						spiSlave_Write8(spi_read_byte);
						spi_state = NORMAL;
						break;
				}
				break;
		}
		
		// Log received bytes to be printed out later.
		spi_read_buf[spi_buf_tail++] = spi_read_byte;
		if(spi_buf_tail == 20) spi_buf_tail = 0;
	}
}

