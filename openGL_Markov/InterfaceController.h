#pragma once
#include <windows.h>
#include <GL/gl.h>
#include <GL/GLU.h>
#include "stdio.h"
#include <string>
#include <vector>
#include <fstream>
#include "MarkovClasses.h"


enum eButtonType
{
	SIMPLE,
	TOGGLE
};

class Button
{
public:
	static std::vector<Button*> allButtons;
public:
	std::string name;
	int posX = 0;
	int posY = 0;
	int width = 0;
	int height = 0;
	bool isHovered = false;
	bool isToggled = false;
	GLfloat toggleColor[3] = { 0.5f, 0.5f, 0.0f };
	eButtonType type = eButtonType::SIMPLE;
public:
	Button(const std::string& name, eButtonType type, int x, int y, int w, int h) : name(name), type(type), posX(x), posY(y), width(w), height(h), isHovered(false)
	{
		allButtons.push_back(this);
	}

};

class ButtonRenderer
{
public:

	static void renderButton(Button& b)
	{
		glPushMatrix(); // Save the current transformation matrix
		glTranslatef(b.posX, b.posY, 0.0f);

		if (b.isToggled)
			glColor3f(0.2f, 0.2f, 0.2f); // Dark color for toggled (WALL)
		else
			glColor3f(0.8f, 0.8f, 0.8f); // Light color for None

		glBegin(GL_QUADS);
		glVertex2f(0, 0);
		glVertex2f(b.width, 0);
		glVertex2f(b.width, b.height);
		glVertex2f(0, b.height);
		glEnd();


		// Hovered button outline
		if (b.isHovered)
		{
			glColor3f(0.1f, 0.1f, 0.1f); // White color for hovered
		}
		else
		{
			glColor3f(0.5f, 0.5f, 0.5f); // Gray hover for normal
		}
		glBegin(GL_LINES);
		for (int i = 0; i < 2; i++) // horizontal
		{
			glVertex2f(0, i * b.height);
			glVertex2f(b.width, i * b.height);
		}
		for (int i = 0; i < 2; i++) // vertical
		{
			glVertex2f(i * b.width, 0);
			glVertex2f(i * b.width, b.height);
		}
		glEnd();
		glPopMatrix();
		return;
	}
};

class TextRenderer
{
private:
	HDC m_HDC = 0;
public:
	GLuint fontBase = 0;
public:
	TextRenderer(HDC hdc, const wchar_t* name, int size)
	{
		m_HDC = hdc;
		fontBase = glGenLists(256);
		HFONT font = createFont(hdc, name, size);
		SelectObject(hdc, font);
		wglUseFontBitmaps(hdc, 0, 256, fontBase);
	}
	HFONT createFont(HDC hdc, const wchar_t* name, int size)
	{
		HFONT font = CreateFont(
			size,
			0,
			0,
			0,
			FW_BOLD,
			FALSE,
			FALSE,
			FALSE,
			ANSI_CHARSET,
			OUT_TT_PRECIS,
			CLIP_DEFAULT_PRECIS,
			ANTIALIASED_QUALITY,
			FF_DONTCARE | DEFAULT_PITCH,
			name);
		return font;

	}

	void renderText(const char* text, float x, float y, bool centered = true)
	{
		if (centered)
		{
			SIZE textSize;
			GetTextExtentPoint32A(m_HDC, text, strlen(text), &textSize);

			x -= textSize.cx / 2.0f;
			y += textSize.cy / 2.0f;
		}

		glRasterPos2f(x, y);
		glPushAttrib(GL_LIST_BIT);
		glListBase(this->fontBase);
		glCallLists(strlen(text), GL_UNSIGNED_BYTE, text);
		glPopAttrib();
	}
};

void drawBorder(int x, int y, int width, int height)
{
	glBegin(GL_LINES);
	glVertex2f(x, y);
	glVertex2f(x+width, y);

	glVertex2f(x + width, y);
	glVertex2f(x + width, y + height);

	glVertex2f(x + width, y + height);
	glVertex2f(x, y + height);

	glVertex2f(x, y + height);
	glVertex2f(x, y);
	glEnd();
}

class EnvironmentUIController
{
private:
	HDC m_hDC = 0;
public:
	Environment* envUp; // Direction: UP
	Environment* envRight; // Direction: RIGHT
	Environment* envDown; // Direction: DOWN
	Environment* envLeft; // Direction: LEFT

	Environment* hoveredEnv = nullptr;
	float maxValue = 0.0f;

