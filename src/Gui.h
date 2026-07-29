#pragma once

struct GLFWwindow;

// Dear ImGui のラッパー。ImGui は GLFW のコールバック・OpenGL の描画・
// フレームの開始/終了にまたがるので、ここに閉じ込めて切り離せるようにしている
class Gui {
public:
	// GLFW のコールバック登録がすべて済んだ「後」に呼ぶこと。
	// ImGui は既存のコールバックを保存して連鎖呼び出しするため、順序が逆だと入力を拾えない
	explicit Gui(GLFWwindow* window);
	~Gui();

	Gui(const Gui&) = delete;
	Gui& operator=(const Gui&) = delete;

	// ImGui:: の呼び出しより前に1回
	void NewFrame();
	// Scene の描画がすべて終わった後に1回（デフォルトFBOに描画される）
	void Render();

	// UI が入力を掴んでいるか。カメラ操作の抑止に使う
	static bool WantCaptureMouse();
	static bool WantCaptureKeyboard();
};
