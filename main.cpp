/*---------------- SANDBOX--------------------------------------------
|version: 0.1  
|author: Bogdan Sandoi
*-------------------------------------------------------------------*/

#include <fstream>
#include <iostream>
#include <stdio.h>
#include <stdint.h>
#include <assert.h>



using namespace std;



#define SYNC_BYTE 0x47
#define TS_PACKET_SIZE 188
#define PES_START_CODE_PREFIX 0x000001

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

enum
{
	RESERVED		= 0x0,
	PAYLOAD_ONLY	= 0x1,
	AF_ONLY			= 0x2,
	AF_AND_PAYLOAD	= 0x3
};

/* 
int8_t buffer[]			 [IN] array containg TS stream
uint32_t &index			 [IN/OUT] starting element from which to look for, it will return the TS offset
int8_t minNumOfSyncBytes [IN] how many sync bytes to look for before choosing offset
uint32_t lastElement     [IN] upper search boundry

return value: returns TRUE if it has found "minNumOfSyncBytes" sync bytes positioned at 188 bytes of eachother
*/
bool syncTS(int8_t buffer[], uint32_t &index, int8_t minNumOfSyncBytes, uint32_t lastElement)
{
	bool found;
	int8_t i;

	assert(buffer != NULL);
	assert(minNumOfSyncBytes > 0);
	assert(index < lastElement - minNumOfSyncBytes + 1);

	for (index = index; index<=lastElement; index++)
	{
		if (buffer[index] == SYNC_BYTE) /* we have found the sync bit */
		{
			found = true;
			printf("Found a sync bit at %d\n", index);
			for (i = 1; i < minNumOfSyncBytes; i++)
			{
				if (buffer[index + i*TS_PACKET_SIZE] != SYNC_BYTE)
				{
					found = false;
					break;
				}
			}
			if (found)
			{
				/* we are sync-ed with the TS */
				return true;
			}
		}
	}
	return false;
	/* We didn't find the sync bit */
}
/* 
This class makes the parsing semi-transparent for us
(we don't need to calculate offsets and masks for each field) 
Notes:
Disavantages: ovearhead
Might implement it as a macro, in the future, so the overhead will be gone,
(the preprocessor will generate the code as we would write the masks by hand),
but as a macro it will be possible to be used only for static structures known at compile time. 
*/
class byteParser
{
private:
	int8_t *m_poz;
	int8_t m_bitPoz;
	uint32_t m_bytesParsed;
	uint32_t m_bitCounter;
public: 
	byteParser(int8_t *initPoz)
	{
		m_poz			= initPoz;
		m_bytesParsed	= 0;
		m_bitPoz		= 0; /*bit offset inside byte, should have values between 0-7*/
		m_bitCounter	= 0;
	}
	template <class T>
	/* To Do, add get bytes */
	T getBits(int8_t numOfBits)
	{
		T finalVal = 0;
		T val;
		int8_t bitsRead = 0;
		int8_t bitsToRead;
		int8_t remainingBits;

		assert(numOfBits > 0);
		m_bitCounter+= numOfBits;

		remainingBits = numOfBits;

		while (remainingBits != 0)
		{
			bitsToRead = min<int8_t>(remainingBits, 8 - m_bitPoz);

			val = *m_poz & (0xff >> m_bitPoz);
			val = val >> (8 - bitsToRead - m_bitPoz);

			bitsRead += bitsToRead;
			finalVal |= val << (numOfBits - bitsRead);


			remainingBits -= bitsToRead;

			m_bitPoz += bitsToRead;
			m_poz    +=  m_bitPoz >> 3;
			m_bitPoz %= 8;
		}
		return finalVal;

	}

	void jumpBytes(int16_t numOfBytes)
	{
		assert(numOfBytes >= 0);
		assert(m_bitPoz == 0); /* check we are at the begining of the byte */
		m_poz += numOfBytes;

		m_bitCounter+= numOfBytes / 8;

	}

	void resetBitCounter()
	{
		m_bitCounter = 0;
	}

	uint32_t getBitCounter()
	{
		return m_bitCounter;
	}
};

struct ESInfo
{
	int16_t PID;
	uint64_t dataRate;
	uint64_t lastPCR;
	uint64_t PES_SizeAccumulator;
}

class ESLib
{

}

