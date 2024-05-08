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
	data_in[packtop+1] =(unsigned char)(crcvalue);			//lower byte at higher register
	data_in[packtop+2] =(unsigned char)(crcvalue>>8);		//higher byte at lower register

	ResponseFrameSize = packtop + 3;
}

void MBException(unsigned char exceptionCode)	//Exception code
{
	data_in[1]|=0x80; //setting MSB of the function code (the exception flag)
	data_in[2]=exceptionCode; //Exceptioncode. Also the last byte containing dat
	ResponseFrameSize = 3;	// 3 bytes to send. No crc calculation.
}
void MBProcessRegisters(unsigned char fcCode)
{
	//First check what is the count of registers requested
	unsigned int RegCount = MBRegisterCount();

	//| SLAVE_ID | FUNCTION_CODE | RETURN BYTES COUNT | DATA      | CRC     |
	//| 1 BYTE   |  1 BYTE       |  1 BYTE            | N*2 BYTES | 2 BYTES |
	//So our final requested data should fit in above 256 size, so data should be max 256-6 bytes
	//As a safeguard we are also checking with maximum limits of query as per modbus function (m584 controller)
	if((RegCount >= 1u) &
		(RegCount*2 <= ((sizeof(data_in)/sizeof(data_in[0])) - 5u)) &
		(RegCount <= fc3_HoldingRegMax))
	{
		//to check if the requested start and end addresses are available in out controller
		//Get to know the starting address of the requested data
		unsigned int StAddress = MBStartAddress();
		unsigned int EndAddress = StAddress + RegCount - 1;

		//We will simply check if the end address is inside the size of our holding register
		if(EndAddress<=(sizeof(HoldingRegisters))/sizeof(HoldingRegisters[0]))
		{
			//Process the request
			data_in[2]=(unsigned char)(RegCount*2);	//fill the byte count in the data array
			AppendDatatoMBRegister(StAddress,RegCount,HoldingRegisters,data_in);	//fill data in the data register
			AppendCRCtoMBRegister(2+RegCount*2);
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


