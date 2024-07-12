/*
 *
 * Tencent is pleased to support the open source community by making
 * Hippy available.
 *
 * Copyright (C) 2019 THL A29 Limited, a Tencent company.
 * All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#pragma once

#include "renderer/arkui/arkui_node.h"
#include "renderer/arkui/arkui_node_registry.h"

namespace hippy {
inline namespace render {
inline namespace native {

class SwiperNodeDelegate {
  public:
  virtual ~SwiperNodeDelegate() = default;

  virtual void OnChange(const int32_t &index) {}
  virtual void OnAnimationStart(const int32_t &currentIndex, const int32_t &targetIndex,
                                const float_t &currentOffset, const float_t &targetOffset,
                                const float_t &swipeVelocity) {}
  virtual void OnAnimationEnd(const int32_t &currentIndex, const float_t &currentOffset) {}
  virtual void OnGestureSwipe(const int32_t &swiperPageIndex,
                              const float_t &elementOffsetFromStart) {}
  virtual void OnNodeTouchEvent(const ArkUI_UIInputEvent *inputEvent) {}
};


class SwiperNode : public ArkUINode {
protected:
  SwiperNodeDelegate *swiperNodeDelegate_ = nullptr;

public:
  SwiperNode();
  ~SwiperNode();

  void OnNodeEvent(ArkUI_NodeEvent *event) override;
  void SetNodeDelegate(SwiperNodeDelegate *swiperNodeDelegate);

  void SetShowIndicator(bool show);
  void SetSwiperIndex(int32_t index);
  void SetSwiperSwipeToIndex(int32_t index, int32_t animation);
  void SetSwiperVertical(int32_t direction);
  void SetSwiperPrevMargin(float fValue);
  void SetSwiperNextMargin(float fValue);
  void SetSwiperLoop(int32_t enable);
  void SetSwiperDisableSwipe(int32_t disable);
};

} // namespace native
} // namespace render
} // namespace hippy
