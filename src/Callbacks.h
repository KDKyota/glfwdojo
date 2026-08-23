#pragma once
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <memory>
#include "Camera.h"
#include "InputState.h"
#include "Mouse.h"

extern std::shared_ptr<Camera> camera;
extern std::shared_ptr<MouseState> mouse;
extern std::shared_ptr<InputState> input;
extern float heightScale;

void error_callback(int error, const char *description);
void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods);
void framebuffer_size_callback(GLFWwindow *window, int width, int height);
void window_focus_callback(GLFWwindow *window, int focused);
void processInput(GLFWwindow *window, float deltaTime);
void mouse_callback(GLFWwindow *window, double xposIn, double yposIn);
void scroll_callback(GLFWwindow *window, double xoffset, double yoffset);
