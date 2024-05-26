#include "OTA_UART.h"
#include "main.h"

#include "st7735.h"
#include "fonts.h"

/* RS232 HAL Handle */
extern UART_HandleTypeDef huart1;

/* Initial OTA state */
ENM_OTA_STATE_ ota_state = ENM_OTA_STATE_IDLE;

/* Firmware Total Size that we are going to receive */
static uint32_t ota_fw_total_size = 0;

/* Firmware Size that we have received */
static uint32_t ota_fw_received_size =0;

/* Global variable that calculates the App's size */
static uint32_t App_pakage_size=0;

/* Buffer Arrays used for Transmit and Receive.
 * They can be merged and made a single array (future) */
uint8_t Rx_Buffer [ DEF_OTA_PACKET_MAX_SIZE ];
uint8_t Tx_Buffer[DEF_OTA_PACKET_MAX_SIZE];

/* Union is to extract the 4 Bytes data from the flash memory */
union flashdata{
  uint32_t data_word;
  uint8_t data_bytes[4];
}flashmem_data;

/* Union is to extract the crc (from datatype uint32_t to bytes) */
union crcdata{
  uint32_t data_word;
  uint8_t data_bytes[4];
}crc_data;

/* ========================  FUNCTION PROTOTPYES ======================== */

/* Main function to Start Backup of running firmware application */
ENM_OTA_RET_ ota_backup_application( uint32_t *app_size );

/* Function to send running firmware application in chunks */
ENM_OTA_RET_ ota_send_APP_Data(uint16_t data_len, uint8_t *buf);

/* Function to read Acknowledgment from PC during Backup of running firmware */
static ENM_OTA_RET_ ota_is_response_received();

/* Function that writes to Flash - NOT USED FOR NOW*/
ENM_OTA_RET_ ota_download_and_flash( void );

/* Main function for this library */
ENM_OTA_RET_ ota_begin(void);

/* Function to receive data from PC tool over UART progocol*/
uint16_t receive_data(void);

/* Function to process received data */
static ENM_OTA_RET_ ota_process_data( uint8_t *buf, uint16_t len );

/* Function to send Acknowledgment after processing of received data */
static void ota_send_resp(ENM_OTA_RET_ returnvalue);	// uint8_t type );

/* Function that writes to Flash */
static HAL_StatusTypeDef write_data_to_flash_app( uint8_t *data,uint16_t data_len, bool is_first_block );

/* Function to calculate a minimal crc32, Function is located in file CRC32Processing.c */
uint32_t crc32b(unsigned char *message,uint16_t usDataLen);

/* Function to Write the End of FLASH that is recently downloaded to a separate FLASH address
 * This function currently write to FLASH. If there is a battery implementation on board then
 * this can be stored in RAM location (Better to define a memory section in BOOTLOADER for this)*/
static HAL_StatusTypeDef Write_APP_End_Address(uint32_t end_of_application);

/* Function to Send running application meta info to PC tool */
ENM_OTA_RET_ send_ota_fw_header(uint32_t fwsize);

/* Low level function to send character buffer over UART */
static void ota_send_chars(uint16_t len, uint8_t *buf);

/* ==================== END OF FUNCTION PROTOTYPES  ==================== */

/****************************************************************
 * @brief	:
 * Receive data over UART from PC tool:
 * During reception, some basic integrity checks of SOF,EOF are checked in this function iteslf
 * Finally after this basic check and collecting all the data from PC tool, length (variable index) is passed back.
 * CRC is not checked in this function so as NOT TO disturb the reception activity
 * In this version, TIMEOUT management of the communication line is not considered
 *
 * @retval 	: Length of Received packet
 * 		  0 - No data received or Error in SOF, EOF
 *
 * @params	:
 * No parameters (void)
 *****************************************************************/
