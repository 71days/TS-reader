#ifndef TS_HEADER_H_
#define TS_HEADER_H_

#include "byteParser.h"
#include "PESHeader.h"

#define SYNC_BYTE 0x47
#define TS_PACKET_SIZE 188
#define PES_START_CODE_PREFIX 0x000001
#define CLOCK_RATE 27 //MHz

enum
{
	RESERVED		= 0x0,
	PAYLOAD_ONLY	= 0x1,
	AF_ONLY			= 0x2,
	AF_AND_PAYLOAD	= 0x3
};

typedef struct
{
	bool discontinuity_indicator;				
	bool random_access_indicator;					
	bool elementary_stream_priority_indicator;	
	bool PCR_flag;
	bool OPCR_flag;								
	bool splicing_point_flag;						
	bool transport_private_data_flag;				
	bool adaptation_field_extension_flag;	

} TS_adaptationField;


class TSHeader
{
public:
	int8_t sync_byte;
	uint8_t transport_error_indicator;
	uint8_t payload_unit_start_indicator; 
	uint8_t transport_priority;
	int16_t PID;
	uint8_t transport_scrambling_control;
	uint8_t adaptation_field_control; 
	uint8_t continuity_counter;	
	int64_t PCR_base;
	uint16_t PCR_extension;

	TS_adaptationField af;
	int64_t PCR;

	PESHeader pes_header;
	bool pesHeaderPrezent;
	/* adaptation field */

	TSHeader();
	int8_t fillHeader(byteParser *pHeader);
};




TSHeader::TSHeader()
{
	PCR = -1;
}
int8_t TSHeader::fillHeader(byteParser *pHeader)
{
	uint8_t af_length;

	sync_byte						= pHeader->getBits<int8_t>(8);
	transport_error_indicator		= pHeader->getBits<int8_t>(1); 
	payload_unit_start_indicator	= pHeader->getBits<int8_t>(1); 
	transport_priority				= pHeader->getBits<int8_t>(1);
	PID								= pHeader->getBits<int16_t>(13);
	transport_scrambling_control	= pHeader->getBits<int8_t>(2);
	adaptation_field_control		= pHeader->getBits<int8_t>(2); 
	continuity_counter				= pHeader->getBits<int8_t>(4);

	assert(sync_byte = 0x47);

	if ((adaptation_field_control == AF_ONLY) || (adaptation_field_control == AF_AND_PAYLOAD))
	{
		af_length = pHeader->getBits<uint8_t>(8);

		if (af_length > 0)
		{
			pHeader->resetBitCounter(); /* Count the number of bits that we read next. Used for sanity check  */

			af.discontinuity_indicator					= pHeader->getBits<uint8_t>(1);
			af.random_access_indicator					= pHeader->getBits<uint8_t>(1);
			af.elementary_stream_priority_indicator	= pHeader->getBits<uint8_t>(1); 
			af.PCR_flag								= pHeader->getBits<uint8_t>(1);
			af.OPCR_flag								= pHeader->getBits<uint8_t>(1);
			af.splicing_point_flag						= pHeader->getBits<uint8_t>(1);
			af.transport_private_data_flag				= pHeader->getBits<uint8_t>(1);
			af.adaptation_field_extension_flag			= pHeader->getBits<uint8_t>(1);

			if (af.PCR_flag)
			{
				PCR_base							= pHeader->getBits<int64_t>(33);
				pHeader->getBits<uint8_t>(6); /* reserved */
				PCR_extension						= pHeader->getBits<uint16_t>(9);

				PCR = PCR_base * 300 + PCR_extension;

				//printf("%d; %lld\n", PID,PCR);
			}

			assert(pHeader->getBitCounter() % 8 == 0);
			assert(af_length >= pHeader->getBitCounter() / 8);
			/* we read a byte */
			af_length -= pHeader->getBitCounter() / 8;
		}
		pHeader->jumpBytes(af_length);

	}
	pesHeaderPrezent = false;
	if (payload_unit_start_indicator)
	{
		if (pHeader->getBits<int32_t>(24) == PES_START_CODE_PREFIX)
		{
			pesHeaderPrezent = true;
			pes_header.fillHeader(pHeader);

			//printf("We found a PES!\n");
		}
	}
	return 0;
};


#endif //BYTE_PARSER_H_