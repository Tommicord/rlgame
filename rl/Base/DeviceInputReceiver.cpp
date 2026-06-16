#include "rl/Base/DeviceInputReceiver.h"
#include <iostream>
#include <algorithm>
#include <mutex>
#include <thread>

namespace Rl::Input {

DeviceInputReceiver::DeviceInputReceiver() {
}

DeviceInputReceiver::~DeviceInputReceiver() {
    Stop();
}

DeviceInputReceiver& DeviceInputReceiver::GetInstance() {
    static DeviceInputReceiver instance;
    return instance;
}

void DeviceInputReceiver::Subscribe(InputObserver* observer) {
    std::lock_guard<std::mutex> lock(observersMutex);
    observers.push_back(observer);
}

void DeviceInputReceiver::Unsubscribe(InputObserver* observer) {
    std::lock_guard<std::mutex> lock(observersMutex);
    observers.erase(
        std::remove(observers.begin(), observers.end(), observer),
        observers.end()
    );
}

void DeviceInputReceiver::NotifyKeyEvent(const KeyEvent& event) {
    std::lock_guard<std::mutex> lock(observersMutex);
    for (auto* observer : observers) {
        observer->OnKeyEvent(event);
    }
}

void DeviceInputReceiver::NotifyMouseButtonEvent(const MouseButtonEvent& event) {
    std::lock_guard<std::mutex> lock(observersMutex);
    for (auto* observer : observers) {
        observer->OnMouseButtonEvent(event);
    }
}

void DeviceInputReceiver::NotifyMouseMoveEvent(const MouseMoveEvent& event) {
    std::lock_guard<std::mutex> lock(observersMutex);
    for (auto* observer : observers) {
        observer->OnMouseMoveEvent(event);
    }
}

void DeviceInputReceiver::NotifyMouseScrollEvent(const MouseScrollEvent& event) {
    std::lock_guard<std::mutex> lock(observersMutex);
    for (auto* observer : observers) {
        observer->OnMouseScrollEvent(event);
    }
}

void DeviceInputReceiver::Start() {
    if (running.load()) {
        return;
    }
    
    running.store(true);
    inputThread = std::thread(&DeviceInputReceiver::InputThread, this);
}

void DeviceInputReceiver::Stop() {
    if (!running.load()) {
        return;
    }
    
    running.store(false);
    cv.notify_all();
    
    if (inputThread.joinable()) {
        inputThread.join();
    }
}

void DeviceInputReceiver::InputThread() {
    while (running.load()) {
        // This thread waits for input events
        // In a real implementation, this would use platform-specific APIs
        // to poll for keyboard and mouse input
        // For now, we'll use a condition variable to wait for notifications
        
        std::unique_lock<std::mutex> lock(cvMutex);
        cv.wait(lock, [this] { return !running.load(); });
        
        // Input polling would go here
        // Since GLFW is event-driven, actual input handling typically happens
        // through callbacks in the main thread
    }
}

} // namespace Input
