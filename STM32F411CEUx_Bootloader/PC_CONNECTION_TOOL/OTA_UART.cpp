
#include "OTA_UART.h"
#include "cppRS232.h"
#include "main.h"

// char *ExecutablePath;

uint8_t READ_DATA_BUF[DEF_OTA_PACKET_MAX_SIZE];
uint8_t DATA_BUF[DEF_OTA_PACKET_MAX_SIZE];
uint8_t APP_BIN[DEF_OTA_MAX_FW_SIZE];

/* Firmware Size that we have received */
static uint32_t ota_fw_received_size = 0;

/* Firmware Total Size that we are going to receive */
static uint32_t ota_fw_total_size = 0;

/* Firmware image's CRC32 */
static uint32_t ota_fw_crc;

/* Initial OTA state */
ENM_OTA_STATE_ ota_state = ENM_OTA_STATE_IDLE;

ENM_OTA_RET_ ota_begin(int argc, char *argv[])
{
    FILE *Fptr = NULL;
    SERIALPORT_ myserialport;
    ENM_OTA_RET_ ret = ENM_OTA_RET_OK;
    char bin_name[1024];
    uint32_t app_size = 0;
    do
    {
        /* STEP 1:
            CHECK IF ALL AGRUMENTS ARE PASSED TO THE APPLICATION OR NOT.
            Two arguments are required
                1. COM PORT NUMBER (ONLY)
                2. PATH OF FIRMWARE FILE (.BIN)
        */
        if (argc <= 2) // for now only com port
        {
            printf("Please feed the COM PORT number and the Application Image....!!!\n");
            printf("Example: .\\RS232FileTransfer.exe 4 ..\\..\\Application\\Debug\\Blinky.bin");
            ret = ENM_OTA_RET_ERR;
            break;
        }

        // get the COM port Number
        myserialport.comport = atoi(argv[1]) - 1;
        // Add more data of serial port if NOT default in the structure;
        strcpy(bin_name, argv[2]);

        /* STEP 2:
        Open COM port and set status.
        If Open is failed then exit the application with an error
        */
        printf("Opening COM%d...\n", myserialport.comport + 1);
        if (!cppRS232_OpenComport(&myserialport))
        {
            printf("Can not open comport\n");
            ret = ENM_OTA_RET_ERR;
            break;
        }
        printf("COM%d IS OPENED AND CONFIGURED SUCCESSFULLY.\n", myserialport.comport + 1);
        ota_state = ENM_OTA_STATE_START;

        /* OTA-STEP-1: SEND OTA START COMMAND AND WAIT FOR ACKNOWLEDGEMENT*/
        {
            ret = sent_ota_start_cmd(myserialport.comport, ENM_OTA_CMD_START);
            if (!ret)
            {
                ret = ENM_OTA_RET_ERR;
                break;
            }
        }

        /* OTA-STEP-2: SEND HEADER DETAILS OF PC BINARY FILE*/
        {
            ota_state = ENM_OTA_STATE_HEADER;
            printf("Opening Binary file : %s\n", bin_name);
            Fptr = fopen(bin_name, "rb");
            if (Fptr == NULL)
            {
                printf("Can not open %s\n", bin_name);
                ret = ENM_OTA_RET_ERR;
                break;
            }
            fseek(Fptr, 0L, SEEK_END);
            app_size = ftell(Fptr);
            fseek(Fptr, 0L, SEEK_SET);
            printf("File size = %d BYTES\n", app_size);

            // read the full image
            if (fread(APP_BIN, 1, app_size, Fptr) != app_size)
            {
                printf("App/FW read Error\n");
                ret = ENM_OTA_RET_ERR;
                break;
            }

            // Send OTA Header
            // meta_info ota_info;
            // uint32_t package_size = app_size;
            /* ota_info.package_crc = 0; // TODO: Add CRC  */

            ret = send_ota_header(myserialport.comport, app_size);
            if (!ret)
            {
                printf("send_ota_header Err\n");
                ret = ENM_OTA_RET_ERR;
                break;
            }
        }

        /* OTA-STEP-3: SEND COMMAND TO RECEIVE RUNNING FIRMWARE APPLICTAION*/
        {
            ota_state = ENM_OTA_STATE_BKP_HDR;
            ret = send_fw_request_cmd(myserialport.comport, ENM_OTA_CMD_RQST_FW_HDR);
            if (!ret)
            {
                ret = ENM_OTA_RET_ERR;
                break;
            }
        }
        /* OTA-STEP-3.1: START BACKUP OF FIRMWARE IF OTA FIRMWARE IS PRESENT*/
        {
            if (ota_fw_total_size > 0)
            {
                ota_state = ENM_OTA_STATE_BKP_DATA;
                printf("Output path of BINARY FILE will be saved at %s%s.BIN\n", GetMyPath(&argv[0]), Time2String());
                ret = Download_Firmware(myserialport.comport, GetMyPath(&argv[0]), Time2String());
                if (!ret)
                {
                    ret = ENM_OTA_RET_ERR;
                    break;
                }
            }
            delay(500); // give a small delay
        }

        /* OTA-STEP-4: DEPLOY FIRMWARE TO THE BOARD*/
        {
            ota_state = ENM_OTA_STATE_DATA;
            uint16_t size = 0;
            for (uint32_t i = 0; i < app_size;)
            {
                if ((app_size - i) >= DEF_OTA_DATA_MAX_SIZE)
                {
                    size = DEF_OTA_DATA_MAX_SIZE;
                }
                else
                {
                    size = app_size - i;
                }

                printf("[%d/%d]\r\n", i / DEF_OTA_DATA_MAX_SIZE, app_size / DEF_OTA_DATA_MAX_SIZE);

                ret = send_ota_data(myserialport.comport, &APP_BIN[i], size);
                if (!ret)
                {
                    printf("send_ota_data Err [i=%d]\n", i);
                    break;
                }
                i += size;
            }
            if (!ret)
            {
                ret = ENM_OTA_RET_ERR;
                break;
            }
        }

        /* OTA-STEP-5: SEND AN END COMMAND*/
        {
            ota_state = ENM_OTA_STATE_END;
            // send OTA END command
            ret = send_ota_end(myserialport.comport);
            if (!ret)
            {
                printf("send_ota_end Err\n");
                break;
            }
            else
            {
                printf("OTA END: ACKED\n");
            }
        }
    } while (0);

    /* OTA-STEP-6: CLEANUP, CLOSE APPLICATION BINARY FILE*/
    if (Fptr)
    {
        fclose(Fptr);
    }
    // Close COM port
    cppRS232_CloseComport(myserialport.comport);
    ota_state = ENM_OTA_STATE_IDLE;
    return ret;
}

