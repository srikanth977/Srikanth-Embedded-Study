#include "cppRS232.h"
#include "main.h"

HANDLE Cport[RS232_PORTNR];

const char *comports[RS232_PORTNR] = {"\\\\.\\COM1", "\\\\.\\COM2", "\\\\.\\COM3", "\\\\.\\COM4",
                                      "\\\\.\\COM5", "\\\\.\\COM6", "\\\\.\\COM7", "\\\\.\\COM8",
                                      "\\\\.\\COM9", "\\\\.\\COM10", "\\\\.\\COM11", "\\\\.\\COM12",
                                      "\\\\.\\COM13", "\\\\.\\COM14", "\\\\.\\COM15", "\\\\.\\COM16",
                                      "\\\\.\\COM17", "\\\\.\\COM18", "\\\\.\\COM19", "\\\\.\\COM20",
                                      "\\\\.\\COM21", "\\\\.\\COM22", "\\\\.\\COM23", "\\\\.\\COM24",
                                      "\\\\.\\COM25", "\\\\.\\COM26", "\\\\.\\COM27", "\\\\.\\COM28",
                                      "\\\\.\\COM29", "\\\\.\\COM30", "\\\\.\\COM31", "\\\\.\\COM32"};

char mode_str[128];

int cppRS232_ValidateComPORT(int *comport_number)
{
    if ((*comport_number >= RS232_PORTNR) || (*comport_number < 0))
    {
        printf("Illegal comport number\n");
        return 0;
    }
    return 1;
}

int cppRS232_OpenComport(SERIALPORT_ *serialport)
{
    int comport_number = (*serialport).comport;
    int baudrate = (*serialport).baudrate;
    int bytesize = (*serialport).bytesize;
    int parity = (*serialport).parity;
    int stopbit = (*serialport).stopbit;
    int flowctrl = (*serialport).flowctrl;
    do
    {
        if (!cppRS232_ValidateComPORT(&((*serialport).comport)))
        {
            break;
        }
        /*
       http://msdn.microsoft.com/en-us/library/windows/desktop/aa363145%28v=vs.85%29.aspx

       http://technet.microsoft.com/en-us/library/cc732236.aspx

       https://docs.microsoft.com/en-us/windows/desktop/api/winbase/ns-winbase-_dcb
       */

        Cport[comport_number] = CreateFileA(comports[comport_number],
                                            GENERIC_READ | GENERIC_WRITE,
                                            0,    /* no share  */
                                            NULL, /* no security */
                                            OPEN_EXISTING,
                                            0,     /* no threads */
                                            NULL); /* no templates */

        if (Cport[comport_number] == INVALID_HANDLE_VALUE)
        {
            printf("Error Code %08x : Unable to open comport\n", GetLastError());
            return (0);
        }

        DCB port_settings;

        if (!GetCommState(Cport[comport_number], &port_settings))
        {
            printf("Error Reading Communication port setting\n");
            return 0;
        }
        memset(&port_settings, 0, sizeof(port_settings)); /* clear the new struct  */
        port_settings.DCBlength = sizeof(port_settings);

        port_settings.BaudRate = baudrate;
        port_settings.ByteSize = bytesize;
        port_settings.fParity = parity;
        port_settings.StopBits = stopbit;
        if (flowctrl)
        {
            port_settings.fOutxCtsFlow = TRUE;
            port_settings.fRtsControl = RTS_CONTROL_HANDSHAKE;
        }

        if (!SetCommState(Cport[comport_number], &port_settings))
        {
            printf("Error Code %08x : Unable to set comport cfg settings\n", GetLastError());
            CloseHandle(Cport[comport_number]);
            return (0);
        }
        COMMTIMEOUTS Cptimeouts;

        Cptimeouts.ReadIntervalTimeout = 10;
        Cptimeouts.ReadTotalTimeoutMultiplier = 0;
        Cptimeouts.ReadTotalTimeoutConstant = 1;
        Cptimeouts.WriteTotalTimeoutMultiplier = 0;
        Cptimeouts.WriteTotalTimeoutConstant = 0;

        if (!SetCommTimeouts(Cport[comport_number], &Cptimeouts))
        {
            printf("Error Code %08x : Unable to set comport time-out settings\n", GetLastError());
            CloseHandle(Cport[comport_number]);
            return (0);
        }
    } while (0);
    return 1;
}

