/*
    Author: Matt Ball
    GitHub: https://github.com/MattDrivenDev/raychip-8
    Documentation: http://devernay.free.fr/hacks/chip8/C8TECH10.HTM
*/

#include <raylib.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

//----------------------------------------------------------------------------------
// Defines / Config
//----------------------------------------------------------------------------------
#define C8_FILENAME             "rom.c8"
#define C8_DEBUG_MODE           true
#define C8_WIDTH                64
#define C8_HEIGHT               32
#define C8_MEMORY               4096
#define C8_START                512
#define C8_STACK_SIZE           16
#define C8_V_REGISTER_COUNT     16
#define C8_PIXEL_WIDTH          10
#define C8_PIXEL_HEIGHT         10
#define C8_VF                   15
#define C8_V0                   0

//----------------------------------------------------------------------------------
// Typedefs
//----------------------------------------------------------------------------------
typedef struct 
{
    unsigned short instruction;
    unsigned char addr;
    unsigned char n;
    unsigned char x;
    unsigned char y;
    unsigned char kk;
} C8_Instruction;

//----------------------------------------------------------------------------------
// Local Variables Definition (local to this module)
//----------------------------------------------------------------------------------

// The Chip-8 language is capable of accessing up to 4KB (4,096 bytes) of RAM, from 
// location 0x000 (0) to 0xFFF (4095). The first 512 bytes, from 0x000 to 0x1FF, are 
// where the original interpreter was located, and should not be used by programs.
// Most Chip-8 programs start at location 0x200 (512)
unsigned char C8_RAM[C8_MEMORY]           = {0};

// Chip-8 has 16 general purpose 8-bit registers, usually referred to as Vx, where x 
// is a hexadecimal digit (0 through F). The VF register should not be used by any 
// program, as it is used as a flag by some instructions. 
unsigned char C8_V[C8_V_REGISTER_COUNT]   = {0};

// There is also a 16-bit register called I. This register is generally used to store 
// memory addresses, so only the lowest (rightmost) 12 bits are usually used.
unsigned short C8_I                       = 0;

// Chip-8 also has two special purpose 8-bit registers, for the delay and sound timers. 
// When these registers are non-zero, they are automatically decremented at a rate of 
// 60Hz. See the section 2.5, Timers & Sound, for more information on these.
unsigned char C8_ST                       = 0;
unsigned char C8_DT                       = 0;

// The program counter (PC) should be 16-bit, and is used to store the currently 
// executing address.
unsigned short C8_PC                      = C8_START;

// The stack pointer (SP) can be 8-bit, it is used to point to the topmost level 
// of the stack.
unsigned char C8_SP                       = 0;

// The stack is an array of 16 16-bit values, used to store the address that the 
// interpreter shoud return to when finished with a subroutine. Chip-8 allows for 
// up to 16 levels of nested subroutines.
unsigned short C8_STACK[C8_STACK_SIZE]    = {0};

// The original implementation of the Chip-8 language used a 64x32-pixel monochrome
// display with this format:
//                           +--------------------+
//                           |(0,0)        (63, 0)|
//                           |                    |
//                           |                    |
//                           |                    |
//                           |(0,31)       (64,31)|
//                           +--------------------+
// Chip-8 draws graphics on screen through the use of sprites. A sprite is a group
// of bytes which are a binary representation of the desired picture. Chip-8 sprites
// may be up to 15 butes, for a possible sprite size of 8x15.
bool C8_Buffer[C8_HEIGHT][C8_WIDTH]       = {false};

