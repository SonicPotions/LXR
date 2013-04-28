// FirmwareImageBuilder.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"


#include <iostream>
#include <fstream>
#include <math.h>
#include <conio.h>

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
	cout << "-----------------------------------" << endl;
	cout << "AVR: " << avrSizeInBytes << endl;
	cout << "STM: " << cortexSizeInBytes <<endl;
	cout << "-----------------------------------" << endl;

	//write info header to file
	output.write((const char*)&infoHdr,512);

	bytesWritten +=512;
	
	cout << "AVR start offset: " << bytesWritten << endl;
	//append avr code

	for(int i=0;i<avrSizeInBytes;i++)
	{
		char data = input.get();
		output << data;
		bytesWritten++;
	}
	cout << "AVR end offset: " << bytesWritten << endl;

	//zero padding to get full code page
	float usedPages = avrSizeInBytes/256.f;
	//float usedPages = avrSizeInBytes/512.f;
	int restForFullPage = (1-(usedPages-floor(usedPages)))*256;

	cout << "current 512 byte block count: " << bytesWritten/512.f << endl;
	cout << "used AVR pages: " << usedPages << endl;
	cout << "missing bytes for full AVR page: " << restForFullPage << endl;
	//int restForFullPage = (usedPages-floor(usedPages))*512;

	//for(int i=0;i<restForFullPage-0xA0;i++)
	for(int i=0;i<restForFullPage;i++)
	{
		char data = 0;
		output << data;
		bytesWritten++;
	}
	cout << "offset after padding: " << bytesWritten << " bytes written = " << bytesWritten/512.f << " blocks" << endl;
	cout << "(AVR + padding) / avr page size = "<< avrSizeInBytes+restForFullPage << " / 256 = " << (avrSizeInBytes+restForFullPage)/256.f << endl;


	input.close();
	cout << "-----------------------------------" << endl;
	cout << "padding to full sd card page 512 byte boundary" << endl;

	input.open("DrumSynthCortex.bin",ios::binary | ios::in);
	
	
	if(!input.is_open())
	{
		printf("Input File DrumSynthCortex.bin not found\n");
		return -1;
	}

	//cortex code has to start on 512byte boundary
	// because of sd card sector size... makes parsing easier
	float boundary = bytesWritten/512.f;
	int itg = ceilf(boundary);
	itg *= 512;
	//unsigned int neededDummyBytes = 0;//itg - bytesWritten;
	unsigned int neededDummyBytes = itg - bytesWritten;

	cout << "current boundary: " << boundary << " 512byte blocks"<<endl;
	cout << "needed dummy bytes: " << neededDummyBytes << endl;
	//append dummies
	for(int i=0;i<neededDummyBytes;i++)
	{
		char data = 0;
		output << data;
		bytesWritten++;
	}

	cout << "offset after padding / start STM: " << bytesWritten << " (" <<bytesWritten/512.f<<" blocks)" << endl;

	//append cortex code

	for(int i=0;i<cortexSizeInBytes;i++)
	{
		char data = input.get();
		output << data;
		bytesWritten++;
	}

	cout << "offset after STM: " << bytesWritten << endl;

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

	getch();
	
	return 0;
}

