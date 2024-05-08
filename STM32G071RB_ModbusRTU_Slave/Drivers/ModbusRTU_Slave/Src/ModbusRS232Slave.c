/*
 * MB_Processor.c
 *
 *  Created on: Jul 12, 2020
 *      Author: Srikanth Chilivery
 */
#include "main.h"
#include "ModbusRS232Slave.h"

volatile unsigned int TotalCharsReceived;
unsigned int HoldingRegSize;
unsigned int InputRegSize;
unsigned int CoilsRegsize;
unsigned int DiscreteInputRegsize;

//Diagnostic counters
unsigned int BusMsgCount;		//Bus Message Count
unsigned int BusCommErrCount;	//Bus Communication Error Count
unsigned int SlaveExErrCount;	//Bus Exception Error Count
unsigned int SlaveMsgCount;		//Slave Message Count
unsigned int SlaveNoRspCount;	//Slave No Response Count
unsigned int SlaveNAKCount;		//Slave NAK Count
unsigned int SlaveBusyCount;	//Slave Busy Count
unsigned int BusChrOvrCount;	//Bus Character Overrun Count


unsigned int CharCount;

void MB_Init()
{
	MY_SLAVE_ID=17u;
	//CLEAR ALL COUNTERS
	ClearModbusCounters();
	//GET SIZE OF ARRAYS
	HoldingRegSize = (sizeof(HoldingRegisters)/sizeof(HoldingRegisters[0]));
	InputRegSize = (sizeof(InputRegisters)/sizeof(InputRegisters[0]));
	CoilsRegsize=(sizeof(Coils)/sizeof(Coils[0]));
	DiscreteInputRegsize = (sizeof(DiscreteInputs)/sizeof(DiscreteInputs[0]));
}
//Starting Modbus function to check the PDU received
void CheckMBPDU()
{
	CharCount=0;
	CharCount = TotalCharsReceived;
	TotalCharsReceived=0;
	if(CharCount>=4u)
	{
		//Check inbound frame CRC
		unsigned int crcvalue=CRC16(data_in,(CharCount-2));

		if((data_in[CharCount-2] ==(unsigned char)(crcvalue)) &					//lower byte at higher register
					(data_in[CharCount-1] ==(unsigned char)(crcvalue>>8)))		//higher byte at lower register
		{
			BusMsgCount+=1;	//increment bus message counter
			if((data_in[0]==MY_SLAVE_ID) | (data_in[0]==0u))
			{
				SlaveMsgCount+=1; //Increment Slave message count
				//STEP 2: Check function code
				SlaveNoRspCount+=data_in[0]==0?1:0;
				switch(data_in[1])
				{
					case 0x01: MBProcessBitsRead(Coils,CoilsRegsize);//read coils
					break;

					case 0x02: MBProcessBitsRead(DiscreteInputs,DiscreteInputRegsize);	//read discrete inputs
					break;

					case 0x03: MBProcessRegisterRead(HoldingRegisters,HoldingRegSize);//read holding register
					break;

					case 0x04: MBProcessRegisterRead(InputRegisters,InputRegSize);//read input register
					break;

					case 0x05: MBForceSingleCoil(Coils,CoilsRegsize);//Write single coil
					break;

					case 0x06: MBPresetSingleRegister(HoldingRegisters,HoldingRegSize);//write single register
					break;

					case 0x08: MBProcessDiagnostics();
					break;

					case 0x0f: MBForceMultipleCoils(Coils,CoilsRegsize);//Write multiple coils
					break;

					case 0x10: MBPresetMultipleRegisters(HoldingRegisters,HoldingRegSize);//write multiple registers
					break;

					default:
					{
						MBException(0x01); //Illegal function code 01
						MBSendData(ResponseFrameSize);		//send data if not broadcast command
					}
					break;
				}
			}
		}
		else BusCommErrCount+=1; //Increment bus communication error counter
	}else BusCommErrCount+=1; //Increment bus communication error counter
}

//Count the number of registers
unsigned int MBRegisterCount(void)
{
	return (data_in[5]|(data_in[4]<<8));
}

//Get Starting Modbus Address
unsigned int MBStartAddress(void)	//Return requested start address
{
	return (data_in[3]|(data_in[2]<<8));
}

