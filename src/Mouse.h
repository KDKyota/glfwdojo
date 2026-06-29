#pragma once
#include <utility>

class MouseState
{
private:
	bool FirstMouse = true;
	bool RightPressed = false;
	float LastX = 800.0f / 2.0f;
	float LastY = 600.0f / 2.0f;

public:
	std::pair<float, float> ComputeOffset(double xpos, double ypos);
	void SetRightPressed(bool pressed);
	bool IsRightPressed() const;

};

