#include "PmodKYPD.h"
#include "sleep.h"
#include "xil_cache.h"
#include "xparameters.h"

#include <stdio.h>
#include "PmodOLED.h"
#include "xil_printf.h"

void Initialize();
void Cleanup();
void DisableCaches();
void EnableCaches();
void DemoSleep(u32 millis);
void game();

//
typedef struct {
    u8 shape[4][4]; // 1 = block, 0 = empty
    int size;       //3 or 4
} Piece;

// New functions
void updateCell(int row, int col, int filled);
void updateBoard();
void clearOLED();
void placePiece(Piece *p, int row, int col);
void drawPiece(Piece *p, int baseRow, int baseCol);
void erasePiece(Piece *p, int baseRow, int baseCol);
void gameOverScreen();

PmodKYPD myDevice2;
PmodOLED myDevice;

const u8 orientation = 0x0; // Set up for Normal PmodOLED(false) vs normal
                            // Onboard OLED(true)
const u8 invert = 0x0; // true = whitebackground/black letters
                       // false = black background /white letters

// Global vars
int gameOver = FALSE;
#define BOARD_ROWS 20
#define BOARD_COLS 10
#define SCALE 3
#define PIECE_WIDTH 4
#define PIECE_HEIGHT 4

volatile u8 board[BOARD_ROWS][BOARD_COLS] = {0}; // 0 = empty, 1 = filled
volatile u16 score = 0;

int main(void) {
    Initialize();
    while (1) {
        char checkKey;
		char scoreString[20];   // enough space for text
		sprintf(scoreString, " Score: %6d", score);
        score = 0;
        game();   
        xil_printf("Retry?\r\n");
        
        u16 keystate;
        u8 key;
        XStatus status;
        keystate = KYPD_getKeyStates(&myDevice2);
        status = KYPD_getKeyPressed(&myDevice2, keystate, &key);

        OLED_SetCursor(&myDevice, 0, 0);
        OLED_PutString(&myDevice, "   Retry?");
        OLED_SetCursor(&myDevice, 0, 1);
        OLED_PutString(&myDevice, "Press 0 to retry");
        OLED_SetCursor(&myDevice, 0, 3);
		OLED_PutString(&myDevice, scoreString);
        OLED_Update(&myDevice);
        sleep(1);

        while(status == KYPD_NO_KEY){
        	keystate = KYPD_getKeyStates(&myDevice2);
			status = KYPD_getKeyPressed(&myDevice2, keystate, &key);
			checkKey = (char) key;
            OLED_DisplayOff(&myDevice);
            usleep(500000);
            OLED_DisplayOn(&myDevice);
            usleep(500000);
        }
        
        clearOLED();
        if(checkKey == '0'){ 
            xil_printf("New game\r\n");
        	score = 0;
        } else{
        	xil_printf("End game \r\n");
            OLED_SetCursor(&myDevice, 0, 1);
            OLED_PutString(&myDevice, "    GGs  :D");
            OLED_Update(&myDevice);
            sleep(3);
            break;
        }

    }
    Cleanup();
    OLED_DisplayOff(&myDevice);
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
    KYPD_begin(&myDevice2, XPAR_PMODKYPD_0_AXI_LITE_GPIO_BASEADDR);
    KYPD_loadKeyTable(&myDevice2, (u8*) DEFAULT_KEYTABLE);
    OLED_Begin(&myDevice, XPAR_PMODOLED_0_AXI_LITE_GPIO_BASEADDR,
		XPAR_PMODOLED_0_AXI_LITE_SPI_BASEADDR, orientation, invert);
    Xil_Out32(myDevice.GPIO_addr, 0xF);
    // Turn automatic updating off
    OLED_SetCharUpdate(&myDevice, 0);
}

// Draw individual cell of board matrix 
void updateCell(int row, int col, int filled) { // applies to tetris cells only
	u8 *pat0;
	u8 *pat1; // be careful about this, might cause problems
	pat0 = OLED_GetStdPattern(0);
	pat1 = OLED_GetStdPattern(1);//fill pattern of fill
    //col = 60 - col;
    row = BOARD_ROWS - row - 1; //inverse row
    int pixelY = 1 + (row * SCALE); // 3 pixels per block
    int pixelX = col * SCALE; // 3 pixels per block, border of 1
    OLED_MoveTo(&myDevice, pixelY, pixelX);
    if (filled) {
        OLED_SetFillPattern(&myDevice, pat1);
    } else {
        OLED_SetFillPattern(&myDevice, pat0);
    }
    OLED_FillRect(&myDevice, pixelY + SCALE-1, pixelX + SCALE-1);
}

