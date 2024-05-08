/*
 * etx_ota_update_main.h
 *
 *  Created on: 26-Jul-2021
 *      Author: EmbeTronicX
 */

#ifndef INC_PCTOOL_OTA_UPDATE_H_
#define INC_PCTOOL_OTA_UPDATE_H_

#define DEF_OTA_SOF  0xAA    // Start of Frame
#define DEF_OTA_EOF  0xBB    // End of Frame
#define DEF_OTA_ACK  0x00    // ACK
#define DEF_OTA_NACK 0x01    // NACK

#define DEF_OTA_DATA_MAX_SIZE ( 1024 )  //Maximum data Size
#define DEF_OTA_DATA_OVERHEAD (    9 )  //data overhead
#define DEF_OTA_PACKET_MAX_SIZE ( DEF_OTA_DATA_MAX_SIZE + DEF_OTA_DATA_OVERHEAD )
#define DEF_OTA_MAX_FW_SIZE ( 1024 * 512 )


/*
 * Exception codes
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
 * Packet type
 */
typedef enum
{
  ENM_OTA_PKT_TYP_CMD     	= 1,    // Command
  ENM_OTA_PKT_TYP_DATA      = 2,    // Data
  ENM_OTA_PKT_TYP_HEADER    = 3,    // Header
  ENM_OTA_PKT_TYP_RESPONSE  = 4,    // Response
  ENM_OTA_PKT_TYP_END       = 5,     // End
  ENM_OTA_PKT_TYP_ERR		    = 0
}ENM_OTA_PKT_TYP_;

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
 * OTA meta info
 */
typedef struct
{
  uint32_t package_size;
  uint32_t package_crc;
  uint32_t reserved1;
  uint32_t reserved2;
}__attribute__((packed)) meta_info;

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
 * OTA Header format
 *
 * __________________________________________
 * |     | Packet |     | Header |     |     |
 * | SOF | Type   | Len |  Data  | CRC | EOF |
 * |_____|________|_____|________|_____|_____|
 *   1B      1B     2B      16B     4B    1B
 */
typedef struct
{
  uint8_t     sof;
  uint8_t     packet_type;
  uint16_t    data_len;
  meta_info   meta_data;
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
  // crc and eof are added in the packet */
}__attribute__((packed)) STU_OTA_DATA_;

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
}__attribute__((packed)) STU_OTA_RESP_;

#endif /* INC_PCTOOL_OTA_UPDATE_H_ */
