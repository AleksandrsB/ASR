#include <windows.h>
#include <tchar.h>
#include "WindowClass.h"
#include "InterfaceController.h"

// OpenGL context and window handles
HDC g_hDC;
HGLRC g_hRC;
HWND g_hWnd;


std::vector<Environment*> Environment::allEnvironments;
std::vector<Button*> Button::allButtons;

EnvironmentUIController ep("map1.txt");
ButtonRenderer br;

Filter f;
SensorModel sm;
MovementModel mm;


void normalize()
{
	double sum = 0;
	for (Environment* e : Environment::allEnvironments)
	{
		sum += e->getSum();
	}
	for (Environment* e : Environment::allEnvironments)
	{
		e->normalizeWithSum(sum); // for gradient rendering
		if (ep.maxValue < e->getMax()) ep.maxValue = e->getMax();
	}
}

void OnApplyFilter(Filter f1)
{
	Filter f2 = f1.rotateRight();
	Filter f3 = f2.rotateRight();
	Filter f4 = f3.rotateRight();

	ep.envUp->applyFilter(f1, sm);
	ep.envRight->applyFilter(f2, sm);
	ep.envDown->applyFilter(f3, sm);
	ep.envLeft->applyFilter(f4, sm);

	normalize();
	return;
}

void OnSendMovement(const std::string s)
{
	if (s == "Forward")
	{
		ep.envUp->applyMovement(eDirection::Up, mm); // pForwardForward + pForwardStop
		ep.envRight->applyMovement(eDirection::Right, mm);
		ep.envDown->applyMovement(eDirection::Down, mm);
		ep.envLeft->applyMovement(eDirection::Left, mm);
	}
	else if(s=="Turn left")
	{
		for (size_t i = 1; i < SIZE_X + 1; i++)
		{
			for (size_t j = 1; j < SIZE_Y + 1; j++)
			{
				double newUp = ep.envUp->data[i][j] * mm.pFail + ep.envRight->data[i][j] * mm.pSuccess;
				double newRight = ep.envRight->data[i][j] * mm.pFail + ep.envDown->data[i][j] * mm.pSuccess;
				double newDown = ep.envRight->data[i][j] * mm.pFail + ep.envLeft->data[i][j] * mm.pSuccess;
				double newLeft = ep.envRight->data[i][j] * mm.pFail + ep.envUp->data[i][j] * mm.pSuccess;

				ep.envUp->data[i][j] = newUp;
				ep.envRight->data[i][j] = newRight;
				ep.envDown->data[i][j] = newDown;
				ep.envLeft->data[i][j] = newLeft;
			}
		}
	}
	else if(s=="Turn right")
	{
		for (size_t i = 1; i < SIZE_X + 1; i++)
		{
			for (size_t j = 1; j < SIZE_Y + 1; j++)
			{
				double newUp = ep.envUp->data[i][j] * mm.pFail + ep.envLeft->data[i][j] * mm.pSuccess;
				double newRight = ep.envRight->data[i][j] * mm.pFail + ep.envUp->data[i][j] * mm.pSuccess;
				double newDown = ep.envRight->data[i][j] * mm.pFail + ep.envRight->data[i][j] * mm.pSuccess;
				double newLeft = ep.envRight->data[i][j] * mm.pFail + ep.envDown->data[i][j] * mm.pSuccess;

				ep.envUp->data[i][j] = newUp;
				ep.envRight->data[i][j] = newRight;
				ep.envDown->data[i][j] = newDown;
				ep.envLeft->data[i][j] = newLeft;
			}
		}
	}
	normalize();
	return;
}

SensorInputUIController sip(30,30, OnApplyFilter);
MovementInputUIController mip(30, 320, OnSendMovement);


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

	sip.setHDC(g_hDC);
	ep.setHDC(g_hDC);
	mip.setHDC(g_hDC);
	return true;
}

// Function to render the scene
void RenderScene()
{
	glClearColor(1.0f, 1.0f, 1.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	ep.renderEnvironments();
	sip.render();
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
		sip.checkHovered(LOWORD(lParam), HIWORD(lParam));
		mip.checkHovered(LOWORD(lParam), HIWORD(lParam));
		break;
	case WM_LBUTTONUP:
		sip.processClick();
		mip.processClick();
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
	WindowClass glWin(hInstance, L"Markov Localization - Aleksandrs Buraks 171RDB289 IRDMR0", NULL, &ctrlGL);
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