bool readTP(byteParser *pHeader, int64_t & PCR )
{

	uint8_t af_length;
	uint8_t streamID;
	bool PESHeaderExt;
	int i;



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


	/* adaptation field */
	bool discontinuity_indicator;				
	bool random_access_indicator;					
	bool elementary_stream_priority_indicator;	
	bool PCR_flag = 0;
	bool OPCR_flag;								
	bool splicing_point_flag;						
	bool transport_private_data_flag;				
	bool adaptation_field_extension_flag;		

	int64_t PCR_base;
	uint16_t PCR_extension;

	PCR = -1; /* invalidate PCR*/
	/*These should go into a TS Packet class*/
	int8_t sync_byte						= pHeader->getBits<int8_t>(8);
	uint8_t transport_error_indicator		= pHeader->getBits<int8_t>(1); 
	uint8_t payload_unit_start_indicator	= pHeader->getBits<int8_t>(1); 
	uint8_t transport_priority				= pHeader->getBits<int8_t>(1);
	int16_t PID								= pHeader->getBits<int16_t>(13);
	uint8_t transport_scrambling_control	= pHeader->getBits<int8_t>(2);
	uint8_t adaptation_field_control		= pHeader->getBits<int8_t>(2); 
	uint8_t continuity_counter				= pHeader->getBits<int8_t>(4);

	assert(sync_byte = 0x47);

#ifdef DEBUG_LEVEL_1
	cout << "sync_byte: 0x"						<< hex << (int)sync_byte					<< endl;
	cout << "transport_error_indicator: 0"		<< dec << (int)transport_error_indicator	<< endl;
	cout << "transport_priority: 0"				<< (int)transport_priority			<< endl;
	cout << "PID: "								<< (int)PID							<< endl;
	cout << "transport_scrambling_control: 0"	<< (int)transport_scrambling_control	<< endl;
	cout << "adaptation_field_control: 0"		<< (int)adaptation_field_control		<< endl;
	cout << "continuity_counter: "				<< (int)continuity_counter			<< endl;
#endif

	if ((adaptation_field_control == AF_ONLY) || (adaptation_field_control == AF_AND_PAYLOAD))
	{
		af_length = pHeader->getBits<uint8_t>(8);

		if (af_length > 0)
		{
			pHeader->resetBitCounter(); /* Count the number of bits that we read next. Used for sanity check  */

			discontinuity_indicator					= pHeader->getBits<uint8_t>(1);
			random_access_indicator					= pHeader->getBits<uint8_t>(1);
			elementary_stream_priority_indicator	= pHeader->getBits<uint8_t>(1); 
			PCR_flag								= pHeader->getBits<uint8_t>(1);
			OPCR_flag								= pHeader->getBits<uint8_t>(1);
			splicing_point_flag						= pHeader->getBits<uint8_t>(1);
			transport_private_data_flag				= pHeader->getBits<uint8_t>(1);
			adaptation_field_extension_flag			= pHeader->getBits<uint8_t>(1);

			if (PCR_flag)
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
	if (PCR_flag )
	{
		if(!payload_unit_start_indicator)
			printf("%d; %lld<----------\n", PID,PCR);
		else
			printf("%d; %lld\n", PID,PCR);
		//printf("Stop\n");
	}
	if (payload_unit_start_indicator)
	{
		if (pHeader->getBits<int32_t>(24) == PES_START_CODE_PREFIX)
		{
			//printf("We found a PES!\n");
			streamID = pHeader->getBits<uint8_t>(8);
			PESPacketLength = pHeader->getBits<uint8_t>(16);

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

#ifdef DEBUG_LEVEL_1
				printf("PES HEADER\n");
				printf("___________\n");
				cout<<"PES_scrambling_control "		<<(int)PES_scrambling_control <<endl;
				cout<<"PES_priority "				<<(int)PES_priority	<<endl;
				cout<<"data_alignment_indicator "	<<(int)data_alignment_indicator <<endl;
				cout<<"copyright "					<<(int)copyright <<endl;
				cout<<"origOrCopy "					<<(int)origOrCopy <<endl;
				cout<<"PTS_DTS_flg "				<<(int)PTS_DTS_flg <<endl;
				cout<<"ESCR_flg "					<<(int)ESCR_flg <<endl;
				cout<<"ESrate_flg"					<<(int)ESrate_flg <<endl;
				cout<<"DSM_trick_mode_flg "			<<(int)DSM_trick_mode_flg <<endl;
				cout<<"additional_cpy_info_flg "	<<(int)additional_cpy_info_flg<<endl;
				cout<<"PES_CRC_flg "				<<(int)PES_CRC_flg<<endl;
				cout<<"PES_extension_flg "			<<(int)PES_extension_flg <<endl;
				cout<<"PES_header_data_length "		<<(int)PES_header_data_length <<endl;
#endif
				if (ESrate_flg || ESCR_flg)
				{
					printf("Stop\n");
				}
			}
			else
			{
				printf("PES Stream doesn't have an extension\n");
			}

		}
		else
		{
			//assert(0);
		}

	}




	return 0;
}

int8_t buffer[300000000];

void main()
{
	int bytesRead;
	int i;
	bool synced;
	int lSize;
	int iterations;

	int64_t PCR;
	uint32_t index = 0;
	FILE *pFile;

	pFile = fopen("h:/mpt-smart-travels-classical-clip.ts","rb");
	fseek (pFile , 0 , SEEK_END);
	lSize = ftell (pFile);
	rewind (pFile);
	bytesRead = fread (buffer,1,30000000,pFile);

	iterations = bytesRead/TS_PACKET_SIZE;
	synced = syncTS(buffer, index, 4, 10000);

	if (synced)
	{
		printf("synced at %d\n", index);
		for (i=0;i<iterations;i++)
		{
#ifdef DEBUG_LEVEL_1
			cout<<"----------------------------"<<endl;
			cout<<"----Packet "<<i<<"----"<<endl;
			cout<<"----------------------------"<<endl;
#endif
			readTP(&byteParser(&buffer[index + i*TS_PACKET_SIZE]), PCR);
			//printf("%lld\n",PCR);
		}

	}
	else
	{	
		printf("Couldn't Sync!\n");
	}


}