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



int packetIter;





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
class PESHeader: public TSHeader
{

}
*/

class ESRateInfo
{
public:
	map<int, int> streamID;
	map<int, int> packetSize;
	map<int, int64_t>lastTime;
};
/*
class ESLib
{

}
*/

//map<int, int> testMap;
//map<int, int> testMap2;

ESRateInfo rateInfo;
bool readTP(byteParser *pHeader, int64_t & PCR )
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

		}
		else
		{
			myfile<<"PCR not present. To be decoded using PES timing information(PTS)"<<endl;
			//TO DO: extract rateInfo.lastTime[ts_header.PID] = ts_header.pes_header.;
		}

		myfile.close();
		rateInfo.packetSize[ts_header.PID] = 0; //reset


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
	int iterations;

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


	iterations = bytesRead/TS_PACKET_SIZE;
	synced = syncTS(buffer, index, 4, 10000);

	if (synced)
	{
		printf("synced at %d\n", index);
		for (packetIter=0;packetIter<iterations;packetIter++)
		{

			readTP(&byteParser(&buffer[index + packetIter*TS_PACKET_SIZE]), PCR);
			//printf("%lld\n",PCR);
		}

	}
	else
	{	
		printf("Couldn't Sync!\n");
	}


}