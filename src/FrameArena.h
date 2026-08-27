#pragma once
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <vector>

namespace gl {

/// FrameArena::Allocate が返す、寿命がフレーム内に限られる配列への参照
template <typename T>
struct ArraySpan {
    T *data = nullptr;    // データが始まるアドレス
    std::size_t size = 0; // 何個あるのか

    // std::vector と同様に最初と最後+1のアドレスを返す
    T *begin() const {
        return data;
    }
    T *end() const {
        return data + size;
    }
};

/**
 * @brief 寿命が1フレームのデータ専用のバンプアロケータ。
 *
 * 個別解放は無く、フレーム先頭の Reset で offset を 0 に戻してまとめて無効化する。
 */
class FrameArena {
  public:
    /// 起動時に一度だけ固定バッファを確保する。
    void Init(std::size_t bytes) {
        buffer_.resize(bytes);
        offset_ = 0;
    }

    /// フレーム先頭で呼び、前フレームの確保をまとめて無効化する。
    void Reset() {
        offset_ = 0;
    }

    /// アライメントを合わせた領域を offset から切り出す。
    template <typename T>
    ArraySpan<T> Allocate(std::size_t count) {
        static_assert(std::is_trivially_destructible_v<T>, "FrameArena はリセット時にデストラクタを呼ばない");

        auto base = reinterpret_cast<std::uintptr_t>(buffer_.data() + offset_);
        std::uintptr_t aligned = (base + alignof(T) - 1) & ~(static_cast<std::uintptr_t>(alignof(T)) - 1);
        std::size_t padding = static_cast<std::size_t>(aligned - base);
        std::size_t bytes = padding + sizeof(T) * count;

        assert(offset_ + bytes <= buffer_.size() && "FrameArena overflow");

        T *ptr = reinterpret_cast<T *>(buffer_.data() + offset_ + padding);
        offset_ += bytes;
        return {ptr, count};
    }

  private:
    std::vector<std::byte> buffer_; // 起動時に確保する領域
    std::size_t offset_ = 0;        // buffer_ のうち何バイトを使ったのか
};

} // namespace gl
