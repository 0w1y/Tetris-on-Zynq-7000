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
         xil_printf("Erase OLED\r\n");
		   OLED_SetDrawColor(&myDevice2, 1);
		   OLED_SetDrawMode(&myDevice2, OledModeSet);
		   OLED_MoveTo(&myDevice2, 0, 0);
		   OLED_FillRect(&myDevice2, 127, 31);
		   OLED_Update(&myDevice2);

         xil_printf("%c\r\n", (char) key);
         char keyType = (char) key;
         char keyChar[2] = {keyType, '\0'};
         OLED_ClearBuffer(&myDevice2);
         OLED_MoveTo(&myDevice2, 0, 0);
         drawPiece(key);
         OLED_Update(&myDevice2);
         usleep(100000); 
         last_key = key;
      } else if (status == KYPD_MULTI_KEY && status != last_status)
         xil_printf("Error: Multiple keys pressed\r\n");

      last_status = status;
      usleep(1000);

   }
}

void drawPiece(int randNum){
   OLED_SetDrawColor(&myDevice2, 0);
   switch(randNum){
      //OLED_MoveTo(&myDevice, 60, 16); // 128 x 32
      case 0: //I
         OLED_MoveTo(&myDevice, 60, 10); // 128 x 32
         OLED_FillRect(&myDevice, 57, 22);
      case 1: //J
         OLED_MoveTo(&myDevice, 60, 16); // 128 x 32
         OLED_FillRect(&myDevice, 51, 19); 
         OLED_MoveTo(&myDevice, 54, 13); 
         OLED_FillRect(&myDevice, 51, 16); 
      case 2: //L
         OLED_MoveTo(&myDevice, 60, 13); // 128 x 32
         OLED_FillRect(&myDevice, 51, 16); 
         OLED_MoveTo(&myDevice, 54, 16); 
         OLED_FillRect(&myDevice, 51, 19); 
      case 3: //O
         OLED_MoveTo(&myDevice, 60, 13); // 128 x 32
         OLED_FillRect(&myDevice, 54, 19); 
      case 4: //S
         OLED_MoveTo(&myDevice, 60, 16); // 128 x 32
         OLED_FillRect(&myDevice, 57, 22); 
         OLED_MoveTo(&myDevice, 57, 13); 
         OLED_FillRect(&myDevice, 54, 19); 
      case 5: //T
         OLED_MoveTo(&myDevice, 60, 13); // 128 x 32
         OLED_FillRect(&myDevice, 57, 22); 
         OLED_MoveTo(&myDevice, 57, 16); 
         OLED_FillRect(&myDevice, 54, 19); 
      case 6: //Z
         OLED_MoveTo(&myDevice, 60, 13); // 128 x 32
         OLED_FillRect(&myDevice, 57, 16); 
         OLED_MoveTo(&myDevice, 57, 16); 
         OLED_FillRect(&myDevice, 53, 22); 
      default:
         xil_printf("Char not a piece\r\n");
   }
   OLED_Update(&myDevice);
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