ENM_OTA_RET_ send_fw_request_cmd(int comport, ENM_OTA_CMD_ cmd)
{
    ENM_OTA_RET_ ret = ENM_OTA_RET_ERR;
    ret = send_ota_cmd(comport, cmd);
    if (!ret)
    {
        printf("send_ota_request_fw_header Err\n");
        ret = ENM_OTA_RET_ERR;
        return ret;
    }

    /*  Response of this command is the APP size from MICRO*/
    uint16_t len = Receive_data(comport, sizeof(STU_OTA_HEADER_));
    STU_OTA_HEADER_ *header = (STU_OTA_HEADER_ *)DATA_BUF;
    if (len > 0 &&                                     // check if length is greater than zero
        header->sof == DEF_OTA_SOF &&                  // SOF is received
        header->eof == DEF_OTA_EOF &&                  // EOF is received
        header->packet_type == ENM_OTA_PKT_TYP_HEADER) // Packet type is HEADER
    {
        ota_fw_total_size = header->package_size;
        ret = ENM_OTA_RET_OK;
    }
    else
    {
        ret = ENM_OTA_RET_ERR;
    }

    ret = send_ota_response(comport, ret);

    printf("OTA REQUEST FIRMWARE HEADER - ack received [ret = %d]\n", ret);
    return ret;
}

ENM_OTA_RET_ sent_ota_start_cmd(int comport, ENM_OTA_CMD_ cmd)
{
    ENM_OTA_RET_ ret = ENM_OTA_RET_OK;

    ret = send_ota_cmd(comport, cmd);
    if (!ret)
    {
        printf("send_ota_start Err\n");
        ret = ENM_OTA_RET_ERR;
        return ENM_OTA_RET_ERR;
    }
    // Check for Acknowledgement from the command
    if (!is_ack_resp_received(comport))
    {
        // Received NACK
        printf("OTA START : NACK\n");
        ret = ENM_OTA_RET_ERR;
        return ENM_OTA_RET_ERR;
    }
    printf("OTA START - ack received [ret = %d]\n", ret);

    return ret;
}

