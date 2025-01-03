#pragma once
#include <Windows.h>

class Controller
{

};
const int MAX_STRING = 256;

class WindowClass
{
private:
	HWND m_winHandle;
	WNDCLASS m_winClass;
	HINSTANCE m_winInstance;
	Controller* m_winController;
	DWORD m_winStyle;
	DWORD m_winStyleEx;
	wchar_t m_winTitle[MAX_STRING];
	wchar_t m_winClassName[MAX_STRING];
	int m_x;
	int m_y;
	int m_width;
	int m_height;

public:
	WindowClass(HINSTANCE hInst, const wchar_t* name, HWND hParent, Controller* ctrl);
	~WindowClass();

	HWND createWindow();
	void showWindow(int nShowCmd = SW_SHOWDEFAULT);
	HWND getHandle() { return m_winHandle; }; // return window handle

	// setter for WNDCLASS, if not specified, use default values
	void setClassStyle(UINT style) { m_winClass.style = style; };

	// setters for CreateWindowEx()
	void setWindowProcedure(WNDPROC proc) { m_winClass.lpfnWndProc = proc; };
	void setWindowStyle(DWORD style) { m_winStyle = style; };
	void setWindowStyleEx(DWORD style) { m_winStyleEx = style; };
	void setPosition(int x, int y) { this->m_x = x; this->m_y = y; };
	void setWidth(int w) { m_width = w; };
	void setHeight(int h) { m_height = h; };
};