	TextRenderer* textRenderer = nullptr;

public:
	EnvironmentUIController(const std::string& mapPath)
	{
		envUp = new Environment(mapPath, "Direction: ^ UP ^ (0 deg)");
		envRight = new Environment(mapPath, "Direction: > RIGHT > (90 deg)");
		envDown = new Environment(mapPath, "Direction: v DOWN v (180 deg)");
		envLeft = new Environment(mapPath, "Direction: < LEFT < (270 deg)");

		const int globalOffsX = 265;
		const int globalOffsY = cellSize;
		envUp->rd.posX = globalOffsX;
		envUp->rd.posY = globalOffsY;
		envRight->rd.posX = globalOffsX + cellSize * 11;
		envRight->rd.posY = globalOffsY;
		envDown->rd.posX = globalOffsX + cellSize * 11;
		envDown->rd.posY = globalOffsY + cellSize * 11;
		envLeft->rd.posX = globalOffsX;
		envLeft->rd.posY = globalOffsY + cellSize * 11;
	}
	void setHDC(HDC hdc)
	{
		m_hDC = hdc;
		textRenderer = new TextRenderer(hdc, L"Arial", -12);
	}

	void checkHovered(int x, int y)
	{
		hoveredEnv = nullptr;
		for (Environment* e : Environment::allEnvironments)
		{
			int cellX = (x - e->rd.posX) / cellSize;
			int cellY = (y - e->rd.posY) / cellSize;

			if (cellX >= 0 && cellY >= 0 && cellX < SIZE_X && cellY < SIZE_Y)
			{
				hoveredEnv = e;
				e->rd.hoveredCellX = cellX;
				e->rd.hoveredCellY = cellY;
				continue; // to prevent bug with double hovered envs
			}
			e->rd.hoveredCellX = -1;
			e->rd.hoveredCellY = -1;
		}
		return;
	}

	void renderEnvironments()
	{
		if (!textRenderer) return;
		std::string dir = "None";
		std::string pos = "(None, None)";
		std::string val = "None";
		if (hoveredEnv != nullptr)
		{
			dir = hoveredEnv->dirName;
			pos = "(" + std::to_string(hoveredEnv->rd.hoveredCellX) + ", " + std::to_string(hoveredEnv->rd.hoveredCellY) + ")";
			val = std::to_string(hoveredEnv->data[hoveredEnv->rd.hoveredCellX + 1][hoveredEnv->rd.hoveredCellY + 1]);
		}
		pos = "Position: " + pos;
		val = "Probability: " + val;

		glColor3f(0.3f, 0.3f, 0.3f); // Text color
		textRenderer->renderText("HOVERED CELL DATA:", 60,500, false);
		textRenderer->renderText(dir.c_str(), 30, 530, false);
		textRenderer->renderText(pos.c_str(), 30, 545, false);
		textRenderer->renderText(val.c_str(), 30, 560, false);
		drawBorder(20, 480, 200, 100);
		for (Environment* e : Environment::allEnvironments)
		{
			glPushMatrix();
			glTranslatef(e->rd.posX, e->rd.posY, 0.0f);

			glColor3f(0.1f, 0.1f, 0.1f); // Text color

			textRenderer->renderText(e->dirName.c_str(), cellSize * SIZE_X / 2, -10);

			for (size_t i = 0; i < SIZE_X; i++)
			{
				for (size_t j = 0; j < SIZE_Y; j++)
				{
					float red = 0; float green = 0;
					float value = (float)e->data[i + 1][j + 1] / maxValue;
					if (value <= 0.5f) {
						red = 1.0f;
						green = value * 2.0f;
					}
					else {
						red = 1.0f - (value - 0.5f) * 2.0f;
						green = 1.0f;
					}
					if (e->cells[i + 1][j + 1] == eCellOccupancy::Empty)
						glColor3f(red, green, 0.0f); // Cell gradient color by its probability value
					else
						glColor3f(0.1f, 0.1f, 0.1f); // Wall

					glBegin(GL_QUADS);
					glVertex2f(i * cellSize, j * cellSize);
					glVertex2f(i * cellSize + cellSize, j * cellSize);
					glVertex2f(i * cellSize + cellSize, j * cellSize + cellSize);
					glVertex2f(i * cellSize, j * cellSize + cellSize);
					glEnd();
				}
			}

			// Draw grid
			glColor3f(0.0f, 0.0f, 0.0f); // Black color for grid
			glBegin(GL_LINES);
			for (int i = 0; i <= SIZE_X; i++) // horizontal
			{
				glVertex2f(0, i * cellSize);
				glVertex2f(SIZE_X * cellSize, i * cellSize);
			}
			for (int i = 0; i <= SIZE_Y; i++) // vertical
			{
				glVertex2f(i * cellSize, 0);
				glVertex2f(i * cellSize, SIZE_Y * cellSize);
			}
			glEnd();

			// color hovered cell
			if (e->rd.hoveredCellX >= 0)
			{
				int hoverCellX = e->rd.hoveredCellX;
				int hoverCellY = e->rd.hoveredCellY;
				glColor3f(0.8f, 0.8f, 0.8f); // White outline color

				glBegin(GL_LINES);
				glVertex2f(hoverCellX * cellSize, hoverCellY * cellSize);
				glVertex2f(hoverCellX * cellSize + cellSize, hoverCellY * cellSize);

				glVertex2f(hoverCellX * cellSize + cellSize, hoverCellY * cellSize);
				glVertex2f(hoverCellX * cellSize + cellSize, hoverCellY * cellSize + cellSize);

				glVertex2f(hoverCellX * cellSize + cellSize, hoverCellY * cellSize + cellSize);
				glVertex2f(hoverCellX * cellSize, hoverCellY * cellSize + cellSize);

				glVertex2f(hoverCellX * cellSize, hoverCellY * cellSize + cellSize);
				glVertex2f(hoverCellX * cellSize, hoverCellY * cellSize);
				glEnd();

			}
			glPopMatrix();
		}
		return;
	}
};

