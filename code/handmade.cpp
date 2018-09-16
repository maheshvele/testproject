
/*
* Little Endian for 0x0A0B0C0D
* Memory layout : a | a+1 | a+2 | a+3
*      	          0D   OC    0B   0A
*    RR GG BB XX <- What you expected the order of bytes to be
* -> XX BB GG RR <- What appears on the Register
* -> XX RR GG BB <- What windows uses on the Register
* Therefore, the Correct order BB GG RR XX
*
*/

#include "handmade.h"

internal void GameOutputSound(game_sound_output_buffer *SoundBuffer, int ToneHz)
{
	local_persist real32 tSine;
	int16 ToneVolume = 3000;
	int WavePeriod = SoundBuffer->SamplesPerSecond / ToneHz;

	int16 *SampleOut = SoundBuffer->Samples;
	for (int SampleIndex = 0;
		SampleIndex < SoundBuffer->SampleCount;
		++SampleIndex)
	{
		// TODO(mahesh): Draw this out for people
		real32 SineValue = sinf(tSine);
		int16 SampleValue = (int16)(SineValue * ToneVolume);
		*SampleOut++ = SampleValue;
		*SampleOut++ = SampleValue;

		tSine += 2.0f*Pi32*1.0f / (real32)WavePeriod;
		if (tSine > 2.0f*Pi32)
		{
			tSine -= 2.0f*Pi32;
		}
	}
}

internal void RenderWeirdGradient(game_offscreen_buffer *Buffer, int BlueOffset, int GreenOffset)
{
	// TODO(mahesh): Let's see what the optimizer does

	uint8 *Row = (uint8 *)Buffer->Memory;
	for (int Y = 0;
		Y < Buffer->Height;
		++Y)
	{
		uint32 *Pixel = (uint32 *)Row;
		for (int X = 0;
			X < Buffer->Width;
			++X)
		{
			uint8 Blue = (uint8)(X + BlueOffset);
			uint8 Green = (uint8)(Y + GreenOffset);

			*Pixel++ = ((Green << 8) | Blue);
		}

		Row += Buffer->Pitch;
	}
}

internal void GameUpdateAndRender(game_memory *Memory, game_input *Input, game_offscreen_buffer *Buffer)
{
	Assert((&Input->Controllers[0].Terminator - &Input->Controllers[0].Buttons[0]) ==
		(ArrayCount(Input->Controllers[0].Buttons)));
	Assert(sizeof(game_state) <= Memory->PermanentStorageSize);

	game_state *GameState = (game_state *)Memory->PermanentStorage;
	if (!Memory->IsInitialized)
	{
		char *Filename = __FILE__;

		debug_read_file_result File = DEBUGPlatformReadEntireFile(Filename);
		if (File.Contents)
		{
			DEBUGPlatformWriteEntireFile("test.out", File.ContentsSize, File.Contents);
			DEBUGPlatformFreeFileMemory(File.Contents);
		}

		GameState->ToneHz = 512;

		// TODO(mahesh): This may be more appropriate to do in the platform layer
		Memory->IsInitialized = true;
	}

	for (int ControllerIndex = 0;
		ControllerIndex < ArrayCount(Input->Controllers);
		++ControllerIndex)
	{
		game_controller_input *Controller = GetController(Input, ControllerIndex);
		if (Controller->IsAnalog)
		{
			// NOTE(mahesh): Use analog movement tuning
			GameState->BlueOffset += (int)(4.0f*Controller->StickAverageX);
			GameState->ToneHz = 512 + (int)(128.0f*Controller->StickAverageY);
		}
		else
		{
			int MovementSpeed = 1;
			// NOTE(mahesh): Use digital movement tuning
			if (Controller->MoveLeft.EndedDown)
			{
				GameState->BlueOffset -= MovementSpeed;
			}

			if (Controller->MoveRight.EndedDown)
			{
				GameState->BlueOffset += MovementSpeed;
			}

			if (Controller->MoveUp.EndedDown)
			{
				GameState->GreenOffset -= MovementSpeed;
			}

			if (Controller->MoveDown.EndedDown)
			{
				GameState->GreenOffset += MovementSpeed;
			}
		}

		// Input.AButtonEndedDown;
		// Input.AButtonHalfTransitionCount;
		if (Controller->ActionDown.EndedDown)
		{
			GameState->GreenOffset += 1;
		}
	}

	RenderWeirdGradient(Buffer, GameState->BlueOffset, GameState->GreenOffset);
}

internal void
GameGetSoundSamples(game_memory *Memory, game_sound_output_buffer *SoundBuffer)
{
	game_state *GameState = (game_state *)Memory->PermanentStorage;
	GameOutputSound(SoundBuffer, GameState->ToneHz);
}
