//**************************************************************
// ****** FUNCTIONS FOR SD RAW DATA TRANSFER *******
//**************************************************************
//Controller: STM32F4 (Clock: 186 Mhz-internal)
//Compiler	: AVR-GCC (winAVR with AVRStudio)
//Project V.: Version - 2.4.1
//Author	: CC Dharmani, Chennai (India)
//			  www.dharmanitech.com
//Date		: 24 Apr 2011
//
//modified for STM32	: Julian Schmidt
//Date		: 24.10.2012
//**************************************************************

//Link to the Post: http://www.dharmanitech.com/2009/01/sd-card-interfacing-with-atmega8-fat32.html
#include "config.h"
#if USE_SD_CARD
//#include <avr/io.h>
//#include <avr/pgmspace.h>
#include "SPI_routines.h"
#include "sd_routines.h"
//#include "UART_routines.h"
//#include "../encoder.h"

//******************************************************************
//Function	: to initialize the SD/SDHC card in SPI mode
//Arguments	: none
//return	: unsigned char; will be 0 if no error,
// 			  otherwise the response byte will be sent
//******************************************************************
unsigned char SD_init(void)
{

	unsigned char i, response, SD_version;
	unsigned int retry=0 ;

	 for(i=0;i<10;i++)
		  SPI_transmit(0xff);   //80 clock pulses spent before sending the first command

	SD_CS_ASSERT;
	do
	{
  
	   response = SD_sendCommand(GO_IDLE_STATE, 0); //send 'reset & go idle' command
	   retry++;
	   if(retry>0x20) 
   		  return 1;   //time out, card not detected
   
	} while(response != 0x01);

	SD_CS_DEASSERT;
	SPI_transmit (0xff);
	SPI_transmit (0xff);

	retry = 0;

	SD_version = 2; //default set to SD compliance with ver2.x; 
					//this may change after checking the next command
	do
	{
	response = SD_sendCommand(SEND_IF_COND,0x000001AA); //Check power supply status, mendatory for SDHC card
	retry++;
	if(retry>0xfe) 
	   {
		 // TX_NEWLINE;
		  SD_version = 1;
		  cardType = 1;
		  break;
	   } //time out

	}while(response != 0x01); 

	retry = 0;
 
	do
	{
	response = SD_sendCommand(APP_CMD,0); //CMD55, must be sent before sending any ACMD command
	response = SD_sendCommand(SD_SEND_OP_COND,0x40000000); //ACMD41

	retry++;
	if(retry>0xfe) 
	   {
		  //TX_NEWLINE;
		  return 2;  //time out, card initialization failed
	   } 

	}while(response != 0x00);


	retry = 0;
	SDHC_flag = 0;

	if (SD_version == 2)
	{ 
	   do
	   {
		 response = SD_sendCommand(READ_OCR,0);
		 retry++;
		 if(retry>0xfe) 
		 {
		   //TX_NEWLINE;
		   cardType = 0;
		   break;
		 } //time out

	   }while(response != 0x00);

	   if(SDHC_flag == 1) cardType = 2;
	   else cardType = 3;
	}

	//SD_sendCommand(CRC_ON_OFF, OFF); //disable CRC; deafault - CRC disabled in SPI mode
	//SD_sendCommand(SET_BLOCK_LEN, 512); //set block size to 512; default size is 512


	return 0; //successful return
}

