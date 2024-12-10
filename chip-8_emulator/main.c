#include <SDL.h>
#include <SDL_mixer.h>
#include <stdio.h>
#include <stdbool.h>
#include <time.h>
#include <conio.h>
#include <stdlib.h>
#include <windows.h>

const int SCREEN_WIDTH = 64;
const int SCREEN_HEIGHT = 32;
#define CLOCK_SPEED 700 // 700 instructions per second
#define CLOCK_PERIOD (1/(float)CLOCK_SPEED) // Instruction period in nanoseconds
#define TIMER_SPEED 60 // 60Hz
#define TIMER_PERIOD (1/(float)TIMER_SPEED)

// Define registers
short program_counter = 0x200; // Interpreter memory ends at 0x1FF, program begins at 0x200
short index_register;
struct stack_struct
{
	short elements[32];
	short length;
};
struct stack_struct stack;
unsigned char delay_timer = 0b11111111; // 255
unsigned char sound_timer = 0b11111111;
unsigned char register_v[0x10];
unsigned char memory[0x1000]; // 4kB (4096 bits)
bool display[32][64] = {0};
bool keypad[16] = { 0 };

struct timespec ts;
long start_time;
long start_time_timer;
long end_time;
long end_time_timer;
long time_since_fetch;
long time_since_timer;
short cur_instruction;
unsigned char x_coord;
unsigned char y_coord;
int input_char;
char char_received;
FILE *ROM_file;
SDL_Renderer* renderer;
SDL_Event event;
SDL_Window* window = NULL;
int ROM_char;
unsigned char temp_reg_value;
bool quit = false;
int timespec_return;
unsigned char key_pressed_FX0A = 0x10;
bool key_up_event = false;
bool FX0A_started = false;
Mix_Chunk* beep_sound = NULL;

char emulation_type = 'c'; // "c" for Chip-8, "s" for Super-Chip, "x" for XO-Chip
bool step_through = false;

char filepath[] =
	"C:\\Users\\astee\\source\\repos\\chip-8_emulator\\ROMs\\superpong.ch8";

void stack_push(short push_value)
{
	stack.elements[stack.length] = push_value;
	stack.length++;
}

short stack_pop()
{
	short temp_stack_value = stack.elements[stack.length - 1];
	stack.elements[stack.length - 1] = 0x00;
	stack.length--;
	return temp_stack_value;
}

