/******************************************************************************/
/*                                                                            */
/* PmodKYPD.c -- Demo for the use of the Pmod Keypad IP core                  */
/*                                                                            */
/******************************************************************************/
/* Author:   Mikel Skreen                                                     */
/* Copyright 2016, Digilent Inc.                                              */
/******************************************************************************/
/* File Description:                                                          */
/*                                                                            */
/* This demo continuously captures keypad data and prints a message to an     */
/* attached serial terminal whenever a positive edge is detected on any of    */
/* the sixteen keys. In order to receive messages, a serial terminal          */
/* application on your PC should be connected to the appropriate COM port for */
/* the micro-USB cable connection to your board's USBUART port. The terminal  */
/* should be configured with 8-bit data, no parity bit, 1 stop bit, and the   */
/* the appropriate Baud rate for your application. If you are using a Zynq    */
/* board, use a baud rate of 115200, if you are using a MicroBlaze system,    */
/* use the Baud rate specified in the AXI UARTLITE IP, typically 115200 or    */
/* 9600 Baud.                                                                 */
/*                                                                            */
/******************************************************************************/
/* Revision History:                                                          */
/*                                                                            */
/*    06/08/2016(MikelS):   Created                                           */
/*    08/17/2017(artvvb):   Validated for Vivado 2015.4                       */
/*    08/30/2017(artvvb):   Validated for Vivado 2016.4                       */
/*                          Added Multiple keypress error detection           */
/*    01/27/2018(atangzwj): Validated for Vivado 2017.4                       */
/*                                                                            */
/******************************************************************************/

#include "PmodKYPD.h"
#include "sleep.h"
#include "xil_cache.h"
#include "xparameters.h"

#include <stdio.h>
#include "PmodOLED.h"
#include "xil_printf.h"

void DemoInitialize();
void DemoRun();
void DemoCleanup();
void DisableCaches();
void EnableCaches();
void DemoSleep(u32 millis);

void clearOLED();
void drawPiece();

PmodKYPD myDevice;
PmodOLED myDevice2;

const u8 orientation = 0x0; // Set up for Normal PmodOLED(false) vs normal
                            // Onboard OLED(true)
const u8 invert = 0x0; // true = whitebackground/black letters
                       // false = black background /white letters

int main(void) {
   DemoInitialize();
   DemoRun();
   DemoCleanup();
   return 0;
}

// keytable is determined as follows (indices shown in Keypad position below)
// 12 13 14 15
// 8  9  10 11
// 4  5  6  7
// 0  1  2  3
#define DEFAULT_KEYTABLE "0FED789C456B123A"

void DemoInitialize() {
   EnableCaches();
   //void KYPD_begin(PmodKYPD *InstancePtr, u32 GPIO_Address);
   KYPD_begin(&myDevice, XPAR_PMODKYPD_0_AXI_LITE_GPIO_BASEADDR);
   KYPD_loadKeyTable(&myDevice, (u8*) DEFAULT_KEYTABLE);
   EnableCaches();
   OLED_Begin(&myDevice2, XPAR_PMODOLED_0_AXI_LITE_GPIO_BASEADDR,
		XPAR_PMODOLED_0_AXI_LITE_SPI_BASEADDR, orientation, invert);
}


