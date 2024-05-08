
/**************************************************

file: etx_ota_update_main.c
purpose: 

compile with the command: gcc etx_ota_update_main.c RS232\rs232.c -IRS232 -Wall -Wextra -o2 -o etx_ota_app

**************************************************/
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

#ifdef _WIN32
#include <Windows.h>
#else
#include <unistd.h>
#endif

#include "rs232.h"
#include "PCTool_ota_update.h"

uint8_t DATA_BUF[DEF_OTA_PACKET_MAX_SIZE];
uint8_t APP_BIN[DEF_OTA_MAX_FW_SIZE];

uint32_t crc32b(unsigned char *message,uint16_t usDataLen);

void delay(uint32_t us)
{
    us *= 10;
#ifdef _WIN32
    //Sleep(ms);
    __int64 time1 = 0, time2 = 0, freq = 0;

    QueryPerformanceCounter((LARGE_INTEGER *) &time1);
    QueryPerformanceFrequency((LARGE_INTEGER *)&freq);

    do {
        QueryPerformanceCounter((LARGE_INTEGER *) &time2);
    } while((time2-time1) < us);
#else
    usleep(us);
#endif
}

// /* read the response */
// bool is_ack_resp_received( int comport)
// {
//   bool is_ack  = false;

//   memset(DATA_BUF, 0, DEF_OTA_PACKET_MAX_SIZE);

//   uint16_t len =  RS232_PollComport( comport, DATA_BUF, sizeof(STU_OTA_RESP_));

//   if( len > 0 )
//   {
//     STU_OTA_RESP_ *resp = (STU_OTA_RESP_*) DATA_BUF;
//     if( resp->packet_type == ENM_OTA_PKT_TYP_RESPONSE && resp->status == DEF_OTA_ACK) //TODO: Add CRC check
//     {
//       //ACK received
//         is_ack = true;
//     }
//   }
//   return is_ack;
// }

// /* Build the OTA START command */
// int send_ota_start(int comport)
// {
//   uint16_t len;
//   STU_OTA_COMMAND_ *ota_start = (STU_OTA_COMMAND_*)DATA_BUF;
//   int ex = 0;
//   len = sizeof(STU_OTA_COMMAND_);
//   memset(DATA_BUF, 0, DEF_OTA_PACKET_MAX_SIZE);

//   ota_start->sof          = DEF_OTA_SOF;
//   ota_start->packet_type  = ENM_OTA_PKT_TYP_CMD;
//   ota_start->data_len     = 1;
//   ota_start->cmd          = ENM_OTA_CMD_START;
//   ota_start->crc          = crc32b(DATA_BUF,len-5);               //TODO: Add CRC
//   ota_start->eof          = DEF_OTA_EOF;



//   //send OTA START
//   for(int i = 0; i < len; i++)
//   {
//     delay(1);
//     if( RS232_SendByte(comport, DATA_BUF[i]) )
//     {
//       //some data missed.
//       printf("OTA START : Send Err\n");
//       ex = -1;
//       break;
//     }
//   }

//   if( ex >= 0 )
//   {
//     if( !is_ack_resp_received( comport ) )
//     {
//       //Received NACK
//       printf("OTA START : NACK\n");
//       ex = -1;
//     }
//   }
//   printf("OTA START - ack received [ex = %d]\n", ex);
//   return ex;
// }

// /* Build and Send the OTA END command */
// uint16_t send_ota_end(int comport)
// {
//   uint16_t len;
//   STU_OTA_COMMAND_ *ota_end = (STU_OTA_COMMAND_*)DATA_BUF;
//   int ex = 0;
// len = sizeof(STU_OTA_COMMAND_);
//   memset(DATA_BUF, 0, DEF_OTA_PACKET_MAX_SIZE);

//   ota_end->sof          = DEF_OTA_SOF;
//   ota_end->packet_type  = ENM_OTA_PKT_TYP_CMD;
//   ota_end->data_len     = 1;
//   ota_end->cmd          = ENM_OTA_CMD_END;
//   ota_end->crc          = crc32b(DATA_BUF,len-5); ;               //TODO: Add CRC
//   ota_end->eof          = DEF_OTA_EOF;

//   //printf("END crc is: %d \n",ota_end->crc);