class SensorInputUIController
{
private:
	Button* m_bFront;
	Button* m_bBack;
	Button* m_bRight;
	Button* m_bLeft;
	Button* m_bApply;
	std::vector<Button*> btns;

	Button* m_hoveredButton = nullptr;

	void (*m_applyFilter)(Filter) = nullptr; // callback function to OnApplyFilter

	int m_offsX = 0; // UI offset
	int m_offsY = 0;

	HDC m_hDC = 0;
public:
	Filter filter; // Result = filter
	TextRenderer* textRenderer = nullptr;
public:
	SensorInputUIController(int offsX, int offsY, void (*applyFilter)(Filter))
	{
		m_applyFilter = applyFilter;

		m_offsX = offsX; m_offsY = offsY;
		m_bFront = new Button("FRONT", eButtonType::TOGGLE, m_offsX + 60, m_offsY + 25, 60, 50);
		m_bBack = new Button("BACK", eButtonType::TOGGLE, m_offsX + 60, m_offsY + 125, 60, 50);
		m_bRight = new Button("LEFT", eButtonType::TOGGLE, m_offsX + 0, m_offsY + 75, 60, 50);
		m_bLeft = new Button("RIGHT", eButtonType::TOGGLE, m_offsX + 120, m_offsY + 75, 60, 50);
		m_bApply = new Button("APPLY FILTER", eButtonType::SIMPLE, m_offsX, m_offsY + 200, 180, 50);

		btns.push_back(m_bFront);
		btns.push_back(m_bBack);
		btns.push_back(m_bRight);
		btns.push_back(m_bLeft);
		btns.push_back(m_bApply);

	}
	void setHDC(HDC hdc)
	{
		m_hDC = hdc;
		textRenderer = new TextRenderer(hdc, L"Arial", -12);
	}
	void render()
	{
		if (!textRenderer) return;

		glColor3f(0.3f, 0.3f, 0.3f);
		textRenderer->renderText("RECEIVED SENSOR DATA:", m_offsX + 90, m_offsY);
		textRenderer->renderText("^ Robot ^", m_offsX + 90, m_offsY + 100);
		for (Button* b : btns)
		{
			ButtonRenderer::renderButton(*b); // render button object

			if (b->type == eButtonType::TOGGLE && b->isToggled) // if button is toggled, button text = WALL
			{
				glColor3f(0.95f, 0.95f, 0.95f);
				textRenderer->renderText("WALL", b->posX + b->width / 2.0, b->posY + b->height / 2.0);
			}
			else if (b->type == eButtonType::TOGGLE && !b->isToggled) // if button not toggled, button text = Sensor Side
			{
				glColor3f(0.3f, 0.3f, 0.3f);
				textRenderer->renderText(b->name.c_str(), b->posX + b->width / 2.0, b->posY + b->height / 2.0);
			}
			else
			{
				glColor3f(0.3f, 0.3f, 0.3f);
				textRenderer->renderText("APPLY FILTER", b->posX + b->width / 2.0, b->posY + b->height / 2.0);
			}
		}
		glColor3f(0.4f, 0.4f, 0.4f);
		drawBorder(m_offsX - 10, m_offsY - 10, 200, 270);
	}