void delay(uint32_t us)
{
    us *= 10;
#ifdef _WIN32
    // Sleep(ms);
    __int64 time1 = 0, time2 = 0, freq = 0;

    QueryPerformanceCounter((LARGE_INTEGER *)&time1);
    QueryPerformanceFrequency((LARGE_INTEGER *)&freq);

    do
    {
        QueryPerformanceCounter((LARGE_INTEGER *)&time2);
    } while ((time2 - time1) < us);
#else
    usleep(us);
#endif
}

/* read the response */
bool is_ack_resp_received(int comport)
{
    bool is_ack = false;

    memset(DATA_BUF, 0, DEF_OTA_PACKET_MAX_SIZE);

    uint16_t len = cppRS232_PollComport(comport, DATA_BUF, sizeof(STU_OTA_RESP_));

    if (len > 0)
    {
        STU_OTA_RESP_ *resp = (STU_OTA_RESP_ *)DATA_BUF;
        if (resp->packet_type == ENM_OTA_PKT_TYP_RESPONSE && resp->status == DEF_OTA_ACK) // TODO: Add CRC check
        {
            // ACK received
            is_ack = true;
        }
    }
    return is_ack;
}

uint16_t Receive_data(int comport, int datalen)
{
    memset(DATA_BUF, 0, DEF_OTA_PACKET_MAX_SIZE);

    uint16_t len = cppRS232_PollComport(comport, DATA_BUF, datalen);
    return len;
}

/* Build the OTA START command */
ENM_OTA_RET_ send_ota_cmd(int comport, ENM_OTA_CMD_ cmd)
{
    uint16_t len;
    STU_OTA_COMMAND_ *ota_cmd = (STU_OTA_COMMAND_ *)DATA_BUF;
    ENM_OTA_RET_ ret = ENM_OTA_RET_OK;
    len = sizeof(STU_OTA_COMMAND_);
    memset(DATA_BUF, 0, DEF_OTA_PACKET_MAX_SIZE);

    ota_cmd->sof = DEF_OTA_SOF;
    ota_cmd->packet_type = ENM_OTA_PKT_TYP_CMD;
    ota_cmd->data_len = 1;
    ota_cmd->cmd = cmd;
    ota_cmd->crc = crc32b(DATA_BUF, len - 5); // Added CRC
    ota_cmd->eof = DEF_OTA_EOF;

    // send OTA START
    for (int i = 0; i < len; i++)
    {
        delay(1);
        if (RS232_SendByte(comport, DATA_BUF[i]))
        {
            // some data missed.
            printf("OTA START : Send Err\n");
            ret = ENM_OTA_RET_ERR;
            break;
        }
    }
    return ret;
}

ENM_OTA_RET_ send_ota_response(int comport, ENM_OTA_RET_ returnvalue)
{
    uint16_t len;
    ENM_OTA_RET_ ret = ENM_OTA_RET_OK;
    memset(DATA_BUF, 0, DEF_OTA_PACKET_MAX_SIZE);
    STU_OTA_RESP_ *ota_rsp = (STU_OTA_RESP_ *)DATA_BUF;
    len = sizeof(STU_OTA_RESP_);
    ota_rsp->sof = DEF_OTA_SOF;
    ota_rsp->packet_type = ENM_OTA_PKT_TYP_RESPONSE;
    ota_rsp->data_len = 1;
    ota_rsp->status = (returnvalue == ENM_OTA_RET_OK) ? DEF_OTA_ACK : DEF_OTA_NACK;
    ota_rsp->crc = 0x00; // TODO : ADD CRC
    ota_rsp->eof = DEF_OTA_EOF;

    // send OTA Response
    for (int i = 0; i < len; i++)
    {
        delay(1);
        if (RS232_SendByte(comport, DATA_BUF[i]))
        {
            // some data missed.
            printf("OTA RESPONSE : Send Err\n");
            ret = ENM_OTA_RET_ERR;
            break;
        }
    }
    return ret;
}

