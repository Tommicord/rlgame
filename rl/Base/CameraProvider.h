#pragma once

#include "rl/Base/DeviceInputReceiver.h"

namespace Rl::Providers
{

class AbstractCamera {
public:
    double far, near;
    double x, y, z;
    float fov;
    float aspectRatio;
    float zoom;
    float pitch, yaw;
    virtual ~AbstractCamera() = default;
    AbstractCamera() = default;
    AbstractCamera(AbstractCamera&& other) = default;
    AbstractCamera& operator=(AbstractCamera&& other) = default;
    AbstractCamera(const AbstractCamera& other) = default;
    AbstractCamera& operator=(const AbstractCamera& other) = default;
    virtual void Update() = 0;
    virtual void OnMouseMove(double x, double y) = 0;
    virtual void OnScroll(double yoffset) = 0;
};

class CameraInputReceiver : public Input::DeviceInputReceiver {
public:g

};

class Camera : public AbstractCamera {
public:
    Camera() {}
    Camera(Camera&& other) = default;
    Camera& operator=(Camera&& other) = default;
    Camera(const Camera& other) = default;
    Camera& operator=(const Camera& other) = default;
    ~Camera() override = default;
    void Update() override;
    void OnMouseMove(double x, double y) override;
    void OnScroll(double yOffset) override;
};

}