uint16_t receive_data()		//return the length of packet
{
  uint16_t data_len=0;
  uint16_t index=0;
  uint16_t i=0;
  do	//need timeout management.
    {
      /*SOF - 1BYTE   */
      while( !( USART1->SR & USART_SR_RXNE )) {};
      Rx_Buffer[index++] = USART1->DR;
      if(Rx_Buffer[0] != DEF_OTA_SOF)
	{
	  index =0;/* SOF error */
	  break;
	}

      /* Packet Type - 1 BYTE */
      while( !( USART1->SR & USART_SR_RXNE )) {};
      Rx_Buffer[index++] = USART1->DR;

      /* PACKET LENGTH - 2 BYTE*/
      for(i =1; i<=2;i++)
	{

	  while( !( USART1->SR & USART_SR_RXNE )) {};
	  Rx_Buffer[index++] = USART1->DR;

	}
      data_len = (Rx_Buffer[index-1] << 8) | (Rx_Buffer[index-2]);	//bitwise OR

      /* DATA or COMMAND */
      for(i =1; i<=data_len;i++)
	{
	  while( !( USART1->SR & USART_SR_RXNE )) {};
	  Rx_Buffer[index++] = USART1->DR;
	}

      /* CRC 4 BYTES*/
      for(i =1; i<=4;i++)
	{
	  while( !( USART1->SR & USART_SR_RXNE )) {};
	  Rx_Buffer[index++] = USART1->DR;
	}

      /* EOF */
      while( !( USART1->SR & USART_SR_RXNE )) {};
      Rx_Buffer[index++] = USART1->DR;
      if(Rx_Buffer[index-1] != DEF_OTA_EOF)
	{
	  index =0;/* EOF error */
	  break;
	}
    }while (0);
  return index;

}

/****************************************************************
 * @brief	:
 * Main function of this library
 * This function checks data reception from PC tool, processes and based on the data received, a STATE MACHINE is updated
 * Following are the STATE MACHINE (enums) for this library
 * 	OTA_STATE_IDLE    = 1,
 *  	OTA_STATE_START   = 2,
 *  	OTA_STATE_HEADER  = 3,
 *	OTA_STATE_DATA    = 4,
 *	OTA_STATE_END     = 5,
 * The program begins with START state and continues till IDLE state.
 * CRC processing on the received data packet is performed before processing of data
 * Based on the return of ota_process_data, response is sent to PC Tool.
 * The expected change of states and their meanings are explained in function ota_process_data
 *
 * @retval 	: Enum ENM_OTA_RET_
 * 		  0 - Error
 * 		  1 - OK
 *
 * @params	:
 * No parameters (void)
 *****************************************************************/
ENM_OTA_RET_ ota_begin(void)
{
  uint32_t crcvalue=0;
  ENM_OTA_RET_ ret = ENM_OTA_RET_ERR;

  //clear the buffer
  uint16_t PacketLength;

  /* Reset the variables */
  ota_fw_total_size    = 0u;
  ota_fw_received_size = 0u;
  ota_state = ENM_OTA_STATE_START;
  do
    {
      memset( Rx_Buffer, 0, DEF_OTA_PACKET_MAX_SIZE );
      PacketLength=receive_data();
      if (PacketLength <= 0)
	{
	  ret=ENM_OTA_RET_ERR;
	  printf("RECEIVE ERR!!\n");
	  break;	//come out of while loop
	}
      uint32_t pktcrc = (Rx_Buffer[PacketLength-2]<<24) |
	  (Rx_Buffer[PacketLength-3]<<16) |
	  (Rx_Buffer[PacketLength-4]<<8) |
	  (Rx_Buffer[PacketLength-5]);
      crcvalue= crc32b(Rx_Buffer,PacketLength-5);
      if (crcvalue != pktcrc)
	{
	  printf("CRC error\n");
	  ret= ENM_OTA_RET_ERR;
	  break;
	}

      //Else Proceed further
      ret =ota_process_data(Rx_Buffer, PacketLength);
      if(!ret)
	{
	  break;
	}
    }while(ota_state != ENM_OTA_STATE_IDLE);
  return ret;
}