//----------------------------------------------------------------------------------
// Chip-8 Instruction Set Declaration
//----------------------------------------------------------------------------------
void C8_SYS_ADDR                (C8_Instruction *instruction);
void C8_CLS                     (C8_Instruction *instruction);
void C8_RET                     (C8_Instruction *instruction);
void C8_JP_ADDR                 (C8_Instruction *instruction);
void C8_CALL_ADDR               (C8_Instruction *instruction);
void C8_SE_VX_BYTE              (C8_Instruction *instruction);
void C8_SNE_VX_BYTE             (C8_Instruction *instruction);
void C8_SE_VX_VY                (C8_Instruction *instruction);
void C8_LD_VX_BYTE              (C8_Instruction *instruction);
void C8_ADD_VX_BYTE             (C8_Instruction *instruction);
void C8_LD_VX_VY                (C8_Instruction *instruction);
void C8_OR_VX_VY                (C8_Instruction *instruction);
void C8_AND_VX_VY               (C8_Instruction *instruction);
void C8_XOR_VX_VY               (C8_Instruction *instruction);
void C8_ADD_VX_VY               (C8_Instruction *instruction);
void C8_SUB_VX_VY               (C8_Instruction *instruction);
void C8_SHR_VX_VY               (C8_Instruction *instruction);
void C8_SUBN_VX_VY              (C8_Instruction *instruction);
void C8_SHL_VX_VY               (C8_Instruction *instruction);
void C8_SNE_VX_VY               (C8_Instruction *instruction);
void C8_LD_I_ADDR               (C8_Instruction *instruction);
void C8_JP_V0_ADDR              (C8_Instruction *instruction);
void C8_RND_VX_BYTE             (C8_Instruction *instruction);
void C8_DRW_VX_VY_NIBBLE        (C8_Instruction *instruction);
void C8_SKP_VX                  (C8_Instruction *instruction);
void C8_SKNP_VX                 (C8_Instruction *instruction);
void C8_LD_VX_DT                (C8_Instruction *instruction);
void C8_LD_VX_K                 (C8_Instruction *instruction);
void C8_LD_DT_VX                (C8_Instruction *instruction);
void C8_LD_ST_VX                (C8_Instruction *instruction);
void C8_ADD_I_VX                (C8_Instruction *instruction);
void C8_LD_F_VX                 (C8_Instruction *instruction);
void C8_LD_B_VX                 (C8_Instruction *instruction);
void C8_LD_I_VX                 (C8_Instruction *instruction);
void C8_LD_VX_I                 (C8_Instruction *instruction);

//----------------------------------------------------------------------------------
// Local Functions Declaration
//----------------------------------------------------------------------------------
void parse_instruction          (C8_Instruction *instruction);
void interpret_instruction      (C8_Instruction *instruction);
void increment_program_counter  ();
void load_rom                   ();
void render_buffer              ();
void helloworld                 ();

//----------------------------------------------------------------------------------
// Main entry point
//----------------------------------------------------------------------------------
int main()
{
    // raylib Initialization
    //--------------------------------------------------------------------------------------
    const int screenWidth       = C8_WIDTH * C8_PIXEL_WIDTH;
    const int screenHeight      = C8_HEIGHT * C8_PIXEL_HEIGHT;
    InitWindow(screenWidth, screenHeight, "raychip-8");  
    SetTargetFPS(60);      
    
    C8_Instruction instruction;
    load_rom();

    //--------------------------------------------------------------------------------------
    // Main Game Loop
    while (!WindowShouldClose())
    {
        parse_instruction(&instruction);
        interpret_instruction(&instruction);
        increment_program_counter();

        // Testing
        helloworld();
        render_buffer();
    }

    // De-Initialization
    //--------------------------------------------------------------------------------------
    CloseWindow();                  // Close window and OpenGL context
    //--------------------------------------------------------------------------------------

    return 0;
}

void interpret_instruction(C8_Instruction *instruction)
{

}

