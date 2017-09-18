#include "Spi.h"

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
#define SPI_HANDLER				DRVSPI_SPI1_HANDLER
#define SPI_DIVIDER     		(_DRVSPI_DIVIDER(DrvCLK_GetHclk(), 8000000))
#define SPI_OPEN_FLAGS 			(DRVSPI_ENDIAN_BIG | \
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

static uint8_t first_byte(int16_t num) {
	return (uint8_t) ((num>>8) & 0xFF);
}

static uint8_t second_byte(int16_t num) {
	return (uint8_t) (num & 0xFF);
}

//
// Global Functions
//

// Enable SPI1 peripheral as a slave and enable SPI1 interrupts.
void SPI_init(void) {
	// Open the SPI driver.
	DrvSPI_Open(SPI_HANDLER, SPI_OPEN_FLAGS, SPI_DIVIDER);

	// Configure for slave mode.
	DrvSPI_SPI1_SetSlaveMode(TRUE);

	// Level trigger for slave mode.
	DrvSPI_SPI1_LevelTriggerInSlave(TRUE);

	// Set a silly initial byte for debugging purposes.
	DrvSPI_SingleWriteData0(SPI_HANDLER, (uint32_t) 0xa6);

	// Initiate the SPI transaction.
	DrvSPI_SetGo(SPI_HANDLER);
	
	// Enable the SPI interrupt
	DrvSPI_EnableInt(SPI_HANDLER);
}

// Write a byte to SPI1 peripheral. It will be sent in the next transaction.
void SPI_write_byte(uint8_t value) {
	// Write a byte to SPI1 peripheral.
	DrvSPI_SingleWriteData0(SPI_HANDLER, (uint32_t) value);

	// Initiate the next SPI transaction.
	DrvSPI_SetGo(SPI_HANDLER);
}

void SPI_clear_interrupt_line(void) {
	// Turn off sound detected pin.
	DrvGPIO_ClearOutputBit(&GPIOB, DRVGPIO_PIN_12);
	Sound_Detect_reset_moving_average();
	LED_blink_for_half_second();
}

void SPI1_IRQHandler() {
	uint8_t spi_read_byte;
	static spi_state_t spi_state = NORMAL;
	static int16_t current_sound_level;
	
	if(DrvSPI_GetIntFlag(DRVSPI_SPI1_HANDLER)) {
		// Clear interrupt flag.
		DrvSPI_ClearIntFlag(DRVSPI_SPI1_HANDLER);
		
		// Read incoming byte.
		spi_read_byte = DrvSPI_SingleReadData0(SPI_HANDLER);
		
		// Write a response based on state and incoming message.
		switch(spi_state) {
			case SOUND_VOL_2ND_BYTE:
				SPI_write_byte(second_byte(current_sound_level));
				spi_state = NORMAL;
				break;
			case READ_THRESHOLD:
				Sound_Detect_set_threshold(spi_read_byte);
				spi_state = NORMAL;
				SPI_write_byte(spi_read_byte);
				break;
			case READ_DETECTION_BOOL:
				if(spi_read_byte == 1) {
					Sound_Detect_start();
				} else {
					Sound_Detect_stop();
				}
				spi_state = NORMAL;
				SPI_write_byte(spi_read_byte);
				break;
			case NORMAL:
			default:
				switch(spi_read_byte) {
					case GET_SOUND_CMD:
						current_sound_level = Sound_Detect_get_current_sound_level();
						SPI_write_byte(first_byte(current_sound_level));
						spi_state = SOUND_VOL_2ND_BYTE;
						break;
					case SET_THRESHOLD_CMD:
						current_sound_level = Sound_Detect_get_current_sound_level();
						spi_state = READ_THRESHOLD;
						SPI_write_byte(spi_read_byte);
						break;
					case TOGGLE_DETECTION_CMD:
						current_sound_level = Sound_Detect_get_current_sound_level();
						spi_state = READ_DETECTION_BOOL;
						SPI_write_byte(spi_read_byte);
						break;
					case CLEAR_INTERRUPT_CMD:
						SPI_clear_interrupt_line();
						SPI_write_byte(spi_read_byte);
						break;
					default:
						SPI_write_byte(spi_read_byte);
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
