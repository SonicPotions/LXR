//*******************************************************
// **** ROUTINES FOR FAT32 IMPLEMENTATION OF SD CARD ****
//**********************************************************
//Controller: ATmega32 (Clock: 16 Mhz-external)
//Compiler	: AVR-GCC (winAVR with AVRStudio)
//Project V.: Version - 2.4.1
//Author	: CC Dharmani, Chennai (India)
//			  www.dharmanitech.com
//Date		: 24 Apr 2011
//
//
// Modified and slimmed down for bootloader use
// by Julian Schmidt 14.7.2012
//********************************************************

//Link to the Post: http://www.dharmanitech.com/2009/01/sd-card-interfacing-with-atmega8-fat32.html

#include <avr/io.h>
#include <avr/pgmspace.h>
#include "FAT32.h"
#include "UART_routines.h"
#include "SD_routines.h"
#include "FileParser.h"

//***************************************************************************
//Function: to read data from boot sector of SD card, to determine important
//parameters like bytesPerSector, sectorsPerCluster etc.
//Arguments: none
//return: none
//***************************************************************************
unsigned char getBootSectorData (void)
{
struct BS_Structure *bpb; //mapping the sd_buffer onto the structure
struct MBRinfo_Structure *mbr;
struct partitionInfo_Structure *partition;
unsigned long dataSectors;

unusedSectors = 0;

SD_readSingleBlock(0);
bpb = (struct BS_Structure *)sd_buffer;

if(bpb->jumpBoot[0]!=0xE9 && bpb->jumpBoot[0]!=0xEB)   //check if it is boot sector
{
  mbr = (struct MBRinfo_Structure *) sd_buffer;       //if it is not boot sector, it must be MBR
  
  if(mbr->signature != 0xaa55) return 1;       //if it is not even MBR then it's not FAT32
  	
  partition = (struct partitionInfo_Structure *)(mbr->partitionData);//first partition
  unusedSectors = partition->firstSector; //the unused sectors, hidden to the FAT
  
  SD_readSingleBlock(partition->firstSector);//read the bpb sector
  bpb = (struct BS_Structure *)sd_buffer;
  if(bpb->jumpBoot[0]!=0xE9 && bpb->jumpBoot[0]!=0xEB) return 1; 
}

bytesPerSector = bpb->bytesPerSector;
sectorPerCluster = bpb->sectorPerCluster;
reservedSectorCount = bpb->reservedSectorCount;
rootCluster = bpb->rootCluster;
firstDataSector = bpb->hiddenSectors + reservedSectorCount + (bpb->numberofFATs * bpb->FATsize_F32);

dataSectors = bpb->totalSectors_F32
              - bpb->reservedSectorCount
              - ( bpb->numberofFATs * bpb->FATsize_F32);
totalClusters = dataSectors / sectorPerCluster;

return 0;
}

//***************************************************************************
//Function: to calculate first sector address of any given cluster
//Arguments: cluster number for which first sector is to be found
//return: first sector address
//***************************************************************************
unsigned long getFirstSector(unsigned long clusterNumber)
{
  return (((clusterNumber - 2) * sectorPerCluster) + firstDataSector);
}

//***************************************************************************
//Function: get cluster entry value from FAT to find out the next cluster in the chain
//Arguments: 1. current cluster number, 
//return: next cluster number
//****************************************************************************
unsigned long getNextCluster (unsigned long clusterNumber)
//unsigned long getSetNextCluster (unsigned long clusterNumber,
                                 //unsigned char get_set,
                                 //unsigned long clusterEntry)
{
	unsigned int FATEntryOffset;
	unsigned long *FATEntryValue;
	unsigned long FATEntrySector;
	unsigned char retry = 0;

	//get sector number of the cluster entry in the FAT
	FATEntrySector = unusedSectors + reservedSectorCount + ((clusterNumber * 4) / bytesPerSector) ;

	//get the offset address in that sector number
	FATEntryOffset = (unsigned int) ((clusterNumber * 4) % bytesPerSector);

	//read the sector into a sd_buffer
	while(retry <10)
	{
		 if(!SD_readSingleBlock(FATEntrySector))
			 break; 
		 
		retry++;
	}
	//get the cluster address from the sd_buffer
	FATEntryValue = (unsigned long *) &sd_buffer[FATEntryOffset];


	  return ((*FATEntryValue) & 0x0fffffff);

}



