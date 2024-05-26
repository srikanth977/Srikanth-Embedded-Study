///////////////////////////////////////////VERSION 3 - CHILI BEGIN
// //SOURCE: https://stackoverflow.com/questions/68624350/readfile-code-using-win32-api-to-read-from-serial-port-very-slow/68628274#comment138391030_68628274

#include "main.h"
#include "cppRS232.h"
#include "OTA_UART.h"


ENM_OTA_RET_ ota_begin(int argc, char *argv[]);



// SEE MAIN.H for serial port initial configurations
int main(int argc, char *argv[])
{
    // uint32_t app_size = 0;
    // uint8_t APP_BIN[DEF_OTA_MAX_FW_SIZE];
    
    // //sprintf(filepath,"%s\\APP%s.BIN",GetMyPath(&argv[0]),Time2String());
    // char bin_name[1024];
    ENM_OTA_RET_ ret = ENM_OTA_RET_OK;
    // SERIALPORT_ myserialport;
    // FILE *Fptr = NULL;
    // do
    // {
        ret = ota_begin(argc, argv);

        // {
        //     if (argc <= 2) // for now only com port
        //     {
        //         printf("Please feed the COM PORT number and the Application Image....!!!\n");
        //         printf("Example: .\\RS232FileTransfer.exe 4 ..\\..\\Application\\Debug\\Blinky.bin");
        //         ret = -1;
        //         break;
        //     }

        //     // get the COM port Number
        //     myserialport.comport = atoi(argv[1]) - 1;
        //     // Add more data of serial port if NOT default in the structure;
        //     strcpy(bin_name, argv[2]);
        // }

        // /* STEP 2:
        //     Open COM port and set status.
        //     If Open is failed then exit the application with an error
        //     */
        // {
        //     printf("Opening COM%d...\n", myserialport.comport + 1);
        //     if (!cppRS232_OpenComport(&myserialport))
        //     {
        //         printf("Can not open comport\n");
        //         ret = -1;
        //         break;
        //     }
        //     printf("COM%d IS OPENED AND CONFIGURED SUCCESSFULLY.\n", myserialport.comport + 1);
        // }

        // send OTA Start command

        // ret = ota_begin(myserialport.comport,);
        //  ret = send_ota_start(myserialport.comport);
        //  if (ret < 0)
        //  {
        //      printf("send_ota_start Err\n");
        //      break;
        //  }

        // if (!BackupApp(myserialport.comport))
        // {
        //     printf("Error in backup.\n");
        //     ret = -1;
        //     break;
        // }

        // printf("Opening Binary file : %s\n", bin_name);

        // Fptr = fopen(bin_name, "rb");

        // if (Fptr == NULL)
        // {
        //     printf("Can not open %s\n", bin_name);
        //     ret = -1;
        //     break;
        // }

        // fseek(Fptr, 0L, SEEK_END);
        // uint32_t app_size = ftell(Fptr);
        // fseek(Fptr, 0L, SEEK_SET);

        // printf("File size = %d\n", app_size);

        // // Send OTA Header
        // meta_info ota_info;
        // ota_info.package_size = app_size;
        // ota_info.package_crc = 0; // TODO: Add CRC

        // ret = send_ota_header(myserialport.comport, &ota_info);
        // if (ret < 0)
        // {
        //     printf("send_ota_header Err\n");
        //     break;
        // }

    //     uint16_t size = 0;
    //     // printf("SARTED to send application");
    //     for (uint32_t i = 0; i < app_size;)
    //     {
    //         if ((app_size - i) >= DEF_OTA_DATA_MAX_SIZE)
    //         {
    //             size = DEF_OTA_DATA_MAX_SIZE;
    //         }
    //         else
    //         {
    //             size = app_size - i;
    //         }

    //         printf("[%d/%d]\r\n", i / DEF_OTA_DATA_MAX_SIZE, app_size / DEF_OTA_DATA_MAX_SIZE);

    //         ret = send_ota_data(myserialport.comport, &APP_BIN[i], size);
    //         if (ret < 0)
    //         {
    //             printf("send_ota_data Err [i=%d]\n", i);
    //             break;
    //         }

    //         i += size;
    //     }
    //     // printf("ENDED to send application\n");

    //     if (ret < 0)
    //     {
    //         break;
    //     }

    //     // send OTA END command
    //     ret = send_ota_end(myserialport.comport);
    //     if (ret < 0)
    //     {
    //         printf("send_ota_end Err\n");
    //         break;
    //     }
    //     else
    //     {
    //         printf("OTA END: ACKED\n");
    //     }
    // } while (0);
    if (!ret)
    {
        printf("Runtime error, program exited.\n");
    }
    //cppRS232_CloseComport(myserialport.comport);
    return ret;
}


char *ReplaceString(char *s)
{
    int i=0;
    while (s[i] != '\n')
    {
        // check for c1 and replace
        if (s[i] == ' ' || s[i]==':')
            s[i] = '_';
        i += 1;
    }
    s[i]='\0';
    return s;
}

char *Time2String()
{
    char *fulltime;
    char *finaltime;
    time_t rawtime;
    struct tm *timeinfo;
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    fulltime = asctime(timeinfo);
    finaltime = ReplaceString(fulltime);
    // printf("Full time is : %s\n", fulltime);
    return finaltime;
}

char *GetMyPath(char *argv[])
{
    char *mypath = strdup(argv[0]);
    char *last_slash = strrchr(mypath, '\\');
    if (last_slash)
    {
#if !PRESERVE_LAST_SLASH
        *(last_slash + 1) = '\0';
#else
        *last_slash = '\0';
#endif
        //printf("Executable path is %s", mypath);
    }
    return mypath;
}

///////////////////////////////////////////VERSION 3 - CHILI END
