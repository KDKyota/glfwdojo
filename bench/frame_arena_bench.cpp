// FrameArena の効果を測るためのベンチマーク。OpenGL には依存しないので単体で実行できる
#include "FrameArena.h"

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <vector>

namespace {

// gl::TransparentDraw と同じレイアウト。GL やウィンドウ生成を持ち込まずに測るため複製している
struct Draw {
    float distance;
    unsigned int index;
};

constexpr int kIterations = 200000;
constexpr int kRepeats = 5;

// 最適化で処理ごと消えないよう、結果を必ず読む
unsigned long long sink = 0;

float pseudoDistance(int frame, unsigned int i) {
    return static_cast<float>((frame * 2654435761u + i * 40503u) & 0xffff);
}

void sortAndConsume(Draw *data, std::size_t count) {
    std::sort(data, data + count, [](const Draw &a, const Draw &b) { return a.distance > b.distance; });
    sink += data[0].index;
}

/// 毎フレーム std::vector をローカルに作る、アリーナ導入前の書き方
double benchLocalVector(std::size_t count) {
    auto start = std::chrono::steady_clock::now();
    for (int frame = 0; frame < kIterations; ++frame) {
        std::vector<Draw> sorted;
        for (unsigned int i = 0; i < count; ++i)
            sorted.push_back({pseudoDistance(frame, i), i});
        sortAndConsume(sorted.data(), sorted.size());
    }
    auto end = std::chrono::steady_clock::now();
    return std::chrono::duration<double, std::nano>(end - start).count() / kIterations;
}

/// reserve だけ足した版。確保回数は減るが解放は毎フレーム残る
double benchReservedVector(std::size_t count) {
    auto start = std::chrono::steady_clock::now();
    for (int frame = 0; frame < kIterations; ++frame) {
        std::vector<Draw> sorted;
        sorted.reserve(count);
        for (unsigned int i = 0; i < count; ++i)
            sorted.push_back({pseudoDistance(frame, i), i});
        sortAndConsume(sorted.data(), sorted.size());
    }
    auto end = std::chrono::steady_clock::now();
    return std::chrono::duration<double, std::nano>(end - start).count() / kIterations;
}

/// メンバに持って clear() で使い回す、transparent_positions_ と同じ書き方
double benchReusedVector(std::size_t count) {
    std::vector<Draw> sorted;
    auto start = std::chrono::steady_clock::now();
    for (int frame = 0; frame < kIterations; ++frame) {
        sorted.clear();
        for (unsigned int i = 0; i < count; ++i)
            sorted.push_back({pseudoDistance(frame, i), i});
        sortAndConsume(sorted.data(), sorted.size());
    }
    auto end = std::chrono::steady_clock::now();
    return std::chrono::duration<double, std::nano>(end - start).count() / kIterations;
}

/// FrameArena から取る、アリーナ導入後の書き方
double benchArena(std::size_t count) {
    gl::FrameArena arena;
    arena.Init(sizeof(Draw) * count * 4);
    auto start = std::chrono::steady_clock::now();
    for (int frame = 0; frame < kIterations; ++frame) {
        arena.Reset();
        auto sorted = arena.Allocate<Draw>(count);
        for (unsigned int i = 0; i < count; ++i)
            sorted.data[i] = {pseudoDistance(frame, i), i};
        sortAndConsume(sorted.data, sorted.size);
    }
    auto end = std::chrono::steady_clock::now();
    return std::chrono::duration<double, std::nano>(end - start).count() / kIterations;
}

// 実行ごとの揺れがあるので最小値で比べる
double best(double (*bench)(std::size_t), std::size_t count) {
    double result = bench(count);
    for (int i = 1; i < kRepeats; ++i)
        result = std::min(result, bench(count));
    return result;
}

void runFor(std::size_t count) {
    const double local = best(benchLocalVector, count);
    const double reserved = best(benchReservedVector, count);
    const double reused = best(benchReusedVector, count);
    const double arena = best(benchArena, count);

    std::printf("\n--- %zu instances (1フレームあたり, %d 回の平均 x %d 試行の最小) ---\n", count, kIterations,
                kRepeats);
    std::printf("  local vector    : %8.1f ns  (x%.2f)\n", local, local / arena);
    std::printf("  reserved vector : %8.1f ns  (x%.2f)\n", reserved, reserved / arena);
    std::printf("  reused vector   : %8.1f ns  (x%.2f)\n", reused, reused / arena);
    std::printf("  frame arena     : %8.1f ns  (x1.00)\n", arena);
    std::printf("  確保/解放ぶんの差 (local - arena) : %.1f ns/frame\n", local - arena);
}

} // namespace

int main() {
    std::printf("FrameArena benchmark (sort まで含めた1フレームぶんの処理)\n");
    // 6 は現在の windows_pos_ の枚数。以降は #35 のストレスモードを見据えた規模
    for (std::size_t count : {std::size_t{6}, std::size_t{64}, std::size_t{1024}})
        runFor(count);

    std::printf("\nchecksum: %llu\n", sink);
    return 0;
}
