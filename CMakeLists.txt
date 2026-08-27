cmake_minimum_required(VERSION 3.20)
project(glfwdojo)

find_package(OpenGL REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(glad CONFIG REQUIRED)
find_package(assimp CONFIG REQUIRED)
# imgui は vcpkg.json で glfw-binding / opengl3-binding の feature を指定している。 feature
# を付けないと imgui 本体だけが入り、backends/imgui_impl_glfw.h などが インストールされないため、include
# できずにビルドが失敗する。
find_package(imgui CONFIG REQUIRED)

add_executable(
  glfwdojo
  src/main.cpp
  # app: ウィンドウ・入力・UI
  src/app/Callbacks.cpp
  src/app/Gui.cpp
  src/app/InputState.cpp
  src/app/Mouse.cpp
  src/app/Window.cpp
  # gl: GL リソースの薄いラッパ
  src/gl/Shader.cpp
  src/gl/Texture.cpp
  src/gl/TextureCache.cpp
  # debug: 計測とデバッグ出力
  src/debug/GpuProfiler.cpp
  src/debug/GlDebug.cpp
  # render: 描画パイプライン
  src/render/GeometryData.cpp
  src/render/Lighting.cpp
  src/render/Material.cpp
  src/render/Scene.cpp
  # asset: 読み込んだモデル
  src/asset/Mesh.cpp
  src/asset/Model.cpp
  # scene: シーン上の存在
  src/scene/Camera.cpp
  src/scene/Character.cpp
  src/scene/Collision.cpp
  third_party/stb_image_impl.cpp)

target_compile_features(glfwdojo PRIVATE cxx_std_17)

target_compile_options(glfwdojo PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/utf-8>)

# src/ を include ルートにしてサブディレクトリを "debug/GpuProfiler.h" のように書けるようにする
target_include_directories(glfwdojo PRIVATE third_party src)

# stb_image.hはサードパーティコードなので警告を全て抑制する
set_source_files_properties(
  third_party/stb_image_impl.cpp
  PROPERTIES COMPILE_OPTIONS $<$<CXX_COMPILER_ID:MSVC>:/W0>
             COMPILE_DEFINITIONS "_CRT_SECURE_NO_WARNINGS")

target_link_libraries(glfwdojo PRIVATE OpenGL::GL glfw glm::glm glad::glad
                                       assimp::assimp imgui::imgui)

# FrameArena の効果測定用。OpenGL に依存しないので単体でビルド・実行できる
add_executable(frame_arena_bench bench/frame_arena_bench.cpp)
target_compile_features(frame_arena_bench PRIVATE cxx_std_17)
target_compile_options(frame_arena_bench
                       PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/utf-8>)
target_include_directories(frame_arena_bench PRIVATE src)

# シェーダーはパス単位でディレクトリを分けている。コピー時に exe の隣へフラットに展開されるので
# ファイル名はディレクトリをまたいで一意に保つこと
set(SHADER_SOURCES
    # common
    ${CMAKE_CURRENT_SOURCE_DIR}/shader_src/common/fragment_quad.vert
    ${CMAKE_CURRENT_SOURCE_DIR}/shader_src/common/lighting_common.glsl
    ${CMAKE_CURRENT_SOURCE_DIR}/shader_src/common/pbr_common.glsl
    ${CMAKE_CURRENT_SOURCE_DIR}/shader_src/common/shadow_common.glsl
    # shadow
    ${CMAKE_CURRENT_SOURCE_DIR}/shader_src/shadow/point_shadow_color.frag
    ${CMAKE_CURRENT_SOURCE_DIR}/shader_src/shadow/point_shadow_depth.frag
    ${CMAKE_CURRENT_SOURCE_DIR}/shader_src/shadow/point_shadow_depth.geom
    ${CMAKE_CURRENT_SOURCE_DIR}/shader_src/shadow/point_shadow_depth.vert
    ${CMAKE_CURRENT_SOURCE_DIR}/shader_src/shadow/shadow_mapping_depth.frag
    ${CMAKE_CURRENT_SOURCE_DIR}/shader_src/shadow/shadow_mapping_depth.vert
    # gbuffer
    ${CMAKE_CURRENT_SOURCE_DIR}/shader_src/gbuffer/cube.vert
    ${CMAKE_CURRENT_SOURCE_DIR}/shader_src/gbuffer/gbuffer_cube.frag
    ${CMAKE_CURRENT_SOURCE_DIR}/shader_src/gbuffer/gbuffer_floor.frag
    ${CMAKE_CURRENT_SOURCE_DIR}/shader_src/gbuffer/gbuffer_model.frag
    ${CMAKE_CURRENT_SOURCE_DIR}/shader_src/gbuffer/gbuffer_model.vert
    ${CMAKE_CURRENT_SOURCE_DIR}/shader_src/gbuffer/gbuffer_wall.frag
    ${CMAKE_CURRENT_SOURCE_DIR}/shader_src/gbuffer/gbuffer_window.frag
    ${CMAKE_CURRENT_SOURCE_DIR}/shader_src/gbuffer/shader.vert
    ${CMAKE_CURRENT_SOURCE_DIR}/shader_src/gbuffer/wall.vert
    ${CMAKE_CURRENT_SOURCE_DIR}/shader_src/gbuffer/window.vert
    # ssao
    ${CMAKE_CURRENT_SOURCE_DIR}/shader_src/ssao/ssao.frag
    ${CMAKE_CURRENT_SOURCE_DIR}/shader_src/ssao/ssao_blur.frag
    # lighting
    ${CMAKE_CURRENT_SOURCE_DIR}/shader_src/lighting/deferred_lighting.frag
    # forward
    ${CMAKE_CURRENT_SOURCE_DIR}/shader_src/forward/glass.frag
    ${CMAKE_CURRENT_SOURCE_DIR}/shader_src/forward/light_cube.frag
    ${CMAKE_CURRENT_SOURCE_DIR}/shader_src/forward/light_cube.vert
    ${CMAKE_CURRENT_SOURCE_DIR}/shader_src/forward/skybox.frag
    ${CMAKE_CURRENT_SOURCE_DIR}/shader_src/forward/skybox.vert
    # post
    ${CMAKE_CURRENT_SOURCE_DIR}/shader_src/post/blur.frag
    ${CMAKE_CURRENT_SOURCE_DIR}/shader_src/post/hdr.frag
    # ibl
    ${CMAKE_CURRENT_SOURCE_DIR}/shader_src/ibl/brdf_lut.frag
    ${CMAKE_CURRENT_SOURCE_DIR}/shader_src/ibl/cubemap_capture.vert
    ${CMAKE_CURRENT_SOURCE_DIR}/shader_src/ibl/equirectangular_to_cubemap.frag
    ${CMAKE_CURRENT_SOURCE_DIR}/shader_src/ibl/irradiance_convolution.frag
    ${CMAKE_CURRENT_SOURCE_DIR}/shader_src/ibl/prefilter.frag
    # debug
    ${CMAKE_CURRENT_SOURCE_DIR}/shader_src/debug/debug_depth.frag
    ${CMAKE_CURRENT_SOURCE_DIR}/shader_src/debug/debug_line.frag
    ${CMAKE_CURRENT_SOURCE_DIR}/shader_src/debug/debug_line.vert)

add_custom_command(
  TARGET glfwdojo
  POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_if_different ${SHADER_SOURCES}
          $<TARGET_FILE_DIR:glfwdojo>
  COMMAND
    ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/resources
    $<TARGET_FILE_DIR:glfwdojo>/resources
  COMMENT "Copying shaders and resources")

# シェーダーと resources/ は exe の隣にコピーされ、コード側は相対パスで開くので
# WORKING_DIRECTORY を exe のフォルダにしないと読み込みに失敗する
add_custom_target(
  run
  COMMAND $<TARGET_FILE:glfwdojo>
  WORKING_DIRECTORY $<TARGET_FILE_DIR:glfwdojo>
  USES_TERMINAL)
add_dependencies(run glfwdojo)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