void DemoRun() {
	int irow, ib, i;
    u8 *pat;
    char c;
    int posX, posY;
   u16 keystate;
   XStatus status, last_status = KYPD_NO_KEY;
   u8 key, last_key = 'x';
   // Initial value of last_key cannot be contained in loaded KEYTABLE string
   xil_printf("\nInitialize\r\n");
   Xil_Out32(myDevice.GPIO_addr, 0xF);

   pat = OLED_GetStdPattern(0);
   OLED_SetFillPattern(&myDevice2, pat);
   // Turn automatic updating off
   OLED_SetCharUpdate(&myDevice2, 0);
   usleep(10000);

   OLED_ClearBuffer(&myDevice2);
   OLED_SetCursor(&myDevice2, 0, 1);
   OLED_PutString(&myDevice2, "    Start");
   OLED_Update(&myDevice2);

   sleep(2);
   xil_printf("\nStart loop.\r\n");

   while (1) {
     // Capture state of each key
	 OLED_ClearBuffer(&myDevice2);
	 OLED_SetCursor(&myDevice2, 0, 0);
     keystate = KYPD_getKeyStates(&myDevice);

      // Determine which single key is pressed, if any
      status = KYPD_getKeyPressed(&myDevice, keystate, &key);

      // Print key detect if a new key is pressed or if status has changed
      if (status == KYPD_SINGLE_KEY
            && (status != last_status || key != last_key)) {
         char keyType = (char) key;
         char keyChar[2] = {keyType, '\0'}; //key to string
         xil_printf("Pressed: %c\r\n", keyType);
         OLED_ClearBuffer(&myDevice2);
         OLED_MoveTo(&myDevice2, 0, 0);
         int number = (int)keyType;

         //3 squares
         /*OLED_SetDrawColor(&myDevice2, 1);
         pat = OLED_GetStdPattern(1);
         OLED_SetFillPattern(&myDevice2, pat);
         OLED_MoveTo(&myDevice2, 0, 0); // 128 x 32
		  OLED_FillRect(&myDevice2, 10, 10);
		  OLED_MoveTo(&myDevice2, 10, 10);
		  OLED_FillRect(&myDevice2, 20, 20);
		  OLED_MoveTo(&myDevice2, 20, 10);
		  OLED_FillRect(&myDevice2, 30, 00);

		  //Cursor test
		  OLED_MoveTo(&myDevice2, 50, 10);
		  OLED_FillRect(&myDevice2, 60, 20);
		  OLED_MoveTo(&myDevice2, 60, 10);
		  OLED_FillRect(&myDevice2, 70, 20);
		  OLED_MoveTo(&myDevice2, 71, 11);
		  OLED_FillRect(&myDevice2, 80, 20);

		  OLED_MoveTo(&myDevice2, 82, 12);
		  OLED_FillRect(&myDevice2, 90, 20);
		  OLED_Update(&myDevice2);
		  sleep(2);
          clearOLED();*/

         //drawPiece(number);
         clearOLED();
         OLED_ClearBuffer(&myDevice2); //after clearBuffer, set color, pattern, etc. each time?
         OLED_SetDrawColor(&myDevice2, 1);
		 pat = OLED_GetStdPattern(1);
		 OLED_SetFillPattern(&myDevice2, pat);

         xil_printf("Z Piece \r\n");
         OLED_MoveTo(&myDevice2, 60, 13);
		  OLED_FillRect(&myDevice2, 58, 18);
		  OLED_MoveTo(&myDevice2, 57, 16);
		  OLED_FillRect(&myDevice2, 55, 21);

		  OLED_MoveTo(&myDevice2, 70, 13); // 3x3 ?
		  OLED_FillRect(&myDevice2, 68, 18);
		  OLED_Update(&myDevice2); // dont forget about update


		  sleep(2);

         last_key = key;
      } else if (status == KYPD_MULTI_KEY && status != last_status)
         xil_printf("Error: Multiple keys pressed\r\n");

      last_status = status;
      usleep(1000);

   }
}
void clearOLED(){
	u8 *pat;
	xil_printf("Erase OLED\r\n");
	   //OLED_SetDrawColor(&myDevice2, 0);
	   pat = OLED_GetStdPattern(0);
	   OLED_SetFillPattern(&myDevice2, pat);
	   OLED_SetDrawMode(&myDevice2, OledModeSet);
	   OLED_MoveTo(&myDevice2, 0, 0);
	   OLED_FillRect(&myDevice2, 127, 31);
	   OLED_Update(&myDevice2);
}


void DemoCleanup() {
   DisableCaches();
}

void EnableCaches() {
#ifdef __MICROBLAZE__
#ifdef XPAR_MICROBLAZE_USE_ICACHE
   Xil_ICacheEnable();
#endif
#ifdef XPAR_MICROBLAZE_USE_DCACHE
   Xil_DCacheEnable();
#endif
#endif
}

void DisableCaches() {
#ifdef __MICROBLAZE__
#ifdef XPAR_MICROBLAZE_USE_DCACHE
   Xil_DCacheDisable();
#endif
#ifdef XPAR_MICROBLAZE_USE_ICACHE
   Xil_ICacheDisable();
#endif
#endif
}
