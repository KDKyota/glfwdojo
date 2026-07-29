#pragma once

struct GLFWwindow;

/*
 * Dear ImGui のラッパー。
 *
 * Window にも Scene にも混ぜず独立させているのは、ImGui が
 * 「GLFW のコールバック」「OpenGL の描画」「毎フレームの開始/終了」という
 * 複数の関心事にまたがるため。ここに閉じ込めておくと、
 * ImGui をやめたくなったときにこのクラスごと外せる。
 */
class Gui {
public:
	// GLFW のコールバック登録がすべて済んだ「後」に呼ぶこと。
	// ImGui_ImplGlfw_InitForOpenGL(window, true) は既存のコールバックを保存して
	// 連鎖呼び出しする仕組みなので、先に ImGui を初期化すると
	// プロジェクト側のコールバックが ImGui を素通りして登録され、
	// UI がマウス入力を受け取れなくなる。
	explicit Gui(GLFWwindow* window);
	~Gui();

	Gui(const Gui&) = delete;
	Gui& operator=(const Gui&) = delete;

	// 毎フレーム、ImGui:: の呼び出しより前に1回
	void NewFrame();
	// 毎フレーム、Scene の描画がすべて終わった後に1回。
	// デフォルトフレームバッファに描画されるので、スクリーンクワッドより後に呼ぶこと。
	void Render();

	// UI がマウス/キーボードを掴んでいるか。
	// これを見てカメラ操作を抑止しないと、スライダーを右ドラッグした瞬間に
	// 視点が回転してしまう（このプロジェクトの視点操作が右ドラッグのため）。
	static bool WantCaptureMouse();
	static bool WantCaptureKeyboard();
};
