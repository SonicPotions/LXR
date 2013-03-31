//*******************************************************
// **** ROUTINES FOR FAT32 IMPLEMATATION OF SD CARD ****
//**********************************************************
//Controller: ATmega32 (Clock: 8 Mhz-internal)
//Compiler	: AVR-GCC (winAVR with AVRStudio)
//Project V.: Version - 2.4.1
//Author	: CC Dharmani, Chennai (India)
//			  www.dharmanitech.com
//Date		: 24 Apr 2011
//
//modified	: Julian Schmidt
//Date		: 8.5.2012
//********************************************************

//Link to the Post: http://www.dharmanitech.com/2009/01/sd-card-interfacing-with-atmega8-fat32.html

#include <avr/io.h>
#include <avr/pgmspace.h>
#include "FAT32.h"
//#include "UART_routines.h"
#include "SD_routines.h"
//#include "RTC_routines.h"
#include "../lcd.h"

//***************************************************************************
//Function: to read data from boot sector of SD card, to determine important
//parameters like bytesPerSector, sectorsPerCluster etc.
//Arguments: none
//return: none
//***************************************************************************
unsigned char fat_getBootSectorData (void)
{
	struct BS_Structure *bpb; //mapping the buffer onto the structure
	struct MBRinfo_Structure *mbr;
	struct partitionInfo_Structure *partition;
	unsigned long dataSectors;

	unusedSectors = 0;

	SD_readSingleBlock(0);
	bpb = (struct BS_Structure *)buffer;

	if(bpb->jumpBoot[0]!=0xE9 && bpb->jumpBoot[0]!=0xEB)   //check if it is boot sector
	{
	  mbr = (struct MBRinfo_Structure *) buffer;       //if it is not boot sector, it must be MBR
  
	  if(mbr->signature != 0xaa55) return 1;       //if it is not even MBR then it's not FAT32
  	
	  partition = (struct partitionInfo_Structure *)(mbr->partitionData);//first partition
	  unusedSectors = partition->firstSector; //the unused sectors, hidden to the FAT
  
	  SD_readSingleBlock(partition->firstSector);//read the bpb sector
	  bpb = (struct BS_Structure *)buffer;
	  if(bpb->jumpBoot[0]!=0xE9 && bpb->jumpBoot[0]!=0xEB) return 1; 
	}

	bytesPerSector = bpb->bytesPerSector;
	//transmitHex(INT, bytesPerSector); transmitByte(' ');
	sectorPerCluster = bpb->sectorPerCluster;
	//transmitHex(INT, sectorPerCluster); transmitByte(' ');
	reservedSectorCount = bpb->reservedSectorCount;
	rootCluster = bpb->rootCluster;// + (sector / sectorPerCluster) +1;
	firstDataSector = bpb->hiddenSectors + reservedSectorCount + (bpb->numberofFATs * bpb->FATsize_F32);

	dataSectors = bpb->totalSectors_F32
				  - bpb->reservedSectorCount
				  - ( bpb->numberofFATs * bpb->FATsize_F32);
	totalClusters = dataSectors / sectorPerCluster;
	//transmitHex(LONG, totalClusters); transmitByte(' ');

	if((fat_getSetFreeCluster (TOTAL_FREE, GET, 0)) > totalClusters)  //check if FSinfo free clusters count is valid
		 freeClusterCountUpdated = 0;
	else
		 freeClusterCountUpdated = 1;
	return 0;
}

//***************************************************************************
//Function: to calculate first sector address of any given cluster
//Arguments: cluster number for which first sector is to be found
//return: first sector address
//***************************************************************************
unsigned long fat_getFirstSector(unsigned long clusterNumber)
{
  return (((clusterNumber - 2) * sectorPerCluster) + firstDataSector);
}