void parse_instruction(C8_Instruction *instruction)
{
    // All instructions are 2 bytes long and are stored most-significant-byte first.
    // In memory, the first byte of each instruction should be located at an even
    // address. If a program includes sprite data, it should be padded so any 
    // instructions following it will be properly situated in RAM.
    int first_byte                  = C8_RAM[C8_PC];
    int second_byte                 = C8_RAM[C8_PC + 1];
    int opcode                      = first_byte << 8 | second_byte;
    instruction->instruction        = opcode;
    
    // nnn or addr - A 12-bit value, the lowest 12 bits of the instruction
    instruction->addr               = opcode & 0x0FFF;

    // n or nibble - A 4-bit value, the lowest 4 bits of the instruction
    instruction->n                  = opcode & 0x000F;

    // x - A 4-bit value, the lower 4 bits of the high byte of the instruction
    instruction->x                  = opcode & 0x0F00 >> 8;

    // y - A 4-bit value, the upper 4 bits of the low byte of the instruction
    instruction->y                  = opcode & 0x00F0 >> 4;

    // kk or byte - An 8-bit value, the lowest 8 bits of the instruction
    instruction->kk                 = opcode & 0x00FF;
}

void increment_program_counter()
{
    C8_PC += 2;
}

void load_rom()
{
    int i;
    int filesize = 0;
    unsigned char *filedata = LoadFileData(C8_FILENAME, &filesize);

    if (filedata != NULL)
    {
        TraceLog(LOG_INFO, "FILEIO: [$s] ROM data loaded %i bytes of data", C8_FILENAME, filesize);
        for (i = 0; i < filesize; i++)
        {
            // Will convert the char to the int representation (note, the char code - not
            // the digital representation of the char). Most Chip-8 programs start at 
            // location 0x200 (512).
            C8_RAM[C8_START + i] = filedata[i];
        }
    }
    else
    {
        TraceLog(LOG_ERROR, "FILEIO: [%s] Failed to find ROM data", C8_FILENAME);
    }
}

void render_buffer()
{
    BeginDrawing();
    ClearBackground(BLACK);
    for (int i = 0; i < C8_HEIGHT; i++)
    {
        for (int j = 0; j < C8_WIDTH; j++)
        {
            if (C8_Buffer[i][j])
            {
                int x = j * C8_PIXEL_WIDTH;
                int y = i * C8_PIXEL_HEIGHT;
                DrawRectangle(x, y, C8_PIXEL_WIDTH, C8_PIXEL_HEIGHT, GREEN);
            }
        }
    }    
    EndDrawing();
}

//----------------------------------------------------------------------------------
// Follows the Chip-8 Instruction Set Functions
//----------------------------------------------------------------------------------

// Jump to a machine code routine at nnn.
// This instruction is only used on the old computers on which the Chip-8
// was originally implemented. It is ignored by modern interpreters.
void C8_SYS_ADDR(C8_Instruction *instruction)
{

}

// Clear the display.
void C8_CLS(C8_Instruction *instruction)
{
    memset(C8_Buffer, false, sizeof(C8_Buffer));
}

// Return from a subroutine.
// The interpreter sets the program counter to the address at the top of 
// the stack, then subtracts 1 from the stack pointer.
void C8_RET(C8_Instruction *instruction)
{
    C8_PC = C8_STACK[C8_SP];
    C8_SP -= 1;
}

// Jump to location nnn.
// The interpreter sets the program counter to nnn.
void C8_JP_ADDR(C8_Instruction *instruction)
{
    C8_PC = instruction->addr;
}

// Call subroutine at nnn.
// The interpreter increments the stack pointer, then puts the current
// PC on top of the stack. The PC is then set to nnn.
void C8_CALL_ADDR(C8_Instruction *instruction)
{
    C8_SP += 1;
    C8_STACK[C8_SP] = C8_PC;
    C8_PC = instruction->addr;
}

// Skip next instruction if Vx = kk.
// The interpreter compares register Vx to kk, and if they are equal,
// increments the program counter by 2.
void C8_SE_VX_BYTE(C8_Instruction *instruction)
{
    if (C8_V[instruction->x] == instruction->kk)
    {
        increment_program_counter();
    }
}

// Skip next instruction if Vx != kk.
// The interpreter compares register Vx to kk, and if they are not
// equal, increments the program counter by 2.
void C8_SNE_VX_BYTE(C8_Instruction *instruction)
{
    if (C8_V[instruction->x] != instruction->kk)
    {
        increment_program_counter();
    }
}

