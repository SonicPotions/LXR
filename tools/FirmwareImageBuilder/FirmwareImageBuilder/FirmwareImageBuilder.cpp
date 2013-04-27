// FirmwareImageBuilder.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"


#include <iostream>
#include <fstream>
#include <math.h>

using namespace std;
#pragma pack(1)
struct InfoHeader
{
	char			headerId[4];	// should be SPFI (Sonic Potions Firmware Image)
	unsigned short	avrCodeSize;	// in byte
	unsigned int	cortexCodeSize;	//in byte
	char			dummy[512-10];
};

int _tmain(int argc, _TCHAR* argv[])
{
	ifstream input;
	ofstream output;

	unsigned int bytesWritten=0;

	//open output file
	output.open("FIRMWARE.BIN",ios::binary | ios::out);


	//get cortex code size
	input.open("DrumSynthCortex.bin",ios::binary | ios::in);
	
	if(!input.is_open())
	{
		printf("Input File DrumSynthCortex.bin not found\n");
		return -1;
	}
	
	long begin,end;
	begin = input.tellg();
	input.seekg (0, ios::end);
	end = input.tellg();
	input.seekg (0, ios::beg);

	long cortexSizeInBytes = (end-begin);

	input.close();

	

	//open avr code and get size

	input.open("DrumSynthAvr.bin",ios::binary | ios::in);
	
	if(!input.is_open())
	{
		printf("Input File DrumSynthAvr.bin not found\n");
		return -1;
	}

	
	begin = input.tellg();
	input.seekg (0, ios::end);
	end = input.tellg();
	input.seekg (0, ios::beg);

	long avrSizeInBytes = (end-begin);

	//generate info header
	struct InfoHeader infoHdr;

	infoHdr.headerId[0] = 'S';
	infoHdr.headerId[1] = 'P';
	infoHdr.headerId[2] = 'F';
	infoHdr.headerId[3] = 'I';

	memset(infoHdr.dummy,0,512-10);

	infoHdr.avrCodeSize = avrSizeInBytes;

	infoHdr.cortexCodeSize = cortexSizeInBytes;

	//write info header to file
	output.write((const char*)&infoHdr,512);

	bytesWritten +=512;
	

	//append avr code

	for(int i=0;i<avrSizeInBytes;i++)
	{
		char data = input.get();
		output << data;
		bytesWritten++;
	}

	//zero padding to get full code page
	//float usedPages = avrSizeInBytes/256.f;
	float usedPages = avrSizeInBytes/512.f;
	//int restForFullPage = (usedPages-floor(usedPages))*256;
	int restForFullPage = (usedPages-floor(usedPages))*512;

	//for(int i=0;i<restForFullPage-0xA0;i++)
	for(int i=0;i<restForFullPage;i++)
	{
		char data = 0;
		output << data;
		bytesWritten++;
	}


	input.close();

	input.open("DrumSynthCortex.bin",ios::binary | ios::in);
	
	
	if(!input.is_open())
	{
		printf("Input File DrumSynthCortex.bin not found\n");
		return -1;
	}

	//cortex code has to start on 512byte boundary
	// because of sd card sector size... makes parsing easier
	float boundary = bytesWritten/512.f;
	int itg = floorf(boundary)+1;
	itg *= 512;
	//unsigned int neededDummyBytes = 0;//itg - bytesWritten;
	unsigned int neededDummyBytes = itg - bytesWritten;

	//append dummies
	for(int i=0;i<neededDummyBytes;i++)
	{
		char data = 0;
		output << data;
		bytesWritten++;
	}
	//append cortex code

	for(int i=0;i<cortexSizeInBytes;i++)
	{
		char data = input.get();
		output << data;
	}

	//zero padding to get full code page
	/*
	float usedPages = cortexCodeSize/128.f;
	int restForFullPage = (usedPages-floor(usedPages))*128;

	for(int i=0;i<restForFullPage;i++)
	{
		char data = 0;
		output << data;
	}
	*/


	//finish
	input.close();
	output.close();
	
	return 0;
}

