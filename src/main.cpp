#include "Callbacks.h"
#include "Camera.h"
#include "Gui.h"
#include "Mouse.h"
#include "Scene.h"
#include "Shader.h"
#include "Window.h"

#include <imgui.h>

constexpr int SCR_WIDTH = 1600;
constexpr int SCR_HEIGHT = 900;

std::shared_ptr<Camera> camera = std::make_shared<Camera>();
std::shared_ptr<MouseState> mouse = std::make_shared<MouseState>();
float heightScale = 0.1f; // Parallax Mapping の強さ（矢印キー↑↓で調整）
int main(void) {
    // インスタンスを作成
    auto window = std::make_unique<Window>(SCR_WIDTH, SCR_HEIGHT, "learnopengl");
    auto scene =
        std::make_unique<Scene>(camera, window->GetWidth(), window->GetHeight());
    // ImGui は既存のコールバックを保存して連鎖させるので、必ず登録がすべて済んだ後に生成する
    auto gui = std::make_unique<Gui>(window->Get());

    struct {
        float delta = 0.0f, last = 0.0f, targetFrameTime = 1.0f / 60.0f;
    } frametime; // ループごとの経過時間を確認する構造体

    while (!window->ShouldClose()) {
        float currentFrame = static_cast<float>(glfwGetTime());
        frametime.delta = currentFrame - frametime.last;
        frametime.last = currentFrame;

        processInput(window->Get(), frametime.delta);

        // ImGui:: の呼び出しより前に必ず1回
        gui->NewFrame();

        // TODO(Phase 4): ライトや SSAO のパラメータもここに追加する
        ImGui::Begin("Debug");
        {
            ImGui::Text("%.1f FPS (%.2f ms)", ImGui::GetIO().Framerate,
                        1000.0f / ImGui::GetIO().Framerate);
            ImGui::Separator();

            static const char *kDebugModes[] = {
                "0: Normal lighting", "1: Shadow (light 0)", "2: shadowMap[0] raw",
                "3: G-Buffer Albedo", "4: G-Buffer Normal", "5: G-Buffer Position",
                "6: Split view", "7: SSAO", "8: Shadow color[0]",
                "9: G-Buffer Metallic", "10: G-Buffer Roughness",
                "11: IBL Irradiance",   "12: IBL Prefilter",
                "13: IBL BRDF LUT"};
            ImGui::Combo("View", &scene->DebugMode(), kDebugModes,
                         IM_ARRAYSIZE(kDebugModes));

            // G-Buffer や AO を見るときはトーンマッピングを切らないと階調が潰れる
            ImGui::Checkbox("Raw output (skip tonemap/bloom)",
                            &scene->DebugRawOutput());
            ImGui::Separator();

            ImGui::SliderFloat("SSAO strength", &scene->SsaoStrength(), 0.0f, 1.0f);
            ImGui::SliderFloat("Ambient", &scene->AmbientStrength(), 0.0f, 1.0f);
            ImGui::SliderFloat("Bloom", &scene->BloomStrength(), 0.0f, 2.0f);
            ImGui::SliderFloat("Exposure", &scene->Exposure(), 0.05f, 5.0f);
            ImGui::Separator();

            // metallic は物理的には 0 か 1 の二択。中間は錆びた鉄のような混在時のみ
            ImGui::SliderFloat("Metallic: cube", &scene->CubeMaterial().metallic, 0.0f, 1.0f);
            ImGui::SliderFloat("Metallic: floor", &scene->FloorMaterial().metallic, 0.0f, 1.0f);
            ImGui::SliderFloat("Metallic: wall", &scene->WallMaterial().metallic, 0.0f, 1.0f);
            ImGui::SliderFloat("Metallic: window", &scene->WindowMaterial().metallic, 0.0f, 1.0f);
            ImGui::Separator();

            // 下限を 0 にしない。GGX の分布が発散して真っ白な点が出る
            ImGui::SliderFloat("Roughness: cube", &scene->CubeMaterial().roughness, 0.05f, 1.0f);
            ImGui::SliderFloat("Roughness: floor", &scene->FloorMaterial().roughness, 0.05f, 1.0f);
            ImGui::SliderFloat("Roughness: wall", &scene->WallMaterial().roughness, 0.05f, 1.0f);
            ImGui::SliderFloat("Roughness: window", &scene->WindowMaterial().roughness, 0.05f, 1.0f);
            ImGui::SliderFloat("Roughness: glass", &scene->GlassMaterial().roughness, 0.05f, 1.0f);
        }
        ImGui::End();

        scene->Render(frametime.delta, heightScale);

        // Scene::Render() に入れるとパスの途中で UI を描くことになる
        gui->Render();

        window->SwapBuffers();
        window->PolleEvents();
    }
}