/****************************************************************
 * @brief	:
 * Data processor and runs the STATEMACHINE
 * This function processes data based on their packet type and current STATE.
 * Flow of STATE MACHINE is  as follows
 * 	OTA_STATE_START    = 1,
 *	- Received data is checked if a start command is received,
 * 	- On verification, STATE is changed to OTA_STATE_HEADER and breaks out of this function with return OK, else Error.
 *
 *  	OTA_STATE_HEADER  = 3,
 *	- Received data is checked if a Header packed is received,
 *	- On verification, Firmware's meta data (total size in bytes) is stored
 *	- STATE is changed to OTA_STATE_DATA and breaks out of this function with return OK, else Error.
 *
 *	OTA_STATE_DATA    = 4,
 *	- Received data is checked if its a DATA packet
 *	- There may be N number of times the STATE may stay in DATA cycle
 *	- Every time a packet is received, this data is directly written to FLASH
 *	- On receiving all the data, state is changed to OTA_STATE_END and breaks out of this function with return OK, else Error.
 *
 *	OTA_STATE_END     = 5,
 *	- Received data is checked if a start command is received,
 * 	- On verification, STATE is changed to OTA_STATE_IDLE and breaks out of this function with return OK, else Error.
 *
 * @retval 	: Enum ENM_OTA_RET_
 * 		  0 - Error
 * 		  1 - OK
 * @params	:
 * uint8_t *buf	: character buffer that is updated with RS232 data
 * uint16_t len : Length of received buffer
 *****************************************************************/
static ENM_OTA_RET_ ota_process_data( uint8_t *buf, uint16_t len )
{
  ENM_OTA_RET_ ret = ENM_OTA_RET_ERR;
  do
    {
      if( ( buf == NULL ) || ( len == 0u) )
	{
	  printf("BUFFER EMPTY!!\n");
	  break;
	}

      //Check we received OTA Abort command
      STU_OTA_COMMAND_ *cmd = (STU_OTA_COMMAND_*)buf;
      if( (cmd->packet_type == ENM_OTA_PKT_TYP_CMD) &&
	  cmd->cmd == ENM_OTA_CMD_ABORT )
	{
	  //received OTA Abort command. Stop the process
	  printf("ABORT CMD!!\n");
	  break;
	}

      switch( ota_state )
      {
	case ENM_OTA_STATE_IDLE:
	  {
	    printf("STATE=IDLE!!\n");
	    ret = ENM_OTA_RET_OK;
	  }
	  break;
	case ENM_OTA_STATE_START:
	  {
	    //STU_OTA_COMMAND_ *cmd = (STU_OTA_COMMAND_*)buf;
	    if( (cmd->packet_type == ENM_OTA_PKT_TYP_CMD) &&
		(cmd->cmd == ENM_OTA_CMD_START) )
	      {
		printf("STATE=START!!\n");
		ota_state = ENM_OTA_STATE_HEADER;
		ret = ENM_OTA_RET_OK;
	      }
	    ota_send_resp(ret);
	  }
	  break;

	case ENM_OTA_STATE_HEADER:
	  {
	    STU_OTA_HEADER_ *header = (STU_OTA_HEADER_*)buf;
	    if( header->packet_type == ENM_OTA_PKT_TYP_HEADER )
	      {
		/* Store FW details */
		ota_fw_total_size = header->package_size;
		//ota_fw_crc        = header->meta_data.package_crc;
		printf("FW size= %ld\n",ota_fw_total_size);
		ota_state = ENM_OTA_STATE_BKP_HDR;
		ret = ENM_OTA_RET_OK;
	      }
	    ota_send_resp(ret);
	    break;

	  }

	case ENM_OTA_STATE_BKP_HDR:
	  /*
	   * PC application will send me a command to send backup
	   */
	  //STU_OTA_COMMAND_ *cmd = (STU_OTA_COMMAND_*)buf;
	  if( (cmd->packet_type == ENM_OTA_PKT_TYP_CMD) &&
	      (cmd->cmd == ENM_OTA_CMD_RQST_FW_HDR) )
	    {
	      printf("STATE=APP_HDR!!\n");
	      volatile uint32_t *checkAPP = (uint32_t *)(OTA_APP_FLASH_ADDR);
	      volatile uint32_t *checkEnd = (uint32_t *)(OTA_APP_CONFIG_END_ADDR);

	      if(*checkAPP == (uint32_t) (&_estack))
		{
		  //APPLICATION IS PRESENT, SO GET PC ACK AND TRANSFER APPLICATION TO PC
		  App_pakage_size = (*checkEnd - OTA_APP_FLASH_ADDR);
		}
	      ret = send_ota_fw_header(App_pakage_size);
	      if (!ret)
		{
		  printf("send_ota_fw_header Err\n");
		  ret = ENM_OTA_RET_ERR;
		  break;
		}
	      ret = ENM_OTA_RET_OK;
	      if (App_pakage_size==0)
		{
		  ota_state = ENM_OTA_STATE_DATA;
		}
	      else
		ota_state = ENM_OTA_STATE_BKP_DATA;
	    }
	  break;

	case ENM_OTA_STATE_BKP_DATA:
	  //STU_OTA_COMMAND_ *cmd = (STU_OTA_COMMAND_*)buf;
	  if( (cmd->packet_type == ENM_OTA_PKT_TYP_CMD) &&
	      (cmd->cmd == ENM_OTA_CMD_RQST_FW_DATA) )
	    {
	      ret = ota_backup_application(&App_pakage_size);
	      if(ret)
		{
		  ota_state = ENM_OTA_STATE_DATA;
		}
	    }
	  break;

	case ENM_OTA_STATE_DATA:
	  STU_OTA_DATA_     *data     = (STU_OTA_DATA_*)buf;
	  uint16_t          data_len = data->data_len;
	  HAL_StatusTypeDef ex;

	  printf("OTA STATE=DATA\n");
	  if( data->packet_type == ENM_OTA_PKT_TYP_DATA )
	    {
	      /* write the chunk to the Flash (App location) */
	      ex = write_data_to_flash_app( buf+4, data_len, ( ota_fw_received_size == 0) );
	      if( ex == HAL_OK )
		{
		  printf("RCVD [%ld/%ld]",ota_fw_received_size/DEF_OTA_DATA_MAX_SIZE, ota_fw_total_size/DEF_OTA_DATA_MAX_SIZE);
		  if( ota_fw_received_size >= ota_fw_total_size )
		    {
		      // before moving to END state, write the end of application at the address location
		      Write_APP_End_Address(((uint32_t) OTA_APP_FLASH_ADDR) + ota_fw_total_size);
		      //received the full data. So, move to end
		      ota_state = ENM_OTA_STATE_END;

		    }
		  ret = ENM_OTA_RET_OK;

		}
	    }
	  ota_send_resp(ret);
	  break;
	case ENM_OTA_STATE_END:
	  //STU_OTA_COMMAND_ *cmd = (STU_OTA_COMMAND_*)buf;
	  if( (cmd->packet_type == ENM_OTA_PKT_TYP_CMD) && (cmd->cmd == ENM_OTA_CMD_END) )
	    {

	      printf("\nSTATE=END\n");
	      ota_state = ENM_OTA_STATE_IDLE;
	      ret = ENM_OTA_RET_OK;
	    }
	  break;
	default:
	  /* Should not come here */
	  ret = ENM_OTA_RET_ERR;

	  break;
      }
    }while( false );

  return ret;
}