void initialize_memory()
{
	// Store font in interpreter's part of the memory (0x000-0x1FF)
	// Font consists of each hexadecimal character (0-F)
	// Each character is 4 pixels wide and 5 pixels tall
	// Hence use 5 'rows' of 4-bit words that map as pixels
	// Choosing to store it between 0x050-0x09F inclusive

	// 0
	memory[0x050] = 0xF0; // xxxx
	memory[0x051] = 0x90; // x..x
	memory[0x052] = 0x90; // x..x
	memory[0x053] = 0x90; // x..x
	memory[0x054] = 0xF0; // xxxx

	// 1
	memory[0x055] = 0x20; // ..x.
	memory[0x056] = 0x60; // x.x.
	memory[0x057] = 0x20; // ..x.
	memory[0x058] = 0x20; // ..x.
	memory[0x059] = 0x20; // ..x.

	// 2
	memory[0x05A] = 0xF0; // xxxx
	memory[0x05B] = 0x10; // ...x
	memory[0x05C] = 0xF0; // xxxx
	memory[0x05D] = 0x80; // x...
	memory[0x05E] = 0xF0; // xxxx

	// 3
	memory[0x05F] = 0xF0; // xxxx
	memory[0x060] = 0x10; // ...x
	memory[0x061] = 0xF0; // xxxx
	memory[0x062] = 0x10; // ...x
	memory[0x063] = 0xF0; // xxxx

	// 4
	memory[0x064] = 0x90; // x..x
	memory[0x065] = 0x90; // x..x
	memory[0x066] = 0xF0; // xxxx
	memory[0x067] = 0x10; // ...x
	memory[0x068] = 0x10; // ...x

	// 5
	memory[0x069] = 0xF0; // xxxx
	memory[0x06A] = 0x80; // x...
	memory[0x06B] = 0xF0; // xxxx
	memory[0x06C] = 0x10; // ...x
	memory[0x06D] = 0xF0; // xxxx

	// 6
	memory[0x06E] = 0xF0; // xxxx
	memory[0x06F] = 0x80; // x...
	memory[0x070] = 0xF0; // xxxx
	memory[0x071] = 0x90; // x..x
	memory[0x072] = 0xF0; // xxxx

	// 7
	memory[0x073] = 0xF0; // xxxx
	memory[0x074] = 0x10; // ...x
	memory[0x075] = 0x20; // ..x.
	memory[0x076] = 0x40; // .x..
	memory[0x077] = 0x40; // .x..

	// 8
	memory[0x078] = 0xF0; // xxxx
	memory[0x079] = 0x90; // x..x
	memory[0x07A] = 0xF0; // xxxx
	memory[0x07B] = 0x90; // x..x
	memory[0x07C] = 0xF0; // xxxx

	// 9
	memory[0x07D] = 0xF0; // xxxx
	memory[0x07E] = 0x90; // x..x
	memory[0x07F] = 0xF0; // xxxx
	memory[0x080] = 0x10; // ...x
	memory[0x081] = 0xF0; // xxxx

	// A
	memory[0x082] = 0xF0; // xxxx
	memory[0x083] = 0x90; // x..x
	memory[0x084] = 0xF0; // xxxx
	memory[0x085] = 0x90; // x..x
	memory[0x086] = 0x90; // x..x

	// B
	memory[0x087] = 0xE0; // xxx.
	memory[0x088] = 0x90; // x..x
	memory[0x089] = 0xE0; // xxx.
	memory[0x08A] = 0x90; // x..x
	memory[0x08B] = 0xE0; // xxx.

	// C
	memory[0x08C] = 0xF0; // xxxx
	memory[0x08D] = 0x80; // x...
	memory[0x08E] = 0x80; // x...
	memory[0x08F] = 0x80; // x...
	memory[0x090] = 0xF0; // xxxx

	// D
	memory[0x091] = 0xE0; // xxx.
	memory[0x092] = 0x90; // x..x
	memory[0x093] = 0x90; // x..x
	memory[0x094] = 0x90; // x..x
	memory[0x095] = 0xE0; // xxx.

	// E
	memory[0x096] = 0xF0; // xxxx
	memory[0x097] = 0x80; // x...
	memory[0x098] = 0xF0; // xxxx
	memory[0x099] = 0x80; // x...
	memory[0x09A] = 0xF0; // xxxx

	// F
	memory[0x09B] = 0xF0; // xxxx
	memory[0x09C] = 0x80; // x...
	memory[0x09D] = 0xF0; // xxxx
	memory[0x09E] = 0x80; // x...
	memory[0x09F] = 0x80; // x...

	// Start of instructions
	/*memory[0x200] = 0x60;
	memory[0x201] = 0x3F;
	memory[0x202] = 0x61;
	memory[0x203] = 0x1F;
	memory[0x204] = 0xA0;
	memory[0x205] = 0x9B;
	memory[0x206] = 0xD0;
	memory[0x207] = 0x15;*/
}

void copy_ROM_to_memory()
{
	fopen_s(&ROM_file, filepath, "rb");

	if (ROM_file == NULL)
	{
		perror("Failed to open file");
		return;
	}
	ROM_char = fgetc(ROM_file);
	while (ROM_char != EOF)
	{
		memory[program_counter] = (unsigned char)ROM_char;
		ROM_char = fgetc(ROM_file);
		program_counter++;
	}
	program_counter = 0x200;
	fclose(ROM_file);
}

