#ifndef __VR_TRACKER_HPP__
#define __VR_TRACKER_HPP__

#include <atomic>
#include <thread>
#include <string>

/**
 * @brief Lightweight UDP receiver for mobile phone gyroscope data.
 * Compatible with Trinus VR, HyperIMU, Sensor Node, and WebXR clients.
 * Expects CSV or space-separated: "pitch,yaw,roll" or "yaw,pitch,roll" in degrees.
 */
class VRTracker {
private:
    std::atomic<bool> running{false};
    std::thread listenerThread;
    int port;

    std::atomic<float> yawDegrees{0.0f};
    std::atomic<float> pitchDegrees{0.0f};
    std::atomic<float> rollDegrees{0.0f};
    std::atomic<bool> hasReceivedData{false};

    void listenLoop();

public:
    VRTracker(int port = 4242);
    ~VRTracker();

    bool start();
    void stop();

    float getYaw() const { return yawDegrees.load(); }
    float getPitch() const { return pitchDegrees.load(); }
    float getRoll() const { return rollDegrees.load(); }
    bool isConnected() const { return hasReceivedData.load(); }
    int getPort() const { return port; }
};

#endif // __VR_TRACKER_HPP__
