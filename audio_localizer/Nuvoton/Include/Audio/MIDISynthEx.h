/****************************************************************
 *                                                              *
 * Copyright (c) Nuvoton Technology Corp. All rights reserved.  *
 *                                                              *
 ****************************************************************/
#ifndef __MIDISYNTHEX_H__
#define __MIDISYNTHEX_H_

#include	"Audio/MidiSynthesizer.h"
#include "Audio/AudioCommon.h"
#ifdef	__cplusplus
extern "C"
{
#endif

//----------------------------Version Numbers------------------------------
#define MIDISYNTHEX_MAJOR_NUM		3
#define MIDISYNTHEX_MINOR_NUM		20
#define MIDISYNTHEX_BUILD_NUM		1

#define MIDISYNTHEX_VERSION_NUM _SYSINFRA_VERSION(MIDISYNTHEX_MAJOR_NUM, MIDISYNTHEX_MINOR_NUM, MIDISYNTHEX_BUILD_NUM)

//----------------------------Define constants------------------------------
#define MIDISYNTHEX_DECODE_SAMPLE_PER_FRAME			8
#define MIDISYNTHEX_DECODE_WORK_BUF_SIZE				524
#define MIDISYNTHEX_DECODE_TEMP_BUF_SIZE				0
#define MIDISYNTHEX_DECODE_POLY_BUF_SIZE				196

//----------------------------Declared APIs---------------------------------

///////////////////////////////////////////////////////////////////////////////////////////////////////
// Description:
// 	Configure midi synthesizer required resource buffer.	
// Arguments:
//  pu8DecodeWorkBuf [in]
//		Buffer size is MIDIEX_DECODE_WORK_BUF_SIZE bytes,
//		this buffer is used to keep Midi decode internal information.
// 		it can not be shared with other. Buffer needs to align at 4's multiple.
//	pu8PolyBuf [in]
//		Buffer size is MIDIEX_DECODE_POLY_BUF_SIZE bytes. Midi synthesizer need
//		this buffer to save polyphone variables.
//	u16PolyNumber [in]
//		Max playing polyphony at the same time
//	u8FIFOBuf [in]
//		FIFO buffer for partial loading midi data. Buffer needs to align at 4's multiple.
//	u16FIFOBufSize [in]
//		FIFO buffer size. Buffer must be 4's multiple.
// Return:
//	None.
///////////////////////////////////////////////////////////////////////////////////////////////////////
void MIDISynthEx_ResoureConfig(
	UINT8 *pu8DecodeWorkBuf,	
	UINT8 *pu8PolyBuf,			
	UINT16 u16PolyNumber,		
	UINT8 *u8FIFOBuf,
	UINT16 u16FIFOBufSize
);

///////////////////////////////////////////////////////////////////////////////////////////////////////
// Description:
// 	Configure wav table address for midi synthesizer.	
// Arguments:
//  pu8DecodeWorkBuf [in]
//		Buffer size is MIDIEX_DECODE_WORK_BUF_SIZE bytes,
//		this buffer is used to keep Midi decode internal information.
// 		it can not be shared with other. Buffer needs to align at 4's multiple.
//	u32WTBAddress [in]
//		Wav table address, when decoded midi file,
// 		it must refer to it to get instruments/drums data.
//	pfnReadWTBCallback [in]
//		Callback function to read wav table data.
// Return:
//	None.
///////////////////////////////////////////////////////////////////////////////////////////////////////
void MIDISynthEx_WTBConfig(
	UINT8 *pu8DecodeWorkBuf,		
	UINT32 u32WTBAddress,		 
	PFN_AUDIO_DATAREQUEST pfnReadWTBCallback
);

///////////////////////////////////////////////////////////////////////////////////////////////////////
// Description:
// 	Confgiure output sample rate and load midi information into FIFO buffer.	
// Arguments:
//	pu8DecodeWorkBuf [in]
//		Buffer size is MIDIEX_DECODE_WORK_BUF_SIZE bytes,
//		this buffer is used to keep Midi decode internal information.
// 		it can not be shared with other. Buffer needs to align at 4's multiple.
//	u32StartAddr [in]
//		The start adddress of the midi data.
//		For SPI flash, it is the SPI address.
//		For file with file system, it is the offset from a file.
//	u32DataSize [in]
//		midi data size.
//	u32OutputSampleRate [in]
//		Output sample rate.
// Return: 
//	None.
///////////////////////////////////////////////////////////////////////////////////////////////////////
void MIDISynthEx_AudioConfig(
	UINT8 *pu8DecodeWorkBuf,		
	UINT32 u32StartAddr,
	UINT32 u32DataSize,
	UINT32 u32OutputSampleRate,
	PFN_AUDIO_DATAREQUEST pfnReadDataCallback	
);

///////////////////////////////////////////////////////////////////////////////////////////////////////
// Description:
// 	Initiate midi synthesizer resource according to MidiSynEx_Configure setting buffer.	
// Arguments:
//  pu8DecodeWorkBuf [in]
//		Buffer size is MIDIEX_DECODE_WORK_BUF_SIZE bytes, 
//		this buffer is used to keep Midi decode internal information.
// 		it can not be shared with other. Buffer needs to align at 4's multiple.
//	pu8DecodeTempBuf [in]
//		Buffer size is MIDIEX_DECODE_TEMP_BUF_SIZE bytes. It's reserved at present.
//	u32StartAddr [in]
//		It's reserved at present.
//	pfnReadDataCallback [in]	
//		Callback function to read MIDI data
// Return:
//	 Return sampling rate. 0 represent failed in initiating.
///////////////////////////////////////////////////////////////////////////////////////////////////////
UINT32 MIDISynthEx_DecodeInitiate(
	UINT8 *pu8DecodeWorkBuf,	
	UINT8 *pu8DecodeTempBuf,
	UINT32 u32StartAddr,
	PFN_AUDIO_DATAREQUEST pfnReadDataCallback 
);

///////////////////////////////////////////////////////////////////////////////////////////////////////
// Description:
// 	Midi synthesizer decoding and mixing.	
// Arguments:
//  pu8DecodeWorkBuf [in]
//		Buffer size is MIDIEX_DECODE_WORK_BUF_SIZE bytes,
//		this buffer is used to keep Midi decode internal information.
// 		it can not be shared with other. Buffer needs to align at 4's multiple.
//	pu8DecodeTempBuf [in]
//		Buffer size is MIDIEX_DECODE_TEMP_BUF_SIZE bytes. It's reserved at present.
//	pi16DecodedPcmBuf [in]
//		This buffer is used to keep decoded PCM data.
//	pfnReadDataCallback [in]
//		Callback function to read MIDI data.
//	pfnUserEventCallback [in]
//		Callback function to handle user event.
// Return:
//	Return decoded PCM count. 0 represent no PCM decoded. Default output
//	samples is MIDIEX_DECODE_SAMPLE_PER_FRAME.
///////////////////////////////////////////////////////////////////////////////////////////////////////
INT32 MIDISynthEx_DecodeProcess(	
	UINT8 *pu8DecodeWorkBuf,	
	UINT8 *pu8DecodeTempBuf,	
	PINT16 pi16DecodedPcmBuf,	
	PFN_AUDIO_DATAREQUEST pfnReadDataCallback,	 
	PFN_AUDIO_USREVENT pfnUserEventCallback	
);

///////////////////////////////////////////////////////////////////////////////////////////////////////
// Description:
// 	Check decoding whether end.	
// Arguments:
//  pu8DecodeWorkBuf [in]
//		Buffer size is MIDIEX_DECODE_WORK_BUF_SIZE bytes,
//		this buffer is used to keep Midi decode internal information.
// 		it can not be shared with other.
// Return:
//	TRUE: decode to end. FALSE: can continue decodeing
///////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL MIDISynthEx_DecodeIsEnd(		
	UINT8 *pu8DecodeWorkBuf		
);

///////////////////////////////////////////////////////////////////////////////////////////////////////
// Description:
// Set decoding sample counts of frame, default is MIDIEX_DECODE_SAMPLE_PER_FRAME.	
// Arguments:
//  pu8DecodeWorkBuf [in]
//		Buffer size is MIDIEX_DECODE_WORK_BUF_SIZE bytes,
//		this buffer is used to keep Midi decode internal information.
// 		it can not be shared with other. Buffer needs to align at 4's multiple.
//	u8SampleCounts [in]
//		Decoding sample counts of one freame
// Return:
//	None
///////////////////////////////////////////////////////////////////////////////////////////////////////
void MIDISynthEx_DecodeSampleCounts(	
	UINT8 *pu8DecodeWorkBuf,  		
	UINT16 u16SampleCounts			
);

#ifdef	__cplusplus
}
#endif

#endif	//#ifndef__MIDISYNTHEX_H_