// Skip next instruction if Vx = Vy.
// The interpreter compares register Vx to register Vy, and if they
// are equal, increments the program counter by 2
void C8_SE_VX_VY(C8_Instruction *instruction)
{
    if (C8_V[instruction->x] == C8_V[instruction->y])
    {
        increment_program_counter();
    }
}

// Set Vx = kk.
// The interpreter puts the value kk into register Vx.
void C8_LD_VX_BYTE(C8_Instruction *instruction)
{
    C8_V[instruction->x] = instruction->kk;
}

// Set Vx = Vx + kk.
// Adds the value of kk to the value of register Vx, then stores the
// result in Vx.
void C8_ADD_VX_BYTE(C8_Instruction *instruction)
{
    C8_V[instruction->x] = C8_V[instruction->x] + instruction->kk;
}

// Set Vx = Vy.
// Stores the value of register Vy in register Vx.
void C8_LD_VX_VY(C8_Instruction *instruction)
{
    C8_V[instruction->x] = C8_V[instruction->y];
}

// Set Vx = Vx OR Vy.
// Performs a bitwise OR on the values of Vx and Vy, then stores the
// result in Vx. A bitwise OR compares the corresponding bits from two
// values, and if either bit is 1, then the same bit in the result is
// also 1. Otherwise, it is 0.
void C8_OR_VX_VY(C8_Instruction *instruction)
{
    C8_V[instruction->x] = C8_V[instruction->x] | C8_V[instruction->y];
}

// Set Vx = Vx AND Vy.
// Performs a bitwise AND on the values of Vx and Vy, then stores the
// result in Vx. A bitwise AND compers the corresponding bits from two
// values, and if both bits are 1, then the same bit in the result is 
// also 1. Otherwise, it is 0.
void C8_AND_VX_VY(C8_Instruction *instruction)
{
    C8_V[instruction->x] = C8_V[instruction->x] & C8_V[instruction->y];
}

// Set Vx = Vx XOR Vy.
// Performs a bitwise exclusiv OR on the values of Vx and Vy, then stores
// the result in Vx. An exclusive OR compares the corresponding bits from
// two values, and if the bits are not both the same, then the corresponding
// bit in the result is set to 1. Otherwise, it is 0.
void C8_XOR_VX_VY(C8_Instruction *instruction)
{
    C8_V[instruction->x] = C8_V[instruction->x] ^ C8_V[instruction->y];
}

// Set Vx = Vx + Vy, set VF = carry.
// The values of Vx and Vy are added together. If the result is greater
// than 8 bits (i.e., > 255) VF is set to 1, otherwise 0. Only the lowest
// 8 bits of the result are kept, and stored in Vx.
void C8_ADD_VX_VY(C8_Instruction *instruction)
{
    short vx = C8_V[instruction->x] + C8_V[instruction->y];
    // TODO is this right?
    C8_V[C8_VF] = vx > sizeof(char);
    C8_V[instruction->x] = vx;
}

// Set Vx = Vx - Vy, set VF = NOT borrow.
// If Vx > Vy, then VF is set to 1, otherwise 0. Then Vy is subtracted 
// from Vx, and the result stored in Vx.
void C8_SUB_VX_VY(C8_Instruction *instruction)
{
    C8_V[C8_VF] = C8_V[instruction->x] > C8_V[instruction->y];
    C8_V[instruction->x] = C8_V[instruction->x] - C8_V[instruction->y];
}

// Set Vx = Vx SHR 1.
// If the least-significant bit of Vx is 1, then VF is set to 1, otherwise
// 0. Then Vx is divided by 2.
void C8_SHR_VX_VY(C8_Instruction *instruction)
{
    // TODO I think this will work but I wonder if there is a bitwise 
    // operation that will work better.
    C8_V[C8_VF] = C8_V[instruction->x] % 2;
    C8_V[instruction->x] = C8_V[instruction->x] / 2;
}

