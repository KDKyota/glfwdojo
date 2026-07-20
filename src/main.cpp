#include "Shader.h"
#include "Camera.h"
#include "Callbacks.h"
#include "Mouse.h"
#include "Scene.h"
#include "Window.h"

constexpr int SCR_WIDTH = 800;
constexpr int SCR_HEIGHT = 600;

std::shared_ptr<Camera> camera = std::make_shared<Camera>();
std::shared_ptr<MouseState> mouse = std::make_shared<MouseState>();
float heightScale = 0.1f; // Parallax Mapping の強さ（矢印キー↑↓で調整）
int main(void)
{
	// インスタンスを作成
	auto window = std::make_unique<Window>(SCR_WIDTH, SCR_HEIGHT, "learnopengl");
	auto scene = std::make_unique<Scene>(camera, window->GetWidth(), window->GetHeight());

	struct { float delta = 0.0f, last = 0.0f, targetFrameTime = 1.0f / 60.0f; } frametime; // ループごとの経過時間を確認する構造体

	while (!window->ShouldClose())
	{
		float currentFrame = static_cast<float> (glfwGetTime());
		frametime.delta = currentFrame - frametime.last;
		frametime.last = currentFrame;

		processInput(window->Get(), frametime.delta);

		scene->Render(frametime.delta, heightScale);
		
		window->SwapBuffers();
		window->PolleEvents();
	}
}