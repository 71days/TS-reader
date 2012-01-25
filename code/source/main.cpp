/*---------------- SANDBOX--------------------------------------------
|version: 0.3 
|author: Bogdan Sandoi
*-------------------------------------------------------------------*/

#include <fstream>
#include <iostream>
#include <map>
#include <stdio.h>
#include <stdint.h>
#include <assert.h>

#include "byteParser.h"
#include "TSHeader.h"

using namespace std;


#define DEBUG_LEVEL_1
#define RATES_DIR_NAME "ratesDir"
#define MAKEDIR "mkdir ratesDir"





class ESRateInfo
{
public:
	map<int, int> streamID;
	map<int, int> packetSize;
	map<int, int64_t>lastTime;
	map<int, float>rateMbps;
};

class packetParser
{
	ESRateInfo	rateInfo;
	int			packetIter;
	int			bufferLength;
	int8_t		*packetPtr;
	uint32_t	TSByteOffset;

	bool syncTS(int8_t buffer[], uint32_t &index, int8_t minNumOfSyncBytes, uint32_t lastElement); 
	bool readTP(byteParser *pHeader, int64_t & PCR );
public:
		packetParser(int8_t *packet, int bufferLength);
		bool sync(int8_t minNumOfSyncBytes);
		void parsePackets(int numOfPackets);
		map<int,float> getPIDRateMap();
};



packetParser::packetParser(int8_t *packet, int bufferLength)
{
	packetPtr			= packet;
	this->bufferLength	= bufferLength;
}

bool packetParser::sync(int8_t minNumOfSyncBytes)
{
	TSByteOffset = 0;
	return syncTS(packetPtr, TSByteOffset, minNumOfSyncBytes, bufferLength);
}

void packetParser::parsePackets(int numOfPackets)
{
		int64_t PCR;
		for (packetIter=0; packetIter < numOfPackets; packetIter++)
		{

			readTP(&byteParser(&packetPtr[TSByteOffset + packetIter*TS_PACKET_SIZE]), PCR);
			//printf("%lld\n",PCR);
		}
}

map<int,float> packetParser::getPIDRateMap()
{
	return rateInfo.rateMbps;
}
/* 
int8_t buffer[]			 [IN] array containg TS stream
uint32_t &index			 [IN/OUT] starting element from which to look for, it will return the TS offset
int8_t minNumOfSyncBytes [IN] how many sync bytes to look for before choosing offset
uint32_t lastElement     [IN] upper search boundry

return value: returns TRUE if it has found "minNumOfSyncBytes" sync bytes positioned at 188 bytes of eachother
*/
bool packetParser::syncTS(int8_t buffer[], uint32_t &index, int8_t minNumOfSyncBytes, uint32_t lastElement)
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




