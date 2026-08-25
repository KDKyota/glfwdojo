#include "Callbacks.h"
#include <iostream>
#include <algorithm>

void error_callback(int error, const char *description) {
    std::cerr << "GLFW Error: " << error << description << std::endl;
}

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods) {
    // UI に文字入力が来ていても必ず効かせる。掴んだカーソルから抜けられなくなるのを防ぐ
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        input->TogglePause();
        // 同じ glfwPollEvents() の中で後続のカーソル移動が届きうる
        // ここで捨てないと、ポーズ中に動かした分がゲームプレイ復帰の1発目に乗る
        mouse->Reset();
    }

    // 押しっぱなしで連続切り替えされないよう、processInput ではなくこちらで拾う
    if (key == GLFW_KEY_F && action == GLFW_PRESS && input->IsGameplay())
        camera->ToggleMode();
}

void framebuffer_size_callback(GLFWwindow *window, int width, int height) {
    glViewport(0, 0, width, height);
}

void window_focus_callback(GLFWwindow *window, int focused) {
    // 裏に回ってもカーソルを掴んだままだと、他のウィンドウを操作できなくなる
    if (!focused)
        input->SetMode(InputMode::Paused);
}

void mouse_callback(GLFWwindow *window, double xposIn, double yposIn) {
    if (!input->IsGameplay()) return;
    auto [xoffset, yoffset] = mouse->ComputeOffset(xposIn, yposIn);
    camera->ProcessMouseMovement(xoffset, yoffset);
}

void scroll_callback(GLFWwindow *window, double xoffset, double yoffset) {
    if (!input->IsGameplay()) return;
    camera->ProcessMouseScroll(static_cast<float>(yoffset));
}

void processInput(GLFWwindow *window, float deltaTime, Character *character, const gl::CollisionWorld &colliders) {
    if (!input->IsGameplay()) return;

    if (camera->Mode() == CameraMode::ThirdPerson && character != nullptr) {
        glm::vec2 move(0.0f);
        // 入力キーごとに移動する方向に割り当てる
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
            move.y += 1.0f;
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
            move.y -= 1.0f;
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
            move.x += 1.0f;
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
            move.x -= 1.0f;
        character->Move(camera->GetViewFront(), move, deltaTime, colliders);
    } else {
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
    }

    // Parallax Mapping の heightScale を矢印キーで調整（1秒あたりの変化量=heightScaleSpeed）
    constexpr float heightScaleSpeed = 0.2f;
    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
        heightScale = std::min(heightScale + heightScaleSpeed * deltaTime, 1.0f);
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
        heightScale = std::max(heightScale - heightScaleSpeed * deltaTime, 0.0f);
}
