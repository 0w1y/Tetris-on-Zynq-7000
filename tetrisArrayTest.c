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

void Initialize();
//void KypdOLEDDemo();
void Cleanup();
void DisableCaches();
void EnableCaches();
void DemoSleep(u32 millis);
void game();

// New functions
void boardInit();

PmodKYPD myDevice;
PmodOLED myDevice2;

const u8 orientation = 0x0; // Set up for Normal PmodOLED(false) vs normal
                            // Onboard OLED(true)
const u8 invert = 0x0; // true = whitebackground/black letters
                       // false = black background /white letters

// Global vars
int gameOver = FALSE;
#define BOARD_ROWS 20
#define BOARD_COLS 10
u8 board[BOARD_ROWS][BOARD_COLS] = {0}; // 0 = empty, 1 = filled

int main(void) {
   Initialize();
   //KypdOLEDDemo();
   Cleanup();
   game();
   //gameOver();
   Cleanup();
   return 0;
}

// keytable is determined as follows (indices shown in Keypad position below)
// 12 13 14 15
// 8  9  10 11
// 4  5  6  7
// 0  1  2  3
#define DEFAULT_KEYTABLE "0FED789C456B123A"

void Initialize() {
   EnableCaches();
   //void KYPD_begin(PmodKYPD *InstancePtr, u32 GPIO_Address);
   KYPD_begin(&myDevice, XPAR_PMODKYPD_0_AXI_LITE_GPIO_BASEADDR);
   KYPD_loadKeyTable(&myDevice, (u8*) DEFAULT_KEYTABLE);
   OLED_Begin(&myDevice2, XPAR_PMODOLED_0_AXI_LITE_GPIO_BASEADDR,
		XPAR_PMODOLED_0_AXI_LITE_SPI_BASEADDR, orientation, invert);
}

void drawCell(int row, int col, int filled) { // applies to tetris cells only
    int pixelX = col * 3; // 3 pixels per block
    int pixelY = row * 3; // 3 pixels per block
    OLED_MoveTo(&myDevice, pixelX, pixelY)
    if (filled) {
        OLED_FillRect(&myDevice2, pixelX + 3, pixelY + 3);
    } else {
    	OLED_SetDrawColor(&myDevice, 0); //draw color clear
    	OLED_FillRect(&myDevice2, pixelX + 3, pixelY + 3);
    }
}

typedef struct {
    u8 shape[4][4]; // 1 = block, 0 = empty
    int size;       // e.g., 3 or 4
} Piece;
// Pieces: I, J, L, O, S, T, Z
Piece I = {
    {{0,0,0,0},
     {1,1,1,1},
     {0,0,0,0},
     {0,0,0,0}},
    4
};

Piece J = {
    {{0,0,1,0},
     {0,0,1,0},
     {0,1,1,0},
     {0,0,0,0}},
    4
};

Piece L = {
    {{0,1,0,0},
     {0,1,0,0},
     {0,1,1,0},
     {0,0,0,0}},
    4
};

Piece O = {
    {{0,0,0,0},
     {0,1,1,0},
     {0,1,1,0},
     {0,0,0,0}},
    4
};

Piece S = {
    {{0,0,0,0},
     {0,0,1,1},
     {0,1,1,0},
     {0,0,0,0}},
    4
};

Piece T = {
    {{0,0,0,0},
     {0,0,1,0},
     {0,1,1,1},
     {0,0,0,0}},
    4
};

Piece Z = {
    {{0,0,0,0},
     {0,1,1,0},
     {0,0,1,1},
     {0,0,0,0}},
    4
};

int checkCollision(Piece *p, int row, int col) {
    for(int r = 0; r < p->size; r++) {
        for(int c = 0; c < p->size; c++) {
            if(p->shape[r][c]) {
                int boardRow = row + r;
                int boardCol = col + c;
                if(boardRow >= BOARD_ROWS || 
                   boardCol < 0 || boardCol >= BOARD_COLS || 
                   board[boardRow][boardCol])
                    return TRUE;
            }
        }
    }
    return FALSE;
}

