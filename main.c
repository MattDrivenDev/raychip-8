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

#define C8_FONT_0_ADDR          0x000
#define C8_FONT_1_ADDR          0x005
#define C8_FONT_2_ADDR          0x00A
#define C8_FONT_3_ADDR          0x00F
#define C8_FONT_4_ADDR          0x014
#define C8_FONT_5_ADDR          0x019
#define C8_FONT_6_ADDR          0x01E
#define C8_FONT_7_ADDR          0x023
#define C8_FONT_8_ADDR          0x028
#define C8_FONT_9_ADDR          0x02D
#define C8_FONT_A_ADDR          0x032
#define C8_FONT_B_ADDR          0x037
#define C8_FONT_C_ADDR          0x03C
#define C8_FONT_D_ADDR          0x041
#define C8_FONT_E_ADDR          0x046
#define C8_FONT_F_ADDR          0x04B

//----------------------------------------------------------------------------------
// Typedefs
//----------------------------------------------------------------------------------
typedef struct C8_Instruction
{
    unsigned short opcode;
    unsigned char addr;
    unsigned char msn;
    unsigned char n;
    unsigned char x;
    unsigned char y;
    unsigned char kk;
    unsigned char skip;
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
// may be up to 15 bytes, for a possible sprite size of 8x15.
bool C8_Buffer[C8_HEIGHT][C8_WIDTH]       = {false};

// The computers which originally used the Chip-8 Language had a 16-key hexadecimal keypad.
bool C8_Keyboard[0xF]                     = {0};

// Somewhere to store the current instruction.
C8_Instruction current_instruction        = {0};

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
void increment_program_counter  (C8_Instruction *instruction);
void load_hexfont_sprites       ();
void load_rom                   ();
void render_buffer              ();
void read_input                 ();
void test_font                  ();

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
    
    load_hexfont_sprites();
    load_rom();

    test_font();

    //--------------------------------------------------------------------------------------
    // Main Game Loop
    while (!WindowShouldClose())
    {
        read_input();
        
        parse_instruction(&current_instruction);

        interpret_instruction(&current_instruction);

        increment_program_counter(&current_instruction);

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
    switch (instruction->msn)
    {
        case 0x0:
            switch (instruction->kk)
            {
                case 0xE0:  C8_CLS(instruction);                    break;
                case 0xEE:  C8_RET(instruction);                    break;        
                default:    C8_SYS_ADDR(instruction);               break;
            }
            break;
        case 0x1:       C8_JP_ADDR(instruction);                    break;
        case 0x2:       C8_CALL_ADDR(instruction);                  break;
        case 0x3:       C8_SE_VX_BYTE(instruction);                 break;
        case 0x4:       C8_SNE_VX_BYTE(instruction);                break;
        case 0x5:       C8_SE_VX_VY(instruction);                   break;
        case 0x6:       C8_LD_VX_BYTE(instruction);                 break;
        case 0x7:       C8_ADD_VX_BYTE(instruction);                break;
        case 0x8:
            switch (instruction->n)
            {
                case 0x0:   C8_LD_VX_VY(instruction);               break;
                case 0x1:   C8_OR_VX_VY(instruction);               break;  
                case 0x2:   C8_AND_VX_VY(instruction);              break;        
                case 0x3:   C8_XOR_VX_VY(instruction);              break;  
                case 0x4:   C8_ADD_VX_VY(instruction);              break;  
                case 0x5:   C8_SUB_VX_VY(instruction);              break;  
                case 0x6:   C8_SHR_VX_VY(instruction);              break;  
                case 0x7:   C8_SUBN_VX_VY(instruction);             break;  
                case 0xE:   C8_SHL_VX_VY(instruction);              break;  
                default:    TraceLog(LOG_ERROR, "Unknown");         break;
            }
            break;
        case 0x9:       C8_SNE_VX_VY(instruction);                  break;
        case 0xA:       C8_LD_I_ADDR(instruction);                  break;
        case 0xB:       C8_JP_V0_ADDR(instruction);                 break;
        case 0xC:       C8_RND_VX_BYTE(instruction);                break;
        case 0xD:       C8_DRW_VX_VY_NIBBLE(instruction);           break;
        case 0xE:
            switch (instruction->kk)
            {
                case 0x9E:  C8_SKP_VX(instruction);                 break;
                case 0xA1:  C8_SKNP_VX(instruction);                break;
                default:    TraceLog(LOG_ERROR, "Unknown");         break;
            }
            break;
        case 0xF:
            switch (instruction->kk)
            {
                case 0x07:  C8_LD_VX_DT(instruction);               break;    
                case 0x0A:  C8_LD_VX_K(instruction);                break;    
                case 0x15:  C8_LD_DT_VX(instruction);               break;    
                case 0x18:  C8_LD_ST_VX(instruction);               break;    
                case 0x1E:  C8_ADD_I_VX(instruction);               break;    
                case 0x29:  C8_LD_F_VX(instruction);                break;    
                case 0x33:  C8_LD_B_VX(instruction);                break;    
                case 0x55:  C8_LD_I_VX(instruction);                break;    
                case 0x65:  C8_LD_VX_I(instruction);                break;                              
                default:    TraceLog(LOG_ERROR, "Unknown");         break;
            }
            break;
        default:            TraceLog(LOG_ERROR, "Unknown");         break;
    }
}

void parse_instruction(C8_Instruction *instruction)
{
    // Note: the ordering in which you & and >> is important, so use brackets
    // to ensure that the ordering happens as desired.

    // All instructions are 2 bytes long and are stored most-significant-byte first.
    // In memory, the first byte of each instruction should be located at an even
    // address. If a program includes sprite data, it should be padded so any 
    // instructions following it will be properly situated in RAM.
    unsigned char first_byte        = C8_RAM[C8_PC];
    unsigned char second_byte       = C8_RAM[C8_PC + 1];
    unsigned short opcode           = first_byte << 8 | second_byte;

    instruction->opcode             = opcode;

    // msn or most-significant-nibble - A 4-bit value, the highest 4 bits of the instruction
    instruction->msn                = (opcode & 0xF000) >> 12;
    
    // nnn or addr - A 12-bit value, the lowest 12 bits of the instruction
    instruction->addr               = (opcode & 0x0FFF);

    // n or nibble - A 4-bit value, the lowest 4 bits of the instruction
    instruction->n                  = (opcode & 0x000F);

    // x - A 4-bit value, the lower 4 bits of the high byte of the instruction
    instruction->x                  = (opcode & 0x0F00) >> 8;

    // y - A 4-bit value, the upper 4 bits of the low byte of the instruction
    instruction->y                  = (opcode & 0x00F0) >> 4;

    // kk or byte - An 8-bit value, the lowest 8 bits of the instruction
    instruction->kk                 = (opcode & 0x00FF);
}

void increment_program_counter(C8_Instruction *instruction)
{
    if (!instruction->skip)
    {
        C8_PC += 2;
    }
    else
    {
        instruction->skip -= 1;
    }
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

void read_input()
{
    // This should of course be replaced with a configurable mapping.
    // I suspect that some kind of hashtable that marries the raylib key
    // enum to the correct key - and then we can check the IsKeyDown
    // for each.
    C8_Keyboard[0x0] = IsKeyDown(KEY_ZERO);
    C8_Keyboard[0x1] = IsKeyDown(KEY_ONE);
    C8_Keyboard[0x2] = IsKeyDown(KEY_TWO);
    C8_Keyboard[0xC] = IsKeyDown(KEY_FOUR);

    C8_Keyboard[0x4] = IsKeyDown(KEY_Q);
    C8_Keyboard[0x5] = IsKeyDown(KEY_W);
    C8_Keyboard[0x6] = IsKeyDown(KEY_E);
    C8_Keyboard[0xD] = IsKeyDown(KEY_R);

    C8_Keyboard[0x7] = IsKeyDown(KEY_A);
    C8_Keyboard[0x8] = IsKeyDown(KEY_S);
    C8_Keyboard[0x9] = IsKeyDown(KEY_D);
    C8_Keyboard[0xE] = IsKeyDown(KEY_F);

    C8_Keyboard[0xA] = IsKeyDown(KEY_Z);
    C8_Keyboard[0x0] = IsKeyDown(KEY_X);
    C8_Keyboard[0xB] = IsKeyDown(KEY_C);
    C8_Keyboard[0xF] = IsKeyDown(KEY_V);
}

void load_hexfont_sprites()
{
    // Programs may also refer to a group of sprites representing the 
    // hexadecimal digits 0 through F. These sprites are 5 bytes long, 
    // or 8x5 pixels. The data should be stored in the interpreter 
    // area of Chip-8 memory (0x000 to 0x1FF). Below is a listing of 
    // each character's bytes, in binary and hexadecimal:
    
    C8_RAM[C8_FONT_0_ADDR] = 0xF0;                  // ****
    C8_RAM[C8_FONT_0_ADDR + 1] = 0x90;              // *  *
    C8_RAM[C8_FONT_0_ADDR + 2] = 0x90;              // *  *
    C8_RAM[C8_FONT_0_ADDR + 3] = 0x90;              // *  *
    C8_RAM[C8_FONT_0_ADDR + 4] = 0xF0;              // ****
    
    C8_RAM[C8_FONT_1_ADDR] = 0x20;                  //   * 
    C8_RAM[C8_FONT_1_ADDR + 1] = 0x60;              //  ** 
    C8_RAM[C8_FONT_1_ADDR + 2] = 0x20;              //   * 
    C8_RAM[C8_FONT_1_ADDR + 3] = 0x20;              //   * 
    C8_RAM[C8_FONT_1_ADDR + 4] = 0x70;              //  ***
    
    C8_RAM[C8_FONT_2_ADDR] = 0xF0;                  // ****
    C8_RAM[C8_FONT_2_ADDR + 1] = 0x10;              //    *
    C8_RAM[C8_FONT_2_ADDR + 2] = 0xF0;              // ****
    C8_RAM[C8_FONT_2_ADDR + 3] = 0x80;              // *   
    C8_RAM[C8_FONT_2_ADDR + 4] = 0xF0;              // ****
    
    C8_RAM[C8_FONT_3_ADDR] = 0xF0;                  // ****
    C8_RAM[C8_FONT_3_ADDR + 1] = 0x10;              //    *
    C8_RAM[C8_FONT_3_ADDR + 2] = 0xF0;              // ****
    C8_RAM[C8_FONT_3_ADDR + 3] = 0x10;              //    *
    C8_RAM[C8_FONT_3_ADDR + 4] = 0xF0;              // ****
    
    C8_RAM[C8_FONT_4_ADDR] = 0x90;                  // *  *
    C8_RAM[C8_FONT_4_ADDR + 1] = 0x90;              // *  *
    C8_RAM[C8_FONT_4_ADDR + 2] = 0xF0;              // ****
    C8_RAM[C8_FONT_4_ADDR + 3] = 0x10;              //    *
    C8_RAM[C8_FONT_4_ADDR + 4] = 0x10;              //    *
    
    C8_RAM[C8_FONT_5_ADDR] = 0xF0;                  // ****
    C8_RAM[C8_FONT_5_ADDR + 1] = 0x80;              // *   
    C8_RAM[C8_FONT_5_ADDR + 2] = 0xF0;              // ****
    C8_RAM[C8_FONT_5_ADDR + 3] = 0x10;              //    *
    C8_RAM[C8_FONT_5_ADDR + 4] = 0xF0;              // ****
    
    C8_RAM[C8_FONT_6_ADDR] = 0xF0;                  // ****
    C8_RAM[C8_FONT_6_ADDR + 1] = 0x80;              // *   
    C8_RAM[C8_FONT_6_ADDR + 2] = 0xF0;              // ****
    C8_RAM[C8_FONT_6_ADDR + 3] = 0x90;              // *  *
    C8_RAM[C8_FONT_6_ADDR + 4] = 0xF0;              // ****
    
    C8_RAM[C8_FONT_7_ADDR] = 0xF0;                  // ****
    C8_RAM[C8_FONT_7_ADDR + 1] = 0x10;              //    *
    C8_RAM[C8_FONT_7_ADDR + 2] = 0x20;              //   * 
    C8_RAM[C8_FONT_7_ADDR + 3] = 0x40;              //  *  
    C8_RAM[C8_FONT_7_ADDR + 4] = 0x40;              //  *  
    
    C8_RAM[C8_FONT_8_ADDR] = 0xF0;                  // ****
    C8_RAM[C8_FONT_8_ADDR + 1] = 0x90;              // *  *
    C8_RAM[C8_FONT_8_ADDR + 2] = 0xF0;              // ****
    C8_RAM[C8_FONT_8_ADDR + 3] = 0x90;              // *  *
    C8_RAM[C8_FONT_8_ADDR + 4] = 0xF0;              // ****
    
    C8_RAM[C8_FONT_9_ADDR] = 0xF0;                  // ****
    C8_RAM[C8_FONT_9_ADDR + 1] = 0x90;              // *  *
    C8_RAM[C8_FONT_9_ADDR + 2] = 0xF0;              // ****
    C8_RAM[C8_FONT_9_ADDR + 3] = 0x10;              //    *
    C8_RAM[C8_FONT_9_ADDR + 4] = 0xF0;              // ****
    
    C8_RAM[C8_FONT_A_ADDR] = 0xF0;                  // ****
    C8_RAM[C8_FONT_A_ADDR + 1] = 0x90;              // *  *
    C8_RAM[C8_FONT_A_ADDR + 2] = 0xF0;              // ****
    C8_RAM[C8_FONT_A_ADDR + 3] = 0x90;              // *  *
    C8_RAM[C8_FONT_A_ADDR + 4] = 0x90;              // *  *
    
    C8_RAM[C8_FONT_B_ADDR] = 0xE0;                  // *** 
    C8_RAM[C8_FONT_B_ADDR + 1] = 0x90;              // *  *
    C8_RAM[C8_FONT_B_ADDR + 2] = 0xE0;              // *** 
    C8_RAM[C8_FONT_B_ADDR + 3] = 0x90;              // *  *
    C8_RAM[C8_FONT_B_ADDR + 4] = 0xE0;              // *** 
    
    C8_RAM[C8_FONT_C_ADDR] = 0xF0;                  // ****
    C8_RAM[C8_FONT_C_ADDR + 1] = 0x80;              // *   
    C8_RAM[C8_FONT_C_ADDR + 2] = 0x80;              // *   
    C8_RAM[C8_FONT_C_ADDR + 3] = 0x80;              // *   
    C8_RAM[C8_FONT_C_ADDR + 4] = 0xF0;              // ****
    
    C8_RAM[C8_FONT_D_ADDR] = 0xE0;                  // *** 
    C8_RAM[C8_FONT_D_ADDR + 1] = 0x90;              // *  *
    C8_RAM[C8_FONT_D_ADDR + 2] = 0x90;              // *  *
    C8_RAM[C8_FONT_D_ADDR + 3] = 0x90;              // *  *
    C8_RAM[C8_FONT_D_ADDR + 4] = 0xE0;              // *** 
    
    C8_RAM[C8_FONT_E_ADDR] = 0xF0;                  // ****
    C8_RAM[C8_FONT_E_ADDR + 1] = 0x80;              // *   
    C8_RAM[C8_FONT_E_ADDR + 2] = 0xF0;              // ****
    C8_RAM[C8_FONT_E_ADDR + 3] = 0x80;              // *   
    C8_RAM[C8_FONT_E_ADDR + 4] = 0xF0;              // ****
    
    C8_RAM[C8_FONT_F_ADDR] = 0xF0;                  // ****
    C8_RAM[C8_FONT_F_ADDR + 1] = 0x80;              // *   
    C8_RAM[C8_FONT_F_ADDR + 2] = 0xF0;              // ****
    C8_RAM[C8_FONT_F_ADDR + 3] = 0x80;              // *   
    C8_RAM[C8_FONT_F_ADDR + 4] = 0x80;              // *   
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
    memset(&C8_Buffer, false, sizeof(C8_Buffer));
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
        increment_program_counter(instruction);
    }
}

// Skip next instruction if Vx != kk.
// The interpreter compares register Vx to kk, and if they are not
// equal, increments the program counter by 2.
void C8_SNE_VX_BYTE(C8_Instruction *instruction)
{
    if (C8_V[instruction->x] != instruction->kk)
    {
        increment_program_counter(instruction);
    }
}

// Skip next instruction if Vx = Vy.
// The interpreter compares register Vx to register Vy, and if they
// are equal, increments the program counter by 2
void C8_SE_VX_VY(C8_Instruction *instruction)
{
    if (C8_V[instruction->x] == C8_V[instruction->y])
    {
        increment_program_counter(instruction);
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
        increment_program_counter(instruction);
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
    // Reset the collision flag.
    C8_V[C8_VF] = 0;

    // A sprite is a group of bytes which are a binary representation of the 
    // desired picture. Chip-8 sprites may be up to 15 bytes, for a possible 
    // sprite size of 8x15.
    unsigned char sprite[0xF];    
    for (unsigned char i = 0; i < instruction->n; i++)
    {
        sprite[i] = C8_RAM[C8_I + i];
    }
    
    for (unsigned char y = 0; y < instruction->n; y++)
    {
        unsigned char ypos = C8_V[instruction->y] + y;
        if (ypos > C8_HEIGHT)
        {
            ypos -= C8_HEIGHT;
        }
        
        for (unsigned char x = 0; x < 8; x++)
        {
            unsigned char xpos = C8_V[instruction->x] + x;
            if (xpos > C8_WIDTH)
            {
                xpos -= C8_WIDTH;
            }
            
            if ((sprite[y] & 0x80) > 0)
            {
                bool bit = (C8_Buffer[ypos][xpos]) ^ (sprite[y] >> x);
                C8_V[C8_VF] = !bit;
                C8_Buffer[ypos][xpos] = bit;
            }

            sprite[y] = sprite[y] << 1;
        }
    }
}

// Skip next instruction if key with the value of Vx is pressed.
// Checks the keyboard, and if the key corresponding to the value of Vx is 
// currently in the down position, PC is increased by 2.
void C8_SKP_VX(C8_Instruction *instruction)
{
    if (C8_Keyboard[C8_V[instruction->x]])
    {
        increment_program_counter(instruction);
    }
}

// Skip next instruction if key with the value of Vx is not pressed.
// Checks the keyboard, and if the key corresponding to the value of Vx
// is currently in the up position, PC is increased by 2.
void C8_SKNP_VX(C8_Instruction *instruction)
{
    if (!C8_Keyboard[C8_V[instruction->x]])
    {
        increment_program_counter(instruction);
    }
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
    instruction->skip = 1;
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
    for (int i = C8_V0; i <= instruction->x; i++)
    {
        C8_RAM[C8_I + i] = C8_V[i];
    }
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

void test_draw_font(unsigned char font, unsigned char xpos, unsigned char ypos)
{
    C8_I = font;
    current_instruction.n = 5;
    current_instruction.x = 0;
    current_instruction.y = 1;
    C8_V[0] = xpos;
    C8_V[1] = ypos;
    C8_DRW_VX_VY_NIBBLE(&current_instruction);
}

// Uses the C8_DRW_VX_VY_NIBBLE() function to draw the hexfont sprites to screen buffer.
void test_font()
{    
    test_draw_font(C8_FONT_0_ADDR, 1, 1);
    test_draw_font(C8_FONT_1_ADDR, 6, 1);
    test_draw_font(C8_FONT_2_ADDR, 11, 1);
    test_draw_font(C8_FONT_3_ADDR, 16, 1);

    test_draw_font(C8_FONT_4_ADDR, 1, 7);
    test_draw_font(C8_FONT_5_ADDR, 6, 7);
    test_draw_font(C8_FONT_6_ADDR, 11, 7);
    test_draw_font(C8_FONT_7_ADDR, 16, 7);
    
    test_draw_font(C8_FONT_8_ADDR, 1, 13);
    test_draw_font(C8_FONT_9_ADDR, 6, 13);
    test_draw_font(C8_FONT_A_ADDR, 11, 13);
    test_draw_font(C8_FONT_B_ADDR, 16, 13);
    
    test_draw_font(C8_FONT_C_ADDR, 1, 19);
    test_draw_font(C8_FONT_D_ADDR, 6, 19);
    test_draw_font(C8_FONT_E_ADDR, 11, 19);
    test_draw_font(C8_FONT_F_ADDR, 16, 19);
}