//******************************************************************
//Function	: to send a command to SD card
//Arguments	: unsigned char (8-bit command value)
// 			  & unsigned long (32-bit command argument)
//return	: unsigned char; response byte
//******************************************************************
unsigned char SD_sendCommand(unsigned char cmd, unsigned long arg)
{
	//encode_setInterrupt(0);
	
	unsigned char response, retry=0, status;

	//SD card accepts byte address while SDHC accepts block address in multiples of 512
	//so, if it's SD card we need to convert block address into corresponding byte address by 
	//multipying it with 512. which is equivalent to shifting it left 9 times
	//following 'if' loop does that

	if(SDHC_flag == 0)		
	if(cmd == READ_SINGLE_BLOCK     ||
	   cmd == READ_MULTIPLE_BLOCKS  ||
	   cmd == WRITE_SINGLE_BLOCK    ||
	   cmd == WRITE_MULTIPLE_BLOCKS ||
	   cmd == ERASE_BLOCK_START_ADDR|| 
	   cmd == ERASE_BLOCK_END_ADDR ) 
	   {
		 arg = arg << 9;
	   }	   

	SD_CS_ASSERT;

	SPI_transmit(cmd | 0x40); //send command, first two bits always '01'
	SPI_transmit(arg>>24);
	SPI_transmit(arg>>16);
	SPI_transmit(arg>>8);
	SPI_transmit(arg);

	if(cmd == SEND_IF_COND)	 //it is compulsory to send correct CRC for CMD8 (CRC=0x87) & CMD0 (CRC=0x95)
	  SPI_transmit(0x87);    //for remaining commands, CRC is ignored in SPI mode
	else 
	  SPI_transmit(0x95); 

	while((response = SPI_receive()) == 0xff) //wait response
	   if(retry++ > 0xfe) break; //time out error

	if(response == 0x00 && cmd == 58)  //checking response of CMD58
	{
	  status = SPI_receive() & 0x40;     //first byte of the OCR register (bit 31:24)
	  if(status == 0x40) SDHC_flag = 1;  //we need it to verify SDHC card
	  else SDHC_flag = 0;

	  SPI_receive(); //remaining 3 bytes of the OCR register are ignored here
	  SPI_receive(); //one can use these bytes to check power supply limits of SD
	  SPI_receive(); 
	}

	SPI_receive(); //extra 8 CLK
	SD_CS_DEASSERT;

	//encode_setInterrupt(1);

	return response; //return state
}

//*****************************************************************
//Function	: to erase specified no. of blocks of SD card
//Arguments	: none
//return	: unsigned char; will be 0 if no error,
// 			  otherwise the response byte will be sent
//*****************************************************************
unsigned char SD_erase (unsigned long startBlock, unsigned long totalBlocks)
{
	unsigned char response;

	response = SD_sendCommand(ERASE_BLOCK_START_ADDR, startBlock); //send starting block address
	if(response != 0x00) //check for SD status: 0x00 - OK (No flags set)
	  return response;

	response = SD_sendCommand(ERASE_BLOCK_END_ADDR,(startBlock + totalBlocks - 1)); //send end block address
	if(response != 0x00)
	  return response;

	response = SD_sendCommand(ERASE_SELECTED_BLOCKS, 0); //erase all selected blocks
	if(response != 0x00)
	  return response;

	return 0; //normal return
}

