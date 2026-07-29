#include "Gui.h"

// vcpkg 版の imgui はバックエンドのヘッダーを include 直下に置く。
// 公式リポジトリを直接使う場合は <backends/imgui_impl_glfw.h> になるので、
// LearnOpenGL や公式のサンプルをそのまま写すとパスが合わない。
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

Gui::Gui(GLFWwindow* window) {
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui::StyleColorsDark();

	// 第2引数 true で、既に登録されている GLFW のコールバックを ImGui が保存し、
	// 自分の処理の後に連鎖して呼んでくれる（chaining）。
	// そのためこの呼び出しは Window のコールバック登録より後でなければならない。
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	// GLSL のバージョン文字列。プロジェクトのコンテキストは 4.6 core。
	ImGui_ImplOpenGL3_Init("#version 460 core");
}

Gui::~Gui() {
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
}

void Gui::NewFrame() {
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
}

void Gui::Render() {
	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

bool Gui::WantCaptureMouse() { return ImGui::GetIO().WantCaptureMouse; }

bool Gui::WantCaptureKeyboard() { return ImGui::GetIO().WantCaptureKeyboard; }