// Redraw area of play based on board matrix
void updateBoard(){
    int fill = FALSE;
    for(int r = 0; r < BOARD_ROWS; r++){
        for(int c = 0; c < BOARD_COLS; c++){
            fill = board[r][c];
            updateCell(r, c, fill);
        }
    }
    OLED_Update(&myDevice);
}

void clearOLED(){
	u8 *pat1; // be careful about this, might cause problems
	pat1 = OLED_GetStdPattern(1);//fill pattern of fill
    //OLED_SetDrawColor(&myDevice, 0);
    OLED_ClearBuffer(&myDevice);
    OLED_SetDrawColor(&myDevice, 1);
    OLED_SetFillPattern(&myDevice, pat1);
    OLED_Update(&myDevice);
}

void clearLines() {
    int linesClear = 0;
    for(int r = 0; r < BOARD_ROWS; r++) {
        int full = TRUE;
        for(int c = 0; c < BOARD_COLS; c++)
            if(board[r][c] == 0) full = FALSE;
        if(full) {
            // Shift rows above down
            linesClear++;
            for(int rr = r; rr > 0; rr--)
                for(int c = 0; c < BOARD_COLS; c++)
                    board[rr][c] = board[rr-1][c];
            // Clear top row
            for(int c = 0; c < BOARD_COLS; c++)
                board[0][c] = 0;
        }
    }
    score += linesClear*100;
}

///

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

void erasePiece(Piece *p, int baseRow, int baseCol) {
    for (int r = 0; r < p->size; r++) {
        for (int c = 0; c < p->size; c++) {
            if (p->shape[r][c]) {

                int br = baseRow + r;
                int bc = baseCol + c;

                // Safety bounds check
                if (br >= 0 && br < BOARD_ROWS &&
                    bc >= 0 && bc < BOARD_COLS)
                {
                    updateCell(br, bc, 0);
                    OLED_Update(&myDevice);
                }
            }
        }
    }
}

void drawPiece(Piece *p, int baseRow, int baseCol) {
    for (int r = 0; r < p->size; r++) {
        for (int c = 0; c < p->size; c++) {
            if (p->shape[r][c]) {

                int br = baseRow + r;
                int bc = baseCol + c;

                // Safety bounds check
                if (br >= 0 && br < BOARD_ROWS &&
                    bc >= 0 && bc < BOARD_COLS)
                {
                    updateCell(br, bc, 1);
                    OLED_Update(&myDevice);
                }
            }
        }
    }
}




