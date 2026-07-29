#include "Callbacks.h"
#include "Gui.h"
#include <iostream>
#include <algorithm>

void error_callback(int error, const char* description)
{
	std::cerr << "GLFW Error: " << error << description << std::endl;
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GLFW_TRUE);
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
	if (button == GLFW_MOUSE_BUTTON_RIGHT)
	{
		if (action == GLFW_PRESS) mouse->SetRightPressed(true);
		if (action == GLFW_RELEASE) mouse->SetRightPressed(false);
	}
}

void mouse_callback(GLFWwindow* window, double xposIn, double yposIn)
{
	// UI 上でドラッグしているときはカメラを回さない。
	// このプロジェクトの視点操作は右ドラッグなので、ガードしないと
	// スライダーを右ドラッグした瞬間に視点が回転してしまう。
	if (Gui::WantCaptureMouse())
	{
		// 追従だけはしておく。そうしないと UI から抜けた瞬間に
		// 前回位置との差分が巨大になり、視点が飛ぶ。
		mouse->ComputeOffset(xposIn, yposIn);
		return;
	}
	if (!mouse->IsRightPressed()) return;
	auto[xoffset, yoffset] = mouse->ComputeOffset(xposIn, yposIn);
	camera->ProcessMouseMovement(xoffset, yoffset);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	// UI 上のスクロールでカメラがズームしないように
	if (Gui::WantCaptureMouse()) return;
	camera->ProcessMouseScroll(static_cast<float>(yoffset));
}

void processInput(GLFWwindow* window, float deltaTime)
{
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);

	// UI のテキスト入力中に WASD でカメラが動かないようにする。
	// Esc は UI に関わらず効かせたいので、このガードより前に置いている。
	if (Gui::WantCaptureKeyboard()) return;

	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		camera->ProcessKeyboard(Camera_Movement::FORWARD, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		camera->ProcessKeyboard(Camera_Movement::BACKWARD, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		camera->ProcessKeyboard(Camera_Movement::LEFT, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		camera->ProcessKeyboard(Camera_Movement::RIGHT, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
		camera->ProcessKeyboard(Camera_Movement::UP, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
		camera->ProcessKeyboard(Camera_Movement::DOWN, deltaTime);

	// Parallax Mapping の heightScale を矢印キーで調整（1秒あたりの変化量 = heightScaleSpeed）
	constexpr float heightScaleSpeed = 0.2f;
	if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
		heightScale = std::min(heightScale + heightScaleSpeed * deltaTime, 1.0f);
	if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
		heightScale = std::max(heightScale - heightScaleSpeed * deltaTime, 0.0f);
}