/****************************************************************
 * @brief	:
 * Function to send response to PC tool
 * Based on the OTA_RET value, ACK OR NACK are sent to PC
 * TODO : CRC May be included in future, ensure it is also implemented in PC tool
 *
 * @retval	:
 * None
 *
 * @parms	:
 * OTA_return value from previous functions.
 *****************************************************************/
static void ota_send_resp( ENM_OTA_RET_ returnvalue)
{
  STU_OTA_RESP_ *rsp = (STU_OTA_RESP_*)Tx_Buffer;
  memset( Tx_Buffer, 0, sizeof(STU_OTA_RESP_) );		//INSETAD OF CLEARING ENTIRE 1033 BYTES, NOW WE CLEAR ONLY REQUIRED BYTES
  rsp->sof 			=DEF_OTA_SOF;
  rsp->packet_type 		=ENM_OTA_PKT_TYP_RESPONSE;
  rsp->data_len 		= sizeof(STU_OTA_RESP_);
  rsp->status			= (returnvalue == ENM_OTA_RET_OK)? DEF_OTA_ACK : DEF_OTA_NACK;
  rsp-> crc 			= 0;
  rsp->eof			= DEF_OTA_EOF;

  //send response
  ota_send_chars(rsp->data_len,Tx_Buffer);
}

/****************************************************************
 * @brief	:
 * Low level Function to send characters over UART
 *
 * @retval	:
 * None
 *
 * @parms	:
 * length and the character buffer to be written
 *****************************************************************/
static void ota_send_chars(uint16_t len, uint8_t *buf)
{
  //send response
    for (uint16_t c=0; c<(len);c++)
      {
        while( !( USART1->SR & USART_SR_TXE) ) {};	//wait till transmit buffer is empty
        USART1->DR = buf[c];
      }
}

