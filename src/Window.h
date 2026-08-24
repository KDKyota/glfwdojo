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

/**
 * @brief GLFW ウィンドウと OpenGL コンテキストを管理する RAII ラッパー。
 */
class Window {
  public:
    /**
     * @brief ウィンドウと OpenGL コンテキストを生成する。
     *
     * @param width,height ウィンドウの初期サイズ。
     * @param title タイトルバーの文字列。
     */
    Window(int width, int height, const std::string &title);
    ~Window(); // glfwTerminal関数を呼び出す

    bool ShouldClose() const;
    void SwapBuffers();
    void PolleEvents();
    /**
     * @brief GLFWwindow の生ポインタを返す。
     */
    GLFWwindow *Get() const;
    int GetWidth() const;
    int GetHeight() const;

    // ゲームプレイ中はカーソルを掴んで、ボタンを押さなくても視点が回るようにする
    void SetCursorCaptured(bool captured);

  private:
    std::unique_ptr<GLFWwindow, GLFWwindowDeleter> handle_;
    int width_, height_;
};