int cppRS232_PollComport(int comport, unsigned char *buffer, int datalen)
{
    int ret = 0;
    DWORD dwEvtMask;
    DWORD ActualReadCount;
    int readcount = 0;
    if (!SetCommMask(Cport[comport], EV_RXCHAR | EV_ERR))
    {
        printf("SetCommMask failed with error code: %ld\n", GetLastError());
        ret = 0;
        return ret;
    }
    while (1)
    {
        ActualReadCount = 0;
        dwEvtMask = 0;

        if (WaitCommEvent(Cport[comport], &dwEvtMask, NULL))
        {
            if (dwEvtMask & EV_ERR)
            {
                printf("Wait failed with error %ld.\n", GetLastError());
                ret = 0;
                break;
            }
            if (dwEvtMask & EV_RXCHAR)
            {
                if (!ReadFile(Cport[comport],
                              &buffer[readcount], datalen - readcount,
                              &ActualReadCount, NULL))
                {
                    printf("\n\nERROR when reading\n\n");
                    ret = 0;
                    break;
                }
                // printf("Actual Read %d bytes so far.\n", ActualReadCount);

                // increase total accumulated count
                readcount += ActualReadCount;
            }
            if (readcount >= datalen)
            {
                //printf("Data Read complete\n");
                //     // double diff_ms = (clock() - start) * 1000. / CLOCKS_PER_SEC;

                //     print_READ_DATA_BUF(readcount); // for testing only, remove later
                //     printf("Read complete\n");
                //     ret = 1;
                //     // printf("Time spent reading: %f ms\n", diff_ms);
                ret = readcount;
                break;
            }
        }
        // handle WaitCommEvent error?
        else
        {
            DWORD dwRet = GetLastError();
            if (ERROR_IO_PENDING == dwRet)
            {
                printf("I/O is pending...\n");
                /* TODO  : What to do is not clear.     */
            }
            else
            {
                // NOTE: no need to call GetLastError twice
                printf("Wait failed with error %ld.\n", dwRet);
                ret = 0;
            }
        }
    }
    return ret;
}

int cppRS232_CloseComport(int comport)
{
    if (!CloseHandle(Cport[comport]))
    {
        printf("Failed to close COM%d\n", comport);
        return 0;
    }
    printf("COM%d CLOSED SUCCESSFULLY\n", comport);
    return 1;
}

int RS232_SendByte(int comport_number, unsigned char byte)
{
  int n;

  if(!WriteFile(Cport[comport_number], &byte, 1, (LPDWORD)((void *)&n), NULL))
  {
    return(1);
  }

  if(n<0)  return(1);

  return(0);
}

int RS232_SendBuf(int comport_number, unsigned char *buf, int size)
{
  int n;

  if(WriteFile(Cport[comport_number], buf, size, (LPDWORD)((void *)&n), NULL))
  {
    return(n);
  }

  return(-1);
}

void RS232_cputs(int comport_number, const char *text)  /* sends a string to serial port */
{
  while(*text != 0)   RS232_SendByte(comport_number, *(text++));
}


/* return index in comports matching to device name or -1 if not found */
int RS232_GetPortnr(const char *devname)
{
  int i;

  char str[32];

#if defined(__linux__) || defined(__FreeBSD__)   /* Linux & FreeBSD */
  strcpy(str, "/dev/");
#else  /* windows */
  strcpy(str, "\\\\.\\");
#endif
  strncat(str, devname, 16);
  str[31] = 0;

  for(i=0; i<RS232_PORTNR; i++)
  {
    if(!strcmp(comports[i], str))
    {
      return i;
    }
  }

  return -1;  /* device not found */
}

int RS232_IsDCDEnabled(int comport_number)
{
  int status;

  GetCommModemStatus(Cport[comport_number], (LPDWORD)((void *)&status));

  if(status&MS_RLSD_ON) return(1);
  else return(0);
}


int RS232_IsRINGEnabled(int comport_number)
{
  int status;

  GetCommModemStatus(Cport[comport_number], (LPDWORD)((void *)&status));

  if(status&MS_RING_ON) return(1);
  else return(0);
}


int RS232_IsCTSEnabled(int comport_number)
{
  int status;

  GetCommModemStatus(Cport[comport_number], (LPDWORD)((void *)&status));

  if(status&MS_CTS_ON) return(1);
  else return(0);
}


int RS232_IsDSREnabled(int comport_number)
{
  int status;

  GetCommModemStatus(Cport[comport_number], (LPDWORD)((void *)&status));

  if(status&MS_DSR_ON) return(1);
  else return(0);
}


void RS232_enableDTR(int comport_number)
{
  EscapeCommFunction(Cport[comport_number], SETDTR);
}


void RS232_disableDTR(int comport_number)
{
  EscapeCommFunction(Cport[comport_number], CLRDTR);
}


void RS232_enableRTS(int comport_number)
{
  EscapeCommFunction(Cport[comport_number], SETRTS);
}


void RS232_disableRTS(int comport_number)
{
  EscapeCommFunction(Cport[comport_number], CLRRTS);
}

/*
https://msdn.microsoft.com/en-us/library/windows/desktop/aa363428%28v=vs.85%29.aspx
*/

void RS232_flushRX(int comport_number)
{
  PurgeComm(Cport[comport_number], PURGE_RXCLEAR | PURGE_RXABORT);
}


void RS232_flushTX(int comport_number)
{
  PurgeComm(Cport[comport_number], PURGE_TXCLEAR | PURGE_TXABORT);
}


void RS232_flushRXTX(int comport_number)
{
  PurgeComm(Cport[comport_number], PURGE_RXCLEAR | PURGE_RXABORT);
  PurgeComm(Cport[comport_number], PURGE_TXCLEAR | PURGE_TXABORT);
}