//   printf("Sending OTA End command \n");
//   //send OTA END
//   for(int i = 0; i < len; i++)
//   {
//     delay(1);
//     if( RS232_SendByte(comport, DATA_BUF[i]) )
//     {
//       //some data missed.
//       printf("OTA END : Send Err\n");
//       ex = -1;
//       break;
//     }
//   }
//   if( ex >= 0 )
//   {
//     if( !is_ack_resp_received( comport ) )
//     {
//       //Received NACK
//       printf("OTA END : NACK\n");
//       ex = -1;
//     }
//     else
//     {
//       printf("OTA END ACKED [ex = %d]\n", ex);
//     }
//   }
//   printf("OTA END [ex = %d]\n", ex);
//   return ex;
// }

// /* Build and send the OTA Header */
// int send_ota_header(int comport, meta_info *ota_info)
// {
//   uint16_t len;
//   uint32_t crcvalue =0;
//   STU_OTA_HEADER_ *ota_header = (STU_OTA_HEADER_*)DATA_BUF;
//   int ex = 0;
// len = sizeof(STU_OTA_HEADER_);
//   memset(DATA_BUF, 0, DEF_OTA_PACKET_MAX_SIZE);

//   ota_header->sof          = DEF_OTA_SOF;
//   ota_header->packet_type  = ENM_OTA_PKT_TYP_HEADER;
//   ota_header->data_len     = sizeof(meta_info);   //changed from meta data to ota header
//   ota_header->crc          = 0x00;              //TODO: Add CRC
//   ota_header->eof          = DEF_OTA_EOF;

//   memcpy(&ota_header->meta_data, ota_info, sizeof(meta_info) );

//   crcvalue=crc32b(DATA_BUF,len-5);
//   memcpy(&ota_header->crc,&crcvalue,sizeof(crcvalue));
//   //printf("Header crc is: %d \n",ota_header->crc);
  

//   //send OTA Header
//   for(int i = 0; i < len; i++)
//   {
//     delay(1);
//     if( RS232_SendByte(comport, DATA_BUF[i]) )
//     {
//       //some data missed.
//       printf("OTA HEADER : Send Err\n");
//       ex = -1;
//       break;
//     }
//   }

//   if( ex >= 0 )
//   {
//     if( !is_ack_resp_received( comport ) )
//     {
//       //Received NACK
//       printf("OTA HEADER : NACK\n");
//       ex = -1;
//     }
//   }
//   printf("OTA HEADER - ack received[ex = %d]\n", ex);
//   return ex;
// }

// /* Build and send the OTA Data */
// int send_ota_data(int comport, uint8_t *data, uint16_t data_len)
// {
//   uint16_t len;
//   STU_OTA_DATA_ *ota_data = (STU_OTA_DATA_*)DATA_BUF;
//   int ex = 0;

//   memset(DATA_BUF, 0, DEF_OTA_PACKET_MAX_SIZE);

//   ota_data->sof          = DEF_OTA_SOF;
//   ota_data->packet_type  = ENM_OTA_PKT_TYP_DATA;
//   ota_data->data_len     = data_len;
//   // crc and eof are added in the packet below*/
//   len = 4;

//   //Copy the data
//   memcpy(&DATA_BUF[len], data, data_len );
//   len += data_len;
//   uint32_t crc = crc32b(DATA_BUF,len);        //TODO: Add CRC

//   //Copy the crc
//   memcpy(&DATA_BUF[len], (uint8_t*)&crc, sizeof(crc) );
//   len += sizeof(crc);
//   //printf("DATA crc is: %d \n", crc);
//   //Add the EOF
//   DATA_BUF[len] = DEF_OTA_EOF;
//   len++;

//   //printf("Sending %d Data\n", len);

//   //send OTA Data
//   for(int i = 0; i < len; i++)
//   {
//     delay(500);
//     //printf("Sending DATA[%d]: %d \n", i,DATA_BUF[i]);
//     if( RS232_SendByte(comport, DATA_BUF[i]) )
//     {
//       //some data missed.
//       printf("OTA DATA : Send Err\n");
//       ex = -1;
//       break;
//     }
//   }

//   if( ex >= 0 )
//   {
//     //printf("Data sent, waiting for Acknowledgement\n");
//     if( !is_ack_resp_received( comport ) )
//     {
//       //Received NACK
//       printf("OTA DATA : NACK\n");
//       ex = -1;
//     }
//     else printf("ACKED, OTA DATA [ex = %d]\n", ex);
//   }
  
//   return ex;
// }

