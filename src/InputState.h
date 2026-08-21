#pragma once

// 入力の宛先を決めるアプリ全体のモード。カメラの FreeLook / ThirdPerson とは直交する
enum class InputMode {
	Gameplay, // カーソルを掴んで視点操作へ。UI は出さない
	Paused,   // カーソルを解放して UI へ ゲーム側の入力は止める
};

class InputState {
private:
	InputMode mode_ = InputMode::Gameplay;
	// 起動直後もカーソル状態を一度適用させたいので true で初期化
	bool modeChanged_ = true;

public:
	InputMode Mode() const;
	bool IsGameplay() const;
	bool IsPaused() const;

	void SetMode(InputMode mode);
	void TogglePause();

	// 切り替わった直後の1回だけ true を返す
	// カーソル捕捉の適用漏れ・二重適用を防ぐ
	bool ConsumeModeChanged();
};
