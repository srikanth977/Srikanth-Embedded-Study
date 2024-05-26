#include <stdbool.h>
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <conio.h>
#include <stdint.h>
#include <time.h>
#include <tchar.h>
#include <iostream>
#include <chrono>
#include <ctime> 
using namespace std;

#ifndef INC_MAIN_H_
#define INC_MAIN_H_


typedef struct 
{
    //setting default values, can be modified in main function before function cppRS232_OpenComport
    int comport=4;      
    int baudrate = 9600;
    int bytesize =8;
    int parity = NOPARITY;
    int stopbit = ONESTOPBIT;
    int flowctrl = 0;
}SERIALPORT_;

char *GetMyPath(char *argv[]);
char *Time2String();
char *ReplaceString(char *s);
#endif