//***************************************************************************
//Function: get cluster entry value from FAT to find out the next cluster in the chain
//or set new cluster entry in FAT
//Arguments: 1. current cluster number, 2. get_set (=GET, if next cluster is to be found or = SET,
//if next cluster is to be set 3. next cluster number, if argument#2 = SET, else 0
//return: next cluster number, if if argument#2 = GET, else 0
//****************************************************************************
unsigned long fat_getSetNextCluster (unsigned long clusterNumber,
                                 unsigned char get_set,
                                 unsigned long clusterEntry)
{
	unsigned int FATEntryOffset;
	unsigned long *FATEntryValue;
	unsigned long FATEntrySector;
	unsigned char retry = 0;

	//get sector number of the cluster entry in the FAT
	FATEntrySector = unusedSectors + reservedSectorCount + ((clusterNumber * 4) / bytesPerSector) ;

	//get the offset address in that sector number
	FATEntryOffset = (unsigned int) ((clusterNumber * 4) % bytesPerSector);

	//read the sector into a buffer
	while(retry <10)
	{ if(!SD_readSingleBlock(FATEntrySector)) break; retry++;}

	//get the cluster address from the buffer
	FATEntryValue = (unsigned long *) &buffer[FATEntryOffset];

	if(get_set == GET)
	  return ((*FATEntryValue) & 0x0fffffff);


	*FATEntryValue = clusterEntry;   //for setting new value in cluster entry in FAT

	SD_writeSingleBlock(FATEntrySector);

	return (0);
}

//********************************************************************************************
//Function: to get or set next free cluster or total free clusters in FSinfo sector of SD card
//Arguments: 1.flag:TOTAL_FREE or NEXT_FREE, 
//			 2.flag: GET or SET 
//			 3.new FS entry, when argument2 is SET; or 0, when argument2 is GET
//return: next free cluster, if arg1 is NEXT_FREE & arg2 is GET
//        total number of free clusters, if arg1 is TOTAL_FREE & arg2 is GET
//		  0xffffffff, if any error or if arg2 is SET
//********************************************************************************************
unsigned long fat_getSetFreeCluster(unsigned char totOrNext, unsigned char get_set, unsigned long FSEntry)
{
	struct FSInfo_Structure *FS = (struct FSInfo_Structure *) &buffer;
	unsigned char error;

	SD_readSingleBlock(unusedSectors + 1);

	if((FS->leadSignature != 0x41615252) || (FS->structureSignature != 0x61417272) || (FS->trailSignature !=0xaa550000))
	  return 0xffffffff;

	 if(get_set == GET)
	 {
	   if(totOrNext == TOTAL_FREE)
		  return(FS->freeClusterCount);
	   else // when totOrNext = NEXT_FREE
		  return(FS->nextFreeCluster);
	 }
	 else
	 {
	   if(totOrNext == TOTAL_FREE)
		  FS->freeClusterCount = FSEntry;
	   else // when totOrNext = NEXT_FREE
		  FS->nextFreeCluster = FSEntry;
 
	   error = SD_writeSingleBlock(unusedSectors + 1);	//update FSinfo
	 }
	 return 0xffffffff;
}

//***************************************************************************
//Function: to get DIR/FILE list or a single file address (cluster number) or to delete a specified file
//Arguments: #1 - flag: GET_FILE or DELETE #2 - pointer to file name 
//return: first cluster of the file, if flag = GET_FILE
//		  Delete the file mentioned in arg#2, if flag = DELETE
//****************************************************************************
struct dir_Structure* fat_findFiles (unsigned char flag, unsigned char *fileName)
{
	unsigned long cluster, sector, firstSector, firstCluster, nextCluster;
	struct dir_Structure *dir;
	unsigned int i;
	unsigned char j;

	cluster = rootCluster; //root cluster

