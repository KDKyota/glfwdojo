#pragma once
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <memory>
#include <string>

// GLFWwindowのDeleter
struct GLFWwindowDeleter {
    void operator()(GLFWwindow *w) const {
        glfwDestroyWindow(w);
    }
};

class Window {
  public:
    Window(int width, int height, const std::string &title);
    ~Window(); // glfwTerminal関数を呼び出す

    bool ShouldClose() const;
    void SwapBuffers();
    void PolleEvents();
    GLFWwindow *Get() const;
    int GetWidth() const;
    int GetHeight() const;

    // ゲームプレイ中はカーソルを掴んで、ボタンを押さなくても視点が回るようにする
    void SetCursorCaptured(bool captured);

  private:
    std::unique_ptr<GLFWwindow, GLFWwindowDeleter> handle_;
    int width_, height_;
};
