/*
 * ModbusRS232Slave.h
 *
 *  Created on: May 8, 2024
 *      Author: Chili
 */

#ifndef MODBUSRTU_SLAVE_INC_MODBUSRS232SLAVE_H_
#define MODBUSRTU_SLAVE_INC_MODBUSRS232SLAVE_H_

#define MaxFrameIndex 255u
#define fc3_HoldingRegMax 125u
#define fc3_HoldingRegOffset 40001u


//Holding Register Array
static unsigned int HoldingRegisters[] = {
24288,	30717,	52317,	58320,	53844,	7596,	50869,	64892,	58209,	15124,	//0-9
38131,	16852,	62343,	25973,	11963,	49434,	31745,	48278,	50955,	50027,	//10-19
62940,	38520,	51144,	54693,	60394,	14244,	62402,	62647,	65457,	56695,	//20-29
40971,	29608,	37659,	60486,	3321,	769,	52458,	61584,	55023,	61523,	//30-39
50744,	51336,	3649,	48678,	29801,	43147,	1325,	32661,	49277,	32353,	//40-49
48858,	41522,	32167,	13748,	24501,	2375,	6957,	45371,	53417,	34799,	//50-59
51057,	3290,	13510,	48962,	65080,	22597,	38672,	14315,	48621,	49290,	//60-69
30178,	1793,	13119,	54566,	1570,	31355,	1754,	21305,	5750,	25520,	//70-79
23688,	54689,	64511,	17349,	33911,	31220,	64245,	17176,	43586,	30999,	//80-89
12761,	42338,	62682,	7424,	6981,	36057,	63189,	4897,	52723,	61279,	//90-99
};
extern unsigned char MY_SLAVE_ID;



extern volatile unsigned char ResponseFrameSize;

extern volatile unsigned char data_in[MaxFrameIndex+1];
extern volatile unsigned int DataPos;




unsigned short CRC16 (volatile unsigned char *puchMsg, unsigned short usDataLen );
unsigned int MBRegisterCount(void);
void AppendDatatoMBRegister(unsigned int StAddr,unsigned int count, unsigned int *inreg, volatile unsigned char *outreg);
unsigned int MBStartAddress(void);
void MBSendData(unsigned char count);
void AppendCRCtoMBRegister(unsigned char packtop);
void MBException(unsigned char exceptionCode);
void MBProcessRegisters(unsigned char fcCode);

#endif /* MODBUSRTU_SLAVE_INC_MODBUSRS232SLAVE_H_ */
