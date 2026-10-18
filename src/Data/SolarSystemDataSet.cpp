/**
 * @file SolarSystemDataSet.cpp
 * @brief Realistic Solar System dataset with Keplerian orbital velocities, asteroid belt, comet, Kuiper belt, and simulated stars.
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
    this->name = "INTERACTIVE 3D SOLAR SYSTEM & UNIVERSE DATA SET";
    this->pMax = pMax;
    this->pMin = pMin;
    this->vMax = vMax;
    this->vMin = vMin;
    this->mMax = mMax;
    this->mMin = mMin;

    const double G = 6.67430e-11;
    const double M_sun = 8.50e29; // Calibrated central mass for stable Keplerian dynamics across visual scales

    // Helper lambda to add a major body in Keplerian orbit in X-Z plane
    auto addBody = [&](const std::string &name,
                       CelestialBodyType type,
                       double mass,
                       double rNorm,
                       double angle,
                       float visualRadius,
                       float orbitalPeriodDays) {
        double r = rNorm * pMax;
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

    // 0. The Central Star: Sun (at origin, visual radius 0.038)
    this->positions.emplace_back(0.0f, 0.0f, 0.0f);
    this->velocities.emplace_back(0.0f, 0.0f, 0.0f);
    this->masses.emplace_back((float)M_sun);
    this->size++;
    {
        BodyMetadata bm;
        bm.name = "SUN";
        bm.type = BODY_SUN;
        bm.visualRadius = 0.038f;
        bm.semiMajorAxis = 0.0f;
        bm.orbitalPeriodDays = 0.0f;
        metadata.push_back(bm);
    }

    // 1. Mercury (scaled distance 0.095 - distinctly outside Sun radius 0.038!)
    addBody("MERCURY", BODY_MERCURY, 0.33011e24, 0.095, 0.35, 0.009f, 88.0f);

    // 2. Venus (scaled distance 0.155)
    addBody("VENUS", BODY_VENUS, 4.8675e24, 0.155, 1.25, 0.013f, 224.7f);

    // 3. Earth (scaled distance 0.225)
    addBody("EARTH", BODY_EARTH, 5.9724e24, 0.225, 2.10, 0.014f, 365.25f);

    // 4. Mars (scaled distance 0.305)
    addBody("MARS", BODY_MARS, 0.64171e24, 0.305, 3.45, 0.011f, 687.0f);

    // 5. Jupiter (scaled distance 0.540)
    addBody("JUPITER", BODY_JUPITER, 1898.19e24, 0.540, 0.85, 0.028f, 4333.0f);

    // 6. Saturn (scaled distance 0.690)
    addBody("SATURN", BODY_SATURN, 568.34e24, 0.690, 4.80, 0.024f, 10759.0f);

    // 7. Uranus (scaled distance 0.840)
    addBody("URANUS", BODY_URANUS, 86.813e24, 0.840, 2.70, 0.018f, 30687.0f);

    // 8. Neptune (scaled distance 0.980)
    addBody("NEPTUNE", BODY_NEPTUNE, 102.413e24, 0.980, 5.60, 0.017f, 60190.0f);

    // 9. Pluto (scaled distance 1.120) - Dwarf Planet
    addBody("PLUTO", BODY_PLUTO, 0.01303e24, 1.120, 1.95, 0.008f, 90560.0f);

    // 10. Comet 1P/Halley (eccentric orbit with perihelion near inner system)
    {
        double a = 0.685 * pMax;
        double r = 0.400 * pMax;
        double angle = 0.65;
        double v = std::sqrt(G * M_sun * (2.0 / r - 1.0 / a));
        double posX = r * std::cos(angle);
        double posZ = r * std::sin(angle);
        double velX = -v * std::sin(angle) - 0.25 * v * std::cos(angle);
        double velZ = v * std::cos(angle) - 0.25 * v * std::sin(angle);
        this->positions.emplace_back((float)posX, 0.0f, (float)posZ);
        this->velocities.emplace_back((float)velX, 0.0f, (float)velZ);
        this->masses.emplace_back(2.2e14f);
        this->size++;

        BodyMetadata bm;
        bm.name = "COMET";
        bm.type = BODY_COMET;
        bm.visualRadius = 0.007f;
        bm.semiMajorAxis = (float)a;
        bm.orbitalPeriodDays = 27500.0f;
        metadata.push_back(bm);
    }

    // Determine counts for N-body bodies (Asteroids, Kuiper belt, and Simulated Stars)
    size_t totalN = (additionalBodies > 0) ? additionalBodies : 1600;
    size_t targetAsteroids = totalN * 50 / 100; // ~50% in Asteroid Belt
    size_t targetKuiper = totalN * 30 / 100;    // ~30% in Kuiper Belt
    size_t targetStars = totalN - targetAsteroids - targetKuiper; // ~20% simulated Stars

    std::mt19937 rng(42);
    std::uniform_real_distribution<double> angleDist(0.0, 2.0 * 3.141592653589793);

    // 11. Asteroid Belt (N-body, between Mars 0.305 and Jupiter 0.540)
    std::uniform_real_distribution<double> asteroidRDist(0.360 * pMax, 0.460 * pMax);
    std::uniform_real_distribution<double> asteroidYDist(-0.015 * pMax, 0.015 * pMax);
    std::uniform_real_distribution<double> asteroidMassDist(1e18, 1e21);

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

    // 12. Kuiper Belt (N-body, outer system beyond Neptune 0.980 and Pluto 1.120)
    std::uniform_real_distribution<double> kuiperRDist(1.060 * pMax, 1.350 * pMax);
    std::uniform_real_distribution<double> kuiperYDist(-0.030 * pMax, 0.030 * pMax);
    std::uniform_real_distribution<double> kuiperMassDist(1e19, 5e22);

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

    // 13. Simulated Stars / Cluster Bodies (N-body, gravitationally interacting massive stars)
    std::uniform_real_distribution<double> starRDist(0.400 * pMax, 1.400 * pMax);
    std::uniform_real_distribution<double> starYDist(-0.250 * pMax, 0.250 * pMax);
    std::uniform_real_distribution<double> starMassDist(1e24, 1e27);

    for (size_t i = 0; i < targetStars; ++i) {
        double r = starRDist(rng);
        double theta = angleDist(rng);
        double y = starYDist(rng);
        double dist3D = std::sqrt(r * r + y * y);
        double v = std::sqrt(G * M_sun / dist3D);

        double posX = r * std::cos(theta);
        double posZ = r * std::sin(theta);
        double velX = -v * std::sin(theta);
        double velZ = v * std::cos(theta);

        this->positions.emplace_back((float)posX, (float)y, (float)posZ);
        this->velocities.emplace_back((float)velX, 0.0f, (float)velZ);
        this->masses.emplace_back((float)starMassDist(rng));
        this->size++;

        BodyMetadata bm;
        bm.name = "SIMULATED STAR";
        bm.type = BODY_STAR;
        bm.visualRadius = 0.005f;
        bm.semiMajorAxis = (float)r;
        bm.orbitalPeriodDays = (float)(2.0 * 3.14159265 * std::sqrt(dist3D * dist3D * dist3D / (G * M_sun)) / 86400.0);
        metadata.push_back(bm);
        this->numSimulatedStars++;
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
