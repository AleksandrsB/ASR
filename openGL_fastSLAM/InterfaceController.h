#pragma once
#include <windows.h>
#include <GL/gl.h>
#include <GL/GLU.h>
#include "stdio.h"
#include <string>
#include <vector>
#include <fstream>
#include "FastSlamClasses.h"

void drawBorder(int x, int y, int width, int height)
{
	glBegin(GL_LINES);
	glVertex2f(x, y);
	glVertex2f(x + width, y);

	glVertex2f(x + width, y);
	glVertex2f(x + width, y + height);

	glVertex2f(x + width, y + height);
	glVertex2f(x, y + height);

	glVertex2f(x, y + height);
	glVertex2f(x, y);
	glEnd();
}

void drawCircle(float centerX, float centerY, float radius, int numSegments) {
	glBegin(GL_TRIANGLE_FAN);
	glVertex2f(centerX, centerY);
	for (int i = 0; i <= numSegments; i++) {
		float angle = 2.0f * 3.14159 * i / numSegments;
		float x = radius * cos(angle);
		float y = radius * sin(angle);
		glVertex2f(centerX + x, centerY + y);
	}
	glEnd();

}

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

class RobotRenderer
{
public:

	static void renderRobot(Robot& r)
	{
		glPushMatrix(); // Save the current transformation matrix
		float angleDegrees = r.Theta * 180.0f / 3.14159;

		glTranslatef(r.posX, r.posY, 0.0f);
		glRotatef(angleDegrees, 0, 0, 1);

		//draw robot
		glColor3f(0.8f, 0.2f, 0.1f);
		drawCircle(0, 0, 10, 10);

		glColor3f(0.1f, 0.1f, 0.1f);
		glBegin(GL_LINES);
		for (int i = 0; i < 2; i++) // horizontal
		{
			glVertex2f(0, 0);
			glVertex2f(20, 0);
		}

		glEnd();
		glPopMatrix();
		return;
	}
};

class EnvironmentUIController
{
private:
	HDC m_hDC = 0;
public:
	Environment* env; // Direction: UP

	float maxValue = 0.0f;

	TextRenderer* textRenderer = nullptr;

public:
	EnvironmentUIController()
	{
		env = new Environment(700, 700);

		const int globalOffsX = 265;
		const int globalOffsY = 20;
		env->rd.posX = globalOffsX;
		env->rd.posY = globalOffsY;

	}
	void setHDC(HDC hdc)
	{
		m_hDC = hdc;
		textRenderer = new TextRenderer(hdc, L"Arial", -12);
	}

	void checkHovered(int x, int y)
	{

		return;
	}

	void render()
	{
		if (!textRenderer) return;

		// draw border
		glColor3f(0.4f, 0.4f, 0.4f);
		drawBorder(env->rd.posX, env->rd.posY, env->sizeX, env->sizeY);

		glPushMatrix();
		glTranslatef(env->rd.posX, env->rd.posY, 0.0f);
		RobotRenderer::renderRobot(*env->robot);

		double robotPosX = env->robot->posX;
		double robotPosY = env->robot->posY;

		for (Obstacle* obs : env->obstacles)
		{
			glColor3f(0.1f, 0.1f, 0.1f); // Obstacle color
			drawCircle(obs->posX, obs->posY, 5, 8);

			glColor3f(0.8f, 0.8f, 0.8f);
			glBegin(GL_LINES);
			for (int i = 0; i < 2; i++) // horizontal
			{
				glVertex2f(robotPosX, robotPosY);
				glVertex2f(obs->posX, obs->posY);
			}
			glEnd();
		}

		for (Particle* p : env->particles)
		{
			glColor3f(0.5f, 0.1f, 0.1f); // Particle
			drawCircle(p->posX, p->posY, 5, 4);

			for (Obstacle& o : p->obstacleHypothesis)
			{
				glColor3f(0.0f, 1.0f, 0.1f); // Obstacle color
				drawCircle(o.posX + o.mean(0), o.posY + o.mean(1), 5, 4);
			}
		}


		glPopMatrix();
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
		drawBorder(m_offsX - 10, m_offsY - 10, 200, 150);
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