void game() {
    // Initialize game state
    int pieceRow, pieceCol;
    Piece currentPiece;
    int gameOver = FALSE;
    u16 keystate;
    char checkKey;
    XStatus status, last_status = KYPD_NO_KEY;
    u8 key, last_key = 'x';
    int pieceIndex, pieceIndex2;
    u8 *pat1; // be careful about this, might cause problems
    int currTick = 500;
	pat1 = OLED_GetStdPattern(1);//fill pattern of fill
	xil_printf("\r\n  Start Screen\r\n");
	//OLED_SetDrawColor(&myDevice, 0);

    OLED_ClearBuffer(&myDevice);
    OLED_SetCursor(&myDevice, 0, 1);
    OLED_PutString(&myDevice, "    Tetris  :)");
    OLED_Update(&myDevice);
    sleep(2);
    //start screen

    for(int count = 3; count >= 0; count--){
    	OLED_ClearBuffer(&myDevice);
    	OLED_SetCursor(&myDevice, 0, 1);
    	char countdown[20];
    	sprintf(countdown, "       %d", count);
		OLED_PutString(&myDevice, countdown);
		sleep(1);
		OLED_Update(&myDevice);
    }
    // Clear board at start
    for(int r=0; r<BOARD_ROWS; r++)
        for(int c=0; c<BOARD_COLS; c++)
            board[r][c] = 0;

    clearOLED();
    updateBoard();
    xil_printf("Game start \r\n");

    // Array of pieces (define them elsewhere)
    // Inversed vertically, swtich S & Z, J & L
    //Piece pieces[7] = {I, J, L, O, S, T, Z};//original
    Piece pieces[7] = {I, L, J, O, Z, T, S};//swapped
    pieceIndex = rand() % 7;
    while(!gameOver) {
        // Spawn new random piece
    	currentPiece = pieces[pieceIndex];
    	// Next piece preview
        pieceIndex2 = rand() % 7;
    	pieceIndex = pieceIndex2;
        xil_printf("Curr piece: %d\r\n", pieceIndex);
        xil_printf("Next piece: %d\r\n", pieceIndex2);
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
            keystate = KYPD_getKeyStates(&myDevice2);
            status = KYPD_getKeyPressed(&myDevice2, keystate, &key);
            for(int loops = 0; loops < currTick; loops++){
            	keystate = KYPD_getKeyStates(&myDevice2);
				status = KYPD_getKeyPressed(&myDevice2, keystate, &key);
				if(status == KYPD_SINGLE_KEY && status != last_status) {
				checkKey = (char) key;
					switch(checkKey) {
						case '5': // Up arrow - rotate
						{
							Piece rotated = rotatePiece(currentPiece);
							if(!checkCollision(&rotated, pieceRow, pieceCol))
								erasePiece(&currentPiece, pieceRow, pieceCol);
								currentPiece = rotated;
								drawPiece(&currentPiece, pieceRow, pieceCol);
								xil_printf("Up \r\n");
							break;
						}
						case '3': // Right arrow
							if(!checkCollision(&currentPiece, pieceRow, pieceCol + 1)){
                                erasePiece(&currentPiece, pieceRow, pieceCol);
								pieceCol++;
								drawPiece(&currentPiece, pieceRow, pieceCol);
								xil_printf("Right \r\n");
                            }
							break;
						case '6': // Down arrow - accelerate gravity
							if(!checkCollision(&currentPiece, pieceRow + 1, pieceCol)){
								erasePiece(&currentPiece, pieceRow, pieceCol);
								pieceRow++;
								drawPiece(&currentPiece, pieceRow, pieceCol);
								xil_printf("Down \r\n");
                            }
							break;
						case '9': // Left arrow
							if(!checkCollision(&currentPiece, pieceRow, pieceCol - 1)){
								erasePiece(&currentPiece, pieceRow, pieceCol);
								pieceCol--;
								drawPiece(&currentPiece, pieceRow, pieceCol);
								xil_printf("Left \r\n");
                            }
							break;
						}
				}
				last_status = status;
				usleep(1000);
            }

            // Gravity
            if(!checkCollision(&currentPiece, pieceRow + 1, pieceCol)) {
            	erasePiece(&currentPiece, pieceRow, pieceCol);
            	pieceRow++;
                drawPiece(&currentPiece, pieceRow, pieceCol);
                updateBoard();
            } else {
                // Piece landed
                placePiece(&currentPiece, pieceRow, pieceCol);
                clearLines();
                pieceLanded = TRUE;
            }
            // Draw the board + current piece
            drawPiece(&currentPiece, pieceRow, pieceCol);
        }
        if(score%100 > 0){
			currTick-= 50;
			if(currTick > 50) currTick = 50;
		}
    }

    gameOverScreen();
}

void gameOverScreen(){
    char checkKey; 
    char scoreString[20];   // enough space for text
    sprintf(scoreString, " Score: %6d", score);

    u16 keystate;
    XStatus status, last_status = KYPD_NO_KEY;
    u8 key, last_key = 'x';
    keystate = KYPD_getKeyStates(&myDevice2);
    status = KYPD_getKeyPressed(&myDevice2, keystate, &key);
    
    clearOLED();
    OLED_SetCursor(&myDevice, 0, 1);
    OLED_PutString(&myDevice, "    Game Over");
    OLED_SetCursor(&myDevice, 0, 2);
    OLED_PutString(&myDevice, scoreString);
    xil_printf("Score: %d\r\n", score);
    OLED_Update(&myDevice);
    sleep(5);
    clearOLED();
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
