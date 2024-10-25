// Copyright 2024 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/picker/picker_caps_lock_bubble_controller.h"

#include "ash/picker/views/picker_caps_lock_state_view.h"
#include "ash/public/cpp/shell_window_ids.h"
#include "ash/shell.h"
#include "ash/wm/window_util.h"
#include "base/location.h"
#include "base/time/time.h"
#include "ui/aura/window.h"
#include "ui/base/ime/ash/ime_bridge.h"
#include "ui/base/ime/ash/ime_keyboard.h"
#include "ui/base/ime/text_input_client.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/native_widget_types.h"

namespace ash {
namespace {

constexpr base::TimeDelta kBubbleViewDisplayTime = base::Seconds(3);

gfx::NativeView GetParentView() {
  aura::Window* active_window = window_util::GetActiveWindow();
  return Shell::GetContainer(active_window
                                 ? active_window->GetRootWindow()
                                 : Shell::GetRootWindowForNewWindows(),
                             kShellWindowId_SettingBubbleContainer);
}

ui::TextInputClient* GetFocusedTextInputClient() {
  const ui::InputMethod* input_method =
      IMEBridge::Get()->GetInputContextHandler()->GetInputMethod();
  if (!input_method || !input_method->GetTextInputClient()) {
    return nullptr;
  }
  return input_method->GetTextInputClient();
}

// Gets the current caret bounds in universal screen coordinates in DIP. Returns
// an empty rect if there is no active caret or the caret bounds can't be
// determined (e.g. no focused input field).
gfx::Rect GetCaretBounds() {
  if (ui::TextInputClient* client = GetFocusedTextInputClient()) {
    return client->GetCaretBounds();
  }
  return gfx::Rect();
}

}  // namespace

PickerCapsLockBubbleController::PickerCapsLockBubbleController(
    input_method::ImeKeyboard* keyboard) {
  ime_keyboard_observation_.Observe(keyboard);
}

PickerCapsLockBubbleController::~PickerCapsLockBubbleController() {
  // Close the bubble if it's open to avoid a dangling pointer.
  CloseBubble();
}

void PickerCapsLockBubbleController::CloseBubble() {
  bubble_close_timer_.Stop();
  if (bubble_view_ != nullptr) {
    bubble_view_->Close();
    bubble_view_ = nullptr;
  }
}

void PickerCapsLockBubbleController::OnCapsLockChanged(bool enabled) {
  CloseBubble();
  if (GetFocusedTextInputClient() == nullptr) {
    return;
  }
  bubble_view_ =
      new PickerCapsLockStateView(GetParentView(), enabled, GetCaretBounds());
  bubble_view_->Show();
  bubble_close_timer_.Start(
      FROM_HERE, kBubbleViewDisplayTime,
      base::BindOnce(&PickerCapsLockBubbleController::CloseBubble,
                     weak_ptr_factory_.GetWeakPtr()));
}

void PickerCapsLockBubbleController::OnLayoutChanging(
    const std::string& layout_name) {}

}  // namespace ash