//******************************************************************
//Function	: to read a single block from SD card
//Arguments	: none
//return	: unsigned char; will be 0 if no error,
// 			  otherwise the response byte will be sent
//******************************************************************
#if 0
unsigned char SD_readSingleBlock(unsigned long startBlock)
{
	unsigned char response;
	unsigned int i, retry=0;

	 response = SD_sendCommand(READ_SINGLE_BLOCK, startBlock); //read a Block command
 
	 if(response != 0x00) return response; //check for SD status: 0x00 - OK (No flags set)

	SD_CS_ASSERT;

	retry = 0;
	while(SPI_receive() != 0xfe) //wait for start block token 0xfe (0x11111110)
	  if(retry++ > 0xfffe){SD_CS_DEASSERT; return 1;} //return if time-out

	for(i=0; i<512; i++) //read 512 bytes
	  buffer[i] = SPI_receive();

	SPI_receive(); //receive incoming CRC (16-bit), CRC is ignored here
	SPI_receive();

	SPI_receive(); //extra 8 clock pulses
	SD_CS_DEASSERT;

	return 0;
}
#endif
//******************************************************************
unsigned char SD_readSingleBlockCustomBuffer(unsigned long startBlock, uint8_t *target)
{
	//encode_setInterrupt(0);
	
	unsigned char response;
	unsigned int i, retry=0;

	 response = SD_sendCommand(READ_SINGLE_BLOCK, startBlock); //read a Block command
 
	 if(response != 0x00) return response; //check for SD status: 0x00 - OK (No flags set)

	SD_CS_ASSERT;

	retry = 0;
	while(SPI_receive() != 0xfe) //wait for start block token 0xfe (0x11111110)
	  if(retry++ > 0xfffe){SD_CS_DEASSERT; return 1;} //return if time-out

	for(i=0; i<512; i++) //read 512 bytes
	  target[i] = SPI_receive();

	SPI_receive(); //receive incoming CRC (16-bit), CRC is ignored here
	SPI_receive();

	SPI_receive(); //extra 8 clock pulses
	SD_CS_DEASSERT;
	
	//encode_setInterrupt(1);

	return 0;
};
//******************************************************************
//Function	: to write to a single block of SD card
//Arguments	: none
//return	: unsigned char; will be 0 if no error,
// 			  otherwise the response byte will be sent
//******************************************************************
#if 0
unsigned char SD_writeSingleBlock(unsigned long startBlock)
{
	unsigned char response;
	unsigned int i, retry=0;

	 response = SD_sendCommand(WRITE_SINGLE_BLOCK, startBlock); //write a Block command
  
	 if(response != 0x00) return response; //check for SD status: 0x00 - OK (No flags set)

	SD_CS_ASSERT;

	SPI_transmit(0xfe);     //Send start block token 0xfe (0x11111110)

	for(i=0; i<512; i++)    //send 512 bytes data
	  SPI_transmit(buffer[i]);

	SPI_transmit(0xff);     //transmit dummy CRC (16-bit), CRC is ignored here
	SPI_transmit(0xff);

	response = SPI_receive();

	if( (response & 0x1f) != 0x05) //response= 0xXXX0AAA1 ; AAA='010' - data accepted
	{                              //AAA='101'-data rejected due to CRC error
	  SD_CS_DEASSERT;              //AAA='110'-data rejected due to write error
	  return response;
	}

	while(!SPI_receive()) //wait for SD card to complete writing and get idle
	if(retry++ > 0xfffe){SD_CS_DEASSERT; return 1;}

	SD_CS_DEASSERT;
	SPI_transmit(0xff);   //just spend 8 clock cycle delay before reasserting the CS line
	SD_CS_ASSERT;         //re-asserting the CS line to verify if card is still busy

	while(!SPI_receive()) //wait for SD card to complete writing and get idle
	   if(retry++ > 0xfffe){SD_CS_DEASSERT; return 1;}
	SD_CS_DEASSERT;

	return 0;
}
#endif
//**************************************************************************
unsigned char SD_writeSingleBlockCustomBuffer(unsigned long startBlock, uint8_t *source)
{
	//encode_setInterrupt(0);
	unsigned char response;
	unsigned int i, retry=0;

	 response = SD_sendCommand(WRITE_SINGLE_BLOCK, startBlock); //write a Block command
  
	 if(response != 0x00) return response; //check for SD status: 0x00 - OK (No flags set)

	SD_CS_ASSERT;

	SPI_transmit(0xfe);     //Send start block token 0xfe (0x11111110)

	for(i=0; i<512; i++)    //send 512 bytes data
	  SPI_transmit(source[i]);

	SPI_transmit(0xff);     //transmit dummy CRC (16-bit), CRC is ignored here
	SPI_transmit(0xff);

	response = SPI_receive();

	if( (response & 0x1f) != 0x05) //response= 0xXXX0AAA1 ; AAA='010' - data accepted
	{                              //AAA='101'-data rejected due to CRC error
	  SD_CS_DEASSERT;              //AAA='110'-data rejected due to write error
	  return response;
	}

	while(!SPI_receive()) //wait for SD card to complete writing and get idle
	if(retry++ > 0xfffe){SD_CS_DEASSERT; return 1;}

	SD_CS_DEASSERT;
	SPI_transmit(0xff);   //just spend 8 clock cycle delay before reasserting the CS line
	SD_CS_ASSERT;         //re-asserting the CS line to verify if card is still busy

	while(!SPI_receive()) //wait for SD card to complete writing and get idle
	   if(retry++ > 0xfffe){SD_CS_DEASSERT; return 1;}
	SD_CS_DEASSERT;

	//encode_setInterrupt(1);
	return 0;
}
#ifndef FAT_TESTING_ONLY

