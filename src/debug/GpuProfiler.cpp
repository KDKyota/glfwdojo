#include "GpuProfiler.h"

#include <iterator>

namespace {

// GpuPass の並びと一致させること ずれても動いてしまい表示だけが入れ替わる
constexpr const char *kPassNames[] = {
    "Shadow", "Geometry", "SSAO", "BlitDepth", "Lighting", "Forward", "Bloom", "ToScreen",
};
static_assert(std::size(kPassNames) == gl::GpuProfiler::kPassCount, "GpuPass と名前配列の数が合っていない");

} // namespace

void gl::GpuProfiler::Init() {
    for (auto &perPass : queries_)
        for (QueryHandle &query : perPass)
            query.create();
}

void gl::GpuProfiler::BeginFrame() {
    writeSlot_ = static_cast<int>(frame_ % kFrameLag);
    for (auto &perPass : issued_)
        perPass[writeSlot_] = false;
}

void gl::GpuProfiler::EndFrame() {
    // 今書いたスロットではなく最も古いスロットを読む
    collect((writeSlot_ + 1) % kFrameLag);
    ++frame_;
}

void gl::GpuProfiler::begin(GpuPass pass) {
    if (measuring_)
        return;
    measuring_ = true;

    const std::size_t index = static_cast<std::size_t>(pass);
    // RenderDoc のイベント一覧をパス単位のツリーにする 入れ子防止の measuring_ が Push/Pop の対応も保証する
    glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, static_cast<GLuint>(index), -1, Name(pass));
    glBeginQuery(GL_TIME_ELAPSED, queries_[index][writeSlot_]);
    issued_[index][writeSlot_] = true;
}

void gl::GpuProfiler::end() {
    if (!measuring_)
        return;
    glEndQuery(GL_TIME_ELAPSED);
    glPopDebugGroup();
    measuring_ = false;
}

void gl::GpuProfiler::collect(int slot) {
    for (std::size_t i = 0; i < kPassCount; ++i) {
        if (!issued_[i][slot])
            continue;

        // 未完了なら読まずに前回値を保つ ここで待つと計測のために CPU を止めることになる
        GLint available = 0;
        glGetQueryObjectiv(queries_[i][slot], GL_QUERY_RESULT_AVAILABLE, &available);
        if (!available)
            continue;

        GLuint64 nanoseconds = 0;
        glGetQueryObjectui64v(queries_[i][slot], GL_QUERY_RESULT, &nanoseconds);
        const float milliseconds = static_cast<float>(nanoseconds) / 1.0e6f;

        // 初回は平均の初期値が無いのでそのまま入れる 0から平均すると表示が数秒かけて立ち上がる
        if (smoothed_[i] == 0.0f)
            smoothed_[i] = milliseconds;
        else
            smoothed_[i] += (milliseconds - smoothed_[i]) * kSmoothing;
    }
}

float gl::GpuProfiler::TotalMilliseconds() const {
    float total = 0.0f;
    for (float milliseconds : smoothed_)
        total += milliseconds;
    return total;
}

const char *gl::GpuProfiler::Name(GpuPass pass) {
    return kPassNames[static_cast<std::size_t>(pass)];
}
