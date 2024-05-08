#include "OTA_UART.h"
#include "main.h"
#include <stdbool.h>
#include "st7735.h"
#include "fonts.h"


extern UART_HandleTypeDef huart1;

ENM_OTA_STATE_ ota_state = ENM_OTA_STATE_IDLE;
/* Firmware Total Size that we are going to receive */
static uint32_t ota_fw_total_size;
/* Firmware image's CRC32 */
static uint32_t ota_fw_crc;
/* Firmware Size that we have received */
static uint32_t ota_fw_received_size;

ENM_OTA_RET_ ota_download_and_flash( void );
ENM_OTA_RET_ ota_begin(void);
//ENM_OTA_PKT_TYP_ receive_data(void);
uint16_t receive_data(void);
static void ota_send_resp( uint8_t type );
static ENM_OTA_RET_ ota_process_data( uint8_t *buf, uint16_t len );
static HAL_StatusTypeDef write_data_to_flash_app( uint8_t *data,uint16_t data_len, bool is_first_block );
uint32_t crc32b(unsigned char *message,uint16_t usDataLen);

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

ENM_OTA_RET_ ota_begin(void)
{
	uint32_t crcvalue=0;
	ENM_OTA_RET_ ret = ENM_OTA_RET_ERR;

	//clear the buffer
	uint16_t PacketLength;

	/* Reset the variables */
	ota_fw_total_size    = 0u;
	ota_fw_received_size = 0u;
	ota_fw_crc           = 0u;
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
		if(ret == ENM_OTA_RET_OK)
		{
			ota_send_resp(DEF_OTA_ACK);
			ret = ENM_OTA_RET_OK;
		}
		else
		{
			ota_send_resp(DEF_OTA_NACK);
			ret = ENM_OTA_RET_ERR;
			break;
		}
	}while(ota_state != ENM_OTA_STATE_IDLE);
	return ret;
}

/**
  * @brief Process the received data from UART4.
  * @param buf buffer to store the received data
  * @param max_len maximum length to receive
  * @retval ETX_OTA_EX_
  */
static ENM_OTA_RET_ ota_process_data( uint8_t *buf, uint16_t len )
{
//	uint32_t crcvalue=0;
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
    	  STU_OTA_COMMAND_ *cmd = (STU_OTA_COMMAND_*)buf;
//    	  /* TODO : check CRC in receive_data() */
//    	  crcvalue= crc32b(buf,len-5);
//    	  if (crcvalue != cmd->crc)
//    	  {
//    		  ret= ENM_OTA_RET_ERR;
//    	  }
        if( (cmd->packet_type == ENM_OTA_PKT_TYP_CMD) &&
        		(cmd->cmd == ENM_OTA_CMD_START) )
        {
        	printf("STATE=START!!\n");
        	ota_state = ENM_OTA_STATE_HEADER;
        	ret = ENM_OTA_RET_OK;
        }
      }
      break;

      case ENM_OTA_STATE_HEADER:
      {
        STU_OTA_HEADER_ *header = (STU_OTA_HEADER_*)buf;
        if( header->packet_type == ENM_OTA_PKT_TYP_HEADER )
        {
//        	/* TODO : check CRC in receive_data() */
//        	crcvalue= crc32b(buf,len-5);
//        	if (crcvalue != header->crc)
//        	{
//        		ret= ENM_OTA_RET_ERR;
//        		break;
//        	}
        	/* Store FW details */
        	ota_fw_total_size = header->meta_data.package_size;
        	ota_fw_crc        = header->meta_data.package_crc;
        	printf("FW size= %ld\n",ota_fw_total_size);
        	ota_state = ENM_OTA_STATE_DATA;
        	ret = ENM_OTA_RET_OK;
        }
      }
      break;

      case ENM_OTA_STATE_DATA:
      {
        STU_OTA_DATA_     *data     = (STU_OTA_DATA_*)buf;
        uint16_t          data_len = data->data_len;
        HAL_StatusTypeDef ex;

        printf("OTA STATE=DATA\n");
        if( data->packet_type == ENM_OTA_PKT_TYP_DATA )
        {
//        	/* TODO : check CRC in receive_data() */
//        	crcvalue= crc32b(buf,len-5);
//        	if (crcvalue != data->crc)
//        	{
//        		ret= ENM_OTA_RET_ERR;
//        		break;
//        	}
          /* write the chunk to the Flash (App location) */
          ex = write_data_to_flash_app( buf+4, data_len, ( ota_fw_received_size == 0) );
          if( ex == HAL_OK )
          {
        	  printf("Received [%ld/[%ld]",ota_fw_received_size/DEF_OTA_DATA_MAX_SIZE, ota_fw_total_size/DEF_OTA_DATA_MAX_SIZE);

            if( ota_fw_received_size >= ota_fw_total_size )
            {
              //received the full data. So, move to end
              ota_state = ENM_OTA_STATE_END;

            }
            ret = ENM_OTA_RET_OK;
          }
        }
      }
      break;

      case ENM_OTA_STATE_END:
      {

    	  STU_OTA_COMMAND_ *cmd = (STU_OTA_COMMAND_*)buf;

        if( (cmd->packet_type == ENM_OTA_PKT_TYP_CMD) && (cmd->cmd == ENM_OTA_CMD_END) )
        {

        	printf("\nSTATE=END\n");


//        	//TODO: Very full package CRC
//        	crcvalue= crc32b(buf,len-5);
//        	if (crcvalue != cmd->crc)
//        	{
//        		ret= ENM_OTA_RET_ERR;
//        		break;
//        	}

        	ota_state = ENM_OTA_STATE_IDLE;
        	ret = ENM_OTA_RET_OK;
          }
//        else ota_send_resp(DEF_OTA_NACK,ENM_OTA_PKT_TYP_CMD);
      }
      break;

      default:
      {
        /* Should not come here */
        ret = ENM_OTA_RET_ERR;
      }
      break;
    };
  }while( false );

  return ret;
}

/**
  * @brief Send the response.
  * @param type ACK or NACK
  * @retval none
  */
static void ota_send_resp( uint8_t feedback)
{
	STU_OTA_RESP_ *rsp = (STU_OTA_RESP_*)Rx_Buffer;
	memset( Rx_Buffer, 0, sizeof(STU_OTA_RESP_) );		//INSETAD OF CLEARING ENTIRE 1033 BYTES, NOW WE CLEAR ONLY REQUIRED BYTES
	rsp->sof 			=DEF_OTA_SOF;
	rsp->packet_type 	=ENM_OTA_PKT_TYP_RESPONSE;
	rsp->data_len 		= sizeof(STU_OTA_RESP_);
	rsp->status			= feedback;
	rsp-> crc 			= 0;		/* TODO  */
	rsp->eof			= DEF_OTA_EOF;

  //send response
  for (unsigned char c=0; c<(rsp->data_len);c++)
  {
	  while( !( USART1->SR & USART_SR_TXE) ) {};	//wait till transmit buffer is empty
	  USART1->DR = Rx_Buffer[c];
  }
  //HAL_UART_Transmit(&huart2, (uint8_t *)&rsp, sizeof(ETX_OTA_RESP_), HAL_MAX_DELAY);
}

/**
  * @brief Write data to the Application's actual flash location.
  * @param data data to be written
  * @param data_len data length
  * @is_first_block true - if this is first block, false - not first block
  * @retval HAL_StatusTypeDef
  */
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
