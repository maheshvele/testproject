#include <Windows.h>

LRESULT CALLBACK
MainWindowCallback(HWND   WindowHandle,
				   UINT   Message,
				   WPARAM WParam,
				   LPARAM LParam)
{
	LRESULT result = 0;
	switch (Message) {

	case WM_CREATE:
	{
		OutputDebugString("WM_CREATE\n");
	} break;

	case WM_DESTROY:
	{
		OutputDebugString("WM_DESTROY\n");
	} break;

	case WM_QUIT:
	{
		OutputDebugString("WM_QUIT\n");
	} break;

	case WM_SIZE:
	{
		OutputDebugString("WM_SIZE\n");
	}break;

	case WM_ACTIVATEAPP:
	{
		OutputDebugString("WM_ACTIVATEAPP\n");
	} break;

	case WM_PAINT:
	{
		PAINTSTRUCT PaintStruct;
		HDC DeviceContext = BeginPaint(WindowHandle, &PaintStruct);
		static DWORD WindowColor = WHITENESS;
		PatBlt(DeviceContext,
			   PaintStruct.rcPaint.left,
			   PaintStruct.rcPaint.top,
			   PaintStruct.rcPaint.right - PaintStruct.rcPaint.left,
			   PaintStruct.rcPaint.bottom - PaintStruct.rcPaint.top,
			   WindowColor);

		if (WindowColor == WHITENESS)
		{
			WindowColor = BLACKNESS;
		}
		else
		{
			WindowColor = WHITENESS;
		}

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

	WNDCLASS WindowClass = {};

	WindowClass.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
	WindowClass.lpfnWndProc = MainWindowCallback;
	WindowClass.hInstance = hInstance;
	WindowClass.lpszClassName = "HandmadeHeroWindowClass";

	if (RegisterClass(&WindowClass))
	{
		//Create Window
		HWND WindowHandle = CreateWindowEx(0,
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
			for (;;)
			{
				MSG Message;
				BOOL MessageResult = GetMessage(&Message, WindowHandle, 0, 0);
				if (MessageResult > 0)
				{
					TranslateMessage(&Message);
					DispatchMessage(&Message);
				}
				else
				{
					break;
				}
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
