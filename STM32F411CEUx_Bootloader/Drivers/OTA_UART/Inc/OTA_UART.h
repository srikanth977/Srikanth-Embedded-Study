/*
 * OTA_UART.h
 *
 *  Created on: May 2, 2024
 *      Author: Srikanth Chilivery
 */

#ifndef INC_OTA_UART_H_
#define INC_OTA_UART_H_

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define DEF_OTA_DATA_MAX_SIZE ( 1024 )  //Maximum data Size
#define DEF_OTA_DATA_OVERHEAD (    9 )  //data overhead
#define DEF_OTA_PACKET_MAX_SIZE ( DEF_OTA_DATA_MAX_SIZE + DEF_OTA_DATA_OVERHEAD )
#define DEF_OTA_SOF  0xAA    // Start of Frame
#define DEF_OTA_EOF  0xBB    // End of Frame
#define DEF_OTA_ACK  0x00    // ACK
#define DEF_OTA_NACK 0x01    // NACK
/* Application's Flash Address
 * Size of BOOTLOADER IS 32K, so based on the calculations, below is the address
 * Currently it looks like we cannot make the chip to jump to variably any address
 * So lets keep a fixed address for now. */
#define OTA_APP_FLASH_ADDR 0x08020000

/*
 * BUFFER FOR STORING UART DATA  */
extern uint8_t Rx_Buffer [ DEF_OTA_PACKET_MAX_SIZE ];


/*
 *BASIC STRUCTURES TO IDENTIFY PACKET TYPES
 */
typedef enum
{
  ENM_OTA_PKT_TYP_CMD		= 1,    // Command
  ENM_OTA_PKT_TYP_DATA		= 2,    // Data
  ENM_OTA_PKT_TYP_HEADER	= 3,    // Header
  ENM_OTA_PKT_TYP_RESPONSE	= 4,    // Response
  ENM_OTA_PKT_TYP_END		= 5,     // End
  ENM_OTA_PKT_TYP_ERR		= 0
}ENM_OTA_PKT_TYP_;
/*
 * OTA Command format
 *
 * ________________________________________
 * |     | Packet |     |     |     |     |
 * | SOF | Type   | Len | CMD | CRC | EOF |
 * |_____|________|_____|_____|_____|_____|
 *   1B      1B     2B    1B     4B    1B   = 10 BYTES
 */
typedef struct
{
  uint8_t   sof;
  uint8_t   packet_type;
  uint16_t  data_len;
  uint8_t   cmd;
  uint32_t  crc;
  uint8_t   eof;
}__attribute__((packed)) STU_OTA_COMMAND_;

/*
 * Return codes
 */
typedef enum
{
	ENM_OTA_RET_OK       = 1,    // Success
	ENM_OTA_RET_ERR      = 0,    // Failure
}ENM_OTA_RET_;


/*
 * OTA process state
 */
typedef enum
{
  ENM_OTA_STATE_IDLE    = 1,
  ENM_OTA_STATE_START   = 2,
  ENM_OTA_STATE_HEADER  = 3,
  ENM_OTA_STATE_DATA    = 4,
  ENM_OTA_STATE_END     = 5,
}ENM_OTA_STATE_;



/*
 * OTA Response format
 *
 * _____________________________________________________________________
 * |     | Packet 					|     	|        	|		|     	|
 * | SOF | Type   					| Len 	| Status 	| CRC 	| EOF	|
 * |	 |(Return rcvd packet type)	|		| (ACK/NACK)|		|		|
 * |_____|__________________________|_______|___________|_______|_______|
 *   1B      		1B     				2B      	1B     4B    	1B		10 BYTES
 */
typedef struct
{
  uint8_t   sof;
  uint8_t   packet_type;
  uint16_t  data_len;
  uint8_t   status;
  uint32_t  crc;
  uint8_t   eof;
}__attribute__((packed)) STU_OTA_RESP_;


/*
 * OTA Commands
 */
typedef enum
{
  ENM_OTA_CMD_START = 1,    // OTA Start command
  ENM_OTA_CMD_END   = 2,    // OTA End command
  ENM_OTA_CMD_ABORT = 3,    // OTA Abort command
}ENM_OTA_CMD_;


/*
 * OTA meta info (INFORMATION RELATED TO THE APPLICATION BINARY)
 */
typedef struct
{
  uint32_t package_size;
  uint32_t package_crc;
  uint32_t reserved1;
  uint32_t reserved2;
}__attribute__((packed)) stu_meta_info;
/*
 * OTA Header format
 *
 * __________________________________________
 * |     | Packet |     | Header |     |     |
 * | SOF | Type   | Len |  Data  | CRC | EOF |
 * |_____|________|_____|________|_____|_____|
 *   1B      1B     2B     16B     4B    1B			==> 25 BYTES
 */
typedef struct
{
  uint8_t     sof;
  uint8_t     packet_type;
  uint16_t    data_len;
  stu_meta_info   meta_data;
  uint32_t    crc;
  uint8_t     eof;
}__attribute__((packed)) STU_OTA_HEADER_;

/*
 * OTA Data format
 *
 * __________________________________________
 * |     | Packet |     |        |     |     |
 * | SOF | Type   | Len |  Data  | CRC | EOF |
 * |_____|________|_____|________|_____|_____|
 *   1B      1B     2B    nBytes   4B    1B
 */
typedef struct
{
  uint8_t     sof;
  uint8_t     packet_type;
  uint16_t    data_len;
  uint8_t     *data;
// CRC & EOF are not considered in the Structure but they are embedded in the Data buffer sent by PCTool
}__attribute__((packed)) STU_OTA_DATA_;


/* Not used for now, let it be for now */
typedef struct
{
	uint16_t pktlen;
	ENM_OTA_PKT_TYP_  pkttype;
}__attribute__((packed)) STU_OTA_PACKET_TYPE_RETURN_;
#endif /* INC_OTA_UART_H_ */