/* Build and send the OTA Header */
ENM_OTA_RET_ send_ota_header(int comport, uint32_t fwsize)
{
    uint16_t len;
    uint32_t crcvalue = 0;
    STU_OTA_HEADER_ *ota_header = (STU_OTA_HEADER_ *)DATA_BUF;
    ENM_OTA_RET_ ret = ENM_OTA_RET_OK;
    len = sizeof(STU_OTA_HEADER_);
    memset(DATA_BUF, 0, DEF_OTA_PACKET_MAX_SIZE);

    ota_header->sof = DEF_OTA_SOF;
    ota_header->packet_type = ENM_OTA_PKT_TYP_HEADER;
    ota_header->data_len = sizeof(fwsize); // changed from meta data to ota header
    ota_header->package_size = fwsize;
    ota_header->crc = crc32b(DATA_BUF, len - 5); // TODO: Added CRC
    ota_header->eof = DEF_OTA_EOF;

    // memcpy(&ota_header->meta_data, ota_info, sizeof(meta_info));

    // crcvalue = crc32b(DATA_BUF, len - 5);
    // memcpy(&ota_header->crc, &crcvalue, sizeof(crcvalue));
    // printf("Header crc is: %d \n",ota_header->crc);

    // send OTA Header
    for (int i = 0; i < len; i++)
    {
        delay(1);
        if (RS232_SendByte(comport, DATA_BUF[i]))
        {
            // some data missed.
            printf("OTA HEADER : Send Err\n");
            ret = ENM_OTA_RET_ERR;
            break;
        }
    }

    if (ret)
    {
        if (!is_ack_resp_received(comport))
        {
            // Received NACK
            printf("OTA HEADER : NACK\n");
            ret = ENM_OTA_RET_ERR;
        }
    }
    printf("OTA HEADER - ack received[ret = %d]\n", ret);

    return ret;
}

/* Build and send the OTA Data */
ENM_OTA_RET_ send_ota_data(int comport, uint8_t *data, uint16_t data_len)
{
    uint16_t len;
    STU_OTA_DATA_ *ota_data = (STU_OTA_DATA_ *)DATA_BUF;
    ENM_OTA_RET_ ret = ENM_OTA_RET_OK;

    memset(DATA_BUF, 0, DEF_OTA_PACKET_MAX_SIZE);

    ota_data->sof = DEF_OTA_SOF;
    ota_data->packet_type = ENM_OTA_PKT_TYP_DATA;
    ota_data->data_len = data_len;
    // crc and eof are added in the packet below*/
    len = 4;

    // Copy the data
    memcpy(&DATA_BUF[len], data, data_len);
    len += data_len;
    uint32_t crc = crc32b(DATA_BUF, len); // TODO: Add CRC

    // Copy the crc
    memcpy(&DATA_BUF[len], (uint8_t *)&crc, sizeof(crc));
    len += sizeof(crc);
    // printf("DATA crc is: %d \n", crc);
    // Add the EOF
    DATA_BUF[len] = DEF_OTA_EOF;
    len++;

    // send OTA Data
    for (int i = 0; i < len; i++)
    {
        delay(500);
        // printf("Sending DATA[%d]: %d \n", i,DATA_BUF[i]);
        if (RS232_SendByte(comport, DATA_BUF[i]))
        {
            // some data missed.
            printf("OTA DATA : Send Err\n");
            ret = ENM_OTA_RET_ERR;
            break;
        }
    }

    if (ret)
    {
        if (!is_ack_resp_received(comport))
        {
            // Received NACK
            printf("OTA DATA : NACK\n");
            ret = ENM_OTA_RET_ERR;
        }
        else
            printf("ACKED, OTA DATA [ex = %d]\n", ret);
    }

    return ret;
}

/* Build and Send the OTA END command */
ENM_OTA_RET_ send_ota_end(int comport)
{
    uint16_t len;
    STU_OTA_COMMAND_ *ota_end = (STU_OTA_COMMAND_ *)DATA_BUF;
    ENM_OTA_RET_ ret = ENM_OTA_RET_OK;
    len = sizeof(STU_OTA_COMMAND_);
    memset(DATA_BUF, 0, DEF_OTA_PACKET_MAX_SIZE);

    ota_end->sof = DEF_OTA_SOF;
    ota_end->packet_type = ENM_OTA_PKT_TYP_CMD;
    ota_end->data_len = 1;
    ota_end->cmd = ENM_OTA_CMD_END;
    ota_end->crc = crc32b(DATA_BUF, len - 5);
    ; // TODO: Add CRC
    ota_end->eof = DEF_OTA_EOF;

    // printf("END crc is: %d \n",ota_end->crc);

    printf("Sending OTA End command \n");
    // send OTA END
    for (int i = 0; i < len; i++)
    {
        delay(1);
        if (RS232_SendByte(comport, DATA_BUF[i]))
        {
            // some data missed.
            printf("OTA END : Send Err\n");
            ret = ENM_OTA_RET_ERR;
            break;
        }
    }
    if (!ret)
    {
        if (!is_ack_resp_received(comport))
        {
            // Received NACK
            printf("OTA END : NACK\n");
            ret = ENM_OTA_RET_ERR;
        }
        else
        {
            printf("OTA END ACKED [ex = %d]\n", ret);
        }
    }
    printf("OTA END [ex = %d]\n", ret);
    return ret;
}

