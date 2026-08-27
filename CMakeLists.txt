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
  src/Callbacks.cpp
  src/Camera.cpp
  src/Mouse.cpp
  src/Scene.cpp
  src/Shader.cpp
  src/Window.cpp
  third_party/stb_image_impl.cpp
  src/Material.cpp
  src/Texture.cpp
  src/TextureCache.cpp
  src/Lighting.cpp
  src/Model.cpp
  src/Mesh.cpp
  src/GeometryData.cpp
  src/Gui.cpp
  src/InputState.cpp
  src/Character.cpp
  src/Collision.cpp
  src/GpuProfiler.cpp)

target_compile_features(glfwdojo PRIVATE cxx_std_17)

target_compile_options(glfwdojo PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/utf-8>)

target_include_directories(glfwdojo PRIVATE third_party)

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

set(SHADER_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/shader_src/shader.vert
    ${CMAKE_CURRENT_SOURCE_DIR}/shader_src/light_cube.vert
    ${CMAKE_CURRENT_SOURCE_DIR}/shader_src/light_cube.frag
    ${CMAKE_CURRENT_SOURCE_DIR}/shader_src/fragment_quad.vert
    ${CMAKE_CURRENT_SOURCE_DIR}/shader_src/skybox.vert
    ${CMAKE_CURRENT_SOURCE_DIR}/shader_src/skybox.frag
    ${CMAKE_CURRENT_SOURCE_DIR}/shader_src/cube.vert
    ${CMAKE_CURRENT_SOURCE_DIR}/shader_src/window.vert
    ${CMAKE_CURRENT_SOURCE_DIR}/shader_src/shadow_mapping_depth.vert
    ${CMAKE_CURRENT_SOURCE_DIR}/shader_src/shadow_mapping_depth.frag
    ${CMAKE_CURRENT_SOURCE_DIR}/shader_src/debug_depth.frag
    ${CMAKE_CURRENT_SOURCE_DIR}/shader_src/wall.vert
    ${CMAKE_CURRENT_SOURCE_DIR}/shader_src/point_shadow_depth.vert
    ${CMAKE_CURRENT_SOURCE_DIR}/shader_src/point_shadow_depth.geom
    ${CMAKE_CURRENT_SOURCE_DIR}/shader_src/point_shadow_depth.frag
    ${CMAKE_CURRENT_SOURCE_DIR}/shader_src/point_shadow_color.frag
    ${CMAKE_CURRENT_SOURCE_DIR}/shader_src/hdr.frag
    ${CMAKE_CURRENT_SOURCE_DIR}/shader_src/blur.frag
    ${CMAKE_CURRENT_SOURCE_DIR}/shader_src/gbuffer_floor.frag
    ${CMAKE_CURRENT_SOURCE_DIR}/shader_src/gbuffer_wall.frag
    ${CMAKE_CURRENT_SOURCE_DIR}/shader_src/gbuffer_cube.frag
    ${CMAKE_CURRENT_SOURCE_DIR}/shader_src/gbuffer_model.vert
    ${CMAKE_CURRENT_SOURCE_DIR}/shader_src/gbuffer_model.frag
    ${CMAKE_CURRENT_SOURCE_DIR}/shader_src/deferred_lighting.frag
    ${CMAKE_CURRENT_SOURCE_DIR}/shader_src/ssao.frag
    ${CMAKE_CURRENT_SOURCE_DIR}/shader_src/ssao_blur.frag
    ${CMAKE_CURRENT_SOURCE_DIR}/shader_src/gbuffer_window.frag
    ${CMAKE_CURRENT_SOURCE_DIR}/shader_src/glass.frag
    # #include される共通ファイル。exe と同じ場所に無いと展開に失敗する
    ${CMAKE_CURRENT_SOURCE_DIR}/shader_src/pbr_common.glsl
    ${CMAKE_CURRENT_SOURCE_DIR}/shader_src/shadow_common.glsl
    ${CMAKE_CURRENT_SOURCE_DIR}/shader_src/lighting_common.glsl
    ${CMAKE_CURRENT_SOURCE_DIR}/shader_src/cubemap_capture.vert
    ${CMAKE_CURRENT_SOURCE_DIR}/shader_src/irradiance_convolution.frag
    ${CMAKE_CURRENT_SOURCE_DIR}/shader_src/equirectangular_to_cubemap.frag
    ${CMAKE_CURRENT_SOURCE_DIR}/shader_src/prefilter.frag
    ${CMAKE_CURRENT_SOURCE_DIR}/shader_src/brdf_lut.frag
    ${CMAKE_CURRENT_SOURCE_DIR}/shader_src/debug_line.vert
    ${CMAKE_CURRENT_SOURCE_DIR}/shader_src/debug_line.frag)

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
