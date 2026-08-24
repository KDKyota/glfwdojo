#include "Window.h"
#include "Callbacks.h"
#include <stdexcept>
#include <iostream>

Window::Window(int width, int height, const std::string &title) : width_(width), height_(height) {
    if (!glfwInit())
        throw std::runtime_error("Failed to initialize GLFW");

    glfwSetErrorCallback(error_callback); // エラーのコールバック関数を登録
    // バージョン指定
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWmonitor *monitor = glfwGetPrimaryMonitor(); // 今回は利用しない
    // フルスクリーンのためのモニターの解像度を取得
    const GLFWvidmode *mode = glfwGetVideoMode(monitor); // 今回は利用しない

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

    // GL_FRAMEBUFFER_SRGB は使わない。hdr.frag のガンマ補正と二重になり白っぽくなる
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    std::cout << glGetString(GL_VERSION) << std::endl;
    std::cout << glGetString(GL_VENDOR) << std::endl;
    std::cout << glGetString(GL_RENDERER) << std::endl;

    // GL_SRGB なら GL_FRAMEBUFFER_SRGB による自動ガンマ補正が効く環境ということ
    {
        GLint encoding = 0;
        glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_BACK_LEFT,
                                              GL_FRAMEBUFFER_ATTACHMENT_COLOR_ENCODING, &encoding);
        std::cout << "default framebuffer color encoding = 0x" << std::hex << encoding
                  << std::dec << (encoding == 0x8C40 ? "  (GL_SRGB)" : "  (GL_LINEAR)")
                  << std::endl;
    }
    glfwSetFramebufferSizeCallback(handle_.get(), framebuffer_size_callback);
    glfwSetCursorPosCallback(handle_.get(), mouse_callback);
    glfwSetScrollCallback(handle_.get(), scroll_callback);
    glfwSetKeyCallback(handle_.get(), key_callback);
    glfwSetWindowFocusCallback(handle_.get(), window_focus_callback);
}

Window::~Window() {
    handle_.reset();
    glfwTerminate();
}

bool Window::ShouldClose() const {
    return glfwWindowShouldClose(handle_.get());
}

void Window::SwapBuffers() {
    glfwSwapBuffers(handle_.get());
}

void Window::PolleEvents() {
    glfwPollEvents();
}

GLFWwindow *Window::Get() const {
    return handle_.get();
}

int Window::GetWidth() const {
    return width_;
}

int Window::GetHeight() const {
    return height_;
}

void Window::SetCursorCaptured(bool captured) {
    glfwSetInputMode(handle_.get(), GLFW_CURSOR,
                     captured ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);

    // カーソル捕捉中しか指定できない
    if (glfwRawMouseMotionSupported())
        glfwSetInputMode(handle_.get(), GLFW_RAW_MOUSE_MOTION,
                         captured ? GLFW_TRUE : GLFW_FALSE);
}
