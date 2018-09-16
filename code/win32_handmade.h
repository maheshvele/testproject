#pragma once

struct win32_offscreen_buffer
{
	// NOTE(mahesh): Pixels are alwasy 32-bits wide, Memory Order BB GG RR XX
	BITMAPINFO Info;
	void *Memory;
	int Width;
	int Height;
	int Pitch;
	int BytesPerPixel;
};

struct win32_window_dimension
{
	int Width;
	int Height;
};

struct win32_sound_output
{
	int SamplesPerSecond;
	uint32 RunningSampleIndex;
	int BytesPerSample;
	DWORD SecondaryBufferSize;
	DWORD SafetyBytes;
	real32 tSine;
	int LatencySampleCount;
	// TODO(mahesh): Should running sample index be in bytes as well
	// TODO(mahesh): Math gets simpler if we add a "bytes per second" field?
};

struct win32_debug_time_marker
{
	DWORD OutputPlayCursor;
	DWORD OutputWriteCursor;
	DWORD OutputLocation;
	DWORD OutputByteCount;
	DWORD ExpectedFlipPlayCursor;

	DWORD FlipPlayCursor;
	DWORD FlipWriteCursor;
};