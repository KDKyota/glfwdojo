#include "app/Mouse.h"

void MouseState::Reset() {
    firstMouse_ = true;
}

std::pair<float, float> MouseState::ComputeOffset(double xpos, double ypos) {
    const float x = static_cast<float>(xpos);
    const float y = static_cast<float>(ypos);

    if (firstMouse_) {
        lastX_ = x;
        lastY_ = y;
        firstMouse_ = false;
    }

    const float xoffset = x - lastX_;
    const float yoffset = lastY_ - y;

    lastX_ = x;
    lastY_ = y;

    return {xoffset, yoffset};
}
