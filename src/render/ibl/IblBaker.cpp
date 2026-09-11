#include "render/ibl/IblBaker.h"

#include "gl/RenderTarget.h"
#include "gl/Shader.h"

#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <stb_image.h>

namespace gl {
namespace {

constexpr unsigned int kEnvCubemapSize = 512;
// 畳み込み後は極めて低周波なので 解像度を上げても情報が増えない
constexpr unsigned int kIrradianceSize = 32;
constexpr unsigned int kPrefilterSize = 128;
constexpr unsigned int kPrefilterMipLevels = 5;
constexpr unsigned int kBrdfLutSize = 512;

} // namespace

IblMaps BakeIblMaps(const SceneGeometry &geometry, int viewportWidth, int viewportHeight) {
    IblMaps maps;

    /* --- 正距円筒図法の HDR を読み込む --- */
    stbi_set_flip_vertically_on_load(true);
    int width, height, nrComponents;
    float *data = stbi_loadf("resources/textures/skybox/studio_small_03_4k.hdr", &width, &height, &nrComponents, 0);
    if (!data) {
        std::cout << "ERROR::IBL:: Failed to load HDR environment map" << std::endl;
        return maps;
    }
    // 焼き終われば用済みなのでここのスコープに閉じる
    TextureHandle hdrTexture;
    hdrTexture.create();
    glBindTexture(GL_TEXTURE_2D, hdrTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width, height, 0, GL_RGB, GL_FLOAT, data);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    stbi_image_free(data);

    Shader equirectToCubemapShader("cubemap_capture.vert", "equirectangular_to_cubemap.frag");
    Shader irradianceShader("cubemap_capture.vert", "irradiance_convolution.frag");
    Shader prefilterShader("cubemap_capture.vert", "prefilter.frag");
    Shader brdfLutShader("fragment_quad.vert", "brdf_lut.frag");

    /* --- 6面ぶんの共通設定 --- */
    const glm::mat4 captureProjection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
    const glm::mat4 captureViews[6] = {
        glm::lookAt(glm::vec3(0.0f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
        glm::lookAt(glm::vec3(0.0f), glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
        glm::lookAt(glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
        glm::lookAt(glm::vec3(0.0f), glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f)),
        glm::lookAt(glm::vec3(0.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
        glm::lookAt(glm::vec3(0.0f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, -1.0f, 0.0f))};

    FramebufferHandle captureFBO;
    RenderbufferHandle captureRBO;
    captureFBO.create();
    captureRBO.create();
    /* 事前計算中だけ変える GL 状態 末尾のベース状態への復帰と対にすること */
    // 立方体の内側から見るので 通常のカリングでは面が消える
    glDisable(GL_CULL_FACE);
    // BRDF LUT は out vec2 でアルファが未定義 切らないと GL_SRC_ALPHA が 0 になり書き込みが消える
    glDisable(GL_BLEND);

    /* --- equirectangular -> cubemap --- */
    // こちらは描き込み先なので 3成分フォーマットを選んではいけない
    // prefilter がサンプルの粗密に応じてミップを引くので ミップ付きにしておく
    CreateCubemap(maps.envCubemap, GL_RGBA16F, GL_RGBA, GL_FLOAT, kEnvCubemapSize, GL_LINEAR_MIPMAP_LINEAR,
                  GL_LINEAR);

    glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
    glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, kEnvCubemapSize, kEnvCubemapSize);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, captureRBO);

    equirectToCubemapShader.use();
    equirectToCubemapShader.setInt("equirectangularMap", 0);
    equirectToCubemapShader.setMat4("projection", captureProjection);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, hdrTexture);

    glViewport(0, 0, kEnvCubemapSize, kEnvCubemapSize);
    for (unsigned int i = 0; i < 6; ++i) {
        equirectToCubemapShader.setMat4("view", captureViews[i]);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
                               maps.envCubemap, 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        geometry.DrawSkyboxMesh();
    }

    // ミップの中身を埋める prefilter がこれを引く
    glBindTexture(GL_TEXTURE_CUBE_MAP, maps.envCubemap);
    glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

    /* --- cubemap -> irradiance --- */
    CreateCubemap(maps.irradianceMap, GL_RGBA16F, GL_RGBA, GL_FLOAT, kIrradianceSize, GL_LINEAR, GL_LINEAR);

    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, kIrradianceSize, kIrradianceSize);

    irradianceShader.use();
    irradianceShader.setInt("environmentMap", 0);
    irradianceShader.setMat4("projection", captureProjection);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, maps.envCubemap);

    glViewport(0, 0, kIrradianceSize, kIrradianceSize);
    for (unsigned int i = 0; i < 6; ++i) {
        irradianceShader.setMat4("view", captureViews[i]);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
                               maps.irradianceMap, 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        geometry.DrawSkyboxMesh();
    }

    /* --- cubemap -> prefilter（roughness ごとにミップへ焼く） --- */
    // roughness の連続変化をミップ間の補間で表現するので TRILINEAR が必須
    CreateCubemap(maps.prefilterMap, GL_RGBA16F, GL_RGBA, GL_FLOAT, kPrefilterSize, GL_LINEAR_MIPMAP_LINEAR,
                  GL_LINEAR);
    glGenerateMipmap(GL_TEXTURE_CUBE_MAP); // 各レベルの領域だけ確保させる

    prefilterShader.use();
    prefilterShader.setInt("environmentMap", 0);
    prefilterShader.setMat4("projection", captureProjection);
    prefilterShader.setFloat("envResolution", static_cast<float>(kEnvCubemapSize));
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, maps.envCubemap);

    for (unsigned int mip = 0; mip < kPrefilterMipLevels; ++mip) {
        const unsigned int mipSize = kPrefilterSize >> mip;
        glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, mipSize, mipSize);
        glViewport(0, 0, mipSize, mipSize);

        const float roughness = static_cast<float>(mip) / static_cast<float>(kPrefilterMipLevels - 1);
        prefilterShader.setFloat("roughness", roughness);
        for (unsigned int i = 0; i < 6; ++i) {
            prefilterShader.setMat4("view", captureViews[i]);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
                                   maps.prefilterMap, mip);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            geometry.DrawSkyboxMesh();
        }
    }

    /* --- BRDF LUT --- */
    maps.brdfLut.create();
    glBindTexture(GL_TEXTURE_2D, maps.brdfLut);
    // 返すのはスケールとバイアスの2値なので2成分で足りる
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RG16F, kBrdfLutSize, kBrdfLutSize, 0, GL_RG, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, kBrdfLutSize, kBrdfLutSize);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, maps.brdfLut, 0);
    glViewport(0, 0, kBrdfLutSize, kBrdfLutSize);
    brdfLutShader.use();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    geometry.DrawScreenQuad();
    glBindVertexArray(0);

    CheckFramebufferComplete("IBL");
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, viewportWidth, viewportHeight);

    /* 冒頭で変えた GL 状態を戻す */
    glEnable(GL_BLEND);
    // カリングのベース状態は無効 ここで有効にすると床・壁・空が消える

    return maps;
}

} // namespace gl
