#pragma once

// OS環境ごとに使えるOpenGLのきのうさをこのファイルにまとめる
namespace gl::platform {
#ifdef __APPLE__ // Mac での設定
// バージョン
inline constexpr int kContextMajorVersion = 4;
inline constexpr int kConstMinorVersion = 1; // Macが対応しているのは4.1まで

inline constexpr bool kRequiresForwardCompatContext = true; // 古い機能を除外
inline constexpr const char* kGlslversionLine = "#version 410 core";
inline constexpr bool kSupportComputeShader = false; // Bloom のコンピュートシェーダーを使うかを決める

#else // Mac 以外

inline constexpr int kContextMajorVersion = 4;
inline constexpr int kConstMinorVersion = 6; 

inline constexpr bool kRequiresForwardCompatContext = false; 
inline constexpr const char* kGlslversionLine = "#version 460 core";
inline constexpr bool kSupportComputeShader = true; 

#endif

}