void update_display(short instruction)
{
	if (register_v[(instruction & 0x0F00) >> 8] < 64)
	{
		x_coord = register_v[(instruction & 0x0F00) >> 8];
	}
	else
	{
		x_coord = register_v[(instruction & 0x0F00) >> 8] % 64;
	}
	if (register_v[(instruction & 0x00F0) >> 4] < 32)
	{
		y_coord = register_v[(instruction & 0x00F0) >> 4];
	}
	else
	{
		y_coord = register_v[(instruction & 0x00F0) >> 4] % 32;
	}

	register_v[0xF] = 0x00;

	for (unsigned char i = 0; i < 8; i++)
	{
		for (unsigned char j = 0; j < (instruction & 0x000F); j++)
		{
			if (x_coord + i >= 64 || y_coord + j >= 32)
			{
				break;
			}
			if (display[y_coord+j][x_coord+i] == false
				&& ((memory[index_register + j] >> (7 - i)) & 0b00000001) == 1)
			{
				display[y_coord+j][x_coord+i] = true;
			}
			else if (display[y_coord+j][x_coord+i] == true
				&& ((memory[index_register + j] >> (7 - i)) & 0b00000001) == 1)
			{
				display[y_coord+j][x_coord+i] = false;
				register_v[0xF] = 0x01;
			}
		}
		/*for (int y = 0; y < 32; y++)
		{
			for (int x = 0; x < 64; x++)
			{
				if (display[y][x])
				{
					printf("#");
				}
				else
				{
					printf(".");
				}
			}
			printf("\n");
		}
		printf("\n");*/
	}
}

void process_keydown_event(SDL_KeyboardEvent* key)
{
	switch (key->keysym.scancode)
	{
	case SDL_SCANCODE_1:
		keypad[0x1] = true;
		break;
	case SDL_SCANCODE_2:
		keypad[0x2] = true;
		break;
	case SDL_SCANCODE_3:
		keypad[0x3] = true;
		break;
	case SDL_SCANCODE_4:
		keypad[0xC] = true;
		break;
	case SDL_SCANCODE_Q:
		keypad[0x4] = true;
		break;
	case SDL_SCANCODE_W:
		keypad[0x5] = true;
		break;
	case SDL_SCANCODE_E:
		keypad[0x6] = true;
		break;
	case SDL_SCANCODE_R:
		keypad[0xD] = true;
		break;
	case SDL_SCANCODE_A:
		keypad[0x7] = true;
		break;
	case SDL_SCANCODE_S:
		keypad[0x8] = true;
		break;
	case SDL_SCANCODE_D:
		keypad[0x9] = true;
		break;
	case SDL_SCANCODE_F:
		keypad[0xE] = true;
		break;
	case SDL_SCANCODE_Z:
		keypad[0xA] = true;
		break;
	case SDL_SCANCODE_X:
		keypad[0x0] = true;
		break;
	case SDL_SCANCODE_C:
		keypad[0xB] = true;
		break;
	case SDL_SCANCODE_V:
		keypad[0xF] = true;
		break;
	case SDL_SCANCODE_ESCAPE:
		// quit
		quit = true;
		break;
	}
}

void process_keyup_event(SDL_KeyboardEvent* key)
{
	switch (key->keysym.scancode)
	{
	case SDL_SCANCODE_1:
		keypad[0x1] = false;
		key_pressed_FX0A = 0x1;
		break;
	case SDL_SCANCODE_2:
		keypad[0x2] = false;
		key_pressed_FX0A = 0x2;
		break;
	case SDL_SCANCODE_3:
		keypad[0x3] = false;
		key_pressed_FX0A = 0x3;
		break;
	case SDL_SCANCODE_4:
		keypad[0xC] = false;
		key_pressed_FX0A = 0xC;
		break;
	case SDL_SCANCODE_Q:
		keypad[0x4] = false;
		key_pressed_FX0A = 0x4;
		break;
	case SDL_SCANCODE_W:
		keypad[0x5] = false;
		key_pressed_FX0A = 0x5;
		break;
	case SDL_SCANCODE_E:
		keypad[0x6] = false;
		key_pressed_FX0A = 0x6;
		break;
	case SDL_SCANCODE_R:
		keypad[0xD] = false;
		key_pressed_FX0A = 0xD;
		break;
	case SDL_SCANCODE_A:
		keypad[0x7] = false;
		key_pressed_FX0A = 0x7;
		break;
	case SDL_SCANCODE_S:
		keypad[0x8] = false;
		key_pressed_FX0A = 0x8;
		break;
	case SDL_SCANCODE_D:
		keypad[0x9] = false;
		key_pressed_FX0A = 0x9;
		break;
	case SDL_SCANCODE_F:
		keypad[0xE] = false;
		key_pressed_FX0A = 0xE;
		break;
	case SDL_SCANCODE_Z:
		keypad[0xA] = false;
		key_pressed_FX0A = 0xA;
		break;
	case SDL_SCANCODE_X:
		keypad[0x0] = false;
		key_pressed_FX0A = 0x0;
		break;
	case SDL_SCANCODE_C:
		keypad[0xB] = false;
		key_pressed_FX0A = 0xB;
		break;
	case SDL_SCANCODE_V:
		keypad[0xF] = false;
		key_pressed_FX0A = 0xF;
		break;
	}
}