//Send data over USART (RS232)
void MBSendData(unsigned char count)	//Send final data over UART
{
	if(data_in[0]!=0)
	{
		for (unsigned char c=0; c<count;c++)
		{
			while( !( USART2->ISR & (1<<7u) ) ) {};	//wait till transmit buffer is empty
			USART2->TDR = data_in[c];
		}
	}
}

/*************** Append CRC ***************/
//C function to append CRC to Slave Modbus response PDU
void AppendCRCtoMBRegister(unsigned char packtop)	//crc is calculated from slave id to last data byte
{
	unsigned short crcvalue=0;
	crcvalue=CRC16(data_in,packtop);
	data_in[packtop] =(unsigned char)(crcvalue);			//lower byte at higher register
	data_in[packtop+1] =(unsigned char)(crcvalue>>8);		//higher byte at lower register
	ResponseFrameSize = packtop + 2;
}
//************** Append CRC ***************

//**************  EXCEPTION  **************
//Write Exception code
void MBException(unsigned char exceptionCode)	//Exception code
{
	SlaveExErrCount+=1;			//Increment Slave Exception Error count
	data_in[1]|=0x80;			//setting MSB of the function code (the exception flag)
	data_in[2]=exceptionCode; 	//Exception code. Also the last byte containing dat
	ResponseFrameSize = 3;		// 3 bytes to send. No crc calculation.
}
//**************  EXCEPTION  **************

//*********Modbus Register Read Operations*************
//C function related to Analog Read operations (Function code 03, 04)
void MBProcessRegisterRead(unsigned int *InArr, unsigned int InArrSize)
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
		unsigned int EndAddress = StAddress + RegCount - 1u;

		//We will simply check if the end address is inside the size of our holding register
		if((StAddress>=0u) & (EndAddress<=(InArrSize-1u)))
		{
			//Process the request
			data_in[2]=(unsigned char)(RegCount*2);	//fill the byte count in the data array
			AppendDatatoMBRegister(StAddress,RegCount,HoldingRegisters,data_in);	//fill data in the data register
			AppendCRCtoMBRegister(3+RegCount*2);
		}
		else MBException(0x02);//Exception code 02 = ILLEGAL DATA ADDRESS
	}
	else MBException(0x03);//Exception code 03 = ILLEGAL DATA VALUE

	MBSendData(ResponseFrameSize);		//send response if not broadcast message
}

//Append Data from unsigned integer Array to Modbus PDU during Read Operation
void AppendDatatoMBRegister(unsigned int StAddr,unsigned int count, unsigned int *inreg, volatile unsigned char *outreg)
{
	for (unsigned char c=0; c<count; c++)
	{
			*(outreg+3+c*2) = (unsigned char)(*(inreg+StAddr+c) >> 8);	//MSB IN HIGHER BYTE
			*(outreg+3+(c*2+1)) = (unsigned char)(*(inreg+StAddr+c));	//LSB IN LOWER BYTE
	}
}
//*********Modbus Register Read Operations*************

//*********Modbus Register Write Operations*************
//C function to Write Single Analog register (Function code 06)
void MBPresetSingleRegister(unsigned int *InArr, unsigned int InArrSize)
{
	unsigned int StAddress = MBStartAddress();
	//exception code 03 cannot be done for function code 06
	//check of exception code 02
	if((StAddress)<=(InArrSize-1u))
	{
		*(InArr+StAddress) = (unsigned int)(data_in[4] << 8) | (unsigned int)(data_in[5]);	//MSB FIRST AND THEN LSB IS ORed
		ResponseFrameSize=CharCount;
	}
	else MBException(0x02);//Exception code 02 = ILLEGAL DATA ADDRESS

	MBSendData(ResponseFrameSize);		//send response if not broadcast message
}

