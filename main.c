/*
    Author: Matt Ball
    GitHub: https://github.com/MattDrivenDev
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

//----------------------------------------------------------------------------------
// Typedefs
//----------------------------------------------------------------------------------
typedef struct 
{
    int instruction;
    int addr;
    int n;
    int x;
    int y;
    int kk;
} C8_Instruction;

//----------------------------------------------------------------------------------
// Local Variables Definition (local to this module)
//----------------------------------------------------------------------------------
int C8_RAM[C8_MEMORY]           = {0};
int C8_PC                       = C8_START;
int C8_SP                       = 0;
int C8_I                        = 0;
int C8_STACK[C8_STACK_SIZE]     = {0};
int C8_V[C8_V_REGISTER_COUNT]   = {0};
int C8_ST                       = 0;
int C8_DT                       = 0;

//----------------------------------------------------------------------------------
// Chip-8 Instruction Set Declaration
//----------------------------------------------------------------------------------
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
        TraceLog(LOG_DEBUG, "FILEIO: [$s] ROM data loaded %i bytes of data", filesize);
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

//----------------------------------------------------------------------------------
// Follows the Chip-8 Instruction Set Functions
//----------------------------------------------------------------------------------

// Clear the display.
void C8_CLS(C8_Instruction *instruction)
{

}

// Return from a subroutine.
// The interpreter sets the program counter to the address at the top of 
// the stack, then subtracts 1 from the stack pointer.
void C8_RET(C8_Instruction *instruction)
{

}

// Jump to location nnn.
// The interpreter sets the program counter to nnn.
void C8_JP_ADDR(C8_Instruction *instruction)
{

}

// Call subroutine at nnn.
// The interpreter increments the stack pointer, then puts the current
// PC on top of the stack. The PC is then set to nnn.
void C8_CALL_ADDR(C8_Instruction *instruction)
{

}

// Skip next instruction if Vx = kk.
// The interpreter compares register Vx to kk, and if they are equal,
// increments the program counter by 2.
void C8_SE_VX_BYTE(C8_Instruction *instruction)
{

}

// Skip next instruction if Vx != kk.
// The interpreter compares register Vx to kk, and if they are not
// equal, increments the program counter by 2.
void C8_SNE_VX_BYTE(C8_Instruction *instruction)
{

}

// Skip next instruction if Vx = Vy.
// The interpreter compares register Vx to register Vy, and if they
// are equal, increments the program counter by 2
void C8_SE_VX_VY(C8_Instruction *instruction)
{

}

// Set Vx = kk.
// The interpreter puts the value kk into register Vx.
void C8_LD_VX_BYTE(C8_Instruction *instruction)
{

}

// Set Vx = Vx + kk.
// Adds the value of kk to the value of register Vx, then stores the
// result in Vx.
void C8_ADD_VX_BYTE(C8_Instruction *instruction)
{

}

// Set Vx = Vy.
// Stores the value of register Vy in register Vx.
void C8_LD_VX_VY(C8_Instruction *instruction)
{

}

// Set Vx = Vx OR Vy.
// Performs a bitwise OR on the values of Vx and Vy, then stores the
// result in Vx. A bitwise OR compares the corresponding bits from two
// values, and if either bit is 1, then the same bit in the result is
// also 1. Otherwise, it is 0.
void C8_OR_VX_VY(C8_Instruction *instruction)
{

}

// Set Vx = Vx AND Vy.
// Performs a bitwise AND on the values of Vx and Vy, then stores the
// result in Vx. A bitwise AND compers the corresponding bits from two
// values, and if both bits are 1, then the same bit in the result is 
// also 1. Otherwise, it is 0.
void C8_AND_VX_VY(C8_Instruction *instruction)
{

}

// Set Vx = Vx XOR Vy.
// Performs a bitwise exclusiv OR on the values of Vx and Vy, then stores
// the result in Vx. An exclusive OR compares the corresponding bits from
// two values, and if the bits are not both the same, then the corresponding
// bit in the result is set to 1. Otherwise, it is 0.
void C8_XOR_VX_VY(C8_Instruction *instruction)
{

}

// Set Vx = Vx + Vy, set VF = carry.
// The values of Vx and Vy are added together. If the result is greater
// than 8 bits (i.e., > 255) VF is set to 1, otherwise 0. Only the lowest
// 8 bits of the result are kept, and stored in Vx.
void C8_ADD_VX_VY(C8_Instruction *instruction)
{

}

// Set Vx = Vx - Vy, set VF = NOT borrow.
// If Vx > Vy, then VF is set to 1, otherwise 0. Then Vy is subtracted 
// from Vx, and the result stored in Vx.
void C8_SUB_VX_VY(C8_Instruction *instruction)
{

}

// Set Vx = Vx SHR 1.
// If the least-significant bit of Vx is 1, then VF is set to 1, otherwise
// 0. Then Vx is divided by 2.
void C8_SHR_VX_VY(C8_Instruction *instruction)
{

}

// Set Vx = Vy - Vx, set VF = NOT borrow.
// If Vy > Vx, then VF is set to 1, otherwise 0. Then Vx is subtracted
// from Vy, and the results are stored in Vx.
void C8_SUBN_VX_VY(C8_Instruction *instruction)
{

}

// Set Vx = Vx SHL 1.
// If the most-significant bit of Vx is 1, then VF is set to 1 otherwise 0.
// Then Vx is multiplied by 2.
void C8_SHL_VX_VY(C8_Instruction *instruction)
{

}

// Skip next instruction if Vx != Vy.
// The values of Vx and Vy are compared, and if they are not equal, the
// program counter is increased by 2.
void C8_SNE_VX_VY(C8_Instruction *instruction)
{

}

// Set I = nnn.
// The value of register I is set to nnn.
void C8_LD_I_ADDR(C8_Instruction *instruction)
{

}

// Jump to location nnn + V0.
// The program counter is set to nnn pls the value of V0.
void C8_JP_V0_ADDR(C8_Instruction *instruction)
{

}

// Set Vx = random byte and kk.
// The interpreter generates a random number from 0 to 255, which
// is then ANDed with the value kk. The results are stored in Vx.
// See instruction C8_AND_VX_VY for more information on AND.
void C8_RND_VX_BYTE(C8_Instruction *instruction)
{

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

}

// Set sound timer = Vx.
// ST is set equal to the value of Vx.
void C8_LD_ST_VX(C8_Instruction *instruction)
{

}

// Set I = I + Vx.
// The values of I and Vx are added, and the results are stored in I.
void C8_ADD_I_VX(C8_Instruction *instruction)
{

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

}