// Set Vx = Vy - Vx, set VF = NOT borrow.
// If Vy > Vx, then VF is set to 1, otherwise 0. Then Vx is subtracted
// from Vy, and the results are stored in Vx.
void C8_SUBN_VX_VY(C8_Instruction *instruction)
{
    C8_V[C8_VF] = C8_V[instruction->y] > C8_V[instruction->x];
    C8_V[instruction->x] = C8_V[instruction->y] - C8_V[instruction->x];
}

// Set Vx = Vx SHL 1.
// If the most-significant bit of Vx is 1, then VF is set to 1 otherwise 0.
// Then Vx is multiplied by 2.
void C8_SHL_VX_VY(C8_Instruction *instruction)
{
    // TODO I think this will work but I wonder if there is a bitwise 
    // operation that will work better.
    C8_V[C8_VF] = C8_V[instruction->x] > 128;
    C8_V[instruction->x] = C8_V[instruction->x] * 2;
}

// Skip next instruction if Vx != Vy.
// The values of Vx and Vy are compared, and if they are not equal, the
// program counter is increased by 2.
void C8_SNE_VX_VY(C8_Instruction *instruction)
{
    if (C8_V[instruction->x] != C8_V[instruction->y])
    {
        increment_program_counter();
    }
}

// Set I = nnn.
// The value of register I is set to nnn.
void C8_LD_I_ADDR(C8_Instruction *instruction)
{
    C8_I = instruction->addr;
}

// Jump to location nnn + V0.
// The program counter is set to nnn plus the value of V0.
void C8_JP_V0_ADDR(C8_Instruction *instruction)
{
    C8_PC = instruction->addr + C8_V[C8_V0];
}

// Set Vx = random byte and kk.
// The interpreter generates a random number from 0 to 255, which
// is then ANDed with the value kk. The results are stored in Vx.
// See instruction C8_AND_VX_VY for more information on AND.
void C8_RND_VX_BYTE(C8_Instruction *instruction)
{
    unsigned char n = GetRandomValue(0, 255);
    C8_V[instruction->x] = n & instruction->kk;
}

// Display n-byte sprite starting at memory location I at (Vx, Vy), set 
// VF = collision.
// The interpreter reads n bytes from memory, starting at the address stored 
// in I. These bytes are then displayed as sprites on screen at coordinates 
// (Vx, Vy). Sprites are XORed onto the existing screen. If this causes any 
// pixels to be erased, VF is set to 1, otherwise it is set to 0. If the 
// sprite is positioned so part of it is outside the coordinates of the 
// display, it wraps around to the opposite side of the screen. See instruction
// C8_XOR_VX_VY for more information on XOR, and secion 2.4, Display, for
// more information on the Chip-8 screen and sprites.
void C8_DRW_VX_VY_NIBBLE(C8_Instruction *instruction)
{

}

// Skip next instruction if key with the value of Vx is pressed.
// Checks the keyboard, and if the key corresponding to the value of Vx is 
// currently in the down position, PC is increased by 2.
void C8_SKP_VX(C8_Instruction *instruction)
{

}

// Skip next instruction if key with the value of Vx is not pressed.
// Checks the keyboard, and if the key corresponding to the value of Vx
// is currently in the up position, PC is increased by 2.
void C8_SKNP_VX(C8_Instruction *instruction)
{

}

// Set Vx = delay timer value.
// The value of DT is placed into Vx.
void C8_LD_VX_DT(C8_Instruction *instruction)
{
    C8_V[instruction->x] = C8_DT;
}

// Wait for a key press, store the value of the key in Vx.
// All execution stops until a key is pressed, then the value of that
// key is stored in Vx.
void C8_LD_VX_K(C8_Instruction *instruction)
{

}

// Set delay timer = Vx.
// DT is set equal to the value of Vx.
void C8_LD_DT_VX(C8_Instruction *instruction)
{
    C8_DT = C8_V[instruction->x];
}

