#pragma once
#include <windows.h>
#include <GL/gl.h>
#include <GL/GLU.h>
#include "stdio.h"
#include <string>
#include <vector>
#include <fstream>
#include "FastSlamClasses.h"
#include <format>

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

	Landmark* hoveredLandmark = nullptr;
	int hoveredLandmarkID = -1;
	Particle* hoveredLandmarkOwner = nullptr;
	int hoveredLandmarkOwnerID = -1;

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
		double mouseX = x - env->rd.posX;
		double mouseY = y - env->rd.posY;

		double minDist = 10;

		int pId = 0;

		for (Particle* p : env->particles)
		{
			int lId = 0;
			for (Landmark& land : p->obstacleHypothesis)
			{
				double distX = abs(p->posX + land.mean(0) - mouseX);
				double distY = abs(p->posY + land.mean(1) - mouseY);
				double dist = sqrt(distX * distX + distY * distY);

				if (dist < minDist)
				{
					hoveredLandmark = &land;
					hoveredLandmarkID = lId;
					hoveredLandmarkOwner = p;
					hoveredLandmarkOwnerID = pId;
				}
				lId++;
			}
			pId++;
		}
		return;
	}

	void render()
	{
		if (!textRenderer) return;

		std::string steps = "Step: ";
		steps += std::to_string(env->stepCounter);
		textRenderer->renderText(steps.c_str(), env->rd.posX, env->rd.posY - 5, false);
		int yPos = 190;
		for (size_t i = 0; i < 10; i++)
		{
			std::string text = std::format("Particle {}:", i);
			textRenderer->renderText(text.c_str(), 20, yPos, false);
			yPos += 14;
			text = std::format("dRobotX: {}, dRobotY: {}",
				std::to_string(env->particles[i]->posX - env->robot->posX), std::to_string(env->particles[i]->posY - env->robot->posY));
			textRenderer->renderText(text.c_str(), 20, yPos, false);
			yPos += 14;
		}
		yPos += 20;

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

			for (Landmark& land : p->obstacleHypothesis)
			{
				float red = 0; float green = 0;
				float value = (float)land.weight;
				if (value <= 0.5f) {
					red = 1.0f;
					green = value * 2.0f;
				}
				else {
					red = 1.0f - (value - 0.5f) * 2.0f;
					green = 1.0f;
				}

				glColor3f(red, green, 0.1f); // Obstacle color
				drawCircle(p->posX + land.mean(0), p->posY + land.mean(1), 5, 4);
			}
		}
		glColor3f(0.0f, 0.0f, 0.0f);
		std::string text = "Hovered landmark data: ";
		textRenderer->renderText(text.c_str(), -env->rd.posX+20, yPos - env->rd.posY, false);
		yPos += 14;
		text = std::format("Landmark ID: {}, Owner: Particle{}", hoveredLandmarkID, hoveredLandmarkOwnerID);
		textRenderer->renderText(text.c_str(), -env->rd.posX+20, yPos - env->rd.posY, false);
		yPos += 14;
		

		if (hoveredLandmark)
		{
			text = std::format("Inovac. Y: ({}, {})", std::to_string(hoveredLandmark->Y(0)), std::to_string(hoveredLandmark->Y(1)));
			textRenderer->renderText(text.c_str(), -env->rd.posX + 20, yPos - env->rd.posY, false);
			yPos += 14;
			text = std::format("Inovac.kovar S: ({})", std::to_string(hoveredLandmark->S(0)));
			textRenderer->renderText(text.c_str(), -env->rd.posX + 20, yPos - env->rd.posY, false);
			yPos += 14;
			text = std::format("Kalman g. K: ({})", std::to_string(hoveredLandmark->K(0)));
			textRenderer->renderText(text.c_str(), -env->rd.posX + 20, yPos - env->rd.posY, false);
			yPos += 14;
			glColor3f(0.0f, 0.8f, 0.8f);
			glBegin(GL_LINES);
			for (int i = 0; i < 2; i++) // horizontal
			{
				glVertex2f(hoveredLandmarkOwner->posX + hoveredLandmark->mean(0), hoveredLandmarkOwner->posY + hoveredLandmark->mean(1));
				glVertex2f(env->obstacles[hoveredLandmarkID]->posX, env->obstacles[hoveredLandmarkID]->posY);
			}
			glEnd();
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
		m_bForward = new Button("Forward(w)", eButtonType::SIMPLE, m_offsX + 70, m_offsY + 25, 70, 50);
		m_bTurnLeft = new Button("Turn left(a)", eButtonType::SIMPLE, m_offsX + 0, m_offsY + 75, 70, 50);
		m_bTurnRight = new Button("Turn right(d)", eButtonType::SIMPLE, m_offsX + 140, m_offsY + 75, 70, 50);

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
		drawBorder(m_offsX - 10, m_offsY - 10, 230, 150);
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