	void checkHovered(int x, int y)
	{
		m_hoveredButton = nullptr;
		for (Button* b : btns)
		{
			int newX = (x - b->posX);
			int newY = (y - b->posY);

			if (newX >= 0 && newY >= 0 && newX < b->width && newY < b->height)
			{
				b->isHovered = true;
				m_hoveredButton = b;
				continue;
			}
			b->isHovered = false;
		}

		return;
	}

	void resetFilter()
	{
		filter.reset();
		m_bFront->isToggled = false;
		m_bRight->isToggled = false;
		m_bBack->isToggled = false;
		m_bLeft->isToggled = false;
	}

	void processClick()
	{
		if (m_hoveredButton == nullptr) return;

		if (m_hoveredButton->type == eButtonType::TOGGLE)
		{
			m_hoveredButton->isToggled = !m_hoveredButton->isToggled; // button state true-false switching
			if (m_hoveredButton->name == "FRONT") filter.up = (eCellOccupancy)(filter.up ^ 1); // filter state WALL-EMPTY switching
			else if (m_hoveredButton->name == "BACK") filter.down = (eCellOccupancy)(filter.down ^ 1);
			else if (m_hoveredButton->name == "RIGHT") filter.right = (eCellOccupancy)(filter.right ^ 1);
			else if (m_hoveredButton->name == "LEFT") filter.left = (eCellOccupancy)(filter.left ^ 1);
		}
		else if (m_hoveredButton->type == eButtonType::SIMPLE)
		{
			if (m_applyFilter == nullptr) return;
			m_applyFilter(filter);
			resetFilter();

		}
		return;
	}
};

class MovementInputUIController
{
private:
	Button* m_bForward;
	Button* m_bTurnLeft;
	Button* m_bTurnRight;

	std::vector<Button*> btns;

	Button* hoveredButton = nullptr;

	void (*m_sendMovement)(std::string) = nullptr;

	int m_offsX = 0;
	int m_offsY = 0;

	HDC m_hDC = 0;
public:
	Filter filter;
	TextRenderer* textRenderer = nullptr;
public:
	MovementInputUIController(int offsX, int offsY, void (*OnSendMovement)(std::string))
	{
		m_sendMovement = OnSendMovement;

		m_offsX = offsX; m_offsY = offsY;
		m_bForward = new Button("Forward", eButtonType::SIMPLE, m_offsX + 60, m_offsY + 25, 60, 50);
		m_bTurnLeft = new Button("Turn left", eButtonType::SIMPLE, m_offsX + 0, m_offsY + 75, 60, 50);
		m_bTurnRight = new Button("Turn right", eButtonType::SIMPLE, m_offsX + 120, m_offsY + 75, 60, 50);

		btns.push_back(m_bForward);
		btns.push_back(m_bTurnLeft);
		btns.push_back(m_bTurnRight);

	}
	void setHDC(HDC hdc)
	{
		m_hDC = hdc;
		textRenderer = new TextRenderer(hdc, L"Arial", -12);
	}
	void render()
	{
		if (!textRenderer) return;

		glColor3f(0.3f, 0.3f, 0.3f);
		textRenderer->renderText("SEND MOVEMENT TO ROBOT:", m_offsX + 90, m_offsY);

		for (Button* b : btns)
		{
			ButtonRenderer::renderButton(*b);
			glColor3f(0.3f, 0.3f, 0.3f);
			textRenderer->renderText(b->name.c_str(), b->posX + b->width / 2.0, b->posY + b->height / 2.0);
		}
		glColor3f(0.4f, 0.4f, 0.4f);
		drawBorder(m_offsX-10, m_offsY-10, 200, 150);
	}

	void checkHovered(int x, int y)
	{
		hoveredButton = nullptr;
		for (Button* b : btns)
		{
			int newX = (x - b->posX);
			int newY = (y - b->posY);

			if (newX >= 0 && newY >= 0 && newX < b->width && newY < b->height)
			{
				b->isHovered = true;
				hoveredButton = b;
				continue; // to prevent bug with double hovered envs
			}
			b->isHovered = false;
		}

		return;
	}

	void processClick()
	{
		if (hoveredButton == nullptr) return;

		m_sendMovement(hoveredButton->name);
		return;
	}
};