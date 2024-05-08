/*
 * MB_Processor.c
 *
 *  Created on: Jul 12, 2020
 *      Author: Srikanth Chilivery
 */
#include "main.h"
#include "ModbusRS232Slave.h"

unsigned int MBRegisterCount(void)
{
	return (data_in[5]|(data_in[4]<<8));
}

void AppendDatatoMBRegister(unsigned int StAddr,unsigned int count, unsigned int *inreg, volatile unsigned char *outreg)
{
	for (unsigned char c=0; c<count; c++)
	{
			*(outreg+3+c*2) = (unsigned char)(*(inreg+StAddr+c) >> 8);	//MSB IN HIGHER BYTE
			*(outreg+3+(c*2+1)) = (unsigned char)(*(inreg+StAddr+c));	//LSB IN LOWER BYTE
	}
}

unsigned int MBStartAddress(void)	//Return requested start address
{
	return (data_in[3]|(data_in[2]<<8));
}

void MBSendData(unsigned char count)	//Send final data over UART
{
	for (unsigned char c=0; c<count;c++)
	{
		while( !( USART2->ISR & (1<<7u) ) ) {};	//wait till transmit buffer is empty
		USART2->TDR = data_in[c];
	}
}

void AppendCRCtoMBRegister(unsigned char packtop)	//crc is calculated from slave id to last data byte
{
	unsigned short crcvalue=0;
	crcvalue=CRC16(data_in,packtop);
	data_in[packtop] =(unsigned char)(crcvalue);			//lower byte at higher register
	data_in[packtop+1] =(unsigned char)(crcvalue>>8);		//higher byte at lower register

	ResponseFrameSize = packtop + 2;
}

void MBException(unsigned char exceptionCode)	//Exception code
{
	data_in[1]|=0x80; //setting MSB of the function code (the exception flag)
	data_in[2]=exceptionCode; //Exceptioncode. Also the last byte containing dat
	ResponseFrameSize = 3;	// 3 bytes to send. No crc calculation.
}
void MBProcessRegisters(unsigned int *InArr, unsigned int InArrSize)
{
	//First check what is the count of registers requested
	unsigned int RegCount = MBRegisterCount();

	//| SLAVE_ID | FUNCTION_CODE | RETURN BYTES COUNT | DATA      | CRC     |
	//| 1 BYTE   |  1 BYTE       |  1 BYTE            | N*2 BYTES | 2 BYTES |
	//So our final requested data should fit in above 256 size, so data should be max 256-6 bytes
	//As a safeguard we are also checking with maximum limits of query as per modbus function (m584 controller)
	if((RegCount >= 1u) &
		(RegCount*2 <= 251u) &
		(RegCount <= 125u))
	{
		//to check if the requested start and end addresses are available in out controller
		//Get to know the starting address of the requested data
		unsigned int StAddress = MBStartAddress();
		unsigned int EndAddress = StAddress + RegCount - 1;

		//We will simply check if the end address is inside the size of our holding register
		if(EndAddress<=(InArrSize-1))
		{
			//Process the request
			data_in[2]=(unsigned char)(RegCount*2);	//fill the byte count in the data array
			AppendDatatoMBRegister(StAddress,RegCount,InArr,data_in);	//fill data in the data register
			AppendCRCtoMBRegister(3+RegCount*2);
			MBSendData(ResponseFrameSize);
		}
		else
		{
			MBException(0x02);//Exception code 02 = ILLEGAL DATA ADDRESS
			MBSendData(ResponseFrameSize);
		}
	}
	else
	{
		MBException(0x03);//Exception code 03 = ILLEGAL DATA VALUE
		MBSendData(ResponseFrameSize);
	}
}
void MBProcessBits(unsigned char *InArr, unsigned int InArrSize)
{
	//First check what is the count of bits requested
	unsigned int BitCount = MBRegisterCount();
	//As a safeguard we are also checking with maximum limits of query as per modbus function (m584 controller)

	unsigned char ByteCount = ((BitCount<=8u)
								?1u
								:((BitCount%8u==0)
									?(BitCount/8u)
									:((BitCount/8u)+1u)));
	//| SLAVE_ID | FUNCTION_CODE | RETURN BYTES COUNT | DATA      | CRC     |
	//| 1 BYTE   |  1 BYTE       |  1 BYTE            | N BYTES   | 2 BYTES |
	if ((BitCount >=1u) &
		(BitCount <= 2000u))
	{
		//to check if the requested start and end addresses are available in out controller
		//As an example we configure 50 holding registers = 100 bytes of data array HoldingRegister
		//Get to know the starting address of the requested data
		unsigned int StAddress = MBStartAddress();				//start coil address
		unsigned int EndAddress = StAddress + BitCount - 1;	//end coil address

		if(EndAddress <=((InArrSize*8)-1))
		{
			//Process the request
			data_in[2]=(unsigned char)ByteCount;	//fill the byte count in the data array


			//Ex. if master request 4 coil statuses, this means that 1 register is response.
			//We need to clear the remaining 4 bits of the data if there is any.
			//Else there will be error
			unsigned char regtoclear = ((BitCount<8u)
										?3u
										:((BitCount%8u>0)
												?((unsigned char) (BitCount/8u+3u))
												:0));
			//clearing last byte of response array
			if (regtoclear>0) data_in[(unsigned char)regtoclear]=0x00;

			AppendBitsToRegisters(StAddress,BitCount,InArr,data_in);
			AppendCRCtoMBRegister(3+ByteCount);
			MBSendData(ResponseFrameSize);
		}
		else
		{
			MBException(0x02);//Exception code 02 = ILLEGAL DATA ADDRESS
			MBSendData(ResponseFrameSize);
		}
	}
	else
	{
		MBException(0x03);//Exception code 03 = ILLEGAL DATA VALUE
		MBSendData(ResponseFrameSize);
	}
}

//Used for appending discrete bits to response register
void AppendBitsToRegisters(unsigned int StAddr, unsigned int count, unsigned char *inreg, volatile unsigned char *outreg)
{
	for (unsigned int c=0;c<count;c++)
	{
		if(*(inreg+((StAddr+c)/8u))&(1<<((StAddr+c)-(((StAddr+c)/8u)*8u))))	//if in outreg array, bit is 1?
		{
			*(outreg+3+(c/8))|=(1<<(c-((c/8)*8)));	//then set bit 1 in target array
		} else *(outreg+3+(c/8))&=~(1<<(c-((c/8)*8)));	//else clear the bit in target array
	}
}


