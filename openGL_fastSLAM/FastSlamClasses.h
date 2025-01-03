#pragma once
#include <windows.h>
#include "stdio.h"
#include <string>
#include <vector>
#include <fstream>
#include "params.h"
#include <Eigen/Dense>
#include <Eigen/Cholesky>
#include <cmath>
#include <random>

struct SensorModel
{
	double sensorError = 0.1; // 0.1/m

	SensorModel(double sensorErr)
	{
		sensorError = sensorErr;
	}

	Eigen::Matrix2d getSensorNoiseWithDist(double dist)
	{
		Eigen::Matrix2d Sigma = Eigen::Matrix2d::Identity();
		double sigma = sensorError * dist;
		double sigma2 = sigma * sigma;
		Sigma << sigma2, 0.0, 0.0, sigma2;
		return Sigma;
	}
};

struct MovementModel
{
	double movementErrorPos = 0.1; // 0.1/m movement noise
	double movementErrorTheta = 0.05; // 0.05/m rotation noise

	MovementModel(double movementErr, double rotErr)
	{
		movementErrorPos = movementErr;
		movementErrorTheta = rotErr;
	}

	Eigen::Matrix3d getMovementNoiseWithDist(double dist, double dTheta)
	{
		double sigmaPos = movementErrorPos * dist;
		double sigmaTheta = movementErrorTheta * std::fabs(dTheta);

		Eigen::Matrix3d Sigma;
		Sigma << sigmaPos * sigmaPos, 0.0, 0.0,
			0.0, sigmaPos* sigmaPos, 0.0,
			0.0, 0.0, sigmaTheta* sigmaTheta;
		return Sigma;
	}
};

struct Robot
{
	double posX = 0, posY = 0;
	double Theta = 0;

	Robot(double x, double y, double Theta) : posX(x), posY(y), Theta(Theta) {}

	void moveForward(double dist)
	{
		posX += dist * cos(Theta);
		posY += dist * sin(Theta);
	}
};

struct Obstacle
{
public:
	bool isInited = false;
	Eigen::Vector2d mean; // [mu_x, mu_y]
	Eigen::Matrix2d covar; // covariance

	double posX = 0, posY = 0; // Ground Truth coords
	Obstacle(double x, double y) : posX(x), posY(y), mean(Eigen::Vector2d::Zero()), covar(Eigen::Matrix2d::Identity()) {}

	void init(const Eigen::Vector2d& measurement)
	{
		mean = measurement;
		covar = Eigen::Matrix2d::Identity() * 100; // H * initial nenoteiktiba
		isInited = true;
	}

	double update(const Eigen::Vector2d& measurement, const Eigen::Matrix2d measurementNoise)
	{
		// EKF:
		// inovacija Y = Zn - H * X_prognoze
		Eigen::Vector2d Y = measurement - mean;

		// inovacijas kovariance S = H * P_prognoze * H^T + Q
		// H = H^T, jo H = Identity
		// tad H * P * H^T = P, jo H*P*H = P
		// tad S = P + Q
		Eigen::Matrix2d S = covar + measurementNoise;

		// Kalmana ieguvums K = P_prognoze * H^T * S^-1
		// H^T = H, tad P * H^T = P * H
		// H = Identity, tad P * H = P
		// tad K = P * S^-1
		Eigen::Matrix2d K = covar * S.inverse();

		// Jauna iezimes pozicija
		// Xn = X_prognoze + KY
		mean = mean + K * Y;

		// Jauna iezimes kovariacija
		// Pn = (I - K * H) * P
		Eigen::Matrix2d I = Eigen::Matrix2d::Identity();
		covar = (I - K) * covar;


		// Gauss

		double exponent = -0.5 * Y.transpose() * S.inverse() * Y;
		double det = S.determinant();
		double normFactor = 1.0 / (2.0 * 3.14159 * std::sqrt(det));
		double pz = normFactor * std::exp(exponent);
		return pz;
	}
};

class Particle
{
public:
	double posX = 0, posY = 0, theta = 0;// robot state hypothesis
	double weight = 1.0;
	std::vector<Obstacle> obstacleHypothesis;
public:
	Particle(double x, double y, double theta, double weight) : posX(x), posY(y), theta(theta), weight(weight) {}

	void updateMeasurements(const std::vector<Eigen::Vector2d>& measurements, SensorModel* sm)
	{
		if (obstacleHypothesis.empty()) // вместо size() == 0
		{
			for (const auto& m : measurements)
			{
				Obstacle obst(posX + m(0), posY + m(1));
				obst.init(m);
				obstacleHypothesis.push_back(obst);
			}
			return;
		}
		else
		{
			if (obstacleHypothesis.size() != measurements.size()) throw "Size mistmatch!";

			for (size_t i = 0; i < measurements.size(); i++)
			{
				double distX = obstacleHypothesis[i].posX - posX;
				double distY = obstacleHypothesis[i].posY - posY;
				double dist = sqrt(distX * distX + distY * distY);

				double likelihood = obstacleHypothesis[i].update(measurements[i], sm->getSensorNoiseWithDist(dist));
				weight *= likelihood;
			}
		}

	}
};

