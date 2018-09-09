
/*
	TODO List

- Saved Game locations'
- Creating a handle to our executable file
- Asset loading path
- Multithreading (launch a thread)
- Raw Input
- sleep/timeBeginPeriod
- Multi monitor support
- FullScreen
- WM_SETCURSOR 
- QueryCancelAutoplay
- WM_ACTIVEAPP
- Blit speed improvements
- Hardware acceleration (OpenGL or Direct 3D)
- GetKeyboardLayout (for french keyboards, international WASD support)
*/

#include <Windows.h>
#include <Xinput.h>
#include <DSound.h>
#include "win32_handmade.h"
#include "handmade.cpp"

GLOBAL bool Running;

typedef struct win32_bitmap_buffer {
	BITMAPINFO Info;
	void* Memory;
	int Width;
	int Height;
	int BytesPerPixel;
} win32_bitmap_buffer;

typedef struct window_dimensions {
	int Width;
	int Height;
}window_dimensions;

GLOBAL win32_bitmap_buffer GlobalBuffer;

LOCAL game_file_data DEBUGPlatformReadEntireFileName(char* FileName)
{
	game_file_data Result = {};
	// OpenFile
	HANDLE FileHandle = CreateFileA(FileName, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
	if (FileHandle != INVALID_HANDLE_VALUE)
	{
		LARGE_INTEGER FileSize ;
		if (GetFileSizeEx(FileHandle, &FileSize))
		{
			//TODO: Define for MaxValues
			Assert(FileSize.QuadPart <= 0xFFFFFFFF);
			Result.Contents = VirtualAlloc(0, FileSize.QuadPart, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
			if (Result.Contents)
			{
				uint32 FileSize32 = SafeTruncateUint64(FileSize.QuadPart);
				DWORD numberOfBytesRead;
				if (ReadFile(FileHandle, Result.Contents, FileSize32, &numberOfBytesRead, 0) && 
					FileSize32 == numberOfBytesRead)
				{
					Result.ContentSize = FileSize32;
				}
				else
				{
					DEBUGPlatformFreeMemory(Result.Contents);
					Result.Contents = nullptr;
				}
			}
		}

		CloseHandle(FileHandle);
	}

	return Result;
}

LOCAL void DEBUGPlatformFreeMemory(void *Memory)
{
	if (Memory)
	{
		VirtualFree(Memory, NULL, MEM_RELEASE);
	}
}

LOCAL bool32 DEBUGPlatformWriteEntireFileName(char* FileName, uint32 MemorySize, void * Memory)
{
	bool32 Result =  false;

	HANDLE FileHandle = CreateFileA(FileName, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);
	if (FileHandle != INVALID_HANDLE_VALUE)
	{
		DWORD BytesWritten;
		if (WriteFile(FileHandle, Memory, MemorySize, &BytesWritten, 0))
		{
			Result = (BytesWritten == MemorySize);
		}
		else
		{

		}

		CloseHandle(FileHandle);
	}
	else
	{
		//TODO: Logging
	}

	return Result;
}

/*
* This method gets called when the window size changes. 
* Create the new buffer and destroy the old one here
* DIB - Device Independent Bitmap
*/
LOCAL void
Win32ResizeDIBSection(win32_bitmap_buffer *Buffer, window_dimensions Dimensions)
{
	if (Buffer->Memory)
	{
		VirtualFree(Buffer->Memory, NULL, MEM_RELEASE);
	}

	Buffer->Width = Dimensions.Width;
	Buffer->Height = Dimensions.Height;
	Buffer->BytesPerPixel = 4;

	Buffer->Info.bmiHeader.biSize = sizeof(Buffer->Info.bmiHeader);
	Buffer->Info.bmiHeader.biWidth = Buffer->Width;
	Buffer->Info.bmiHeader.biHeight = -Buffer->Height;  //Rows start from top and go down if this is negative.
	Buffer->Info.bmiHeader.biPlanes = 1;
	Buffer->Info.bmiHeader.biBitCount = 32;
	Buffer->Info.bmiHeader.biCompression = BI_RGB;


	int BitmapMemorySize = Buffer->BytesPerPixel * Buffer->Width * Buffer->Height;

	Buffer->Memory = VirtualAlloc(0, BitmapMemorySize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
}

LOCAL window_dimensions
Win32GetWindowDimensions(HWND WindowHandle)
{
	window_dimensions Dimensions;
	RECT WindowRect;

	GetClientRect(WindowHandle, &WindowRect);
	
	Dimensions.Height = WindowRect.bottom - WindowRect.top;
	Dimensions.Width = WindowRect.right - WindowRect.left;

	return Dimensions;
}

#define X_INPUT_GET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_STATE* pState)
typedef X_INPUT_GET_STATE(x_input_get_state);
X_INPUT_GET_STATE(XInputGetStateStub)
{
	return 0;
}
GLOBAL x_input_get_state *XInputGetState_ = XInputGetStateStub;
#define XInputGetState XInputGetState_

#define X_INPUT_SET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_VIBRATION* pVibration)
typedef X_INPUT_SET_STATE(x_input_set_state);
X_INPUT_SET_STATE(XInputSetStateStub)
{
	return 0;
}
GLOBAL x_input_set_state *XInputSetState_ = XInputSetStateStub;
#define XInputSetState XInputSetState_

LOCAL void Win32LoadXInput() 
{
	HMODULE XInputLibrary = LoadLibraryA("xinput1_3.dll");
	if (!XInputLibrary)
	{
		//TODO : Mahesh Log here
		XInputLibrary = LoadLibraryA("xinput1_3.dll");
	}

	if (XInputLibrary) 
	{
		XInputGetState = (x_input_get_state *)GetProcAddress(XInputLibrary, "XInputGetState");
		XInputSetState = (x_input_set_state *)GetProcAddress(XInputLibrary, "XInputSetState");
	}
	else
	{
		//TODO: Mahesh log here
	}
}

LOCAL void Win32ProcessXInputDigitalButton(DWORD XInputButtonState, game_button_state* OldState, DWORD ButtonBit, game_button_state* NewState)
{
	NewState->EndedDown = ((XInputButtonState & ButtonBit) == ButtonBit);
	NewState->HalfTransitionCount = (NewState->EndedDown != OldState->EndedDown) ? 1 : 0;
}


#define DIRECT_SOUND_CREATE(name) HRESULT WINAPI name(LPCGUID pcGuidDevice, LPDIRECTSOUND *ppDS, LPUNKNOWN pUnkOuter);
typedef DIRECT_SOUND_CREATE(direct_sound_create);

GLOBAL LPDIRECTSOUNDBUFFER GlobalSecondaryBuffer;

LOCAL void Win32InitDirectSound(HWND Window, int32 SamplesPerSec, int32 BufferSize)
{
	//Load DirectSound library - Its a object oriented library
	HMODULE DSoundLibrary = LoadLibraryA("dsound.dll");

	if (DSoundLibrary)
	{
		direct_sound_create *DirectSoundCreate = (direct_sound_create *)GetProcAddress(DSoundLibrary, "DirectSoundCreate");

		// Get DirectSound Object
		IDirectSound *DirectSoundObject;
		if (DirectSoundCreate && SUCCEEDED(DirectSoundCreate(0, &DirectSoundObject, 0)))
		{
			WAVEFORMATEX WaveFormat = {};
			WaveFormat.wFormatTag = WAVE_FORMAT_PCM;
			WaveFormat.nSamplesPerSec = SamplesPerSec;
			WaveFormat.nChannels = 2; // L & R
			WaveFormat.wBitsPerSample = 16;
			WaveFormat.nBlockAlign = (WaveFormat.nChannels * WaveFormat.wBitsPerSample) / 8; // Number of bytes for [Left Right]
			WaveFormat.nAvgBytesPerSec = WaveFormat.nSamplesPerSec * WaveFormat.nBlockAlign;
			WaveFormat.cbSize = 0;

			if (SUCCEEDED(DirectSoundObject->SetCooperativeLevel(Window, DSSCL_PRIORITY)))
			{
				// Get Primary Buffer
				
				DSBUFFERDESC BufferDescription = {};
				BufferDescription.dwSize = sizeof(DSBUFFERDESC);
				BufferDescription.dwFlags = DSBCAPS_PRIMARYBUFFER;

				LPDIRECTSOUNDBUFFER PrimaryBuffer;
				if (SUCCEEDED(DirectSoundObject->CreateSoundBuffer(&BufferDescription, &PrimaryBuffer, 0)))
				{
					// Samples are written as follows: 
					// LEFT(16 bits) RIGHT (16 bits) LEFT RIGHT.....

					if (SUCCEEDED(PrimaryBuffer->SetFormat(&WaveFormat)))
					{
						OutputDebugStringA("Primary Buffer has been created!\n");
					}
					else
					{
						//TODO Mahesh Log
					}
				}
				else
				{
					// TODO Mahesh Log
				}

				// Get Secondary Buffer
			}
			else
			{ 
				// TODO: Mahesh log here
			}
			
			DSBUFFERDESC BufferDescription = {};
			BufferDescription.dwSize = sizeof(DSBUFFERDESC);
			BufferDescription.dwFlags = 0;
			BufferDescription.lpwfxFormat = &WaveFormat;
			BufferDescription.dwBufferBytes = BufferSize;
			
			
			HRESULT error = DirectSoundObject->CreateSoundBuffer(&BufferDescription, &GlobalSecondaryBuffer, 0);

			if (SUCCEEDED(error))
			{
				OutputDebugStringA("Secondary Buffer has been created!\n");
			}
			else
			{
				//TODO Mahesh log here
			}

		}
		else
		{
			//TODO : Mahesh Log here
		}
	}
	else
	{
		//TODO : Mahesh Log here
	}



}

typedef struct win32_sound_output
{
	int RunningSampleIndex;
	int SamplesPerSec;
	int BytesPerSample;
	int SecondaryBufferSize;
	int LatencySampleCount; // How far ahead of the playbuffer we are going to write.
} win32_sound_output;

LOCAL void Win32ClearSoundBuffer(win32_sound_output *SoundOutput)
{
	VOID *Region1;
	DWORD Region1Size;
	VOID* Region2;
	DWORD Region2Size;

	if (SUCCEEDED(GlobalSecondaryBuffer->Lock(0, SoundOutput->SecondaryBufferSize, &Region1, &Region1Size, &Region2, &Region2Size, 0)))
	{
		uint8* DestSample = (uint8 *)Region1;
		for (DWORD ByteIndex = 0; ByteIndex < Region1Size; ++ByteIndex)
		{
			*DestSample++ = 0;
		}

		DestSample = (uint8 *)Region2;
		for (DWORD ByteIndex = 0; ByteIndex < Region2Size; ++ByteIndex)
		{
			*DestSample++ = 0;
		}

		GlobalSecondaryBuffer->Unlock(Region1, Region1Size, Region2, Region2Size);
	}
	else
	{
		//TODO : logging
	}
}

LOCAL void Win32FillSoundBuffer(win32_sound_output* SoundOutput, DWORD BytesToLock, DWORD BytesToWrite, game_sound_output_buffer *SourceSoundBuffer)
{	
	// 16 bit 16 bit  16 bit 16 bit
	// [Left  Right] [Left	Right] [Left Right] ....
	VOID *Region1;
	DWORD Region1Size;
	VOID* Region2;
	DWORD Region2Size;

	if (SUCCEEDED(GlobalSecondaryBuffer->Lock(BytesToLock, BytesToWrite, &Region1, &Region1Size, &Region2, &Region2Size, 0)))
	{
		int16 *DestSample = (int16 *)Region1;
		int16 *SrcSample = SourceSoundBuffer->Samples;
		DWORD Region1SampleCount = Region1Size / SoundOutput->BytesPerSample;
		for (DWORD SampleIndex = 0; SampleIndex < Region1SampleCount; ++SampleIndex)
		{
			*DestSample++ = *SrcSample++;
			*DestSample++ = *SrcSample++;
			++SoundOutput->RunningSampleIndex;
		}

		DestSample = (int16 *)Region2;
		DWORD Region2SampleCount = Region2Size / SoundOutput->BytesPerSample;
		for (DWORD SampleIndex = 0; SampleIndex < Region2SampleCount; ++SampleIndex)
		{
			*DestSample++ = *SrcSample++;
			*DestSample++ = *SrcSample++;
			++SoundOutput->RunningSampleIndex;
		}

		GlobalSecondaryBuffer->Unlock(Region1, Region1Size, Region2, Region2Size);
	}
	else
	{
		//TODO log here!
	}
	
}

/************************************************************************/
/* Displays the buffer in a window.                                     */
/************************************************************************/
LOCAL void
Win32UpdateWindow(HDC DeviceContext, window_dimensions Dimensions, win32_bitmap_buffer* Buffer)
{
	StretchDIBits(DeviceContext,
				  0, 0, Dimensions.Width, Dimensions.Height,
				  0, 0, Buffer->Width, Buffer->Height,
				  Buffer->Memory,
				  &Buffer->Info,
				  DIB_RGB_COLORS,
				  SRCCOPY);
}

LRESULT CALLBACK
Win32MainWindowCallback(HWND   WindowHandle,
				   UINT   Message,
				   WPARAM WParam,
				   LPARAM LParam)
{
	LRESULT result = 0;
	switch (Message) {

	case WM_CREATE:
	{
		OutputDebugStringA("WM_CREATE\n");
	} break;

	case WM_DESTROY:
	{
		Running = false;
		OutputDebugStringA("WM_DESTROY\n");
	} break;

	case WM_SYSKEYDOWN:
	case WM_SYSKEYUP:
	case WM_KEYDOWN:
	case WM_KEYUP:
	{
		uint32 VKCode = (uint32) WParam;
		bool WasDown = ((1 << 30) & LParam) != 0;
		bool IsDown =  ((1 << 31) & LParam) == 0;
		bool AltKeyDown = ((1 << 29) & LParam) != 0;

		if (WasDown != IsDown)
		{
			if (VKCode == 'W')
			{
				OutputDebugStringA("W: ");
				if (WasDown)
				{
					OutputDebugStringA("WasDown ");

				}
				if (IsDown)
				{
					OutputDebugStringA("IsDown");
				}
				OutputDebugStringA("\n");
			}
			else if (VKCode == 'S')
			{

			}
			else if (VKCode == 'A')
			{

			}
			else if (VKCode == 'D')
			{

			}
			else if (VKCode == 'Q')
			{

			}
			else if (VKCode == 'E')
			{

			}
			else if (VKCode == VK_UP)
			{

			}
			else if (VKCode == VK_DOWN)
			{

			}
			else if (VKCode == VK_RIGHT)
			{

			}
			else if (VKCode == VK_LEFT)
			{

			}
			else if (VKCode == VK_SPACE)
			{

			}
			else if (VKCode == VK_ESCAPE)
			{

			}
			else if (VKCode == VK_LSHIFT)
			{

			}
			else if (VKCode == VK_LCONTROL)
			{

			}
			else if (VKCode == VK_F4)
			{
				if (AltKeyDown)
				{
					Running = false;
				}
			}
		}
	} break;

	case WM_CLOSE:
	{
		Running = false;
		OutputDebugStringA("WM_CLOSE\n");
	} break;

	case WM_SIZE:
	{
		window_dimensions Dimensions = Win32GetWindowDimensions(WindowHandle);
		Win32ResizeDIBSection(&GlobalBuffer, Dimensions);
		OutputDebugStringA("WM_SIZE\n");
	}break;

	case WM_ACTIVATEAPP:
	{
		OutputDebugStringA("WM_ACTIVATEAPP\n");
	} break;

	case WM_PAINT:
	{
		PAINTSTRUCT PaintStruct;
		HDC DeviceContext = BeginPaint(WindowHandle, &PaintStruct);
		window_dimensions Dimensions;
		Dimensions.Height = PaintStruct.rcPaint.bottom - PaintStruct.rcPaint.top;
		Dimensions.Width = PaintStruct.rcPaint.right - PaintStruct.rcPaint.left;

		Win32UpdateWindow(DeviceContext, Dimensions, &GlobalBuffer);
		EndPaint(WindowHandle, &PaintStruct);
	} break;

	default:
		result = DefWindowProc(WindowHandle, Message, WParam, LParam);
	}

	return result;
}

int CALLBACK
WinMain(HINSTANCE hInstance,
		HINSTANCE hPrevInstance,
		LPSTR     lpCmdLine,
		int       nCmdShow
) {

	LARGE_INTEGER PerfCountFrequencyResult;
	QueryPerformanceFrequency(&PerfCountFrequencyResult);
	int64 PerfCountFrequency = PerfCountFrequencyResult.QuadPart;

	Win32LoadXInput();
	WNDCLASS WindowClass = {};

	WindowClass.style = CS_HREDRAW | CS_VREDRAW;
	WindowClass.lpfnWndProc = Win32MainWindowCallback;
	WindowClass.hInstance = hInstance;
	WindowClass.lpszClassName = "HandmadeHeroWindowClass";

	if (RegisterClass(&WindowClass))
	{
		//Create Window
		HWND WindowHandle = CreateWindowExA(0,
									 WindowClass.lpszClassName,
									 "Handmade Hero",
									 WS_OVERLAPPEDWINDOW | WS_VISIBLE,
									 CW_USEDEFAULT,
									 CW_USEDEFAULT,
									 CW_USEDEFAULT,
									 CW_USEDEFAULT,
									 0,
									 0,
									 hInstance,
									 0);

		

		if (WindowHandle)
		{
			
			HDC DeviceContext = GetDC(WindowHandle);
			Running = true;

			// Sound Test
			
			bool32 IsSoundPlaying = false;

			win32_sound_output SoundOutput = {};
			SoundOutput.RunningSampleIndex = 0;
			SoundOutput.SamplesPerSec = 48000;
			SoundOutput.BytesPerSample = 2 * sizeof(int16);
			SoundOutput.SecondaryBufferSize = SoundOutput.SamplesPerSec * SoundOutput.BytesPerSample;
			SoundOutput.LatencySampleCount = SoundOutput.SamplesPerSec / 10;

			Win32InitDirectSound(WindowHandle, SoundOutput.SamplesPerSec, SoundOutput.SecondaryBufferSize);
			Win32ClearSoundBuffer(&SoundOutput);
			GlobalSecondaryBuffer->Play(0, 0, DSBPLAY_LOOPING);

			int16 *Samples = (int16 *)VirtualAlloc(0, SoundOutput.SecondaryBufferSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

			game_memory GameMemory = {};
			GameMemory.PermanentStorageSize = Megabytes(64);
			GameMemory.TransientStorageSize = Gigabytes(4);
			uint64 TotalSize = GameMemory.PermanentStorageSize + GameMemory.TransientStorageSize;

#if _DEBUG
			LPVOID BaseAddress = (LPVOID)Terabytes(2);
#else
			LPVOID BaseAddress = 0;
#endif

			GameMemory.PermanentStorage = VirtualAlloc(BaseAddress, TotalSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
			GameMemory.TransientStorage = (uint8*)GameMemory.PermanentStorage + GameMemory.PermanentStorageSize;

			LARGE_INTEGER LastCounter;
			QueryPerformanceCounter(&LastCounter);
			uint64 LastCycleCount = __rdtsc();

			game_input Input[2] = {};
			game_input* NewInput = &Input[0];
			game_input* OldInput = &Input[1];

			while (Running)
			{
				MSG Message;
				while (PeekMessageA(&Message, WindowHandle, 0, 0, PM_REMOVE))
				{
					if (Message.message == WM_QUIT)
					{
						Running = false;
					}
					TranslateMessage(&Message);
					DispatchMessageA(&Message);
				}

				int MaxControllerCount = XUSER_MAX_COUNT;
				if (MaxControllerCount > ArrayCount(NewInput->Controllers))
				{
					MaxControllerCount = ArrayCount(NewInput->Controllers);
				}

				for (int ControllerIndex = 0; ControllerIndex < XUSER_MAX_COUNT; ++ControllerIndex)
				{
					game_controller_input* OldController = &OldInput->Controllers[ControllerIndex];
					game_controller_input* NewController = &NewInput->Controllers[ControllerIndex];

					XINPUT_STATE ControllerState = {};
					if (XInputGetState(ControllerIndex, &ControllerState) == ERROR_SUCCESS)
					{
					;

						XINPUT_GAMEPAD GamePad = ControllerState.Gamepad;
						bool Up = GamePad.wButtons & XINPUT_GAMEPAD_DPAD_UP;
						bool Down = GamePad.wButtons & XINPUT_GAMEPAD_DPAD_DOWN;
						bool Left = GamePad.wButtons & XINPUT_GAMEPAD_DPAD_LEFT;
						bool Right = GamePad.wButtons & XINPUT_GAMEPAD_DPAD_RIGHT;
					
						bool Start = GamePad.wButtons & XINPUT_GAMEPAD_START;
						bool Back = GamePad.wButtons & XINPUT_GAMEPAD_BACK;
						bool LeftThumb = GamePad.wButtons & XINPUT_GAMEPAD_LEFT_THUMB;
						bool RightThumb = GamePad.wButtons & XINPUT_GAMEPAD_RIGHT_THUMB;
						
						NewController->StartX = OldController->EndX;
						NewController->StartY = OldController->EndY;
						NewController->IsAnalog = true;
						real32 X;
						if (GamePad.sThumbLX < 0)
						{
							X = (real32)GamePad.sThumbLX / 32768.0f;
						}
						else
						{
							X = (real32)GamePad.sThumbLX / 32767.0f;
						}
						NewController->MinX = NewController->MaxX = NewController->EndX = X;

						real32 Y;
						if (GamePad.sThumbLY < 0)
						{
							Y = (real32)GamePad.sThumbLY / 32768.0f;
						}
						else
						{
							Y = (real32)GamePad.sThumbLY / 32767.0f;
						}
						NewController->MinY = NewController->MaxY = NewController->EndY = Y;

						Win32ProcessXInputDigitalButton(GamePad.wButtons, &OldController->Down, XINPUT_GAMEPAD_A, &NewController->Down);
						Win32ProcessXInputDigitalButton(GamePad.wButtons, &OldController->Right, XINPUT_GAMEPAD_B, &NewController->Right);
						Win32ProcessXInputDigitalButton(GamePad.wButtons, &OldController->Left, XINPUT_GAMEPAD_X, &NewController->Left);
						Win32ProcessXInputDigitalButton(GamePad.wButtons, &OldController->Up, XINPUT_GAMEPAD_Y, &NewController->Up);
						Win32ProcessXInputDigitalButton(GamePad.wButtons, &OldController->LeftShoulder, XINPUT_GAMEPAD_LEFT_SHOULDER, &NewController->LeftShoulder);					
					}
					else
					{
						//TODO The Controller is not available
					}

				}

				/*
				Controller Vibration test.
				*/
				/*			XINPUT_VIBRATION Vibration;
							Vibration.wLeftMotorSpeed = 60000;
							Vibration.wRightMotorSpeed = 60000;
							XInputSetState(0, &Vibration);*/


				DWORD PlayCursor;
				DWORD WriteCursor;
				bool32 isSoundValid = false;
				DWORD BytesToLock = 0;
				DWORD BytesToWrite = 0;
				if (SUCCEEDED(GlobalSecondaryBuffer->GetCurrentPosition(&PlayCursor, &WriteCursor)))
				{
					BytesToLock = (SoundOutput.RunningSampleIndex * SoundOutput.BytesPerSample) % SoundOutput.SecondaryBufferSize;
					DWORD TargetCursor = (PlayCursor + (SoundOutput.LatencySampleCount * SoundOutput.BytesPerSample)) % SoundOutput.SecondaryBufferSize;

					if (BytesToLock > TargetCursor)
					{
						BytesToWrite = SoundOutput.SecondaryBufferSize - BytesToLock;
						BytesToWrite += TargetCursor;
					}
					else
					{
						BytesToWrite = TargetCursor - BytesToLock;
					}

					isSoundValid = true;
				}

				game_bitmap_buffer BitmapBuffer = {};
				BitmapBuffer.Memory = GlobalBuffer.Memory;
				BitmapBuffer.Height = GlobalBuffer.Height;
				BitmapBuffer.Width = GlobalBuffer.Width;
				BitmapBuffer.BytesPerPixel = GlobalBuffer.BytesPerPixel;

				game_sound_output_buffer SoundBuffer = {};
				SoundBuffer.Samples = Samples;
				SoundBuffer.SamplesPerSecond = SoundOutput.SamplesPerSec;
				SoundBuffer.SampleCount = BytesToWrite / SoundOutput.BytesPerSample; // 30 Frames per second assumption.

				GameUpdateAndRender(&GameMemory, NewInput, &BitmapBuffer, &SoundBuffer);
				
				if (isSoundValid)
				{
					Win32FillSoundBuffer(&SoundOutput, BytesToLock, BytesToWrite, &SoundBuffer);
				}
				else
				{
					//TODO Log error;
				}
				
				window_dimensions Dimensions = Win32GetWindowDimensions(WindowHandle);
				Win32UpdateWindow(DeviceContext, Dimensions, &GlobalBuffer);

				LARGE_INTEGER EndCounter;
				QueryPerformanceCounter(&EndCounter); 

				uint64 EndCycleCount = __rdtsc();
				int64 CounterElapsed = EndCounter.QuadPart - LastCounter.QuadPart;
				
				real32 MilliSecPerFrame = (real32)(CounterElapsed * 1000.0f) / (real32)PerfCountFrequency;
				real32 FramesPerSecond = (real32)PerfCountFrequency / (real32)CounterElapsed;
				real32 MegaCyclesPerFrame = (real32)(EndCycleCount - LastCycleCount) / (1000.0f * 1000.0f);

				char Buffer[256];
				sprintf_s(Buffer, "%0.02f-ms/frame, %0.02f-frames/sec, %0.02f-MegaCycles/frame\n", MilliSecPerFrame, FramesPerSecond, MegaCyclesPerFrame);
				OutputDebugStringA(Buffer);

				LastCounter = EndCounter;
				LastCycleCount = EndCycleCount;

				game_input *temp = NewInput;
				NewInput = OldInput;
				OldInput = temp;
			}
		}
		else
		{
			//Mahesh TODO: log errors here!
		}
	}
	else
	{
		//Mahesh TODO: log errors here!
	}

	return 0;
}