// Set sound timer = Vx.
// ST is set equal to the value of Vx.
void C8_LD_ST_VX(C8_Instruction *instruction)
{
    C8_ST = C8_V[instruction->x];
}

// Set I = I + Vx.
// The values of I and Vx are added, and the results are stored in I.
void C8_ADD_I_VX(C8_Instruction *instruction)
{
    C8_I = C8_I + C8_V[instruction->x];
}

// Set I = location of sprite for digit Vx.
// The value of I is set to the location for the hexedecimal sprite
// corresponding to the value of Vx. See section 2.4, Display, for more 
// information on the Chip-8 hexedecimal font.
void C8_LD_F_VX(C8_Instruction *instruction)
{

}

// Store BCD represnetation of Vx in memory locations I, I+1, and I+2.
// The interpreter takes the decimal value of Vx, and places the hundreds
// digit in memory at location in I, the tens digit at location I+1, and
// the ones digit at location I+2.
void C8_LD_B_VX(C8_Instruction *instruction)
{

}

// Store registers V0 through Vx in memory starting at location I.
// The interpreter copiues the values of registers V0 through Vx into
// memory, starting at the address in I.
void C8_LD_I_VX(C8_Instruction *instruction)
{

}

// Read registers V0 through Vx from memory starting at location I.
// The interpreter reads values from memory starting at location I
// into registers V0 through Vx.
void C8_LD_VX_I(C8_Instruction *instruction)
{
    int i;
    for (i = C8_V0; i <= instruction->x; i++)
    {
        C8_V[i] = C8_RAM[C8_I + i];
    }
}

//----------------------------------------------------------------------------------
// Follows Testing-Only Functions
//----------------------------------------------------------------------------------

