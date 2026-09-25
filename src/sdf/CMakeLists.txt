# 距離場の焼き込みは Debug でも 5 億回規模の三角形テストを回すため このターゲットだけ最適化を有効にする
# MSVC の /RTC1 は /O 系と排他（D8016）なのでここで外す ディレクトリスコープなので他のターゲットには影響しない
string(REPLACE "/RTC1" "" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")

find_package(Threads REQUIRED)

# OpenGL に依存しないので glfwdojo とは別ターゲットとして最適化フラグを切り替えられる
add_library(sdf_builder STATIC DistanceFieldBuilder.cpp)
target_compile_features(sdf_builder PRIVATE cxx_std_17)
target_include_directories(sdf_builder PUBLIC ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(sdf_builder PUBLIC glm::glm Threads::Threads)
target_compile_options(
  sdf_builder
  PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/utf-8>
          $<$<AND:$<CONFIG:Debug>,$<CXX_COMPILER_ID:MSVC>>:/O2>
          $<$<AND:$<CONFIG:Debug>,$<NOT:$<CXX_COMPILER_ID:MSVC>>>:-O2>)
