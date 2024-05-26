#ifndef INC_OTA_UART_H_
#define INC_OTA_UART_H_

#include "main.h"
#include "cppRS232.h"




void print_READ_DATA_BUF(int BUF_SIZE);


#define DEF_OTA_SOF  0xAA    // Start of Frame
#define DEF_OTA_EOF  0xBB    // End of Frame
#define DEF_OTA_ACK  0x00    // ACK
#define DEF_OTA_NACK 0x01    // NACK

#define DEF_OTA_DATA_MAX_SIZE ( 1024 )  //Maximum data Size
#define DEF_OTA_DATA_OVERHEAD (    9 )  //data overhead
#define DEF_OTA_PACKET_MAX_SIZE ( DEF_OTA_DATA_MAX_SIZE + DEF_OTA_DATA_OVERHEAD )
#define DEF_OTA_MAX_FW_SIZE ( 1024 * 128 )


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
  ENM_OTA_STATE_IDLE    	= 1,
  ENM_OTA_STATE_START   	= 2,
  ENM_OTA_STATE_HEADER  	= 3,
  ENM_OTA_STATE_BKP_HDR		= 11,
  ENM_OTA_STATE_BKP_DATA 	= 12,
  ENM_OTA_STATE_DATA    	= 7,
  ENM_OTA_STATE_END     	= 8,
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
  ENM_OTA_CMD_START 		      = 1,    // OTA Start command
  ENM_OTA_CMD_END   		      = 2,    // OTA End command
  ENM_OTA_CMD_ABORT 		      = 3,    // OTA Abort command
  ENM_OTA_CMD_RQST_FW_HDR     = 4,	//PC Requests running FW details
  ENM_OTA_CMD_RQST_FW_DATA	  = 5	//PC Requests running FW data
}ENM_OTA_CMD_;

/*
 * OTA meta info
 */
// typedef struct
// {
//   uint32_t package_size;
//   /*uint32_t package_crc;
//   uint32_t reserved1;
//   uint32_t reserved2;*/
// }__attribute__((packed)) meta_info;

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

///*
// * OTA Header format - OLD
// *
// * __________________________________________
// * |     | Packet |     | Header |     |     |
// * | SOF | Type   | Len |  Data  | CRC | EOF |
// * |_____|________|_____|________|_____|_____|
// *   1B      1B     2B     16B     4B    1B			==> 25 BYTES
// */
//typedef struct
//{
//  uint8_t     sof;
//  uint8_t     packet_type;
//  uint16_t    data_len;
//  meta_info   meta_data;
//  uint32_t    crc;
//  uint8_t     eof;
//}__attribute__((packed)) STU_OTA_HEADER_;


/*
 * OTA Header format - NEW
 *
 * __________________________________________
 * |     | Packet |     |   FW 	 |     |     |
 * | SOF | Type   | Len |  Size  | CRC | EOF |
 * |_____|________|_____|________|_____|_____|
 *   1B      1B     2B     4B     4B    1B			==> 13 BYTES
 *   */
typedef struct
{
  uint8_t     sof;
  uint8_t     packet_type;
  uint16_t    data_len;
  uint32_t   package_size;
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


ENM_OTA_RET_ send_ota_cmd(int comport, ENM_OTA_CMD_ cmd);
uint32_t crc32b(unsigned char *message,uint16_t usDataLen);
void delay(uint32_t us);
bool is_ack_resp_received( int comport);
ENM_OTA_RET_ Download_Firmware(int comport, char * FolderPath, char * FileName);
ENM_OTA_RET_ send_ota_data(int comport, uint8_t *data, uint16_t data_len);
//ENM_OTA_RET_ send_ota_header(int comport, meta_info *ota_info);
ENM_OTA_RET_ send_ota_header(int comport, uint32_t fwsize);
ENM_OTA_RET_ send_ota_end(int comport);
ENM_OTA_RET_ ota_begin(int argc, char *argv[]);
uint16_t Receive_data(int comport , int datalen);
//ENM_OTA_RET_ send_ota_response(int comport, uint8_t feedback);
ENM_OTA_RET_ send_ota_response(int comport,  ENM_OTA_RET_ returnvalue);
ENM_OTA_RET_ sent_ota_start_cmd(int comport, ENM_OTA_CMD_ cmd);
ENM_OTA_RET_ send_fw_request_cmd(int comport, ENM_OTA_CMD_ cmd);
ENM_OTA_RET_ send_fw_data_request_cmd(int comport, ENM_OTA_CMD_ cmd);
#endif