Piece rotatePiece(Piece p) { 
    Piece rotated = p;
    for(int r=0;r<4;r++)
        for(int c=0;c<4;c++)
            rotated.shape[c][3-r] = p.shape[r][c];
    return rotated;
}

void placePiece(Piece *p, int row, int col) {
    for(int r=0;r<p->size;r++)
        for(int c=0;c<p->size;c++)
            if(p->shape[r][c])
                board[row+r][col+c] = 1;
}

void clearLines() {
    for(int r = 0; r < BOARD_ROWS; r++) {
        int full = TRUE;
        for(int c = 0; c < BOARD_COLS; c++)
            if(board[r][c] == 0) full = FALSE;
        if(full) {
            // Shift rows above down
            for(int rr = r; rr > 0; rr--)
                for(int c = 0; c < BOARD_COLS; c++)
                    board[rr][c] = board[rr-1][c];
            // Clear top row
            for(int c = 0; c < BOARD_COLS; c++)
                board[0][c] = 0;
        }
    }
}

void drawBoard() {
    for(int r=0;r<BOARD_ROWS;r++)
        for(int c=0;c<BOARD_COLS;c++)
            drawCell(r, c, board[r][c]);
    OLED_Update(&myDevice2);
}

void game() {
    // Initialize game state
    int pieceRow, pieceCol;
    Piece currentPiece;
    int gameOver = FALSE;
    u16 keystate;
    XStatus status, last_status = KYPD_NO_KEY;
    u8 key;

    // Array of pieces (you should define them elsewhere)
    Piece pieces[7] = {I, J, L, O, S, T, Z};
    int pieceIndex;

    // Clear board at start
    for(int r=0; r<BOARD_ROWS; r++)
        for(int c=0; c<BOARD_COLS; c++)
            board[r][c] = 0;

    drawBoard();

    while(!gameOver) {
        // Spawn new random piece
        pieceIndex = rand() % 7;
        currentPiece = pieces[pieceIndex];
        pieceRow = 0;
        pieceCol = (BOARD_COLS - currentPiece.size)/2; // center horizontally

        // Check if spawn collides: game over
        if(checkCollision(&currentPiece, pieceRow, pieceCol)) {
            xil_printf("Game Over!\r\n");
            gameOver = TRUE;
            break;
        }

        int pieceLanded = FALSE;

        while(!pieceLanded) {
            // --- Handle user input ---
            keystate = KYPD_getKeyStates(&myDevice);
            status = KYPD_getKeyPressed(&myDevice, keystate, &key);

            if(status == KYPD_SINGLE_KEY && status != last_status) {
                switch(key) {
                    case '5': // Up arrow - rotate
                    {
                        Piece rotated = rotatePiece(currentPiece);
                        if(!checkCollision(&rotated, pieceRow, pieceCol))
                            currentPiece = rotated;
                        break;
                    }
                    case '3': // Right arrow
                        if(!checkCollision(&currentPiece, pieceRow, pieceCol + 1))
                            pieceCol++;
                        break;
                    case '6': // Down arrow - accelerate gravity
                        if(!checkCollision(&currentPiece, pieceRow + 1, pieceCol))
                            pieceRow++;
                        break;
                    case '9': // Left arrow
                        if(!checkCollision(&currentPiece, pieceRow, pieceCol - 1))
                            pieceCol--;
                        break;
                }
            }

            last_status = status;

            // --- Gravity: move piece down ---
            if(!checkCollision(&currentPiece, pieceRow + 1, pieceCol)) {
                pieceRow++;
            } else {
                // Piece landed
                placePiece(&currentPiece, pieceRow, pieceCol);
                clearLines();
                pieceLanded = TRUE;
            }

            // --- Draw the board + current piece ---
            drawBoard();
            //drawPiece(&currentPiece, pieceRow, pieceCol);

            usleep(500000); // gravity tick 0.5 sec
        }
    }
}


// ******************************************************************** //

void Cleanup() {
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
