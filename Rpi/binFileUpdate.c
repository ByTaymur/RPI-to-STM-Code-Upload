#include "RpiUart.h"
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <math.h>

#define MAX_BLOCK_SIZE ( 1024 )
#define MAX_FW_SIZE    ( MAX_BLOCK_SIZE * 48 ) // 48 BLOCK DAN BUYUK OLABİLR KONTROL ED 
uint8_t sum=0;
uint8_t BinFile[MAX_FW_SIZE];
uint8_t DataControl=0;
int BlockLen=MAX_BLOCK_SIZE;
int BlockArray =0;
int BlockInxStart =0 ; 
int BlockInx = 0 ;
int BlokInxEnd =0 ;
uint32_t  BinFileSize = 0 ;
int BlockNumber = 0;
int BlockNumberEnd = 0;
int comport ;
int ex = 0;
unsigned int BlockEndSize =0;
int BlockArrayNumber=0;
int SumCount=0;
int main(int argc, char *argv[])
{
	int bdrate   = 115200;       /* 115200 baud */
	char mode[]={'8','N','1',0}; /* *-bits, No parity, 1 stop bit */
	char bin_name[1024];
	comport = atoi(argv[2]) -1;
	strcpy(bin_name, argv[1]);
	if( RpiUart_OpenComport(comport, bdrate, mode, 0) )
	{
		printf("Can not open comport\n");
		ex = -1;
	}
	if( argc <= 2 )
	{
		printf("Please feed the COM PORT number and the Application Image....!!!\n");
		printf("Example: .\\etx_ota_app.exe 8 ..\\..\\Application\\Debug\\Blinky.bin");
	}  
	printf("Opening Binary file : %s\n", bin_name);
	FILE *Fptr = NULL;
	Fptr = fopen(bin_name,"rb");
	if( Fptr == NULL )	  
		printf("Can not open %s\n", bin_name);
	fseek(Fptr, 0L, SEEK_END);
	uint32_t BinFileSize = ftell(Fptr);
	fseek(Fptr, 0L, SEEK_SET);
	printf("File size = %d\n", BinFileSize);
	if( BinFileSize > MAX_FW_SIZE ) 
		printf("Application Size is more than the Maximum Size (%dKb)\n", MAX_FW_SIZE/MAX_BLOCK_SIZE);
	if( fread( BinFile, 1, BinFileSize, Fptr ) != BinFileSize )
		printf("App/FW read Error\n");
	BlockArrayNumber = (int)(BinFileSize/BlockLen);
	BlockEndSize = BinFileSize - BlockArrayNumber*BlockLen;
	if(BlockEndSize>0)
	{
		BlockArrayNumber=BlockArrayNumber+1;
	}
	float Deger=0;
	float DegerF=0;
	DegerF = 100/BlockArrayNumber;
	Deger=DegerF;
	printf("- %d - \n",BinFileSize);
	printf("- %d - \n",BlockArrayNumber);
	BlokInxEnd = 0;
	BlockInxStart=0;
	char StringAry[]="[1000]:";
     uint8_t DataLeng[4];
     DataLeng[0] =(BinFileSize & 0xff000000) >> 24;
     DataLeng[1] =(BinFileSize & 0x00ff0000) >> 16;
     DataLeng[2] =(BinFileSize & 0x0000ff00) >> 8;
     DataLeng[3] =(BinFileSize & 0x000000ff) ;
	 RpiUart_SendByte(comport, '{');
	 RpiUart_SendByte(comport,  (uint8_t)DataLeng[0]);
	 RpiUart_SendByte(comport,  (uint8_t)DataLeng[1]);
	 RpiUart_SendByte(comport,  (uint8_t)DataLeng[2]);
	 RpiUart_SendByte(comport,  (uint8_t)DataLeng[3]);
	 RpiUart_SendByte(comport, '}');
	 int Status=1;
	while(Status==1)
	{ 
		printf("\r\n\r\n-------Ok-----%x------------\r\n\r\n",DataControl);
		RpiUart_PollComport( comport, &DataControl, 1);
		if(DataControl == 'O')
		{
			Status=0;
			printf("\r\n\r\n-------Ok-----------------\r\n\r\n");
		}
	}
	for(BlockArray = 0 ; BlockArray < BlockArrayNumber  ; BlockArray++)
	{
		if(BlokInxEnd + BlockLen <= BinFileSize)
		{
			BlokInxEnd=BlokInxEnd+BlockLen;
			//printf("\r\n\r\n-------%4d-------%4d-----------------\r\n\r\n",BlockArray,BlockLen);
		}
		else
		{
			//printf("\r\n\r\n--------%4d------%4d-----------------\r\n\r\n",BlockArray,BinFileSize-BlokInxEnd);
			BlokInxEnd = BinFileSize;
		}
		sprintf(StringAry,"[%4d]:",BlokInxEnd-BlockInxStart);
		SumCount=0;
		sleep(0.025);
		for( BlockInx=BlockInxStart; BlockInx < BlokInxEnd ; BlockInx++)
		{
			sleep(0.05);
			if( RpiUart_SendByte(comport, BinFile[BlockInx]) )
			{
				printf("--- DATA : Send Err\n");
				ex = -1;
			}
		}
		sum = (BinFile[BlockInxStart] + BinFile[BlockInx-1]) & 0x000000ff;
		//printf("\r\n\r\n---- %x == %x---\r\n\r\n",BinFile[BlockInxStart],BinFile[BlockInx-1]);
		SumCount=1;
		int IslemBasariziCount=0;
		while(SumCount==1)
		{
			sleep(0.01);
			if(RpiUart_PollComport( comport, &DataControl, 1)==0)
			{
				//printf("\r\n\r\n-1-1--2-- %x == %x--2--1-1-\r\n\r\n",sum,DataControl);
			}
			if(DataControl==sum)
			{
				Deger = (BlockArray+1)*DegerF;
				printf("\r\n\r\n---- Sum OK  %.1f--\r\n\r\n",Deger);
				IslemBasariziCount=0;
				SumCount=0;
			}
			else
			{
				IslemBasariziCount++;
				printf("\r\n\r\n---- Basarisiz  %d--\r\n\r\n",IslemBasariziCount);
				if(IslemBasariziCount>10)
				{
					printf("\r\n\r\n---- İşlem iptal edildi --\r\n\r\n");
					break;
				}
			}
			
		}
		BlockInxStart=BlokInxEnd;
		SumCount=0;
		sum=0;
	}

}
