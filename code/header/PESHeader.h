#ifndef PES_HEADER_H_
#define PES_HEADER_H_

#include "byteParser.h"

#define NO_OF_DEFINED_STREAMS 5

typedef struct PESStream
{
	uint8_t id;
	uint8_t mask;
	bool hasHeaderExt;
};

PESStream PES_STREAMS[NO_OF_DEFINED_STREAMS] =
{   /*id*  mask  extension*/
	{0xBD, 0xFF, true},
	{0xBE, 0xFF, false},
	{0xBF, 0xFF, false},
	{0xC0, 0xE0, true},
	{0xE0, 0xF0, true}
};

class PESHeader
{
public:
	uint8_t streamID;
	uint16_t PESPacketLength;
	uint8_t PES_scrambling_control;		
	uint8_t	PES_priority;				
	uint8_t	data_alignment_indicator;	
	uint8_t	copyright;					
	uint8_t	origOrCopy;					
	uint8_t	PTS_DTS_flg; 				
	uint8_t	ESCR_flg;					
	uint8_t	ESrate_flg;					
	uint8_t	DSM_trick_mode_flg;			
	uint8_t	additional_cpy_info_flg;	
	uint8_t	PES_CRC_flg;				
	uint8_t	PES_extension_flg;			
	uint8_t	PES_header_data_length;	
	uint64_t PTS;



	PESHeader(){};
	int8_t fillHeader(byteParser *pHeader);
};

int8_t PESHeader::fillHeader(byteParser *pHeader)
{
	bool PESHeaderExt;
	int i, aux;

	streamID = pHeader->getBits<uint8_t>(8);
	PESPacketLength = pHeader->getBits<uint16_t>(16);

	PESHeaderExt = false;
	i = 0;
	while (i < NO_OF_DEFINED_STREAMS)
	{
		if ( (streamID & PES_STREAMS[i].mask) == PES_STREAMS[i].id )
		{
			PESHeaderExt = PES_STREAMS[i].hasHeaderExt;
			break;
		}
		i++;

	}

	if ( i == NO_OF_DEFINED_STREAMS )
	{
		printf("Couldn't identify PES stream type\n");
		assert(0);
		return 0;
	}

	if (PESHeaderExt)
	{
		/*These should go into a PES header class*/
		assert( pHeader->getBits<uint8_t>(2) == 2);
		PES_scrambling_control		= pHeader->getBits<uint8_t>(2);
		PES_priority				= pHeader->getBits<uint8_t>(1);		
		data_alignment_indicator	= pHeader->getBits<uint8_t>(1);
		copyright					= pHeader->getBits<uint8_t>(1);
		origOrCopy					= pHeader->getBits<uint8_t>(1);
		PTS_DTS_flg 				= pHeader->getBits<uint8_t>(2);
		ESCR_flg					= pHeader->getBits<uint8_t>(1);
		ESrate_flg					= pHeader->getBits<uint8_t>(1);
		DSM_trick_mode_flg			= pHeader->getBits<uint8_t>(1);
		additional_cpy_info_flg		= pHeader->getBits<uint8_t>(1);
		PES_CRC_flg					= pHeader->getBits<uint8_t>(1);
		PES_extension_flg			= pHeader->getBits<uint8_t>(1);
		PES_header_data_length		= pHeader->getBits<uint8_t>(8);
		if (PTS_DTS_flg >= 2)
		{
			aux = pHeader->getBits<uint8_t>(4);
			assert(aux == PTS_DTS_flg);
			PTS = ((uint64_t)(pHeader->getBits<uint8_t>(3))) << 30;
			aux = pHeader->getBits<uint8_t>(1);
			assert(aux == 1);
			PTS |= ((uint64_t)(pHeader->getBits<uint32_t>(15))) << 15;
			aux = pHeader->getBits<uint8_t>(1);
			assert(aux == 1);
			PTS |= ((uint64_t)(pHeader->getBits<uint32_t>(15)));
		}

	}
};


#endif //PES_HEADER_H_