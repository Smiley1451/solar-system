#include "vr_tracker.hpp"
#include <iostream>
#include <sstream>
#include <vector>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#endif

VRTracker::VRTracker(int port) : port(port) {}

VRTracker::~VRTracker() {
    stop();
}

bool VRTracker::start() {
    if (running.load()) return true;
    running.store(true);
    listenerThread = std::thread(&VRTracker::listenLoop, this);
    return true;
}

void VRTracker::stop() {
    if (!running.load()) return;
    running.store(false);
    if (listenerThread.joinable()) {
        listenerThread.join();
    }
}

void VRTracker::listenLoop() {
#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "[VRTracker] WSAStartup failed.\n";
        return;
    }
    SOCKET sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sockfd == INVALID_SOCKET) {
        std::cerr << "[VRTracker] Failed to create socket.\n";
        WSACleanup();
        return;
    }

    // Set non-blocking timeout for clean termination
    DWORD timeout = 250;
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));
#else
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) return;
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 250000;
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
#endif

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(static_cast<u_short>(port));

    if (bind(sockfd, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr)) < 0) {
        std::cerr << "[VRTracker] Failed to bind to UDP port " << port << ".\n";
#ifdef _WIN32
        closesocket(sockfd);
        WSACleanup();
#else
        close(sockfd);
#endif
        return;
    }

    std::cout << "[VRTracker] Listening for Mobile Phone Gyroscope UDP packets on port " << port << "...\n";

    char buffer[512];
    sockaddr_in clientAddr{};
#ifdef _WIN32
    int clientLen = sizeof(clientAddr);
#else
    socklen_t clientLen = sizeof(clientAddr);
#endif

    while (running.load()) {
        int bytes = recvfrom(sockfd, buffer, sizeof(buffer) - 1, 0, reinterpret_cast<sockaddr*>(&clientAddr), &clientLen);
        if (bytes > 0) {
            buffer[bytes] = '\0';
            std::string msg(buffer);

            // Replace commas and colons with spaces for simple tokenization
            for (char &c : msg) {
                if (c == ',' || c == ':' || c == ';' || c == '{' || c == '}' || c == '"') {
                    c = ' ';
                }
            }

            std::stringstream ss(msg);
            float v1 = 0.0f, v2 = 0.0f, v3 = 0.0f;
            if (ss >> v1 >> v2) {
                if (!(ss >> v3)) v3 = 0.0f;
                // Standard gyro packing: pitch, yaw, roll
                pitchDegrees.store(v1);
                yawDegrees.store(v2);
                rollDegrees.store(v3);
                hasReceivedData.store(true);
            }
        }
    }

#ifdef _WIN32
    closesocket(sockfd);
    WSACleanup();
#else
    close(sockfd);
#endif
}
