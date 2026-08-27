#include "app/Mouse.h"

void MouseState::Reset() {
    FirstMouse = true;
}

std::pair<float, float> MouseState::ComputeOffset(double xpos, double ypos) {
    float fx = static_cast<float>(xpos);
    float fy = static_cast<float>(ypos);

    if (this->FirstMouse) {
        this->LastX = fx;
        this->LastY = fy;
        FirstMouse = false;
    }

    float xoffset = fx - this->LastX;
    float yoffset = this->LastY - fy;

    this->LastX = fx;
    this->LastY = fy;

    return {xoffset, yoffset};
}
