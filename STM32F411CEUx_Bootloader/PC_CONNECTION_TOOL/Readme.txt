This logic has beeen implemented using the https://www.teuniz.net/RS-232/

Run the below command to compile the application.

	gcc PCTool_ota_update.c RS232\rs232.c -IRS232 -Wall -Wextra -o2 -o etx_ota_app


Once you have build the application, then run the application like below.

		.\etx_ota_app.exe COMPORT_NUM APPLICATION_BIN_PATH
		
		example:
			.\etx_ota_app.exe 8 ..\..\Application\Debug\Blinky.bin






.\etx_ota_app.exe 4 "D:\STM32CUBEIDE_WORKSPACE\STM32F411CEUx_Boot_Application\Release\STM32F411CEUx_Boot_Application.bin"