void execute_zero_instructions(short instruction)
{
	if (instruction == 0x00E0)
	{
		// 00E0: Clear the screen

		SDL_SetRenderDrawColor(renderer, 79, 58, 42, 255);
		SDL_RenderClear(renderer);
		SDL_RenderPresent(renderer);
		
		// Clear display buffer
		for (int x = 0; x < 64; x++)
		{
			for (int y = 0; y < 32; y++)
			{
				display[y][x] = false;
			}
		}
	}
	else if (instruction == 0x00EE)
	{
		// 00EE: Return from a subroutine

		program_counter = stack_pop();
	}
}

void execute_eight_instructions(short instruction)
{
	switch (instruction & 0x000F)
	{
	case 0:
		// 8XY0: Store the value of register VY in register VX
		register_v[(instruction & 0x0F00) >> 8] = register_v[(instruction & 0x00F0) >> 4];
		break;

	case 1:
		// 8XY1: Set VX to VX OR VY
		register_v[(instruction & 0x0F00) >> 8] = 
			register_v[(instruction & 0x0F00) >> 8] | register_v[(instruction & 0x00F0) >> 4];
		register_v[0xF] = 0;
		break;

	case 2:
		// 8XY2: Set VX to VX AND VY
		register_v[(instruction & 0x0F00) >> 8] = 
			register_v[(instruction & 0x0F00) >> 8] & register_v[(instruction & 0x00F0) >> 4];
		register_v[0xF] = 0;
		break;

	case 3:
		// 8XY3: Set VX to VX XOR VY
		register_v[(instruction & 0x0F00) >> 8] = 
			register_v[(instruction & 0x0F00) >> 8] ^ register_v[(instruction & 0x00F0) >> 4];
		register_v[0xF] = 0;
		break;

	case 4:
		// 8XY4: Add the value of register VY to register VX
		// Set VF to 01 if a carry occurs
		// Set VF to 00 if a carry does not occur
		if ((int)(register_v[(instruction & 0x0F00) >> 8] + register_v[(instruction & 0x00F0) >> 4]) > 0xFF)
		{
			register_v[(instruction & 0x0F00) >> 8] = register_v[(instruction & 0x0F00) >> 8] + register_v[(instruction & 0x00F0) >> 4];
			register_v[0xF] = 0x01;
		}
		else
		{
			register_v[(instruction & 0x0F00) >> 8] = register_v[(instruction & 0x0F00) >> 8] + register_v[(instruction & 0x00F0) >> 4];
			register_v[0xF] = 0x00;
		}
		break;

	case 5:
		// 8XY5: Subtract the value of register VY from register VX
		// Set VF to 00 if a borrow occurs
		// Set VF to 01 if a borrow does not occur
		if (register_v[(instruction & 0x0F00) >> 8] < register_v[(instruction & 0x00F0) >> 4])
		{
			register_v[(instruction & 0x0F00) >> 8] = register_v[(instruction & 0x0F00) >> 8] - register_v[(instruction & 0x00F0) >> 4];
			register_v[0xF] = 0x00;
		}
		else
		{
			register_v[(instruction & 0x0F00) >> 8] = register_v[(instruction & 0x0F00) >> 8] - register_v[(instruction & 0x00F0) >> 4];
			register_v[0xF] = 0x01;
		}
		break;

	case 6:
		// 8XY6: Store the value of register VY shifted right one bit in register VX¹
		// Set register VF to the least significant bit prior to the shift
		// VY is unchanged
		temp_reg_value = register_v[(instruction & 0x00F0) >> 4] & 0b00000001;
		register_v[(instruction & 0x0F00) >> 8] = register_v[(instruction & 0x00F0) >> 4] >> 1;
		register_v[0xF] = temp_reg_value;
		break;

	case 7:
		// 8XY7: Set register VX to the value of VY minus VX
		// Set VF to 00 if a borrow occurs
		// Set VF to 01 if a borrow does not occur
		if (register_v[(instruction & 0x0F00) >> 8] > register_v[(instruction & 0x00F0) >> 4])
		{
			register_v[(instruction & 0x0F00) >> 8] = register_v[(instruction & 0x00F0) >> 4] - register_v[(instruction & 0x0F00) >> 8];
			register_v[0xF] = 0x00;
		}
		else
		{
			register_v[(instruction & 0x0F00) >> 8] = register_v[(instruction & 0x00F0) >> 4] - register_v[(instruction & 0x0F00) >> 8];
			register_v[0xF] = 0x01;
		}
		break;

	case 0xE:
		// 8XYE: Store the value of register VY shifted left one bit in register VX¹
		// Set register VF to the most significant bit prior to the shift
		// VY is unchanged
		temp_reg_value = (register_v[(instruction & 0x00F0) >> 4] & 0b10000000) >> 7;
		register_v[(instruction & 0x0F00) >> 8] = register_v[(instruction & 0x00F0) >> 4] << 1;
		register_v[0xF] = temp_reg_value;
		break;
	}
}