/****************************************************************
 * @brief	:
 * Main function to Write data to the Application's actual flash location.
 *
 *****************************************************************/
static HAL_StatusTypeDef write_data_to_flash_app( uint8_t *data,
						  uint16_t data_len, bool is_first_block )
{
  HAL_StatusTypeDef ret;

  do
    {
      ret = HAL_FLASH_Unlock();
      if( ret != HAL_OK )
	{
	  break;
	}

      //No need to erase every time. Erase only the first time.
      if( is_first_block )
	{

	  printf("Erasing Flash...\n");

	  //Erase the Flash
	  FLASH_EraseInitTypeDef EraseInitStruct;
	  uint32_t SectorError;

	  EraseInitStruct.TypeErase     = FLASH_TYPEERASE_SECTORS;
	  EraseInitStruct.Sector        = FLASH_SECTOR_5;
	  EraseInitStruct.NbSectors     = 1;                    //erase 2 sectors(5,6)
	  EraseInitStruct.VoltageRange  = FLASH_VOLTAGE_RANGE_3;

	  ret = HAL_FLASHEx_Erase( &EraseInitStruct, &SectorError );
	  if( ret != HAL_OK )
	    {
	      break;
	    }
	}

      for(int i = 0; i < data_len; i++ )
	{
	  ret = HAL_FLASH_Program( FLASH_TYPEPROGRAM_BYTE,
				   (OTA_APP_FLASH_ADDR + ota_fw_received_size),
				   data[i]
	  );
	  if( ret == HAL_OK )
	    {
	      //update the data count
	      ota_fw_received_size += 1;
	    }
	  else
	    {

	      printf("FLASH ERROR!\n");
	      break;
	    }
	}

      if( ret != HAL_OK )
	{
	  break;
	}

      ret = HAL_FLASH_Lock();
      if( ret != HAL_OK )
	{
	  break;
	}
    }while( false );

  return ret;
}

/****************************************************************
 * @brief	:
 * Main function to Write Newly loaded APPLICATION size details
 * on to a designated flash location (also defined in linker script).
 *
 *****************************************************************/
static HAL_StatusTypeDef Write_APP_End_Address(uint32_t end_of_application)
{
  HAL_StatusTypeDef ret;
  do
    {
      ret = HAL_FLASH_Unlock();
      if( ret != HAL_OK )
	{
	  break;
	}

      //Erase the Flash
      FLASH_EraseInitTypeDef EraseInitStruct;
      uint32_t SectorError;

      EraseInitStruct.TypeErase     = FLASH_TYPEERASE_SECTORS;
      EraseInitStruct.Sector        = FLASH_SECTOR_2;
      EraseInitStruct.NbSectors     = 1;                    //erase 1 SECTOR
      EraseInitStruct.VoltageRange  = FLASH_VOLTAGE_RANGE_3;
      ret = HAL_FLASHEx_Erase( &EraseInitStruct, &SectorError );
      if( ret != HAL_OK )
	{
	  break;
	}

      ret = HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD,
			      0x08008000,end_of_application);
      if( ret != HAL_OK )
	{
	  printf("END WRITE FAILED.");
	}
      ret = HAL_FLASH_Lock();
      if( ret != HAL_OK )
	{
	  break;
	}

    }while(0);

  return ret;
}

/****************************************************************
 * @brief	:
 * Function to read response from PC tool
 * on to a designated flash location (also defined in linker script).
 *
 *****************************************************************/
static ENM_OTA_RET_ ota_is_response_received()
{
  ENM_OTA_RET_ ret = ENM_OTA_RET_ERR;
  uint16_t pktlen = receive_data();	//get data
  if(pktlen>0)
    {
      STU_OTA_RESP_ *rsp = (STU_OTA_RESP_*)Rx_Buffer;
      if(rsp->packet_type == ENM_OTA_PKT_TYP_RESPONSE && rsp->status== DEF_OTA_ACK)
	{
	  return ENM_OTA_RET_OK;
	}
    }
  return ret;
}

/****************************************************************
 * @brief	:
 * Function to export meta data of running firmware application to PC
 *
 *****************************************************************/
