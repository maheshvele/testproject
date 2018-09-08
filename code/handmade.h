#pragma once

#include <stdint.h>
#include <stdio.h>
#include <math.h>


#define GLOBAL static
#define LOCAL static
#define LOCAL_PERSIST static


typedef uint8_t uint8;
typedef uint32_t uint32;
typedef uint64_t uint64;
typedef uint16_t uint16;

typedef int8_t int8;
typedef int32_t int32;
typedef int64_t int64;
typedef int16_t int16;

typedef int32 bool32;

typedef float real32;
typedef double real64;

#define Pi32 3.14159265359f

typedef struct game_bitmap_buffer 
{
	void* Memory;
	int Width;
	int Height;
	int BytesPerPixel;
} game_bitmap_buffer;

typedef struct game_sound_output_buffer
{
	int SampleCount;
	int SamplesPerSecond;
	int16* Samples;
} game_sound_output_buffer;

typedef struct game_button_state
{
	int HalfTransitionCount;
	bool32 EndedDown;
} game_button_state;

typedef struct game_controller_input
{

	bool32 IsAnalog;

	real32 StartX;
	real32 StartY;

	real32 MaxX;
	real32 MaxY;

	real32 MinX;
	real32 MinY;

	real32 EndX;
	real32 EndY;

	union 
	{
		game_button_state Buttons[6];
		struct 
		{
			game_button_state Up;
			game_button_state Down;
			game_button_state Left;
			game_button_state Right;
			game_button_state LeftShoulder;
			game_button_state RightShoulder;
		};
	};

} game_controller_input;

typedef struct game_input
{
	game_controller_input Controllers[4];
} game_input;

LOCAL void GameUpdateAndRender(game_bitmap_buffer* Buffer, uint8 BlueOffset, uint8 GreenOffset, game_sound_output_buffer* SoundBuffer, int ToneHz);