cmake_minimum_required(VERSION 3.20)
project(glfwdojo)

find_package(OpenGL REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(glad CONFIG REQUIRED)

add_executable(glfwdojo
    src/main.cpp
    src/Callbacks.cpp
    src/Camera.cpp
    src/Mouse.cpp
    src/Scene.cpp
    src/Shader.cpp
    src/Window.cpp
    third_party/stb_image_impl.cpp
    src/Material.cpp
 "src/Texture.h" "src/Texture.cpp" "src/TextureCache.cpp" "src/TextureCache.h")

target_compile_features(glfwdojo PRIVATE cxx_std_17)

target_compile_options(glfwdojo PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/utf-8>
)

target_include_directories(glfwdojo PRIVATE
    third_party
)

# stb_image.hはサードパーティコードなので警告を全て抑制する
set_source_files_properties(third_party/stb_image_impl.cpp PROPERTIES
    COMPILE_OPTIONS $<$<CXX_COMPILER_ID:MSVC>:/W0>
    COMPILE_DEFINITIONS "_CRT_SECURE_NO_WARNINGS"
)

target_link_libraries(glfwdojo PRIVATE
    OpenGL::GL
    glfw
    glm::glm
    glad::glad
)

add_custom_command(TARGET glfwdojo POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/shader_src/shader.vert
        ${CMAKE_CURRENT_SOURCE_DIR}/shader_src/shader.frag
        ${CMAKE_CURRENT_SOURCE_DIR}/shader_src/light_cube.vert
        ${CMAKE_CURRENT_SOURCE_DIR}/shader_src/light_cube.frag
        $<TARGET_FILE_DIR:glfwdojo>
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/resources
        $<TARGET_FILE_DIR:glfwdojo>/resources
)