ENM_OTA_RET_ Download_Firmware(int comport, char *FolderPath, char *FileName)
{
    ENM_OTA_RET_ ret = ENM_OTA_RET_OK;

    /* FIRST SEND THE COMMAND TO START DOWNLOAD FIRMWARE */
    delay(500);
    ret = send_fw_data_request_cmd(comport, ENM_OTA_CMD_RQST_FW_DATA);
    if(!ret)
    {
        ret = ENM_OTA_RET_ERR;
        return ret;
    }
    FILE *read_file_ptr;
    uint16_t len = 0;
    char filepath[strlen(FolderPath) + strlen(FileName) + 9]; // = GetMyPath(&argv[0]);// + FileName + (char *)(".BIN"));
    snprintf(filepath, sizeof(filepath), "%sAPP_%s.BIN", FolderPath, FileName);

    read_file_ptr = fopen(filepath, "wb"); // w for write, b for binary
    if (!read_file_ptr)
    {
        printf("Error in creating file. Aborting.\n");
        ret = ENM_OTA_RET_ERR;
        return ret;
    }
    while (ota_fw_received_size < ota_fw_total_size)
    {
        /* Concept is that we need to Read only those number of bytes which are required */
        uint32_t ReadSize = (ota_fw_total_size - ota_fw_received_size);
        if(ReadSize>=1024)
        {
            ReadSize = DEF_OTA_DATA_MAX_SIZE;
        }
        ReadSize +=DEF_OTA_DATA_OVERHEAD;
        memset(READ_DATA_BUF, 0, DEF_OTA_PACKET_MAX_SIZE);
        len = cppRS232_PollComport(comport, READ_DATA_BUF, ReadSize);
        STU_OTA_DATA_ *data = (STU_OTA_DATA_ *)READ_DATA_BUF;
        uint16_t data_len = data->data_len;

        if (len > 0 &&
            data->sof == DEF_OTA_SOF &&
            data->packet_type == ENM_OTA_PKT_TYP_DATA &&
            READ_DATA_BUF[data_len + 9 - 1] == DEF_OTA_EOF)
        {
            /* Extracting the firmware data from received packet and writing to file*/
            uint8_t mydata[data_len];
            len=4;
            for (uint16_t i=0;i<data_len;i++)
            {
                mydata[i] = READ_DATA_BUF[len++]; 
            }
            fwrite(mydata, data_len, 1, read_file_ptr);
            /* TODO : Error in fwrite is not considered yet */
            ota_fw_received_size += data_len;
        }
        else
        {
            printf("DID NOT RECEIVE DATA\n");
            ret = ENM_OTA_RET_ERR;
            // break;
        }
        if (!send_ota_response(comport, ret))
        {
            ret = ENM_OTA_RET_ERR;
            break;
        }
    }
    // finally close the file irrespective of status
    fclose(read_file_ptr);

    return ret;
}


ENM_OTA_RET_ send_fw_data_request_cmd(int comport, ENM_OTA_CMD_ cmd)
{
    ENM_OTA_RET_ ret = ENM_OTA_RET_OK;
    ret = send_ota_cmd(comport, cmd);
    if (!ret)
    {
        printf("send_ota_request_fw_data Err\n");
        ret = ENM_OTA_RET_ERR;
    }
    return ret;
}

void print_READ_DATA_BUF(int BUF_SIZE)
{
    for (unsigned int i = 0; i < BUF_SIZE; i = i + 4)
    {
        printf("Address [%08X] : 0x %02X %02X %02X %02X\n", i, READ_DATA_BUF[i], READ_DATA_BUF[i + 1], READ_DATA_BUF[i + 2], READ_DATA_BUF[i + 3]);
    }
}