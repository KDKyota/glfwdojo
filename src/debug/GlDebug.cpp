#include "GlDebug.h"

#include <glad/glad.h>

#include <iostream>
#include <iterator>

namespace {

const char *sourceName(GLenum source) {
    switch (source) {
    case GL_DEBUG_SOURCE_API:
        return "API";
    case GL_DEBUG_SOURCE_WINDOW_SYSTEM:
        return "WindowSystem";
    case GL_DEBUG_SOURCE_SHADER_COMPILER:
        return "ShaderCompiler";
    case GL_DEBUG_SOURCE_THIRD_PARTY:
        return "ThirdParty";
    case GL_DEBUG_SOURCE_APPLICATION:
        return "Application";
    default:
        return "Other";
    }
}

const char *typeName(GLenum type) {
    switch (type) {
    case GL_DEBUG_TYPE_ERROR:
        return "Error";
    case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
        return "Deprecated";
    case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
        return "UndefinedBehavior";
    case GL_DEBUG_TYPE_PORTABILITY:
        return "Portability";
    case GL_DEBUG_TYPE_PERFORMANCE:
        return "Performance";
    default:
        return "Other";
    }
}

const char *severityName(GLenum severity) {
    switch (severity) {
    case GL_DEBUG_SEVERITY_HIGH:
        return "HIGH";
    case GL_DEBUG_SEVERITY_MEDIUM:
        return "MEDIUM";
    case GL_DEBUG_SEVERITY_LOW:
        return "LOW";
    default:
        return "NOTIFICATION";
    }
}

// GL から呼ばれるので APIENTRY が要る 中から GL 関数を呼ぶと再帰しうるので出力だけに留める
void APIENTRY debugCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei, const GLchar *message,
                            const void *) {
    std::cerr << "[GL " << severityName(severity) << "] " << sourceName(source) << " / " << typeName(type) << " (id "
              << id << ")\n  " << message << std::endl;
}

} // namespace

void gl::EnableDebugOutput() {
    GLint flags = 0;
    glGetIntegerv(GL_CONTEXT_FLAGS, &flags);
    // Release では debug context を要求していないので、ここで抜けるのが既定の動作
    if (!(flags & GL_CONTEXT_FLAG_DEBUG_BIT))
        return;

    glEnable(GL_DEBUG_OUTPUT);
    // 原因となった呼び出しのその場でコールバックが走る コールスタックが犯人を直接指すので必須
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glDebugMessageCallback(debugCallback, nullptr);

    glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
    // ドライバが大量に出すため、切らないとログが実用にならない
    glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION, 0, nullptr, GL_FALSE);

    // NVIDIA が LOW で出す「確保しました」系の報告。性能警告(131218)は残すので個別に指定する
    const GLuint noisyIds[] = {131169, 131185, 131204};
    glDebugMessageControl(GL_DEBUG_SOURCE_API, GL_DEBUG_TYPE_OTHER, GL_DONT_CARE,
                          static_cast<GLsizei>(std::size(noisyIds)), noisyIds, GL_FALSE);
}