int main(int argc, char *argv[])
{
  int comport;
  int bdrate   = 9600;       /* 115200 baud */
  char mode[]={'8','N','1',0}; /* *-bits, No parity, 1 stop bit */
  char bin_name[1024];
  int ex = 0;
  FILE *Fptr = NULL;

  do
  {
    if( argc <= 2 )
    {
      printf("Please feed the COM PORT number and the Application Image....!!!\n");
      printf("Example: .\\etx_ota_app.exe 8 ..\\..\\Application\\Debug\\Blinky.bin");
      ex = -1;
      break;
    }

    //get the COM port Number
    comport = atoi(argv[1]) -1;
    strcpy(bin_name, argv[2]);

    printf("Opening COM%d...\n", comport+1 );

    if( RS232_OpenComport(comport, bdrate, mode, 0) )
    {
      printf("Can not open comport\n");
      ex = -1;
      break;
    }
    printf("COMPORT opened, send_ota_start - begin\n");
    //send OTA Start command
    ex = send_ota_start(comport);
    if( ex < 0 )
    {
      printf("send_ota_start Err\n");
      break;
    }

    printf("Opening Binary file : %s\n", bin_name);

    Fptr = fopen(bin_name,"rb");

    if( Fptr == NULL )
    {
      printf("Can not open %s\n", bin_name);
      ex = -1;
      break;
    }

    fseek(Fptr, 0L, SEEK_END);
    uint32_t app_size = ftell(Fptr);
    fseek(Fptr, 0L, SEEK_SET);

    printf("File size = %d\n", app_size);

    //Send OTA Header
    meta_info ota_info;
    ota_info.package_size = app_size;
    ota_info.package_crc  = 0;          //TODO: Add CRC

    ex = send_ota_header( comport, &ota_info );
    if( ex < 0 )
    {
      printf("send_ota_header Err\n");
      break;
    }

    //read the full image
    if( fread( APP_BIN, 1, app_size, Fptr ) != app_size )
    {
      printf("App/FW read Error\n");
      ex = -1;
      break;
    }

    uint16_t size = 0;
    //printf("SARTED to send application");
    for( uint32_t i = 0; i < app_size; )
    {
      if( ( app_size - i ) >= DEF_OTA_DATA_MAX_SIZE )
      {
        size = DEF_OTA_DATA_MAX_SIZE;
      }
      else
      {
        size = app_size - i;
      }

      printf("[%d/%d]\r\n", i/DEF_OTA_DATA_MAX_SIZE, app_size/DEF_OTA_DATA_MAX_SIZE);

      ex = send_ota_data( comport, &APP_BIN[i], size );
      if( ex < 0 )
      {
        printf("send_ota_data Err [i=%d]\n", i);
        break;
      }

      i += size;
    }
    //printf("ENDED to send application\n");

    if( ex < 0 )
    {
      break;
    }

    //send OTA END command
    ex = send_ota_end(comport);
    if( ex < 0 )
    {
      printf("send_ota_end Err\n");
      break;
    }
    else
    {
      printf("OTA END: ACKED\n");
    }    

  } while (false);

  if(Fptr)
  {
    fclose(Fptr);
  }

  if( ex < 0 )
  {
    printf("OTA ERROR\n");
  }
  return(ex);
}

// // ----------------------------- crc32b --------------------------------

// /* This is the basic CRC-32 calculation with some optimization but no
// table lookup. The the byte reversal is avoided by shifting the crc reg
// right instead of left and by using a reversed 32-bit word to represent
// the polynomial.
//    When compiled to Cyclops with GCC, this function executes in 8 + 72n
// instructions, where n is the number of bytes in the input message. It
// should be doable in 4 + 61n instructions.
//    If the inner loop is strung out (approx. 5*8 = 40 instructions),
// it would take about 6 + 46n instructions. */

// uint32_t crc32b(unsigned char *message,uint16_t usDataLen) {
//    uint16_t i;
//    uint16_t j;
//    uint32_t byte, crc, mask;

//    i = 0;
//    crc = 0xFFFFFFFF;
//     while (usDataLen--) /* pass through message buffer  */
//     {
//       byte = message[i];            // Get next byte.
//       crc = crc ^ byte;
//       for (j = 0; j <=7; j++) {    // Do eight times.
//          mask = -(crc & 1);
//          crc = (crc >> 1) ^ (0xEDB88320 & mask);
//       }
//       i = i + 1;
//    }
//    return ~crc;
// }