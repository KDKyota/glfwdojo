#pragma once

struct GLFWwindow;

// ImGui は GLFW・OpenGL・フレーム管理にまたがるので、ここに閉じ込めて切り離せるようにする
class Gui {
public:
	// 既存のコールバックを保存して連鎖させるので、登録がすべて済んだ「後」に呼ぶこと
	explicit Gui(GLFWwindow* window);
	~Gui();

	Gui(const Gui&) = delete;
	Gui& operator=(const Gui&) = delete;

	// ImGui:: の呼び出しより前に1回
	void NewFrame();
	// Scene の描画がすべて終わった後に1回（デフォルトFBOに描画される）
	void Render();
};
