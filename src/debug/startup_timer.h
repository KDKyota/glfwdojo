#pragma once

#include <glad/glad.h>

#include <chrono>
#include <iostream>
#include <string>
#include <utility>

namespace gl {
/**
 * @brief 起動処理の一区間の所要時間をスコープの終わりで出力する
 *
 * waitForGpu が true のときは開始と終了で GL 命令の完了を待ち GPU の処理時間も区間に含める
 * 注意: waitForGpu が true のままでは GL コンテキストを持たないスレッドで使えない
 */
class ScopedStartupTimer {
  public:

    explicit ScopedStartupTimer(std::string label, bool waitForGpu = true)
        : label_(std::move(label)), waitForGpu_(waitForGpu) {
        if (waitForGpu_) glFinish();
        start_ = std::chrono::steady_clock::now();
    }
    ~ScopedStartupTimer() {
        if (waitForGpu_) glFinish();
        const std::chrono::duration<double, std::milli> elapsed = std::chrono::steady_clock::now() - start_;
        std::cout << "[Startup]" << label_ << ": " << elapsed.count() << " ms" << std::endl;
    }

    // コピー禁止
    ScopedStartupTimer(const ScopedStartupTimer &) = delete;
    ScopedStartupTimer &operator=(const ScopedStartupTimer &) = delete;

  private:
    std::string label_; // 記録するタスクの区間名
    bool waitForGpu_;   // GPU の処理時間を含めるかどうか
    std::chrono::steady_clock::time_point start_;
};

/**
 * @brief メンバ初期化子の中で build の所要時間を計測し その戻り値をそのまま返す
 *
 * @tparam F 引数なしで呼べて 構築したオブジェクトを返す関数オブジェクト
 * @param label 出力に使う区間名
 * @param build 計測対象の構築処理（ラムダ式として利用する想定）
 */
template <typename F> 
decltype(auto) MeasureStartupStage(const char *label, F &&build) {
    ScopedStartupTimer timer(label);
    return std::forward<F>(build)();
}

} // namespace gl