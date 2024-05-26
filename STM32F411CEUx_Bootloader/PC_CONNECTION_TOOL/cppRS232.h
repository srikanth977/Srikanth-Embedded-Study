/* cppRS232*/


#ifndef INC_CPPRS232_H_
#define INC_CPPRS232_H_

#include "main.h"
#include <stdio.h>
#include <string.h>

#define RS232_PORTNR  32


int cppRS232_OpenComport( SERIALPORT_ *serialport);
int cppRS232_PollComport(int comport, unsigned char *buffer, int datalen);
int BackupApp(int comport);
int cppRS232_CloseComport(int comport);
int RS232_SendByte(int, unsigned char);
int RS232_SendBuf(int, unsigned char *, int);
void RS232_cputs(int, const char *);
int RS232_IsDCDEnabled(int);
int RS232_IsRINGEnabled(int);
int RS232_IsCTSEnabled(int);
int RS232_IsDSREnabled(int);
void RS232_enableDTR(int);
void RS232_disableDTR(int);
void RS232_enableRTS(int);
void RS232_disableRTS(int);
void RS232_flushRX(int);
void RS232_flushTX(int);
void RS232_flushRXTX(int);
int RS232_GetPortnr(const char *);







#endif
