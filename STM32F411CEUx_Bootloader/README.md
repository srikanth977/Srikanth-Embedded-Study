# Bootloader for STM32F411CEUx
## Version : 1.0
### Introduction:
This is a simple bootloader designed with inspiration from youtube video series (links given below in References section).

### Design Abstract
I am using STM32F411CEUx Black Pill board for learning this project.

For burning the bootloader, I am using STLINK from a Nucleo board

Below are the peripherals being used
1. UART1 --> For transfering of application firmware using a PC tool
2. Serial Wire Viewer --> To view debug prints
3. PORT PA1 --> LED to indicate microcontroller is in bootmode
4. PORT PA2 --> LED to indicate microcontroller is in Application mode
5. PORT PA3 --> LED to blink in the application (Configured in Application code project)
6. PORT PA0 (Input) --> BUTTON to trigger fiwmware upgrade 


### Memory Layout
STM32F411CEUx has 512kb of Flash memory which is used for our application as below:
<table>
    <thead>
        <tr>
            <th>Block</th>
            <th>Name</th>
            <th>Block base addresses</th>
            <th>Size</th>
            <Th>Usage</Th>
        </tr>
    </thead>
    <tbody>
        <tr>
            <td rowspan=8>Main Memory</td>
            <td>Sector 0</td>
            <td>0x0800 0000 - 0x0800 3FFF</td>
            <td>16KB</td>
            <td rowspan=2>BOOTLOADER </td>
        </tr>
        <tr>
            <td>Sector 1</td>
            <td>0x0800 4000 - 0x0800 7FF</td>
            <td>16KB</td> 
        </tr>
        <tr>
            <td>Sector 2</td>
            <td>0x0800 8000 - 0x0800 BFFF</td>
            <td>16KB</td>
            <td rowspan=3>NOT USED (Reserved) </td>
        </tr>
        <tr>
            <td>Sector 3</td>
            <td>0x0800 C000 - 0x0800 FFFF</td>
            <td>16KB</td>
        </tr>
        <tr>
            <td>Sector 4</td>
            <td>0x0801 0000 - 0x0801 FFFF</td>
            <td>64KB</td>
        </tr>
        <tr>
            <td>Sector 5</td>
            <td>0x0802 0000 - 0x0803 FFFF</td>
            <td>128KB</td>
            <td>Application/Firmware </td>
        </tr>
        <tr>
            <td>Sector 6</td>
            <td>0x0804 0000 - 0x0805 FFFF</td>
            <td>128KB</td>
             <td rowspan=2>NOT USED (Reserved) </td>
        </tr>
        <tr>
            <td>Sector 7</td>
            <td>0x0806 0000 - 0x0807 FFFF</td>
            <td>128KB</td>
        </tr>
</table>

Flash linker file ```` STM32F411CEUX_FLASH.ld ```` is edited accordingly to mention the size of the application:
        
  ````FLASH    (rx)    : ORIGIN = 0x8000000,   LENGTH = 32K	/* Bootloader block size is set to 32KB */ ````

  ### Flow of the program:
  1. Flash the bootloader through STLINK
  2. Once powerup, press button connected to ````PORT PA0```` within 3 seconds of powerup.
  3. Initiate the binary transfer from the PC tool (attached in the project)
  4. In the reference design,
        First step is to establish handshake between PC tool and microcontroller board and then the FW is transmitted in chunks of 1KB every time.
5. But now in this version of the tool, handshaking is bypassed and directly FW is transmitted. This done to understand and get more ideas on the improvement of the firmware loading application to avoid any security/data integrity failures.
### PC tool:
PC tool that is used to transfer bytes over UART is present in folder ````PcTool_modified````

### References
A good resource for understanding of bootloader working and its design is present in folder ````REFERENCE````.

git - https://github.com/Embetronicx/STM32-Bootloader/tree/main/Bootloader_Example

Web version - https://embetronicx.com/bootloader-tutorials/

Video version - https://www.youtube.com/watch?v=jzo7z2gNBgg&list=PLArwqFvBIlwHRgPtsQAhgZavlp42qpkiG

