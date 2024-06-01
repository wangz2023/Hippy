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

#include "renderer/components/pull_header_view.h"
#include "renderer/utils/hr_value_utils.h"

namespace hippy {
inline namespace render {
inline namespace native {

PullHeaderView::PullHeaderView(std::shared_ptr<NativeRenderContext> &ctx) : BaseView(ctx) {}

PullHeaderView::~PullHeaderView() {}

StackNode &PullHeaderView::GetLocalRootArkUINode() { return stackNode_; }

bool PullHeaderView::SetProp(const std::string &propKey, const HippyValue &propValue) {
  return BaseView::SetProp(propKey, propValue);
}

void PullHeaderView::Call(const std::string &method, const std::vector<HippyValue> params,
                    std::function<void(const HippyValue &result)> callback) {
  if (method == "collapsePullHeader") {

  } else if (method == "collapsePullHeaderWithOptions") {

  } else if (method == "expandPullHeader") {

  }
}

void PullHeaderView::OnChildInserted(std::shared_ptr<BaseView> const &childView, int32_t index) {
  BaseView::OnChildInserted(childView, index);
  stackNode_.InsertChild(childView->GetLocalRootArkUINode(), index);
}

void PullHeaderView::OnChildRemoved(std::shared_ptr<BaseView> const &childView) {
  BaseView::OnChildRemoved(childView);
  stackNode_.RemoveChild(childView->GetLocalRootArkUINode());
}

void PullHeaderView::UpdateRenderViewFrame(const HRRect &frame, const HRPadding &padding) {
  stackNode_.SetPosition(HRPosition(0, 0));
  stackNode_.SetSize(HRSize(frame.width, frame.height));
}

void PullHeaderView::OnHeadRefreshFinish() {
  
}

void PullHeaderView::OnHeaderRefresh() {
  
}

} // namespace native
} // namespace render
} // namespace hippy
