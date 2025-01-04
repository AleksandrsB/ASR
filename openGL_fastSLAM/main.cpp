#include <windows.h>
#include <tchar.h>
#include "WindowClass.h"
#include "InterfaceController.h"

// OpenGL context and window handles
HDC g_hDC;
HGLRC g_hRC;
HWND g_hWnd;

uint64_t stepCounter = 0;

std::vector<Button*> Button::allButtons;

EnvironmentUIController ep;
ButtonRenderer br;


void normalize()
{

}

void OnApplyFilter()
{

	return;
}

void OnSendMovement(const std::string s)
{
	if (s == "Forward") ep.env->step(5,0);
	else if (s == "Turn left") ep.env->step(0, -0.1);
	else if (s == "Turn right") ep.env->step(0, 0.1);
	normalize();
	return;
}

MovementInputUIController mip(30, 30, OnSendMovement);


// Function to initialize the OpenGL context
bool InitGL()
{
	PIXELFORMATDESCRIPTOR pfd = { 0 };
	pfd.nSize = sizeof(pfd);
	pfd.nVersion = 1;
	pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
	pfd.iPixelType = PFD_TYPE_RGBA;
	pfd.cColorBits = 32;
	pfd.cDepthBits = 24;
	pfd.iLayerType = PFD_MAIN_PLANE;

	g_hDC = GetDC(g_hWnd);
	int pf = ChoosePixelFormat(g_hDC, &pfd);
	SetPixelFormat(g_hDC, pf, &pfd);

	g_hRC = wglCreateContext(g_hDC);
	wglMakeCurrent(g_hDC, g_hRC);

	ep.setHDC(g_hDC);
	mip.setHDC(g_hDC);
	return true;
}

// Function to render the scene
void RenderScene()
{
	glClearColor(1.0f, 1.0f, 1.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	ep.render();

	mip.render();

	SwapBuffers(g_hDC);
}
void resizeViewport()
{
	RECT rect;
	::GetClientRect(g_hWnd, &rect);
	glViewport(0, 0, rect.right, rect.bottom);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0.0, rect.right, rect.bottom, 0.0, -1.0, 1.0);
}
// Window procedure function
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_SIZE:
		if (g_hWnd) resizeViewport();
		break;
	case WM_PAINT:
		RenderScene();
		ValidateRect(hWnd, NULL);
		break;
	case WM_MOUSEMOVE:
		ep.checkHovered(LOWORD(lParam), HIWORD(lParam));
		mip.checkHovered(LOWORD(lParam), HIWORD(lParam));
		break;
	case WM_LBUTTONUP:
		mip.processClick();
		break;
	case WM_KEYDOWN:
		if (wParam == 'W') OnSendMovement("Forward");
		else if (wParam == 'A') OnSendMovement("Turn left");
		else if (wParam == 'D') OnSendMovement("Turn right");
		
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

// WinMain - Entry point of the Windows application
int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	Controller ctrlGL;
	WindowClass glWin(hInstance, L"fastSLAM - Aleksandrs Buraks 171RDB289 IRDMR0", NULL, &ctrlGL);
	glWin.setWindowStyle(WS_OVERLAPPEDWINDOW);
	glWin.setClassStyle(CS_OWNDC);
	glWin.setWidth(WINDOW_WIDTH);
	glWin.setHeight(WINDOW_HEIGHT);
	glWin.setWindowProcedure(WndProc);

	if (!glWin.createWindow())
	{
		MessageBox(NULL, L"[Error] Failed to create GL window!", L"Error info", MB_OK);
		return -1;
	}
	g_hWnd = glWin.getHandle();
	//ctrlGL.setHandle(glWin.getHandle());

	glWin.showWindow(nCmdShow);

	if (!InitGL())
	{
		return -1;
	}
	resizeViewport();

	MSG msg = { 0 };
	while (msg.message != WM_QUIT)
	{
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{
			RenderScene();
		}
	}

	wglDeleteContext(g_hRC);
	ReleaseDC(g_hWnd, g_hDC);

	return (int)msg.wParam;
}