//C function to Write Multiple Analog registers (Function code 16)
void MBPresetMultipleRegisters(unsigned int *InArr, unsigned int InArrSize)
{

	//Get to know the starting address of the requested data
	unsigned int StAddress = MBStartAddress();
	unsigned int RegCount = MBRegisterCount();
	unsigned int EndAddress = StAddress + RegCount - 1;

	//| SLAVE_ID | FUNCTION_CODE | RETURN BYTES COUNT | DATA      | CRC     |
	//| 1 BYTE   |  1 BYTE       |  1 BYTE            | N*2 BYTES | 2 BYTES |
	//So our final requested data should fit in above 256 size, so data should be max 256-6 bytes
	//As a safeguard we are also checking with maximum limits of query as per modbus function (m584 controller)
	if((RegCount >= 1u) &
		(RegCount <= 100u) &
		(data_in[6]==RegCount*2))	//for fc16, number of bytes requested is embedded in modbus message
	{
		//We will simply check if the end address is inside the size of our holding register
		if((StAddress>=0) & (EndAddress<=(InArrSize-1u)))
		{
			//Process the request
			WriteMBRegistertoData(StAddress,RegCount, data_in,HoldingRegisters);
			ResponseFrameSize=CharCount;
		}
		else MBException(0x02);//Exception code 02 = ILLEGAL DATA ADDRESS
	}
	else MBException(0x03);//Exception code 03 = ILLEGAL DATA VALUE

	MBSendData(ResponseFrameSize);		//send response if not broadcast message
}

//C function to Store Analog Write operations to unsigned integer Variable array
void WriteMBRegistertoData(unsigned int StAddr,unsigned int count, volatile unsigned char *inreg, unsigned int *outreg)
{
	for (unsigned char c=0; c<count; c++) *(outreg+StAddr+c) = (unsigned int)(*(inreg+7+(c*2)) << 8) | (unsigned int)(*(inreg+7+(c*2)+1));
}
//*********Modbus Register Write Operations*************

//*********Modbus Discrete Read Operations*************
//C function related to Discrete Read operations (Function code 01, 02)
void MBProcessBitsRead(unsigned char *InArr, unsigned int InArrSize)
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
		}
		else MBException(0x02);//Exception code 02 = ILLEGAL DATA ADDRESS
	}
	else MBException(0x03);//Exception code 03 = ILLEGAL DATA VALUE

	MBSendData(ResponseFrameSize);		//send response if not broadcast message
}

//C function to append array bits to modbus pDU
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
//*********Modbus Discrete Read Operations*************

//*********Modbus Discrete Write Operations*************
//C function for writing one coil (Function code 05)
void MBForceSingleCoil(unsigned char *InArr, unsigned int InArrSize)
{
	unsigned int StAddress = MBStartAddress();
	unsigned int outputvalue =MBRegisterCount();		//using existing c function instead of creating a new one
	if((outputvalue==0x0000) | (outputvalue==0xff00))
	{
		if(StAddress <=((InArrSize*8)-1))
		{
			//Not calling separate function. Writing here itself
			if(outputvalue==0xff00)
			{
				*(InArr+(StAddress/8)) |=(1u<<(StAddress-((StAddress/8)*8)));
			}else *(InArr+(StAddress/8)) &=~(1u<<(StAddress-((StAddress/8)*8)));
			ResponseFrameSize=CharCount;
		}
		else MBException(0x02);//Exception code 02 = ILLEGAL DATA ADDRESS
	}
	else MBException(0x03);//Exception code 03 = ILLEGAL DATA VALUE

	MBSendData(ResponseFrameSize);		//send response if not broadcast message
}

//C function for writing multiple coils (Function code 15)
void MBForceMultipleCoils(unsigned char *InArr, unsigned int InArrSize)
{
	//First check what is the count of bits requested
	unsigned int BitCount = MBRegisterCount();
	unsigned int StAddress = MBStartAddress();

	//As a safeguard we are also checking with maximum limits of query as per modbus function (m584 controller)
	unsigned char ByteCount = ((BitCount<=8u)
								?1u
								:((BitCount%8u==0)
									?(BitCount/8u)
									:((BitCount/8u)+1u)));
	//| SLAVE_ID | FUNCTION_CODE | RETURN BYTES COUNT | DATA      | CRC     |
	//| 1 BYTE   |  1 BYTE       |  1 BYTE            | N BYTES   | 2 BYTES |
	if ((BitCount >=1u) &
		(BitCount <= 800u) &
		(ByteCount==data_in[6]))
	{
		//to check if the requested start and end addresses are available in out controller
		//As an example we configure 50 holding registers = 100 bytes of data array HoldingRegister
		//Get to know the starting address of the requested data
			//start coil address
		unsigned int EndAddress = StAddress + BitCount - 1;	//end coil address

		if((StAddress>=0) & (EndAddress <=((InArrSize*8)-1)))
		{
			//Process the requests
			WriteMBRegistersToBits(StAddress,BitCount,data_in,InArr);
			ResponseFrameSize=CharCount;
		}
		else MBException(0x02);//Exception code 02 = ILLEGAL DATA ADDRESS
	}
	else MBException(0x03);//Exception code 03 = ILLEGAL DATA VALUE

	MBSendData(ResponseFrameSize);		//send response if not broadcast message
}

