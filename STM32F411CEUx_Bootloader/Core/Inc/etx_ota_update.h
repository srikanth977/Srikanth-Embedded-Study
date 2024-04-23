/*
 * etx_ota_update.h
 *
 *  Created on: 26-Jul-2021
 *      Original Author	: EmbeTronicX
 *      Modified by		: Srikanth Chilivery
 */

#include <stdint.h>

#ifndef INC_ETX_OTA_UPDATE_H_
#define INC_ETX_OTA_UPDATE_H_

/* Flag definitions */
#define ETX_OTA_SOF  0x2a    // Start of Frame
#define ETX_OTA_EOF  0x23    // End of Frame
#define ETX_OTA_ACK  0x00    // ACK
#define ETX_OTA_NACK 0x01    // NACK

/* Application's Flash Address
 * Size of BOOTLOADER IS 32K, so based on the calculations, below is the address*/
#define ETX_APP_FLASH_ADDR 0x08020000

/* Definitions for Communication over UART */
#define ETX_OTA_DATA_MAX_SIZE ( 1024 )  //Maximum data Size
#define ETX_OTA_DATA_OVERHEAD (    9 )  //data overhead
#define ETX_OTA_PACKET_MAX_SIZE ( ETX_OTA_DATA_MAX_SIZE + ETX_OTA_DATA_OVERHEAD )

///*
// * Exception codes
// */
//typedef enum
//{
//  ETX_OTA_EX_OK       = 0,    // Success
//  ETX_OTA_EX_ERR      = 1,    // Failure
//}ETX_OTA_EX_;
//
///*
// * OTA process state
// */
//typedef enum
//{
//  ETX_OTA_STATE_IDLE    = 0,
//  ETX_OTA_STATE_START   = 1,
//  ETX_OTA_STATE_HEADER  = 2,
//  ETX_OTA_STATE_DATA    = 3,
//  ETX_OTA_STATE_END     = 4,
//}ETX_OTA_STATE_;
//
/*
 * Packet type
 */
typedef enum
{
  ETX_OTA_PACKET_TYPE_CMD       = 0,    // Command
  ETX_OTA_PACKET_TYPE_DATA      = 1,    // Data
  ETX_OTA_PACKET_TYPE_HEADER    = 2,    // Header
  ETX_OTA_PACKET_TYPE_RESPONSE  = 3,    // Response
}ETX_OTA_PACKET_TYPE_;
//
///*
// * OTA Commands
// */
//typedef enum
//{
//  ETX_OTA_CMD_START = 0,    // OTA Start command
//  ETX_OTA_CMD_END   = 1,    // OTA End command
//  ETX_OTA_CMD_ABORT = 2,    // OTA Abort command
//}ETX_OTA_CMD_;
//
///*
// * OTA meta info
// */
//typedef struct
//{
//  uint32_t package_size;
//  uint32_t package_crc;
//  uint32_t reserved1;
//  uint32_t reserved2;
//}__attribute__((packed)) meta_info;
//
//typedef struct
//{
//  uint8_t sof_received;
//}__attribute__((packed)) data_in_info;
//

///*
// * OTA Command format
// *
// * ________________________________________
// * |     | Packet |     |     |     |     |
// * | SOF | Type   | Len | CMD | CRC | EOF |
// * |_____|________|_____|_____|_____|_____|
// *   1B      1B     2B    1B     4B    1B
// */
//typedef struct
//{
//  uint8_t   sof;
//  uint8_t   packet_type;
//  uint16_t  data_len;
//  uint8_t   cmd;
//  uint32_t  crc;
//  uint8_t   eof;
//}__attribute__((packed)) ETX_OTA_COMMAND_;

///*
// * OTA Header format
// *
// * __________________________________________
// * |     | Packet |     | Header |     |     |
// * | SOF | Type   | Len |  Data  | CRC | EOF |
// * |_____|________|_____|________|_____|_____|
// *   1B      1B     2B     16B     4B    1B
// */
//typedef struct
//{
//  uint8_t     sof;
//  uint8_t     packet_type;
//  uint16_t    data_len;
//  meta_info   meta_data;
//  uint32_t    crc;
//  uint8_t     eof;
//}__attribute__((packed)) ETX_OTA_HEADER_;

/*
 * OTA Data format. Changed by Srikanth Chilivery
 *
 * _____________________________________________________________________
 * |     | Packet |     		|        	|     		|     	|		|
 * | SOF | Type   | Chunk Len 	|  	FW SIZE	|	Data  	| 	CRC | 	EOF	|
 * |_____|________|_____________|___________|___________|_______|_______|
 *   1B      1B     	2B    		2B			nBytes   	4B    	1B
 */
typedef struct
{
  uint8_t     sof;
  uint8_t     packet_type;
  uint16_t    data_len;
  uint16_t	  fw_size;
  uint8_t     *data;
}__attribute__((packed)) ETX_OTA_DATA_;

/*
 * OTA Response format
 *
 * __________________________________________
 * |     | Packet |     |        |     |     |
 * | SOF | Type   | Len | Status | CRC | EOF |
 * |_____|________|_____|________|_____|_____|
 *   1B      1B     2B      1B     4B    1B
 */
typedef struct
{
  uint8_t   sof;
  uint8_t   packet_type;
  uint16_t  data_len;
  uint8_t   status;
  uint32_t  crc;
  uint8_t   eof;
}__attribute__((packed)) ETX_OTA_RESP_;

//ETX_OTA_EX_ etx_ota_download_and_flash( void );
#endif /* INC_ETX_OTA_UPDATE_H_ */
