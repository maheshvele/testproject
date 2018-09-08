#include "handmade.h"


/*
* Little Endian for 0x0A0B0C0D
* Memory layout : a | a+1 | a+2 | a+3
*    	         0D   OC    0B   0A
* RR GG BB XX <- What you expected the order of bytes to be
* -> XX BB GG RR <- What appears on the Register
*  -> XX RR GG BB <- What windows uses on the Register
* Therefore, the Correct order BB GG RR XX
*
*/
LOCAL void RenderWeirdGradient(game_bitmap_buffer* Buffer, int BlueOffset, int GreenOffset)
{
	//Fill the memory
	int Height = Buffer->Height;
	int Width = Buffer->Width;

	int Pitch = Width * Buffer->BytesPerPixel;
	uint8* Row = (uint8 *)Buffer->Memory;

	for (int Y = 0; Y < Height; ++Y)
	{
		uint32 *Pixel = (uint32 *)Row;
		for (int X = 0; X < Width; ++X)
		{

			uint8 Blue = (uint8)X + (uint8)BlueOffset;
			uint8 Green = (uint8)Y + (uint8)GreenOffset;
			*Pixel = ((Green << 8) | Blue);
			++Pixel;
		}
		Row += Pitch;
	}
}

LOCAL void GameOutputSound(game_sound_output_buffer* SoundBuffer, int ToneHz)
{
	static real32 tSine;
	int16 ToneVolume = 1000;
	int WavePeriod = SoundBuffer->SamplesPerSecond / ToneHz;

	int16 *SampleOut = SoundBuffer->Samples;
	for (int SampleIndex = 0; SampleIndex < SoundBuffer->SampleCount; ++SampleIndex)
	{
		real32 SineValue = sinf(tSine);
		int16 SampleValue = (int16)(SineValue * ToneVolume);
		*SampleOut++ = SampleValue;
		*SampleOut++ = SampleValue;
		tSine += 2.0f * Pi32 * 1.0f / ((real32)WavePeriod);
	}
}

LOCAL void GameUpdateAndRender(game_input *Input, game_bitmap_buffer* Buffer, game_sound_output_buffer* SoundBuffer)
{
	LOCAL_PERSIST int BlueOffset = 0;
	LOCAL_PERSIST int GreenOffset = 0;
	LOCAL_PERSIST int ToneHz = 256;

	game_controller_input *Input0 = &Input->Controllers[0];

	if (Input0->IsAnalog)
	{
		ToneHz = 256 + (int)(128.0f * Input0->EndX);
		BlueOffset += (int)(4.0f * Input0->EndY);
	}
	else
	{
		// Use digital movement here!
	}

	if (Input0->Down.EndedDown)
	{
		GreenOffset += 1;
	}

	GameOutputSound(SoundBuffer, ToneHz);
	RenderWeirdGradient(Buffer, BlueOffset, GreenOffset);
}