void execute_E_instructions(short instruction)
{
	if ((instruction & 0x00FF) == 0x9E)
	{
		// EX9E: Skip the following instruction if the key corresponding to the hex value currently stored in
		// register VX is pressed
		if (keypad[register_v[(instruction & 0x0F00) >> 8]])
		{
			program_counter += 2;
		}
	}
	else if ((instruction & 0x00FF) == 0xA1)
	{
		// EXA1: Skip the following instruction if the key corresponding to the hex value currently stored in
		// register VX is not pressed
		if (!keypad[register_v[(instruction & 0x0F00) >> 8]])
		{
			program_counter += 2;
		}
	}
}

void execute_F_instructions(short instruction)
{
	switch (instruction & 0x000F)
	{
	case 3:
		// FX33: Store the binary-coded decimal equivalent of the value stored in register VX at addresses I, 
		// I + 1, and I + 2
		memory[index_register] = register_v[(instruction & 0x0F00) >> 8] / 100;
		memory[index_register + 1] = (register_v[(instruction & 0x0F00) >> 8] % 100) / 10;
		memory[index_register + 2] = register_v[(instruction & 0x0F00) >> 8] % 10;
		break;

	case 5:
		if ((instruction & 0x00F0) >> 4 == 0x1)
		{
			// FX15: Set the delay timer to the value of register VX
			delay_timer = register_v[(instruction & 0x0F00) >> 8];
		}
		else if ((instruction & 0x00F0) >> 4 == 0x5)
		{
			// FX55: Store the values of registers V0 to VX inclusive in memory starting at address I
			// I is set to I + X + 1 after operation²
			int temp_index_reg = index_register;
			for (int i = 0; i <= (int)((instruction & 0x0F00) >> 8); i++)
			{
				memory[temp_index_reg] = register_v[i];
				temp_index_reg++;
			}
			if (emulation_type == 'c')
			{
				index_register = temp_index_reg;
			}
		}
		else if ((instruction & 0x00F0) >> 4 == 0x6)
		{
			// FX65: Fill registers V0 to VX inclusive with the values stored in memory starting at address I
			// I is set to I + X + 1 after operation²
			int temp_index_reg = index_register;
			for (int i = 0; i <= (int)((instruction & 0x0F00) >> 8); i++)
			{
				register_v[i] = memory[temp_index_reg];
				temp_index_reg++;
			}
			if (emulation_type == 'c')
			{
				index_register = temp_index_reg;
			}
		}
		break;
	case 7:
		// FX07: Store the current value of the delay timer in register VX
		register_v[(instruction & 0x0F00) >> 8] = delay_timer;
		break;

	case 8:
		// FX18: Set the sound timer to the value of register VX
		sound_timer = register_v[(instruction & 0x0F00) >> 8];
		break;

	case 9:
		// FX29: Set I to the memory address of the sprite data corresponding to the hexadecimal digit stored 
		// in register VX
		index_register = 0x50 + (register_v[(instruction & 0x0F00) >> 8] * 5);
		break;

	case 0xA:
		// FX0A: Wait for a keypress and store the result in register VX

		FX0A_started = true;
		if (key_up_event)
		{
			register_v[(instruction & 0x0F00) >> 8] = key_pressed_FX0A;
			FX0A_started = false;
			key_up_event = false;
		}
		else
		{
			program_counter -= 2;
		}
		break;

	case 0xE:
		// FX1E: Add the value stored in register VX to register I
		index_register += register_v[(instruction & 0x0F00) >> 8];
		break;
	}
}

