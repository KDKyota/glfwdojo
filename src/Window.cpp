#include "Window.h"
#include "Callbacks.h"
#include <stdexcept>

Window::Window(int width, int height, const std::string& title) : width_(width), height_(height)
{
	if (!glfwInit())
		throw std::runtime_error("Failed to initialize GLFW");

	glfwSetErrorCallback(error_callback); // エラーのコールバック関数を登録
	// バージョン指定
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	// スマートポインタの変数handle_にglfwCreateWindowの返り値を右辺値として代入するイメージ
	handle_.reset(glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr));
	if (!handle_)
		throw std::runtime_error("Failed to create GLFW window");

	glfwMakeContextCurrent(handle_.get());
	glfwSwapInterval(1); // fpsをフレームレートに合わせて固定
	
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
		throw std::runtime_error("Failed to initialize GLAD");

	glClearColor(0.1f, 0.1f, 0.1f, 1.0f);

	glViewport(0, 0, width, height);

	glEnable(GL_DEPTH_TEST);

	// コールマック関数を登録
	// ふつうは第一引数がwindowだが、今回はスマートポインタにhandle_代入しているのでその先頭ポインタという意味でhandle_.get()
	glfwSetFramebufferSizeCallback(handle_.get(), framebuffer_size_callback);
    glfwSetMouseButtonCallback(handle_.get(), mouse_button_callback);
    glfwSetCursorPosCallback(handle_.get(), mouse_callback);
    glfwSetScrollCallback(handle_.get(), scroll_callback);
    glfwSetKeyCallback(handle_.get(), key_callback);
}

Window::~Window() 
{ 
	handle_.reset();
	glfwTerminate(); 
}

bool Window::ShouldClose() const
{
	return glfwWindowShouldClose(handle_.get());
}

void Window::SwapBuffers() {
	glfwSwapBuffers(handle_.get());
}

void Window::PolleEvents() {
	glfwPollEvents();
}

GLFWwindow* Window::Get() const
{
	return handle_.get();
}

int Window::GetWidth() const
{
	return width_;
}

int Window::GetHeight() const
{
	return height_;
}