// Prints some "HELLOWORLD" text on into the screen buffer.
void helloworld()
{
    // H
    C8_Buffer[1][1] = true; C8_Buffer[1][2] = false; C8_Buffer[1][3] = true;
    C8_Buffer[2][1] = true; C8_Buffer[2][2] = false; C8_Buffer[2][3] = true;
    C8_Buffer[3][1] = true; C8_Buffer[3][2] = true;  C8_Buffer[3][3] = true;
    C8_Buffer[4][1] = true; C8_Buffer[4][2] = false; C8_Buffer[4][3] = true;
    C8_Buffer[5][1] = true; C8_Buffer[5][2] = false; C8_Buffer[5][3] = true; 

    // E
    C8_Buffer[1][5] = true; C8_Buffer[1][6] = true;  C8_Buffer[1][7] = true;
    C8_Buffer[2][5] = true; C8_Buffer[2][6] = false; C8_Buffer[2][7] = false;
    C8_Buffer[3][5] = true; C8_Buffer[3][6] = true;  C8_Buffer[3][7] = true;
    C8_Buffer[4][5] = true; C8_Buffer[4][6] = false; C8_Buffer[4][7] = false;
    C8_Buffer[5][5] = true; C8_Buffer[5][6] = true;  C8_Buffer[5][7] = true; 

    // L
    C8_Buffer[1][9] = true; C8_Buffer[1][10] = false; C8_Buffer[1][11] = false;
    C8_Buffer[2][9] = true; C8_Buffer[2][10] = false; C8_Buffer[2][11] = false;
    C8_Buffer[3][9] = true; C8_Buffer[3][10] = false; C8_Buffer[3][11] = false;
    C8_Buffer[4][9] = true; C8_Buffer[4][10] = false; C8_Buffer[4][11] = false;
    C8_Buffer[5][9] = true; C8_Buffer[5][10] = true;  C8_Buffer[5][11] = true; 

    // L
    C8_Buffer[1][13] = true; C8_Buffer[1][14] = false; C8_Buffer[1][15] = false;
    C8_Buffer[2][13] = true; C8_Buffer[2][14] = false; C8_Buffer[2][15] = false;
    C8_Buffer[3][13] = true; C8_Buffer[3][14] = false; C8_Buffer[3][15] = false;
    C8_Buffer[4][13] = true; C8_Buffer[4][14] = false; C8_Buffer[4][15] = false;
    C8_Buffer[5][13] = true; C8_Buffer[5][14] = true;  C8_Buffer[5][15] = true; 

    // O
    C8_Buffer[1][17] = true; C8_Buffer[1][18] = true;  C8_Buffer[1][19] = true;
    C8_Buffer[2][17] = true; C8_Buffer[2][18] = false; C8_Buffer[2][19] = true;
    C8_Buffer[3][17] = true; C8_Buffer[3][18] = false; C8_Buffer[3][19] = true;
    C8_Buffer[4][17] = true; C8_Buffer[4][18] = false; C8_Buffer[4][19] = true;
    C8_Buffer[5][17] = true; C8_Buffer[5][18] = true;  C8_Buffer[5][19] = true; 

    // W
    C8_Buffer[1][21] = true; C8_Buffer[1][22] = false; C8_Buffer[1][23] = true;
    C8_Buffer[2][21] = true; C8_Buffer[2][22] = false; C8_Buffer[2][23] = true;
    C8_Buffer[3][21] = true; C8_Buffer[3][22] = false; C8_Buffer[3][23] = true;
    C8_Buffer[4][21] = true; C8_Buffer[4][22] = true;  C8_Buffer[4][23] = true;
    C8_Buffer[5][21] = true; C8_Buffer[5][22] = false; C8_Buffer[5][23] = true; 

    // O
    C8_Buffer[1][25] = true; C8_Buffer[1][26] = true;  C8_Buffer[1][27] = true;
    C8_Buffer[2][25] = true; C8_Buffer[2][26] = false; C8_Buffer[2][27] = true;
    C8_Buffer[3][25] = true; C8_Buffer[3][26] = false; C8_Buffer[3][27] = true;
    C8_Buffer[4][25] = true; C8_Buffer[4][26] = false; C8_Buffer[4][27] = true;
    C8_Buffer[5][25] = true; C8_Buffer[5][26] = true;  C8_Buffer[5][27] = true; 

    // R
    C8_Buffer[1][29] = true; C8_Buffer[1][30] = true;  C8_Buffer[1][31] = false;
    C8_Buffer[2][29] = true; C8_Buffer[2][30] = false; C8_Buffer[2][31] = true;
    C8_Buffer[3][29] = true; C8_Buffer[3][30] = true;  C8_Buffer[3][31] = false;
    C8_Buffer[4][29] = true; C8_Buffer[4][30] = false; C8_Buffer[4][31] = true;
    C8_Buffer[5][29] = true; C8_Buffer[5][30] = false; C8_Buffer[5][31] = true; 

    // L
    C8_Buffer[1][33] = true; C8_Buffer[1][34] = false; C8_Buffer[1][35] = false;
    C8_Buffer[2][33] = true; C8_Buffer[2][34] = false; C8_Buffer[2][35] = false;
    C8_Buffer[3][33] = true; C8_Buffer[3][34] = false; C8_Buffer[3][35] = false;
    C8_Buffer[4][33] = true; C8_Buffer[4][34] = false; C8_Buffer[4][35] = false;
    C8_Buffer[5][33] = true; C8_Buffer[5][34] = true;  C8_Buffer[5][35] = true; 

    // O
    C8_Buffer[1][37] = true; C8_Buffer[1][38] = true;  C8_Buffer[1][39] = false;
    C8_Buffer[2][37] = true; C8_Buffer[2][38] = false; C8_Buffer[2][39] = true;
    C8_Buffer[3][37] = true; C8_Buffer[3][38] = false; C8_Buffer[3][39] = true;
    C8_Buffer[4][37] = true; C8_Buffer[4][38] = false; C8_Buffer[4][39] = true;
    C8_Buffer[5][37] = true; C8_Buffer[5][38] = true;  C8_Buffer[5][39] = false; 
}