#pragma once

#include "GlHandle.h"

#include <array>
#include <cstddef>
#include <cstdint>

namespace gl {

/// GPU での処理時間を計測するパス
enum class GpuPass {
    Shadow,
    Geometry,
    Ssao,
    BlitDepth,
    Lighting,
    Forward,
    Bloom,
    ToScreen,
    Count,
};

/**
 * @brief パスごとの GPU 実行時間を計測する
 *
 * 結果は数フレーム前のクエリから回収する
 * 同じフレームで読むと GPU の完了待ちで CPU が止まり計測対象そのものを遅くしてしまう
 */
class GpuProfiler {
  public:
    static constexpr std::size_t kPassCount = static_cast<std::size_t>(GpuPass::Count);

    /// クエリオブジェクトを生成する GL コンテキストの作成後に呼ぶ
    void Init();

    void BeginFrame();
    void EndFrame();

    /// body の GPU 実行時間を計測する
    template <typename F>
    void Measure(GpuPass pass, F &&body) { // body とは計測したい処理そのもの F && としているので一時オブジェクト（ラムダ式）を代入可能
        begin(pass);
        body();
        end();
    }

    /// 平滑化済みの実行時間をミリ秒で返す
    float Milliseconds(GpuPass pass) const {
        return smoothed_[static_cast<std::size_t>(pass)];
    }

    float TotalMilliseconds() const;

    static const char *Name(GpuPass pass);

  private:
    // 2フレーム前を読めば GPU は確実に処理を終えている（余裕を持たせる）
    static constexpr int kFrameLag = 3;
    // 生の値はフレームごとに大きく揺れるので指数移動平均で均す
    static constexpr float kSmoothing = 0.05f;

    void begin(GpuPass pass);
    void end();
    void collect(int slot);

    std::array<std::array<QueryHandle, kFrameLag>, kPassCount> queries_;
    // そのスロットへ実際に計測を仕込んだか 起動直後は結果が存在しない
    std::array<std::array<bool, kFrameLag>, kPassCount> issued_{};
    std::array<float, kPassCount> smoothed_{};

    std::uint64_t frame_ = 0;
    int writeSlot_ = 0; // 今書く場所と読む場所を意図的にずらすインデックス
    // glBeginQuery は同じターゲットで入れ子にできない
    bool measuring_ = false;
};

} // namespace gl
