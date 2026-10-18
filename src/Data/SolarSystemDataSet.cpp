/**
 * @file SolarSystemDataSet.cpp
 * @brief Realistic Solar System dataset with Keplerian orbital velocities, asteroid belt, comet, and Kuiper belt.
 */

#include "../include/Data/SolarSystemDataSet.hpp"
#include <cmath>
#include <random>

SolarSystemDataSet::SolarSystemDataSet(std::size_t additionalBodies,
                                       std::string initDistribution,
                                       double mMax,
                                       double mMin,
                                       double pMax,
                                       double pMin,
                                       double vMax,
                                       double vMin) {
    this->name = "REALISTIC SOLAR SYSTEM DATA SET";
    this->pMax = pMax;
    this->pMin = pMin;
    this->vMax = vMax;
    this->vMin = vMin;
    this->mMax = mMax;
    this->mMin = mMin;

    const double G = 6.67430e-11;
    const double M_sun = 1.989e30;

    // Helper lambda to add a major body in Keplerian orbit in X-Z plane
    auto addBody = [&](const std::string &name,
                       CelestialBodyType type,
                       double mass,
                       double r,
                       double angle,
                       float visualRadius,
                       float orbitalPeriodDays) {
        double v = std::sqrt(G * M_sun / r);
        double posX = r * std::cos(angle);
        double posZ = r * std::sin(angle);
        double velX = -v * std::sin(angle);
        double velZ = v * std::cos(angle);

        this->positions.emplace_back((float)posX, 0.0f, (float)posZ);
        this->velocities.emplace_back((float)velX, 0.0f, (float)velZ);
        this->masses.emplace_back((float)mass);
        this->size++;

        BodyMetadata bm;
        bm.name = name;
        bm.type = type;
        bm.visualRadius = visualRadius;
        bm.semiMajorAxis = (float)r;
        bm.orbitalPeriodDays = orbitalPeriodDays;
        metadata.push_back(bm);
    };

    // 0. The Central Star: Sun
    this->positions.emplace_back(0.0f, 0.0f, 0.0f);
    this->velocities.emplace_back(0.0f, 0.0f, 0.0f);
    this->masses.emplace_back((float)M_sun);
    this->size++;
    {
        BodyMetadata bm;
        bm.name = "SUN";
        bm.type = BODY_SUN;
        bm.visualRadius = 0.055f;
        bm.semiMajorAxis = 0.0f;
        bm.orbitalPeriodDays = 0.0f;
        metadata.push_back(bm);
    }

    // 1. Mercury (0.387 AU)
    addBody("MERCURY", BODY_MERCURY, 0.33011e24, 57.909e9, 0.35, 0.011f, 88.0f);

    // 2. Venus (0.723 AU)
    addBody("VENUS", BODY_VENUS, 4.8675e24, 108.209e9, 1.25, 0.016f, 224.7f);

    // 3. Earth (1.000 AU)
    addBody("EARTH", BODY_EARTH, 5.9724e24, 149.596e9, 2.10, 0.017f, 365.25f);

    // 4. Mars (1.524 AU)
    addBody("MARS", BODY_MARS, 0.64171e24, 227.923e9, 3.45, 0.013f, 687.0f);

    // 5. Jupiter (5.204 AU)
    addBody("JUPITER", BODY_JUPITER, 1898.19e24, 778.570e9, 0.85, 0.038f, 4333.0f);

    // 6. Saturn (9.582 AU)
    addBody("SATURN", BODY_SATURN, 568.34e24, 1433.529e9, 4.80, 0.032f, 10759.0f);

    // 7. Uranus (19.20 AU)
    addBody("URANUS", BODY_URANUS, 86.813e24, 2872.463e9, 2.70, 0.024f, 30687.0f);

    // 8. Neptune (30.05 AU)
    addBody("NEPTUNE", BODY_NEPTUNE, 102.413e24, 4495.060e9, 5.60, 0.023f, 60190.0f);

    // 9. Pluto (39.48 AU) - Dwarf Planet
    addBody("PLUTO", BODY_PLUTO, 0.01303e24, 5906.380e9, 1.95, 0.009f, 90560.0f);

    // 10. Comet (e.g. 1P/Halley - highly eccentric orbit)
    {
        double a = 2.66e12; // semi-major axis (~17.8 AU)
        double r = 1.35e12; // current position
        double angle = 0.65;
        double v = std::sqrt(G * M_sun * (2.0 / r - 1.0 / a));
        double posX = r * std::cos(angle);
        double posZ = r * std::sin(angle);
        // Eccentric velocity vector has inward radial component
        double velX = -v * std::sin(angle) - 0.3 * v * std::cos(angle);
        double velZ = v * std::cos(angle) - 0.3 * v * std::sin(angle);
        this->positions.emplace_back((float)posX, 0.0f, (float)posZ);
        this->velocities.emplace_back((float)velX, 0.0f, (float)velZ);
        this->masses.emplace_back(2.2e14f);
        this->size++;

        BodyMetadata bm;
        bm.name = "COMET";
        bm.type = BODY_COMET;
        bm.visualRadius = 0.008f;
        bm.semiMajorAxis = (float)a;
        bm.orbitalPeriodDays = 27500.0f;
        metadata.push_back(bm);
    }

    // Determine counts for Asteroids and Kuiper Belt objects
    size_t targetAsteroids = 600;
    size_t targetKuiper = 400;

    if (additionalBodies > 0) {
        targetAsteroids = additionalBodies * 6 / 10;
        targetKuiper = additionalBodies - targetAsteroids;
    }

    std::mt19937 rng(42); // deterministic seed for beautiful reproducible distribution
    std::uniform_real_distribution<double> angleDist(0.0, 2.0 * 3.141592653589793);
    std::uniform_real_distribution<double> asteroidRDist(3.1e11, 4.9e11); // between Mars (2.28e11) & Jupiter (7.78e11)
    std::uniform_real_distribution<double> asteroidYDist(-1.5e10, 1.5e10);
    std::uniform_real_distribution<double> asteroidMassDist(1e15, 1e19);

    // 11. Asteroid Belt
    for (size_t i = 0; i < targetAsteroids; ++i) {
        double r = asteroidRDist(rng);
        double theta = angleDist(rng);
        double y = asteroidYDist(rng);
        double v = std::sqrt(G * M_sun / r);

        double posX = r * std::cos(theta);
        double posZ = r * std::sin(theta);
        double velX = -v * std::sin(theta);
        double velZ = v * std::cos(theta);

        this->positions.emplace_back((float)posX, (float)y, (float)posZ);
        this->velocities.emplace_back((float)velX, 0.0f, (float)velZ);
        this->masses.emplace_back((float)asteroidMassDist(rng));
        this->size++;

        BodyMetadata bm;
        bm.name = "ASTEROID";
        bm.type = BODY_ASTEROID;
        bm.visualRadius = 0.0025f;
        bm.semiMajorAxis = (float)r;
        bm.orbitalPeriodDays = (float)(2.0 * 3.14159265 * std::sqrt(r * r * r / (G * M_sun)) / 86400.0);
        metadata.push_back(bm);
        this->numAsteroids++;
    }

    // 12. Kuiper Belt (Outer-system icy bodies beyond Neptune)
    std::uniform_real_distribution<double> kuiperRDist(4.6e12, 6.4e12); // beyond Neptune (4.5e12)
    std::uniform_real_distribution<double> kuiperYDist(-3.0e10, 3.0e10);
    std::uniform_real_distribution<double> kuiperMassDist(1e16, 5e20);

    for (size_t i = 0; i < targetKuiper; ++i) {
        double r = kuiperRDist(rng);
        double theta = angleDist(rng);
        double y = kuiperYDist(rng);
        double v = std::sqrt(G * M_sun / r);

        double posX = r * std::cos(theta);
        double posZ = r * std::sin(theta);
        double velX = -v * std::sin(theta);
        double velZ = v * std::cos(theta);

        this->positions.emplace_back((float)posX, (float)y, (float)posZ);
        this->velocities.emplace_back((float)velX, 0.0f, (float)velZ);
        this->masses.emplace_back((float)kuiperMassDist(rng));
        this->size++;

        BodyMetadata bm;
        bm.name = "KUIPER OBJECT";
        bm.type = BODY_KUIPER;
        bm.visualRadius = 0.003f;
        bm.semiMajorAxis = (float)r;
        bm.orbitalPeriodDays = (float)(2.0 * 3.14159265 * std::sqrt(r * r * r / (G * M_sun)) / 86400.0);
        metadata.push_back(bm);
        this->numKuiper++;
    }
}

std::vector<float> SolarSystemDataSet::getFlatPositions() {
    this->flatPositions.clear();
    this->flatPositions.reserve(this->size * 3);
    for (const auto &p : this->positions) {
        this->flatPositions.push_back(p.x);
        this->flatPositions.push_back(p.y);
        this->flatPositions.push_back(p.z);
    }
    return this->flatPositions;
}

std::vector<float> SolarSystemDataSet::getFlatVelocities() {
    this->flatVelocities.clear();
    this->flatVelocities.reserve(this->size * 3);
    for (const auto &v : this->velocities) {
        this->flatVelocities.push_back(v.x);
        this->flatVelocities.push_back(v.y);
        this->flatVelocities.push_back(v.z);
    }
    return this->flatVelocities;
}

double SolarSystemDataSet::getMaxMass() const {
    return this->mMax;
}