	while(1)
	{
	   firstSector = fat_getFirstSector (cluster);

	   for(sector = 0; sector < sectorPerCluster; sector++)
	   {
		 SD_readSingleBlock (firstSector + sector);
	

		 for(i=0; i<bytesPerSector; i+=32)
		 {
			dir = (struct dir_Structure *) &buffer[i];

			if(dir->name[0] == EMPTY) //indicates end of the file list of the directory
			{
			  if((flag == GET_FILE) || (flag == DELETE))
			  {
				 // transmitString_F(PSTR("File does not exist!"));
			  }			  
			  return 0;   
			}
			if((dir->name[0] != DELETED) && (dir->attrib != ATTR_LONG_NAME))
			{
			  if((flag == GET_FILE) || (flag == DELETE))
			  {
				for(j=0; j<11; j++)
				if(dir->name[j] != fileName[j]) break;
				if(j == 11)
				{
				  if(flag == GET_FILE)
				  {
					appendFileSector = firstSector + sector;
					appendFileLocation = i;
					appendStartCluster = (((unsigned long) dir->firstClusterHI) << 16) | dir->firstClusterLO;
					fileSize = dir->fileSize;
					return (dir);
				  }	
				  else    //when flag = DELETE
				  {
					  /*
					 TX_NEWLINE;
					 transmitString_F(PSTR("Deleting.."));
					 TX_NEWLINE;
					 TX_NEWLINE;
					 */
					 firstCluster = (((unsigned long) dir->firstClusterHI) << 16) | dir->firstClusterLO;
                
					 //mark file as 'deleted' in FAT table
					 dir->name[0] = DELETED;    
					 SD_writeSingleBlock (firstSector+sector);
				 			 
					 fat_freeMemoryUpdate (ADD, dir->fileSize);

					 //update next free cluster entry in FSinfo sector
					 cluster = fat_getSetFreeCluster (NEXT_FREE, GET, 0); 
					 if(firstCluster < cluster)
						 fat_getSetFreeCluster (NEXT_FREE, SET, firstCluster);

					 //mark all the clusters allocated to the file as 'free'
					 while(1)  
					 {
						nextCluster = fat_getSetNextCluster (firstCluster, GET, 0);
						fat_getSetNextCluster (firstCluster, SET, 0);
						if(nextCluster > 0x0ffffff6) 
						{
							//transmitString_F(PSTR("File deleted!"));
							return 0;
						}
						firstCluster = nextCluster;
			  		 } 
				  }
				}
			  }
			  else  //when flag = GET_LIST
			  {
				  /*
				 TX_NEWLINE;
				 for(j=0; j<11; j++)
				 {
				   if(j == 8) transmitByte(' ');
				   transmitByte (dir->name[j]);
				 }
				 transmitString_F (PSTR("   "));
				 if((dir->attrib != 0x10) && (dir->attrib != 0x08))
				 {
					 transmitString_F (PSTR("FILE" ));
					 transmitString_F (PSTR("   "));
					 displayMemory (LOW, dir->fileSize);
				 }
				 else
				   transmitString_F ((dir->attrib == 0x10)? PSTR("DIR") : PSTR("ROOT"));
				   */
			  }
		   }
		 }
	   }

	   cluster = (fat_getSetNextCluster (cluster, GET, 0));

	   if(cluster > 0x0ffffff6)
   		 return 0;
	   if(cluster == 0) 
	   {
		   lcd_home();
		   lcd_string(("Error in getting cluster"));
			//transmitString_F(PSTR("Error in getting cluster"));  
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
unsigned char fat_readFile (unsigned char flag, unsigned char *fileName)
{
	struct dir_Structure *dir;
	unsigned long cluster, byteCounter = 0, fileSize, firstSector;
	unsigned int k;
	unsigned char j, error;

	error = fat_convertFileName (fileName); //convert fileName into FAT format
	if(error) return 2;

	dir = fat_findFiles (GET_FILE, fileName); //get the file location
	if(dir == 0) 
	  return (0);

	if(flag == VERIFY) return (1);	//specified file name is already existing

	cluster = (((unsigned long) dir->firstClusterHI) << 16) | dir->firstClusterLO;

	fileSize = dir->fileSize;

	/*
	TX_NEWLINE;
	TX_NEWLINE;
	*/

	while(1)
	{
	  firstSector = fat_getFirstSector (cluster);

	  for(j=0; j<sectorPerCluster; j++)
	  {
		SD_readSingleBlock(firstSector + j);
    
		for(k=0; k<512; k++)
		{
		  //transmitByte(buffer[k]);
		  fat_processData(buffer[k]);
		  if ((byteCounter++) >= fileSize ) return 0;
		}
	  }
	  cluster = fat_getSetNextCluster (cluster, GET, 0);
	  if(cluster == 0) 
	  {
		   lcd_home();
		   lcd_string(("Error in getting cluster"));
			//transmitString_F(PSTR("Error in getting cluster")); 
		return 0;
	  }
	}
	return 0;
}

//***************************************************************************
//Function: to convert normal short file name into FAT format
//Arguments: pointer to the file name
//return: 0, if successful else 1.
//***************************************************************************
unsigned char fat_convertFileName (unsigned char *fileName)
{
	unsigned char fileNameFAT[11];
	unsigned char j, k;

	for(j=0; j<12; j++)
	if(fileName[j] == '.') break;

	if(j>8) 
	{
		//transmitString_F(PSTR("Invalid fileName..")); 
		lcd_home();
		lcd_string(("Invalid fileName.."));
		
		return 1;
	}

	for(k=0; k<j; k++) //setting file name
	  fileNameFAT[k] = fileName[k];

	for(k=j; k<=7; k++) //filling file name trail with blanks
	  fileNameFAT[k] = ' ';

	j++;
	for(k=8; k<11; k++) //setting file extention
	{
	  if(fileName[j] != 0)
		fileNameFAT[k] = fileName[j++];
	  else //filling extension trail with blanks
		while(k<11)
		  fileNameFAT[k++] = ' ';
	}

	for(j=0; j<11; j++) //converting small letters to caps
	  if((fileNameFAT[j] >= 0x61) && (fileNameFAT[j] <= 0x7a))
		fileNameFAT[j] -= 0x20;

	for(j=0; j<11; j++)
	  fileName[j] = fileNameFAT[j];

	return 0;
}

//************************************************************************************
//Function: to create a file in FAT32 format in the root directory if given 
//			file name does not exist; if the file already exists then append the data
//Arguments: pointer to the file name
//return: none
//************************************************************************************
void fat_writeFile (unsigned char *fileName, unsigned char* data, uint16_t size)
{
	//unsigned char j, data, error, fileCreatedFlag = 0, start = 0, appendFile = 0, sectorEndFlag = 0, sector=0;
	unsigned char j,  error, fileCreatedFlag = 0, start = 0, appendFile = 0, sectorEndFlag = 0, sector=0;
	unsigned int i, firstClusterHigh=0, firstClusterLow=0;  //value 0 is assigned just to avoid warning in compilation
	struct dir_Structure *dir;
	unsigned long cluster, nextCluster, prevCluster, firstSector, clusterCount, extraMemory;

	j = fat_readFile (VERIFY, fileName);

	if(j == 1) 
	{
	  //transmitString_F(PSTR(" File already exists, appending data..")); 
	  	  lcd_home();
		  lcd_string((" append data.."));
		  
	  appendFile = 1;
	  cluster = appendStartCluster;
	  clusterCount=0;
	  while(1)
	  {
		nextCluster = fat_getSetNextCluster (cluster, GET, 0);
		if(nextCluster == FAT_EOF) break;
		cluster = nextCluster;
		clusterCount++;
	  }

	  sector = (fileSize - (clusterCount * sectorPerCluster * bytesPerSector)) / bytesPerSector; //last sector number of the last cluster of the file
	  start = 1;
	//  appendFile();
	//  return;
	}
	else if(j == 2) 
	{
	   return; //invalid file name
	}
	else
	{
	 // TX_NEWLINE;
	 // transmitString_F(PSTR(" Creating File.."));
	  lcd_home();
		  lcd_string((" Creating File.."));

	  cluster = fat_getSetFreeCluster (NEXT_FREE, GET, 0);
	  if(cluster > totalClusters)
		 cluster = rootCluster;

	  cluster = fat_searchNextFreeCluster(cluster);
	   if(cluster == 0)
	   {
		 // TX_NEWLINE;
		 // transmitString_F(PSTR(" No free cluster!"));
		  lcd_home();
		  lcd_string((" No free cluster!"));
		 
		  return;
	   }
	  fat_getSetNextCluster(cluster, SET, FAT_EOF);   //last cluster of the file, marked EOF
   
	  firstClusterHigh = (unsigned int) ((cluster & 0xffff0000) >> 16 );
	  firstClusterLow = (unsigned int) ( cluster & 0x0000ffff);
	  fileSize = 0;
	}



//	while(1)
	{
	   if(start)
	   {
		  start = 0;
		  startBlock = fat_getFirstSector (cluster) + sector;
		  SD_readSingleBlock (startBlock);
		  i = fileSize % bytesPerSector;
		  j = sector;
	   }
	   else
	   {
		  startBlock = fat_getFirstSector (cluster);
		  i=0;
		  j=0;
	   }
   

	  // TX_NEWLINE;
	  // transmitString_F(PSTR(" Enter text (end with ~):"));
   
	   uint16_t dataCnt = 0;
	   do
	   {
		   /*
		 if(sectorEndFlag == 1) //special case when the last character in previous sector was '\r'
		 {
	 		transmitByte ('\n');
			buffer[i++] = '\n'; //appending 'Line Feed (LF)' character
			fileSize++;
		 }
		 */

		sectorEndFlag = 0;

		 //data = receiveByte();
		 /*
		 if(data == 0x08)	//'Back Space' key pressed
		 { 
		   if(i != 0)
		   { 
			 transmitByte(data);
			 transmitByte(' '); 
			 transmitByte(data); 
			 i--; 
			 fileSize--;
		   } 
		   continue;     
		 }
		 */
		 //transmitByte(data);
		 buffer[i++] = data[dataCnt++];
		 fileSize++;
		 /*
		 if(data == '\r')  //'Carriege Return (CR)' character
		 {
			if(i == 512)
			   sectorEndFlag = 1;  //flag to indicate that the appended '\n' char should be put in the next sector
			else
			{ 
			   transmitByte ('\n');
			   buffer[i++] = '\n'; //appending 'Line Feed (LF)' character
			   fileSize++;
			}
		 }
		 */
	 
		 if(i >= 512)   //though 'i' will never become greater than 512, it's kept here to avoid 
		 {				//infinite loop in case it happens to be greater than 512 due to some data corruption
		   i=0;
		   error = SD_writeSingleBlock (startBlock);
		   j++;
		   if(j == sectorPerCluster) 
		   {
				j = 0; 
				break;
		   }
		   startBlock++; 
		 }
		}while (dataCnt < size);

/*
	   if(data == '~') 
	   {
		  fileSize--;	//to remove the last entered '~' character
		  i--;
		  */
		  for(;i<512;i++)  //fill the rest of the buffer with 0x00
			buffer[i]= 0x00;
   		  error = SD_writeSingleBlock (startBlock);

		  //break;
	//   } 
	  
	   prevCluster = cluster;

	   cluster = fat_searchNextFreeCluster(prevCluster); //look for a free cluster starting from the current cluster

	   if(cluster == 0)
	   {
		//  TX_NEWLINE;
		//  transmitString_F(PSTR(" No free cluster!"));
		  return;
	   }

	   fat_getSetNextCluster(prevCluster, SET, cluster);
	   fat_getSetNextCluster(cluster, SET, FAT_EOF);   //last cluster of the file, marked EOF
	}        

	fat_getSetFreeCluster (NEXT_FREE, SET, cluster); //update FSinfo next free cluster entry

/*
	error = getDateTime_FAT();    //get current date & time from the RTC
	if(error) 
	{ 
		
		dateFAT = 0; 
		timeFAT = 0;
		
	}
	*/

	if(appendFile)  //executes this loop if file is to be appended
	{
	  SD_readSingleBlock (appendFileSector);    
	  dir = (struct dir_Structure *) &buffer[appendFileLocation]; 

	  dir->lastAccessDate = 0;   //date of last access ignored
	  /*
	  dir->writeTime = timeFAT;  //setting new time of last write, obtained from RTC
	  dir->writeDate = dateFAT;  //setting new date of last write, obtained from RTC
	  */
	  dir->writeTime = 0;
	  dir->writeDate = 0; //no rtc available
	  
	  extraMemory = fileSize - dir->fileSize;
	  dir->fileSize = fileSize;
	  SD_writeSingleBlock (appendFileSector);
	  fat_freeMemoryUpdate (REMOVE, extraMemory); //updating free memory count in FSinfo sector;

  /*
	  TX_NEWLINE;
	  transmitString_F(PSTR(" File appended!"));
	  TX_NEWLINE;
*/
	  return;
	}

	//executes following portion when new file is created

	prevCluster = rootCluster; //root cluster

	while(1)
	{
	   firstSector = fat_getFirstSector (prevCluster);

	   for(sector = 0; sector < sectorPerCluster; sector++)
	   {
		 SD_readSingleBlock (firstSector + sector);
	

		 for(i=0; i<bytesPerSector; i+=32)
		 {
			dir = (struct dir_Structure *) &buffer[i];

			if(fileCreatedFlag)   //to mark last directory entry with 0x00 (empty) mark
			 { 					  //indicating end of the directory file list
			   //dir->name[0] = EMPTY;
			   //SD_writeSingleBlock (firstSector + sector);
			   return;
			 }

			if((dir->name[0] == EMPTY) || (dir->name[0] == DELETED))  //looking for an empty slot to enter file info
			{
			  for(j=0; j<11; j++)
  				dir->name[j] = fileName[j];
			  dir->attrib = ATTR_ARCHIVE;	//settting file attribute as 'archive'
			  dir->NTreserved = 0;			//always set to 0
			  dir->timeTenth = 0;			//always set to 0
			  /*
			  dir->createTime = timeFAT; 	//setting time of file creation, obtained from RTC
			  dir->createDate = dateFAT; 	//setting date of file creation, obtained from RTC
			  */
			  dir->createTime = 0; 	
			  dir->createDate = 0; 	
			  
			  dir->lastAccessDate = 0;   	//date of last access ignored
			  /*
			  dir->writeTime = timeFAT;  	//setting new time of last write, obtained from RTC
			  dir->writeDate = dateFAT;  	//setting new date of last write, obtained from RTC
			  */
			  dir->writeTime = 0;  
			  dir->writeDate = 0;  
			  
			  dir->firstClusterHI = firstClusterHigh;
			  dir->firstClusterLO = firstClusterLow;
			  dir->fileSize = fileSize;

			  SD_writeSingleBlock (firstSector + sector);
			  fileCreatedFlag = 1;

/*
			  TX_NEWLINE;
			  TX_NEWLINE;
			  transmitString_F(PSTR(" File Created! "));
			  */

			  fat_freeMemoryUpdate (REMOVE, fileSize); //updating free memory count in FSinfo sector
	     
			}
		 }
	   }

	   cluster = fat_getSetNextCluster (prevCluster, GET, 0);

	   if(cluster > 0x0ffffff6)
	   {
		  if(cluster == FAT_EOF)   //this situation will come when total files in root is multiple of (32*sectorPerCluster)
		  {  
			cluster = fat_searchNextFreeCluster(prevCluster); //find next cluster for root directory entries
			fat_getSetNextCluster(prevCluster, SET, cluster); //link the new cluster of root to the previous cluster
			fat_getSetNextCluster(cluster, SET, FAT_EOF);  //set the new cluster as end of the root directory
		  } 

		  else
		  {	
			//transmitString_F(PSTR("End of Cluster Chain")); 
			return;
		  }
	   }
	   if(cluster == 0) 
	   {
			//transmitString_F(PSTR("Error in getting cluster")); 
			return;
	   }
   
	   prevCluster = cluster;
	 }
 
	 return;
}


//***************************************************************************
//Function: to search for the next free cluster in the root directory
//          starting from a specified cluster
//Arguments: Starting cluster
//return: the next free cluster
//****************************************************************
unsigned long fat_searchNextFreeCluster (unsigned long startCluster)
{
  unsigned long cluster, *value, sector;
  unsigned char i;
    
	startCluster -=  (startCluster % 128);   //to start with the first file in a FAT sector
    for(cluster =startCluster; cluster <totalClusters; cluster+=128) 
    {
      sector = unusedSectors + reservedSectorCount + ((cluster * 4) / bytesPerSector);
      SD_readSingleBlock(sector);
      for(i=0; i<128; i++)
      {
       	 value = (unsigned long *) &buffer[i*4];
         if(((*value) & 0x0fffffff) == 0)
            return(cluster+i);
      }  
    } 

 return 0;
}

//***************************************************************************
//Function: to display total memory and free memory of SD card, using UART
//Arguments: none
//return: none
//Note: this routine can take upto 15sec for 1GB card (@1MHz clock)
//it tries to read from SD whether a free cluster count is stored, if it is stored
//then it will return immediately. Otherwise it will count the total number of
//free clusters, which takes time
//****************************************************************************
void fat_memoryStatistics (void)
{
	unsigned long freeClusters, totalClusterCount, cluster;
	unsigned long totalMemory, freeMemory;
	unsigned long sector, *value;
	unsigned int i;


	totalMemory = totalClusters * sectorPerCluster / 1024;
	totalMemory *= bytesPerSector;

	/*
	TX_NEWLINE;
	TX_NEWLINE;
	transmitString_F(PSTR("Total Memory: "));
	*/
	lcd_home();
	lcd_string(("Total Memory: "));

	fat_displayMemory (HIGH, totalMemory);

	freeClusters = fat_getSetFreeCluster (TOTAL_FREE, GET, 0);
	//freeClusters = 0xffffffff;    

	if(freeClusters > totalClusters)
	{
	   freeClusterCountUpdated = 0;
	   freeClusters = 0;
	   totalClusterCount = 0;
	   cluster = rootCluster;    
		while(1)
		{
		  sector = unusedSectors + reservedSectorCount + ((cluster * 4) / bytesPerSector) ;
		  SD_readSingleBlock(sector);
		  for(i=0; i<128; i++)
		  {
			   value = (unsigned long *) &buffer[i*4];
			 if(((*value)& 0x0fffffff) == 0)
				freeClusters++;;
        
			 totalClusterCount++;
			 if(totalClusterCount == (totalClusters+2)) break;
		  }  
		  if(i < 128) break;
		  cluster+=128;
		} 
	}

	if(!freeClusterCountUpdated)
	  fat_getSetFreeCluster (TOTAL_FREE, SET, freeClusters); //update FSinfo next free cluster entry
	freeClusterCountUpdated = 1;  //set flag
	freeMemory = freeClusters * sectorPerCluster / 1024;
	freeMemory *= bytesPerSector ;
	//TX_NEWLINE;
	//transmitString_F(PSTR(" Free Memory: "));
	lcd_home();
	lcd_string((" Free Memory: "));
	fat_displayMemory (HIGH, freeMemory);
	//TX_NEWLINE; 
}

//************************************************************
//Function: To convert the unsigned long value of memory into 
//          text string and send to UART
//Arguments: 1. unsigned char flag. If flag is HIGH, memory will be displayed in KBytes, else in Bytes. 
//			 2. unsigned long memory value
//return: none
//************************************************************
void fat_displayMemory (unsigned char flag, unsigned long memory)
{
  unsigned char memoryString[] = "              Bytes"; //19 character long string for memory display
  unsigned char i;
  for(i=12; i>0; i--) //converting freeMemory into ASCII string
  {
    if(i==5 || i==9) 
	{
	   memoryString[i-1] = ',';  
	   i--;
	}
    memoryString[i-1] = (memory % 10) | 0x30;
    memory /= 10;
	if(memory == 0) break;
  }
  if(flag == HIGH)  memoryString[13] = 'K';
  
  //transmitString(memoryString);
  lcd_string_F((const char*)memoryString);
}

//********************************************************************
//Function: to delete a specified file from the root directory
//Arguments: pointer to the file name
//return: none
//********************************************************************
void fat_deleteFile (unsigned char *fileName)
{
  unsigned char error;

  error = fat_convertFileName (fileName);
  if(error) return;

  fat_findFiles (DELETE, fileName);
}

//********************************************************************
//Function: update the free memory count in the FSinfo sector. 
//			Whenever a file is deleted or created, this function will be called
//			to ADD or REMOVE clusters occupied by the file
//Arguments: #1.flag ADD or REMOVE #2.file size in Bytes
//return: none
//********************************************************************
void fat_freeMemoryUpdate (unsigned char flag, unsigned long size)
{
  unsigned long freeClusters;
  //convert file size into number of clusters occupied
  if((size % 512) == 0) size = size / 512;
  else size = (size / 512) +1;
  if((size % 8) == 0) size = size / 8;
  else size = (size / 8) +1;

  if(freeClusterCountUpdated)
  {
	freeClusters = fat_getSetFreeCluster (TOTAL_FREE, GET, 0);
	if(flag == ADD)
  	   freeClusters = freeClusters + size;
	else  //when flag = REMOVE
	   freeClusters = freeClusters - size;
	fat_getSetFreeCluster (TOTAL_FREE, SET, freeClusters);
  }
}

//******** END ****** www.dharmanitech.com *****