//***************************************************************************
//Function: to get DIR/FILE list or a single file address (cluster number) or to delete a specified file
//Arguments: #1 - flag: GET_LIST, GET_FILE or DELETE #2 - pointer to file name (0 if arg#1 is GET_LIST)
//return: first cluster of the file, if flag = GET_FILE
//        print file/dir list of the root directory, if flag = GET_LIST
//		  Delete the file mentioned in arg#2, if flag = DELETE
//****************************************************************************
struct dir_Structure* findFiles (unsigned char *fileName)
{
	unsigned long cluster, sector, firstSector;//, firstCluster, nextCluster;
	struct dir_Structure *dir;
	unsigned int i;
	unsigned char j;

	cluster = rootCluster; //root cluster

	while(1)
	{
	   firstSector = getFirstSector (cluster);

	   for(sector = 0; sector < sectorPerCluster; sector++)
	   {
		 SD_readSingleBlock (firstSector + sector);

		 for(i=0; i<bytesPerSector; i+=32)
		 {
			dir = (struct dir_Structure *) &sd_buffer[i];

			if(dir->name[0] == EMPTY) //indicates end of the file list of the directory
			{
			  return 0;   
			}
			if((dir->name[0] != DELETED) && (dir->attrib != ATTR_LONG_NAME))
			{
			  {
				for(j=0; j<11; j++)
				if(dir->name[j] != fileName[j]) break;
				if(j == 11)
				{
					appendFileSector = firstSector + sector;
					appendFileLocation = i;
					appendStartCluster = (((unsigned long) dir->firstClusterHI) << 16) | dir->firstClusterLO;
					fileSize = dir->fileSize;
					return (dir);
				}
			  }
	  	     }
		 }
	   }
		
		cluster = (getNextCluster (cluster));

		if(cluster > 0x0ffffff6)
   			return 0;
		if(cluster == 0) 
		{
			return 0;
		}
	
	 }
	return 0;
}

//***************************************************************************
//Function: if flag=READ then to read file from SD card and send contents to UART 
//if flag=VERIFY then functions will verify whether a specified file is already existing
//Arguments: flag (READ or VERIFY) and pointer to the file name
//return: 0, if normal operation or flag is READ
//	      1, if file is already existing and flag = VERIFY
//		  2, if file name is incompatible
//***************************************************************************

void readFile ( unsigned char *fileName)
{
struct dir_Structure *dir;
unsigned long cluster, fileSize, firstSector;
//unsigned int k;
//unsigned int byteCounter = 0;
unsigned char j;//, error;

//Filename MUST BE 8.3 all caps e.g. FILENAME.TXT not filename.txt
//the first 8 vyte are the name, the next 3 byte are the extension
//to save space from the name conversion tool this has to be done manually by hand
//if a filename shorter than 8 byte is used. the 8 bytes need to be zero padded
// same for the extension
/*
error = convertFileName (fileName); //convert fileName into FAT format
if(error) return 2;
*/

dir = findFiles (fileName); //get the file location
if(dir == 0) 
{
	lcd_clear();
	lcd_string("FW error");
	while(1);
	return;
}	
 

cluster = (((unsigned long) dir->firstClusterHI) << 16) | dir->firstClusterLO;

fileSize = dir->fileSize;


while(1)
{
  firstSector = getFirstSector (cluster);

  for(j=0; j<sectorPerCluster; j++)
  {
    SD_readSingleBlock(firstSector + j);
	

	if(!fileParser_parseNextBlock(fileSize))
	{
		return;
	};
	
  }

  cluster = getNextCluster (cluster);
  if(cluster == 0) 
  {
	return;
  }
}
return;
}

//******** END ****** www.dharmanitech.com *****
