/**
 * @file SolarSystemDataSet.hpp
 * @brief Realistic Solar System dataset with Keplerian orbital velocities, asteroid belt, comet, Kuiper belt, and simulated stars.
 */

#ifndef __N_BODY_SIMULATION_SOLARSYSTEMDATASET_HPP__
#define __N_BODY_SIMULATION_SOLARSYSTEMDATASET_HPP__

#include "AbstractData.hpp"
#include <string>
#include <vector>

enum CelestialBodyType {
    BODY_SUN = 0,
    BODY_MERCURY = 1,
    BODY_VENUS = 2,
    BODY_EARTH = 3,
    BODY_MARS = 4,
    BODY_JUPITER = 5,
    BODY_SATURN = 6,
    BODY_URANUS = 7,
    BODY_NEPTUNE = 8,
    BODY_PLUTO = 9,
    BODY_COMET = 10,
    BODY_ASTEROID = 11,
    BODY_KUIPER = 12,
    BODY_STAR = 13
};

struct BodyMetadata {
    std::string name;
    CelestialBodyType type;
    float visualRadius;
    float semiMajorAxis; // in meters
    float orbitalPeriodDays;
};

class SolarSystemDataSet : public AbstractData {
private:
    std::vector<BodyMetadata> metadata;
    size_t numPlanetsAndSun = 10;
    size_t numComets = 1;
    size_t numAsteroids = 0;
    size_t numKuiper = 0;
    size_t numSimulatedStars = 0;

public:
    SolarSystemDataSet(std::size_t additionalBodies = 1600,
                       std::string initDistribution = "normal",
                       double mMax = 8.5e29,
                       double mMin = 1e16,
                       double pMax = 5.0e11,
                       double pMin = 0,
                       double vMax = 4e4,
                       double vMin = 0);

    std::vector<float> getFlatPositions() override;
    std::vector<float> getFlatVelocities() override;
    double getMaxMass() const override;

    const std::vector<BodyMetadata>& getMetadata() const { return metadata; }
    size_t getMajorBodyCount() const { return numPlanetsAndSun + numComets; }
};

#endif // __N_BODY_SIMULATION_SOLARSYSTEMDATASET_HPP__