bool packetParser::readTP(byteParser *pHeader, int64_t & PCR )
{
	ofstream myfile;
	char fileName[255];
	uint8_t af_length;
	uint8_t streamID;
	bool PESHeaderExt;
	int i;
	float instantRate;
	int64_t deltaTime;

	TSHeader ts_header;



	ts_header.fillHeader(pHeader);


#ifdef DEBUG_LEVEL_1
	if (ts_header.pesHeaderPrezent)
	{


		if (rateInfo.streamID.count(ts_header.PID))
		{
			assert(rateInfo.streamID[ts_header.PID] ==  ts_header.pes_header.streamID);
		}
		else
		{
			rateInfo.streamID[ts_header.PID] = ts_header.pes_header.streamID;
			rateInfo.packetSize[ts_header.PID] = 0;
			rateInfo.lastTime[ts_header.PID] = 0; // init
			sprintf(fileName,"%s/%d", RATES_DIR_NAME,ts_header.PID);
			myfile.open(fileName);
			myfile<<"Rate(Mbps)"<<"\t\t\t"<<"Packet Size"<<"\t\t\t"<<"deltaPCR"<<endl;
			myfile.close();
		}


		sprintf(fileName,"%s/%d", RATES_DIR_NAME,ts_header.PID);
		myfile.open(fileName,ios::app);

		if(ts_header.PCR != -1)
		{
			/*Mbps*/
			instantRate = rateInfo.packetSize[ts_header.PID] * CLOCK_RATE * BITS_IN_BYTE;  
			deltaTime = (ts_header.PCR - rateInfo.lastTime[ts_header.PID]);
			instantRate /= deltaTime; 
			myfile<<instantRate<<"\t\t\t"<<rateInfo.packetSize[ts_header.PID]<<"\t\t\t"<<ts_header.PCR - rateInfo.lastTime[ts_header.PID]<<endl;

			rateInfo.lastTime[ts_header.PID] = ts_header.PCR;
			rateInfo.rateMbps[ts_header.PID] = instantRate;

		}
		else
		{
			instantRate = rateInfo.packetSize[ts_header.PID] * (float)PTS_CLOCK_RATE * BITS_IN_BYTE;  
			deltaTime = (ts_header.pes_header.PTS - rateInfo.lastTime[ts_header.PID]);
			instantRate /= deltaTime; 
			myfile<<instantRate<<"\t\t\t"<<rateInfo.packetSize[ts_header.PID]<<"\t\t\t"<<ts_header.pes_header.PTS  - rateInfo.lastTime[ts_header.PID]<<endl;

			rateInfo.lastTime[ts_header.PID] = ts_header.pes_header.PTS;
			rateInfo.rateMbps[ts_header.PID] = instantRate;
		}

		myfile.close();
		rateInfo.packetSize[ts_header.PID] = 0; //reset


#ifdef CONSOLE_PRINT
		cout<<"-------------------------------"<<endl;
		cout<<"----Packet "<<packetIter<<"----"<<endl;
		cout<<"-------------------------------"<<endl;
		cout <<endl<< "========================================================"<<endl;
		cout << "sync_byte: 0x"						<< hex << (int)ts_header.sync_byte					<< endl;
		cout << "transport_error_indicator: 0"		<< dec << (int)ts_header.transport_error_indicator	<< endl;
		cout << "transport_priority: 0"				<< (int)ts_header.transport_priority			<< endl;
		cout << "PID: "								<< (int)ts_header.PID							<< endl;
		cout << "transport_scrambling_control: 0"	<< (int)ts_header.transport_scrambling_control	<< endl;
		cout << "adaptation_field_control: 0"		<< (int)ts_header.adaptation_field_control		<< endl;
		cout << "continuity_counter: "				<< (int)ts_header.continuity_counter			<< endl;
		cout << "PCR: "				<< ts_header.PCR << endl;
		printf("PES HEADER\n");
		printf("___________\n");

		cout<<"PES_packet length "			<<(int)ts_header.pes_header.PESPacketLength <<endl;
		cout<<"PES_scrambling_control "		<<(int)ts_header.pes_header.PES_scrambling_control <<endl;
		cout<<"PES_priority "				<<(int)ts_header.pes_header.PES_priority	<<endl;
		cout<<"data_alignment_indicator "	<<(int)ts_header.pes_header.data_alignment_indicator <<endl;
		cout<<"copyright "					<<(int)ts_header.pes_header.copyright <<endl;
		cout<<"origOrCopy "					<<(int)ts_header.pes_header.origOrCopy <<endl;
		cout<<"PTS_DTS_flg "				<<(int)ts_header.pes_header.PTS_DTS_flg <<endl;
		cout<<"ESCR_flg "					<<(int)ts_header.pes_header.ESCR_flg <<endl;
		cout<<"ESrate_flg"					<<(int)ts_header.pes_header.ESrate_flg <<endl;
		cout<<"DSM_trick_mode_flg "			<<(int)ts_header.pes_header.DSM_trick_mode_flg <<endl;
		cout<<"additional_cpy_info_flg "	<<(int)ts_header.pes_header.additional_cpy_info_flg<<endl;
		cout<<"PES_CRC_flg "				<<(int)ts_header.pes_header.PES_CRC_flg<<endl;
		cout<<"PES_extension_flg "			<<(int)ts_header.pes_header.PES_extension_flg <<endl;
		cout<<"PES_header_data_length "		<<(int)ts_header.pes_header.PES_header_data_length <<endl;
		cout<<"Bytes parsed "				<<pHeader->getBytesParsed()<<endl;
		cout << "Press any key to continue" 	<< endl;			
		cin.get();
#endif
	}

	rateInfo.packetSize[ts_header.PID] += TS_PACKET_SIZE - pHeader->getBytesParsed();
#endif
	return 0;
}

int8_t buffer[300000000];

void main()
{
	int bytesRead;
	int i;
	bool synced;
	int lSize;

	int64_t PCR;
	uint32_t index = 0;
	FILE *pFile;

	pFile = fopen("stream.ts","rb");
	fseek (pFile , 0 , SEEK_END);
	lSize = ftell (pFile);
	rewind (pFile);
	bytesRead = fread (buffer,1,30000000,pFile);
	fclose(pFile);


	system(MAKEDIR);

	packetParser packetReader(buffer,10000);
	synced = packetReader.sync(4);
	packetReader.parsePackets(10000);

	if (synced)
	{
		printf("synced at %d\n", index);
	}
	else
	{	
		printf("Couldn't Sync!\n");
	}

	map<int,float> dataRates = packetReader.getPIDRateMap();
}