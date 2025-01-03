#pragma once
#include <windows.h>
#include "stdio.h"
#include <string>
#include <vector>
#include <fstream>
#include "params.h"


enum eCellOccupancy
{
	Empty,
	Wall
};
enum eDirection
{
	Up,
	Right,
	Down,
	Left
};
struct SensorModel
{
	// Wall-Wall = 0.8
	// None-None = 0.9
	// Wall-None = 0.2
	// None-Wall = 0.1

	double pWallWall = 0.8;
	double pWallNone = 1 - pWallWall;
	double pNoneNone = 0.9;
	double pNoneWall = 1 - pNoneNone;
	double probModel[2][2] =
	{
		pNoneNone, pNoneWall,
		pWallNone, pWallWall
	};
};

struct MovementModel
{
	double pSuccess = 0.8;
	double pFail = 0.2;
};

struct Filter
{
	eCellOccupancy up = eCellOccupancy::Empty;
	eCellOccupancy right = eCellOccupancy::Empty;
	eCellOccupancy down = eCellOccupancy::Empty;
	eCellOccupancy left = eCellOccupancy::Empty;
	Filter rotateRight()
	{
		Filter f;
		f.up = left;
		f.right = up;
		f.down = right;
		f.left = down;
		return f;
	}
	void reset()
	{
		up = Empty;
		right = Empty;
		down = Empty;
		left = Empty;
	}
};

struct RenderData
{
	int posX = 0;
	int posY = 0;
	int hoveredCellY = -1;
	int hoveredCellX = -1;
};


class Environment
{
private:
	int m_emptyCount = 0; // for initial probabilities
public:
	static std::vector<Environment*> allEnvironments;

public:
	RenderData rd;
	std::string dirName;
	eCellOccupancy cells[SIZE_X + 2][SIZE_Y + 2] = { eCellOccupancy::Empty };
	double data[SIZE_X + 2][SIZE_Y + 2] = { 0 };

public:
	Environment(const std::string& mapPath, const std::string& directionName)
	{
		dirName = directionName;
		for (size_t i = 0; i < SIZE_X + 2; i++)
		{
			for (size_t j = 0; j < SIZE_Y + 2; j++)
			{
				if (i == 0 || i == SIZE_X + 1) cells[i][j] = eCellOccupancy::Wall;
				else if (j == 0 || j == SIZE_Y + 1) cells[i][j] = eCellOccupancy::Wall;
			}
		}
		loadMapFromFile(mapPath);
		for (size_t i = 1; i < SIZE_X + 1; i++)
		{
			for (size_t j = 1; j < SIZE_Y + 1; j++)
			{
				if (cells[i][j] == eCellOccupancy::Empty) data[i][j] = 1.0 / m_emptyCount;
				else data[i][j] = 0.0;
			}
		}
		allEnvironments.push_back(this);
		return;
	}

	void loadMapFromFile(const std::string& mapPath) {
		std::ifstream file(mapPath);
		if (!file.is_open()) {
			std::string output = "Error: Unable to open file: ";
			output += mapPath;
			MessageBoxA(0, output.c_str(), "Error", MB_OK);

			return;
		}

		std::string line;
		size_t row = 1; // Счётчик строк, начинаем с 1, так как 0 и SIZE_X+1 — внешние стены
		while (std::getline(file, line) && row <= SIZE_X) {
			for (size_t col = 1; col <= SIZE_Y && col <= line.size(); ++col) {
				char s = line[col - 1];
				if (line[col - 1] == 'w') {
					cells[col][row] = eCellOccupancy::Wall;
				}
				else if (line[col - 1] == '0') {
					cells[col][row] = eCellOccupancy::Empty;
					m_emptyCount++;
				}
			}
			row++;
		}
		file.close();
	}

	void applyFilter(Filter f, SensorModel sm)
	{
		for (size_t i = 1; i < SIZE_X + 1; i++)
		{
			for (size_t j = 1; j < SIZE_Y + 1; j++)
			{
				if (cells[i][j] == eCellOccupancy::Wall)
				{
					data[i][j] = 0;
					continue;
				}

				double probValue = 1.0;

				probValue *= sm.probModel[cells[i - 1][j]][f.left];
				probValue *= sm.probModel[cells[i + 1][j]][f.right];
				probValue *= sm.probModel[cells[i][j - 1]][f.up];
				probValue *= sm.probModel[cells[i][j + 1]][f.down];

				data[i][j] *= probValue;
			}
		}
		return;
	}
	void applyMovement(eDirection mtype, MovementModel mm)
	{
		double dataCopy[SIZE_X + 2][SIZE_Y + 2] = { 0 };

		// array address direction
		int di = 0;
		int dj = 0;

		switch (mtype)
		{
		case eDirection::Up:
			di = 0;
			dj = 1;
			break;
		case eDirection::Right:
			di = -1;
			dj = 0;
			break;
		case eDirection::Down:
			di = 0;
			dj = -1;
			break;
		case eDirection::Left:
			di = 1;
			dj = 0;
			break;
		}

		for (size_t i = 1; i < SIZE_X + 1; i++)
		{
			for (size_t j = 1; j < SIZE_Y + 1; j++)
			{
				if (cells[i][j] == eCellOccupancy::Wall) continue; // skip walls

				double probValue = data[i][j] * mm.pFail;

				if (cells[i + di][j + dj] == eCellOccupancy::Empty) probValue += (data[i + di][j + dj] * mm.pSuccess);

				// probValue = pFail of current cell + pSuccess from previous cell
				dataCopy[i][j] = probValue;
			}
		}

		// apply all calculations
		for (size_t i = 1; i < SIZE_X + 1; i++)
		{
			for (size_t j = 1; j < SIZE_Y + 1; j++)
			{
				data[i][j] = dataCopy[i][j];
			}
		}
	}

	// for gradient
	double getMax()
	{
		double max = 0.0;
		for (size_t i = 1; i < SIZE_X + 1; i++)
		{
			for (size_t j = 1; j < SIZE_Y + 1; j++)
			{
				if (data[i][j] > max) max = data[i][j];
			}
		}
		return max;
	}
	// for normalizing
	double getSum()
	{
		double result = 0.0;
		for (size_t i = 1; i < SIZE_X + 1; i++)
		{
			for (size_t j = 1; j < SIZE_Y + 1; j++)
			{
				result += data[i][j];
			}
		}
		return result;
	}

	void normalizeWithSum(double sum)
	{
		for (size_t i = 1; i < SIZE_X + 1; i++)
		{
			for (size_t j = 1; j < SIZE_Y + 1; j++)
			{
				data[i][j] = data[i][j] / sum;
			}
		}
	}
};