void execute_instruction(short instruction)
{
	switch (instruction & 0xF000) // Mask off last 12 bits, leaving only first 4 bits
	{
	case 0x0000:
		execute_zero_instructions(instruction);
		break;

	case 0x1000:
		// 1NNN: Jump to address NNN

		program_counter = instruction & 0x0FFF;
		break;

	case 0x2000:
		// 2NNN: Execute subroutine starting at address NNN

		stack_push(program_counter);
		program_counter = instruction & 0x0FFF;
		break;

	case 0x3000:
		// 3XNN: Skip the following instruction if the value of register VX equals NN
		if (register_v[(instruction & 0x0F00) >> 8] == (unsigned char)(instruction & 0x00FF))
		{
			program_counter += 2;
		}
		break;

	case 0x4000:
		// 4XNN: Skip the following instruction if the value of register VX is not equal to NN
		if (register_v[(instruction & 0x0F00) >> 8] != (unsigned char)(instruction & 0x00FF))
		{
			program_counter += 2;
		}
		break;

	case 0x5000:
		// 5XY0: Skip the following instruction if the value of register VX is equal to the value of register VY
		if (register_v[(instruction & 0x0F00) >> 8] == register_v[(instruction & 0x00F0) >> 4])
		{
			program_counter += 2;
		}
		break;

	case 0x6000:
		// 6XNN: Store number NN in register VX
		register_v[(instruction & 0x0F00) >> 8] = instruction & 0x00FF;
		break;

	case 0x7000:
		// 7XNN: Add the value NN to register VX
		register_v[(instruction & 0x0F00) >> 8] += instruction & 0x00FF;
		break;

	case 0x8000:
		execute_eight_instructions(instruction);
		break;

	case 0x9000:
		// 9XY0: Skip the following instruction if the value of register VX is not equal to the value of 
		// register VY
		if (register_v[(instruction & 0x0F00) >> 8] != register_v[(instruction & 0x00F0) >> 4])
		{
			program_counter += 2;
		}
		break;

	case 0xA000:
		// ANNN: Store memory address NNN in register I
		index_register = instruction & 0x0FFF;
		break;

	case 0xB000:
		// BNNN: Jump to address NNN + V0
		program_counter = (instruction & 0x0FFF) + register_v[0];
		break;
		
	case 0xC000:
		// CXNN: Set VX to a random number with a mask of NN
		register_v[(instruction & 0x0F00) >> 8] = rand() & (instruction & 0x00FF);
		break;

	case 0xD000:
		// DXYN: Draw a sprite at position VX, VY with N bytes of sprite data starting at the address stored in I
		// Set VF to 01 if any set pixels are changed to unset, and 00 otherwise
		update_display(instruction);
		break;

	case 0xE000:
		execute_E_instructions(instruction);
		break;

	case 0xF000:
		execute_F_instructions(instruction);
		break;
	}
}

void process_not_step_through()
{
	timespec_return = timespec_get(&ts, TIME_UTC);
	end_time = ts.tv_nsec;
	time_since_fetch = end_time - start_time;
	if (time_since_fetch < 0)
	{
		start_time = ts.tv_nsec;
		time_since_fetch = end_time - start_time;
	}
	if ((float)time_since_fetch / (float)1000000000 > CLOCK_PERIOD)
	{
		cur_instruction = (memory[program_counter] << 8) | (memory[program_counter + 1]);
		program_counter += 2;

		execute_instruction(cur_instruction);

		timespec_return = timespec_get(&ts, TIME_UTC);
		start_time = ts.tv_nsec;
	}
}

