#pragma once
#include <utility>

/**
 * @brief マウス座標イベントから前フレームとの差分を計算する。
 */
class MouseState {
  private:
    bool FirstMouse = true;
    float LastX = 800.0f / 2.0f;
    float LastY = 600.0f / 2.0f;

  public:
    /**
     * @brief 直前に呼んだ位置との差分を返す。
     *
     * @param xpos,ypos 現在のカーソル座標。
     */
    std::pair<float, float> ComputeOffset(double xpos, double ypos);

    // 次の1回を差分0にする
    // カーソル捕捉の切り替えで座標が飛ぶため、その後に呼ぶ
    void Reset();
};