ENM_OTA_RET_ send_ota_fw_header(uint32_t fwsize)
{
    memset(Tx_Buffer, 0, DEF_OTA_PACKET_MAX_SIZE);
    STU_OTA_HEADER_ *ota_header = (STU_OTA_HEADER_ *)Tx_Buffer;
    ENM_OTA_RET_ ret = ENM_OTA_RET_OK;
    uint16_t len = sizeof(STU_OTA_HEADER_);


    ota_header->sof = DEF_OTA_SOF;
    ota_header->packet_type = ENM_OTA_PKT_TYP_HEADER;
    ota_header->data_len = sizeof(fwsize); // changed from meta data to ota header
    ota_header->package_size = fwsize;
    ota_header->crc = crc32b(Tx_Buffer, len - 5);
    ota_header->eof = DEF_OTA_EOF;

    // send OTA Header
    ota_send_chars(len, Tx_Buffer);

    ret = ota_is_response_received();
    printf("OTA HDR - ack[ret=%d]\n", ret);
    return ret;
}

/****************************************************************
 * @brief	:
 * Main Function to export running firmware application to PC
 *
 *****************************************************************/
ENM_OTA_RET_ ota_backup_application(uint32_t *app_size)
{
  uint16_t len=0;
  uint16_t dataIndex=0;
  ENM_OTA_RET_ ret = ENM_OTA_RET_ERR;
  uint32_t size = 0;
  volatile uint32_t *memptr = (uint32_t *)(OTA_APP_FLASH_ADDR);
  for (uint32_t i = 0; i < *app_size;)
    {
      len=0;
      dataIndex=0;
      if ((*app_size - i) >= DEF_OTA_DATA_MAX_SIZE)
	{
	  size = DEF_OTA_DATA_MAX_SIZE;
	}
      else
	{
	  size = *app_size - i;
	}
      memset( Tx_Buffer, 0, DEF_OTA_PACKET_MAX_SIZE );
      STU_OTA_DATA_ *ota_data = (STU_OTA_DATA_ *)Tx_Buffer;
      ota_data->sof = DEF_OTA_SOF;
      ota_data->packet_type = ENM_OTA_PKT_TYP_DATA;
      ota_data->data_len = size;
      // crc and eof are added in the packet below*/
      len=4;

      while(dataIndex<size)
	{
	  flashmem_data.data_word = *memptr++;
	  Tx_Buffer[len++] = flashmem_data.data_bytes[0];
	  Tx_Buffer[len++] = flashmem_data.data_bytes[1];
	  Tx_Buffer[len++] = flashmem_data.data_bytes[2];
	  Tx_Buffer[len++] = flashmem_data.data_bytes[3];
	  dataIndex +=4;
	}
      //len += size;

      //Adding CRC
      crc_data.data_word = crc32b(Tx_Buffer, len); // Add CRC
      Tx_Buffer[len++] = crc_data.data_bytes[0];
      Tx_Buffer[len++] = crc_data.data_bytes[1];
      Tx_Buffer[len++] = crc_data.data_bytes[2];
      Tx_Buffer[len++] = crc_data.data_bytes[3];

      //END OF FLAG
      Tx_Buffer[len++] = DEF_OTA_EOF;


      //SEND THIS DATA
      ret = ota_send_APP_Data(len, Tx_Buffer);
      if (!ret)
	{
	  printf("send_ota_data Err [i=%ld]\n", i);
	  ret = ENM_OTA_RET_ERR;
	  break;
	}

      printf("[%ld/%ld]\r\n", i / DEF_OTA_DATA_MAX_SIZE, *app_size / DEF_OTA_DATA_MAX_SIZE);

      //ret = ota_send_chars(size, Tx_Buffer); send_ota_data(myserialport.comport, &APP_BIN[i], size);

      i += size;
    }
  return ret;
}

/****************************************************************
 * @brief	:
 * Main Function to export running firmware application to PC in chunks
 * It is called by function "ota_backup_application"
 *
 *****************************************************************/
ENM_OTA_RET_ ota_send_APP_Data(uint16_t data_len, uint8_t *buf)
{
  ENM_OTA_RET_ ret = ENM_OTA_RET_ERR;
  ota_send_chars(data_len, buf);

  ret = ota_is_response_received();
  printf("OTA HEADER - ack received[ret = %d]\n", ret);
  return ret;
}