//C function to store MODBUS master's discrete signals to Coil array
void WriteMBRegistersToBits(unsigned int StAddr, unsigned int count, volatile unsigned char *inreg,unsigned char *outreg)
{
	for (unsigned int c=0;c<count;c++)
	{
		if(*(inreg+7+(c/8u))&(1<<(c-((c/8u)*8u))))	//if in outreg array, bit is 1?
		{
			*(outreg+((StAddr+c)/8)) |=(1u<<((StAddr+c)-(((StAddr+c)/8)*8)));
		} else *(outreg+((StAddr+c)/8)) &=~(1u<<((StAddr+c)-(((StAddr+c)/8)*8)));	//else clear the bit in target array
	}
}
//*********Modbus Discrete Write Operations*************

void MBProcessDiagnostics(void)
{
	unsigned int subcode=MBStartAddress();	//using existing c function instead of creating new one
	unsigned int Data=MBRegisterCount();	//using existing c function
	switch(subcode)
	{
		case 0x00:			//Return Query Data
			ResponseFrameSize=CharCount;
			break;

		case 0x01:			//Restart Communications Option
			break;

		case 0x02:			//Return Diagnostic Register - No diagnostic register in modbus slave
			break;

		case 0x03:			//Change ASCII Input Delimiter - Not for our application as we are using Modbus RTU
			break;

		case 0x04:			//Force Listen Only Mode
			break;

		case 0x0a:			//Clear Counters and Diagnostic Register
			if(Data==0x0000)
			{
				ClearModbusCounters();
				ResponseFrameSize=CharCount;
			}
			else MBException(0x03);//Exception code 03 = ILLEGAL DATA VALUE
			break;

		case 0x0b:			//Return Bus Message Count
		case 0x0c:			//Return Bus Communication Error Count
		case 0x0d:			//Return Bus Exception Error Count
		case 0x0e:			//Return Slave Message Count
		case 0x0f:			//Return Slave No Response Count
		case 0x10:			//Return Slave NAK Count
		case 0x11:			//Return Slave Busy Count
		case 0x012:			//Return Bus Character Overrun Count
			if(Data==0x0000)
			{
				unsigned int counterval=ReturnDiagCounter(subcode);
				data_in[4]=(counterval<<8);
				data_in[5]=(counterval);
				ResponseFrameSize=CharCount;
			}
			else MBException(0x03);//Exception code 03 = ILLEGAL DATA VALUE
			break;

		case 0x14:			//Clear Overrun Counter and Flag
			if(Data==0x0000)
			{
				BusChrOvrCount=0;
				ResponseFrameSize=CharCount;
			}
			else MBException(0x03);//Exception code 03 = ILLEGAL DATA VALUE
			break;

		default:
		{
			MBException(0x01); //Illegal function code 01
			MBSendData(ResponseFrameSize);		//send data if not broadcast command
			break;
		}

	}
	MBSendData(CharCount);	//echo back the data
}

unsigned int ReturnDiagCounter(unsigned int scode)
{
	switch (scode)
	{
	case 0x0b: return BusMsgCount;
	break;

	case 0x0c: return BusCommErrCount;
	break;

	case 0x0d: return SlaveExErrCount;
	break;

	case 0x0e: return SlaveMsgCount;
	break;

	case 0x0f: return SlaveNoRspCount;
	break;

	case 0x10: return SlaveNAKCount;
	break;

	case 0x11: return SlaveBusyCount;
	break;

	case 0x12: return BusChrOvrCount;
	break;
	}
	return 0;
}
void ClearModbusCounters()
{
	  BusMsgCount=0;		//Bus Message Count
	  BusCommErrCount=0; 	//Bus Communication Error Count
	  SlaveExErrCount=0;	//Bus Exception Error Count
	  SlaveMsgCount=0;		//Slave Message Count
	  SlaveNoRspCount=0;	//Slave No Response Count
	  SlaveNAKCount=0;		//Slave NAK Count
	  SlaveBusyCount=0;		//Slave Busy Count
	  BusChrOvrCount=0;		//Bus Character Overrun Count
}

