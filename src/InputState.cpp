#include "InputState.h"

InputMode InputState::Mode() const { return mode_; }

bool InputState::IsGameplay() const { return mode_ == InputMode::Gameplay; }

bool InputState::IsPaused() const { return mode_ == InputMode::Paused; }

void InputState::SetMode(InputMode mode) {
    if (mode_ == mode) return;
    mode_ = mode;
    modeChanged_ = true;
}

void InputState::TogglePause() {
    SetMode(IsGameplay() ? InputMode::Paused : InputMode::Gameplay);
}

bool InputState::ConsumeModeChanged() {
    const bool changed = modeChanged_;
    modeChanged_ = false;
    return changed;
}