void process_step_through()
{
	input_char = _getch();
	if (input_char == 108) // L
	{
		printf("Registers: ");
		for (int i = 0; i < 16; i++)
		{
			printf("V%X: %02x ", i, register_v[i]);
		}
		printf("Index register: %03X ", index_register);
		printf("Delay timer: %03X ", delay_timer);
		printf("Sound timer: %03X\n", sound_timer);

	}
	else if (input_char == 13) // Enter
	{
		cur_instruction = (memory[program_counter] << 8) | (memory[program_counter + 1]);
		printf("Current instruction: %04hX, Program counter: %hX.\n", cur_instruction, program_counter);
		program_counter += 2;
		execute_instruction(cur_instruction);
	}
	else if (input_char == 27) // Esc
	{
		quit = true;
	}
}

void play_sound()
{
	if (Mix_Playing(-1) == 0)
	{
		Mix_PlayChannel(-1, beep_sound, 0);
	}
}

DWORD WINAPI SDL_worker_thread()
{

	if (SDL_Init(SDL_INIT_VIDEO) < 0)
	{
		printf("SDL could not initialise! SDL_Error: %s\n", SDL_GetError());
		return 1;
	}

	SDL_CreateWindowAndRenderer(SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN, &window, &renderer);
	SDL_SetWindowTitle(window, "Chip-8 Emulator");
	SDL_RenderSetLogicalSize(renderer, 64, 32);
	SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN_DESKTOP);
	SDL_SetRenderDrawColor(renderer, 79, 58, 42, 255);
	SDL_RenderClear(renderer);
	Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048);
	beep_sound = Mix_LoadWAV("C:\\Users\\astee\\source\\repos\\chip-8_emulator\\beep-01b.wav");
	if (window == NULL)
	{
		printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
		return 1;
	}

	while (!quit)
	{
		SDL_PollEvent(&event);
		switch (event.type)
		{
		case SDL_KEYDOWN:
			process_keydown_event(&event.key);
			break;
		case SDL_KEYUP:
			process_keyup_event(&event.key);
			if (FX0A_started)
			{
				key_up_event = true;
			}
			break;
		case SDL_QUIT:
			quit = true;
			break;
		}
		timespec_return = timespec_get(&ts, TIME_UTC);
		end_time_timer = ts.tv_nsec;
		time_since_timer = end_time_timer - start_time_timer;
		if (time_since_timer < 0)
		{
			timespec_return = timespec_get(&ts, TIME_UTC);
			start_time_timer = ts.tv_nsec;
			time_since_timer = end_time_timer - start_time_timer;
		}
		if ((float)time_since_timer / (float)1000000000 > TIMER_PERIOD)
		{
			if (delay_timer > 0)
			{
				delay_timer--;
			}
			if (sound_timer > 0)
			{
				play_sound();
				sound_timer--;
			}

			for (int i = 0; i < 32; i++)
			{
				for (int j = 0; j < 64; j++)
				{
					if (display[i][j])
					{
						SDL_SetRenderDrawColor(renderer, 242, 200, 167, 255);
						SDL_RenderDrawPoint(renderer, j, i);
					}
					else
					{
						SDL_SetRenderDrawColor(renderer, 79, 58, 42, 255);
						SDL_RenderDrawPoint(renderer, j, i);
					}
				}
			}
			SDL_RenderPresent(renderer);

			timespec_return = timespec_get(&ts, TIME_UTC);
			start_time_timer = ts.tv_nsec;
		}
	}
	return 0;
}

int main( int argc, char *args[] )
{
	srand( (unsigned int) time(NULL));
	initialize_memory();
	copy_ROM_to_memory();


	// Create window and start keyboard checking thread
	HANDLE key_press_thread = CreateThread(NULL, 0, SDL_worker_thread, NULL, 0, NULL);

	timespec_return = timespec_get(&ts, TIME_UTC);
	start_time = ts.tv_nsec;
	start_time_timer = ts.tv_nsec;

	while (!quit) {
		if (!step_through)
		{
			process_not_step_through();
		}
		else if (step_through)
		{
			process_step_through();
		}
	}
	//Destroy window
	Mix_FreeChunk(beep_sound);
	beep_sound = NULL;
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	//Quit SDL subsystems
	Mix_Quit();
	SDL_Quit();
	return 0;
}