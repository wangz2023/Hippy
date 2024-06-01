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

#include <vector>
#include "renderer/components/base_view.h"
#include "renderer/arkui/stack_node.h"
#include "renderer/arkui/list_node.h"
#include "renderer/arkui/list_item_node.h"

namespace hippy {
inline namespace render {
inline namespace native {

class ListView : public BaseView, public ListNodeDelegate {
public:
  ListView(std::shared_ptr<NativeRenderContext> &ctx);
  ~ListView();
  
  void Init();

  StackNode &GetLocalRootArkUINode() override;
  bool SetProp(const std::string &propKey, const HippyValue &propValue) override;
  void Call(const std::string &method, const std::vector<HippyValue> params,
                    std::function<void(const HippyValue &result)> callback) override;
  
  void OnChildInserted(std::shared_ptr<BaseView> const &childView, int32_t index) override;
  void OnChildRemoved(std::shared_ptr<BaseView> const &childView) override;

  void OnAppear() override;
  void OnDisappear() override;
  void OnScroll(float scrollOffsetX, float scrollOffsetY) override;
  void OnScrollStart() override;
  void OnScrollStop() override;

private:
  void HandleOnChildrenUpdated();
  
  void EmitScrollEvent(const std::string &eventName);
  void CheckSendOnScrollEvent();
  void CheckSendReachEndEvent(int32_t lastIndex);
  bool IsReachEnd(int32_t lastIndex);
  void SendOnReachedEvent();
  void CheckBeginDrag();
  void CheckEndDrag();
  void CheckPullOnItemVisibleAreaChange(int32_t index, bool isVisible, float currentRatio);
  void CheckPullOnScroll();
  void CheckStickyOnItemVisibleAreaChange(int32_t index, bool isVisible, float currentRatio);
  
  StackNode stackNode_;
  ListNode listNode_;
  std::vector<ListItemNode> listItemNodes_;
  uint64_t end_batch_callback_id_ = 0;
};

} // namespace native
} // namespace render
} // namespace hippy
