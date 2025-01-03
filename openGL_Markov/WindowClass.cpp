#include "WindowClass.h"


WindowClass::WindowClass(HINSTANCE hInst, const wchar_t* name, HWND hParent, Controller* ctrl)
	:
	m_winHandle(0), m_winInstance(hInst), m_winController(ctrl),
	m_winStyle(WS_OVERLAPPEDWINDOW), m_winStyleEx(0),
	m_x(CW_USEDEFAULT), m_y(CW_USEDEFAULT), m_width(CW_USEDEFAULT), m_height(CW_USEDEFAULT)
{
	// copy string
	wcsncpy_s(this->m_winTitle, name, MAX_STRING - 1);
	wcsncpy_s(this->m_winClassName, name, MAX_STRING - 1);

	m_winClass = { 0 };

	m_winClass.hInstance = hInst;
	m_winClass.lpszClassName = m_winClassName;
	m_winClass.style = CS_OWNDC;								// Ensure a unique device context for the OpenGL window
	m_winClass.hIcon = LoadIcon(hInst, IDI_APPLICATION);		// default icon
	m_winClass.hCursor = LoadCursor(0, IDC_ARROW);              // default arrow cursor
}

WindowClass::~WindowClass()
{
	::UnregisterClass(m_winClassName, m_winInstance);
}

HWND WindowClass::createWindow()
{
	// register win class
	if (!::RegisterClass(&m_winClass)) return 0;

	m_winHandle =
		::CreateWindowEx(m_winStyleEx,	// default 0
			m_winClassName,				// registered winClass name
			m_winTitle,					// window caption
			m_winStyle,					// window style
			m_x,						// x position
			m_y,						// y position
			m_width,					// width
			m_height,					// height
			0,							// handle to parent window
			0,							// handle to menu
			m_winInstance,				// app instance
			0);	// window creation data

	return m_winHandle;
}

void WindowClass::showWindow(int nShowCmd)
{
	::ShowWindow(m_winHandle, nShowCmd);
	::UpdateWindow(m_winHandle);
}