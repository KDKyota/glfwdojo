#pragma once
#include <utility>

class MouseState
{
private:
	bool FirstMouse = true;
	float LastX = 800.0f / 2.0f;
	float LastY = 600.0f / 2.0f;

public:
	std::pair<float, float> ComputeOffset(double xpos, double ypos);

	// 次の1回を差分0にする
	// カーソル捕捉の切り替えで座標が飛ぶため、その後に呼ぶ
	void Reset();
};