struct RenderData
{
	int posX = 0, posY = 0;
};

class Environment
{
public:
	std::vector<Obstacle*> obstacles;
	std::vector<Particle*> particles;

	RenderData rd;

	Robot* robot = nullptr;
	SensorModel* sensorModel;
	MovementModel* movementModel;

	int sizeX = 0, sizeY = 0;

private:
	std::mt19937 gen;
	std::normal_distribution<double> distN;
	std::default_random_engine rng;

public:
	Environment(int sizeX, int sizeY) : sizeX(sizeX), sizeY(sizeY), distN(0.0, 1.0), gen(std::random_device{}())
	{
		robot = new Robot(30, 30, 0);

		for (size_t i = 0; i < 10; i++)
		{
			obstacles.push_back(new Obstacle(rand() % sizeX, rand() % sizeY));
		}

		for (size_t i = 0; i < 10; i++)
		{
			particles.push_back(new Particle(robot->posX, robot->posY, robot->Theta, 1.0));
		}

		sensorModel = new SensorModel(0.2);
		movementModel = new MovementModel(0.1, 0.05);
	}

	std::vector<Eigen::Vector2d> getRobotObstacleRelativeMeasurements()
	{
		std::vector<Eigen::Vector2d> measurements;
		for (Obstacle* obst : obstacles)
		{
			measurements.push_back(Eigen::Vector2d(obst->posX - robot->posX, obst->posY - robot->posY));
		}
		return measurements;
	}

	void predictParticlePos(double dist, double dx, double dy, double dTheta)
	{
		Eigen::Matrix3d Sigma = movementModel->getMovementNoiseWithDist(dist, std::fabs(dTheta));

		// find cholesky decomposition (A)
		Eigen::LLT<Eigen::Matrix3d> lltOfSigma(Sigma);
		Eigen::Matrix3d A = lltOfSigma.matrixL();

		Eigen::Vector3d idealMove(dx, dy, dTheta);

		for (Particle* p : particles)
		{
			// 3D noise z = (z1, z2, z3)
			double z1 = distN(gen);
			double z2 = distN(gen);
			double z3 = distN(gen);
			Eigen::Vector3d z(z1, z2, z3);

			Eigen::Vector3d noise = A * z;  // shape 3x1  (nx, ny, ntheta)


			// Az + X
			Eigen::Vector3d moveWithNoise = idealMove + noise;

			p->posX += moveWithNoise(0);
			p->posY += moveWithNoise(1);
			p->theta += moveWithNoise(2);
		}
	}

	void updateParticleObstacles(const std::vector<Eigen::Vector2d>& measurements)
	{
		for (Particle* p : particles)
		{
			p->updateMeasurements(measurements, sensorModel);
			//p->weight *= p->calcMeasurementP(measurements, sensorModel);
		}

		// Normalisation
		double sumWeights = 0.0;
		for (Particle* p : particles)
		{
			sumWeights += p->weight;
		}

		for (Particle* p : particles)
		{
			if (sumWeights > 1e-15)
			{
				p->weight /= sumWeights;
			}
			else
			{
				// all particles are bad
				p->weight = 1.0 / particles.size();
			}
		}
	}

	void resample()
	{
		std::vector<double> cdf(particles.size(), 0.0);
		cdf[0] = particles[0]->weight;
		for (size_t i = 1; i < particles.size(); i++)
		{
			cdf[i] = cdf[i - 1] + particles[i]->weight;
		}

		// new particle vector
		std::vector<Particle*> newParticles;
		newParticles.reserve(particles.size());

		// between 0 and 1
		double step = 1.0 / particles.size();
		std::uniform_real_distribution<double> dist(0.0, step);
		double r = dist(rng);

		int idx = 0;
		for (size_t i = 0; i < particles.size(); ++i)
		{
			double u = r + i * step;
			while (u > cdf[idx] && idx < (int)particles.size() - 1)
			{
				idx++;
			}
			newParticles.push_back(particles[idx]);
			// same maps after copying!!
			newParticles.back()->weight = 1.0; // reset
		}
		particles = newParticles;
	}

	void step(double dist, double dTheta)
	{
		//robot->moveForward(dist);
		double dx = dist * cos(robot->Theta);
		double dy = dist * sin(robot->Theta);
		robot->posX += dx;
		robot->posY += dy;
		robot->Theta += dTheta;

		std::vector<Eigen::Vector2d> measurements = getRobotObstacleRelativeMeasurements();

		predictParticlePos(dist, dx, dy, dTheta);
		updateParticleObstacles(measurements);
		resample();
		return;
	}

};