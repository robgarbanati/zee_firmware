#include <string.h>
#include "cmsis_os.h"
#include "Driver/DrvGPIO.h"
#include "Driver/DrvSPI.h"
#include "Driver/DrvSYS.h"
#include "Spi.h"
#include "Led.h"
#include "Command.h"
#include "Debug.h"

//
// Global Variables
//


//
// Local Defines
//

// SPI master mode for communication with IO Expander.
#define SPI_MASTER_HANDLER		DRVSPI_SPI0_HANDLER
#define SPI_MASTER_DEVICE  		eDRVSPI_SLAVE_1
#define SPI_MASTER_DIVIDER    (_DRVSPI_DIVIDER(DrvCLK_GetHclk(), 100000))
#define SPI_MASTER_OPEN_FLAGS (DRVSPI_ENDIAN_BIG | \
																	 DRVSPI_IDEL_CLK_LOW | \
																	 DRVSPI_MSB_FIRST | \
																	 DRVSPI_TX_1DATA | \
																	 DRVSPI_TX_NEGEDGE | \
																	 DRVSPI_RX_POSEDGE | \
																	 _DRVSPI_SLEEP(2) | \
																	 _DRVSPI_DATA_BITS(8))

// SPI slave mode for communication with robot body.
#define SPI_SLAVE_HANDLER					DRVSPI_SPI1_HANDLER
#define SPI_SLAVE_DIVIDER     			(_DRVSPI_DIVIDER(DrvCLK_GetHclk(), 8000000))
#define SPI_SLAVE_OPEN_FLAGS 			(DRVSPI_ENDIAN_BIG | \
																	 DRVSPI_MSB_FIRST | \
																	 DRVSPI_TX_1DATA | \
																	 DRVSPI_TX_NEGEDGE | \
																	 DRVSPI_RX_POSEDGE | \
																	 _DRVSPI_DATA_BITS(8))


int button1, button2, button3, button4;
int pwm1, pwm2, pwm3, pwm4;

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
	DrvSPI_SingleWriteData0(SPI_SLAVE_HANDLER, (UINT32) 0x00);

	// Initiate the SPI transaction.
	DrvSPI_SetGo(SPI_SLAVE_HANDLER);

	// Enable interupt on SPI CS falling.
	DrvGPIO_SetFallingInt(&SPI_HEAD_GPIO, SPI_HEAD_CS_PIN, TRUE);
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
//
// return 8 bits read from spi.
//
static UINT8 spiSlave_Read8(void)
{
	UINT8 readValue;

	// Read the value shifted out.
	readValue = (UINT8) DrvSPI_SingleReadData0(SPI_SLAVE_HANDLER);

	return readValue;
}
//
// open spi master driver to talk to CryDetect
//
void spiMaster_Init(void) {
	// Open the SPI driver.
	DrvSPI_Open(SPI_MASTER_HANDLER, SPI_MASTER_OPEN_FLAGS, SPI_MASTER_DIVIDER);

	// Select the slave.
	DrvSPI_SlaveSelect(SPI_MASTER_HANDLER, TRUE, DRVSPI_IDEL_CLK_LOW);
	DrvSPI_SelectSlave(SPI_MASTER_HANDLER, SPI_MASTER_DEVICE);

	// Read/write data in 16 bit chunks.
	DrvSPI_SetDataConfig(SPI_MASTER_HANDLER, 1, 8);
}

//
// close spi master driver
//
void spiMaster_Close(void) {
	// Close the SPI driver.
	DrvSPI_Close(SPI_MASTER_HANDLER);
}

//
// return 8 bits read from spi.
//
static UINT8 spiMaster_Read8(void)
{
	UINT8 readValue;

	// Read the value shifted out.
	readValue = (UINT8) DrvSPI_SingleReadData0(SPI_MASTER_HANDLER);

	return readValue;
}

// 
// write a value of 8 bits to an address
//
void spiMaster_Write8(UINT8 value)
{
	// Set the data to shift out of the SPI port.
	DrvSPI_SingleWriteData0(SPI_MASTER_HANDLER, (UINT32) value);
	
	// Initiate the SPI transaction.
	DrvSPI_SetGo(SPI_MASTER_HANDLER);
}




//
// Handle the send/receive of the SPI packets at interrupt time.
// On the order of 10 to 100 microseconds
//
void spiHeadHandler(void)
{
	UINT8 index = 0;
	UINT8 sendData[SPI_BUF_LENGTH];
	UINT8 spiSlave_Data[SPI_BUF_LENGTH];
	UINT8 spiMaster_Data[SPI_BUF_LENGTH];

	// Assume we receive a zero length packet.
	spiSlave_Data[0] = 0;
	spiMaster_Data[0] = 0;

	// Send/receive bytes until the SPI CS pin is raised.
	while (!DrvGPIO_GetInputPinValue(&SPI_HEAD_GPIO, SPI_HEAD_CS_PIN))
	{
		// Wait while the SPI ports are busy and the SPI CS line is low.
		while (DrvSPI_GetBusy(SPI_SLAVE_HANDLER) && !DrvGPIO_GetInputPinValue(&SPI_HEAD_GPIO, SPI_HEAD_CS_PIN));  // && DrvSPI_GetBusy(SPI_MASTER_HANDLER)

		// Process the next byte if the SPI CS line is still low.
		if (!DrvGPIO_GetInputPinValue(&SPI_HEAD_GPIO, SPI_HEAD_CS_PIN))
		{
			// Read the value shifted in.
			spiSlave_Data[index] = (UINT8) DrvSPI_SingleReadData0(SPI_SLAVE_HANDLER);
			spiMaster_Data[index] = (UINT8) DrvSPI_SingleReadData0(SPI_MASTER_HANDLER);
		
			// Set the data to shift out.
			spiSlave_Write8(spiMaster_Data[index]);
			spiMaster_Write8(spiSlave_Data[index]);
			
//			// mirror mode
//			spiSlave_Write8(spiSlave_Data[index]);
//			spiMaster_Write8(spiMaster_Data[index]);
			
//			// constant mode
//			spiSlave_Write8(0xC3);
//			spiMaster_Write8(0x42);

			// Increment the index, but prevent overflow.
			if (index < SPI_BUF_LENGTH) ++index;
		}
	}

	// Zero the length of the send packet to indicate it has been sent.
	sendData[0] = 0;

	// Initialize the first zero status byte to shift out on the next packet.
	spiSlave_Write8(spiMaster_Data[index-1]);
	spiMaster_Write8(spiSlave_Data[index-1]);

//	// mirror mode
//	spiSlave_Write8(spiSlave_Data[index-1]);
//	spiMaster_Write8(spiMaster_Data[index]);
	
//	// constant mode
//	spiSlave_Write8(0xF0);
//	spiMaster_Write8(0xBF);
}