//***************************************************************************
//Function	: to read multiple blocks from SD card & send every block to UART
//Arguments	: none
//return	: unsigned char; will be 0 if no error,
// 			  otherwise the response byte will be sent
//****************************************************************************
#if 0
unsigned char SD_readMultipleBlock (unsigned long startBlock, unsigned long totalBlocks)
{
	unsigned char response;
	unsigned int i, retry=0;

	retry = 0;

	response = SD_sendCommand(READ_MULTIPLE_BLOCKS, startBlock); //write a Block command
  
	if(response != 0x00) return response; //check for SD status: 0x00 - OK (No flags set)

	SD_CS_ASSERT;

	while( totalBlocks )
	{
	  retry = 0;
	  while(SPI_receive() != 0xfe) //wait for start block token 0xfe (0x11111110)
	  if(retry++ > 0xfffe){SD_CS_DEASSERT; return 1;} //return if time-out

	  for(i=0; i<512; i++) //read 512 bytes
		buffer[i] = SPI_receive();

	  SPI_receive(); //receive incoming CRC (16-bit), CRC is ignored here
	  SPI_receive();

	  SPI_receive(); //extra 8 cycles
	  TX_NEWLINE;
	  transmitString_F(PSTR(" --------- "));
	  TX_NEWLINE;

	  for(i=0; i<512; i++) //send the block to UART
	  {
		if(buffer[i] == '~') break;
		transmitByte ( buffer[i] );
	  }

	  TX_NEWLINE;
	  transmitString_F(PSTR(" --------- "));
	  TX_NEWLINE;
	  totalBlocks--;
	}

	SD_sendCommand(STOP_TRANSMISSION, 0); //command to stop transmission
	SD_CS_DEASSERT;
	SPI_receive(); //extra 8 clock pulses

	return 0;
}

//***************************************************************************
//Function: to receive data from UART and write to multiple blocks of SD card
//Arguments: none
//return: unsigned char; will be 0 if no error,
// otherwise the response byte will be sent
//****************************************************************************
unsigned char SD_writeMultipleBlock(unsigned long startBlock, unsigned long totalBlocks)
{
	unsigned char response, data;
	unsigned int i, retry=0;
	unsigned long blockCounter=0, size;

	response = SD_sendCommand(WRITE_MULTIPLE_BLOCKS, startBlock); //write a Block command

	if(response != 0x00) return response; //check for SD status: 0x00 - OK (No flags set)

	SD_CS_ASSERT;

	TX_NEWLINE;
	transmitString_F(PSTR(" Enter text (End with ~): "));
	TX_NEWLINE;

	while( blockCounter < totalBlocks )
	{
	   i=0;
	   do
	   {
		 data = receiveByte();
		 if(data == 0x08)	//'Back Space' key pressed
		 { 
		   if(i != 0)
		   { 
			 transmitByte(data);
			 transmitByte(' '); 
			 transmitByte(data); 
			 i--; 
			 size--;
		   } 
		   continue;     
		 }
		 transmitByte(data);
		 buffer[i++] = data;
		 if(data == 0x0d)
		 {
			transmitByte(0x0a);
			buffer[i++] = 0x0a;
		 }
		 if(i == 512) break;
	   }while (data != '~');

	   TX_NEWLINE;
	   transmitString_F(PSTR(" ---- "));
	   TX_NEWLINE;

	   SPI_transmit(0xfc); //Send start block token 0xfc (0x11111100)

	   for(i=0; i<512; i++) //send 512 bytes data
		 SPI_transmit( buffer[i] );

	   SPI_transmit(0xff); //transmit dummy CRC (16-bit), CRC is ignored here
	   SPI_transmit(0xff);

	   response = SPI_receive();
	   if( (response & 0x1f) != 0x05) //response= 0xXXX0AAA1 ; AAA='010' - data accepted
	   {                              //AAA='101'-data rejected due to CRC error
		  SD_CS_DEASSERT;             //AAA='110'-data rejected due to write error
		  return response;
	   }

	   while(!SPI_receive()) //wait for SD card to complete writing and get idle
		 if(retry++ > 0xfffe){SD_CS_DEASSERT; return 1;}

	   SPI_receive(); //extra 8 bits
	   blockCounter++;
	}

	SPI_transmit(0xfd); //send 'stop transmission token'

	retry = 0;

	while(!SPI_receive()) //wait for SD card to complete writing and get idle
	   if(retry++ > 0xfffe){SD_CS_DEASSERT; return 1;}

	SD_CS_DEASSERT;
	SPI_transmit(0xff); //just spend 8 clock cycle delay before reasserting the CS signal
	SD_CS_ASSERT; //re assertion of the CS signal is required to verify if card is still busy

	while(!SPI_receive()) //wait for SD card to complete writing and get idle
	   if(retry++ > 0xfffe){SD_CS_DEASSERT; return 1;}
	SD_CS_DEASSERT;

	return 0;
}
//*********************************************
#endif
#endif
#endif
//******** END ****** www.dharmanitech.com *****
