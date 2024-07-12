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
#include "renderer/components/list_view.h"
#include "renderer/components/waterfall_view.h"
#include "renderer/utils/hr_value_utils.h"

namespace hippy {
inline namespace render {
inline namespace native {

PullHeaderView::PullHeaderView(std::shared_ptr<NativeRenderContext> &ctx) : ListItemView(ctx) {}

PullHeaderView::~PullHeaderView() {}

bool PullHeaderView::SetProp(const std::string &propKey, const HippyValue &propValue) {
//  FOOTSTONE_DLOG(INFO)<<__FUNCTION__<<" propKey = "<<propKey; 
  return ListItemView::SetProp(propKey, propValue);
}

void PullHeaderView::OnSetPropsEnd(){
  return ListItemView::OnSetPropsEnd();  
}

void PullHeaderView::Call(const std::string &method, const std::vector<HippyValue> params,
                    std::function<void(const HippyValue &result)> callback) {
//  FOOTSTONE_DLOG(INFO)<<__FUNCTION__<<" method = "<<method; 
  if (method == "collapsePullHeader") {
    OnHeadRefreshFinish();
  } else if (method == "collapsePullHeaderWithOptions") {
    HippyValueObjectType map;
    bool r = params[0].ToObject(map);
    if (r && map.size() > 0) {
      auto collapseTime = HRValueUtils::GetInt32(map["time"]);
      if (collapseTime > 0) {
        // TODO(hot):
        OnHeadRefreshFinish(collapseTime);
      } else {
        OnHeadRefreshFinish();
      }
    }
  } else if (method == "expandPullHeader") {
    OnHeaderRefresh();
  } else {
    BaseView::Call(method, params, callback);
  }
}

void PullHeaderView::OnHeadRefreshFinish(int32_t delay) {
  auto parentView = parent_.lock();
  if (parentView) {
    if (parentView->GetViewType() == "ListView") {
      auto listView = std::static_pointer_cast<ListView>(parentView);
      listView->ScrollToIndex(1, true);
    } else if (parentView->GetViewType() == "WaterfallView") {
      auto waterView = std::static_pointer_cast<WaterfallView>(parentView);
      // TODO(hot):
      waterView->OnHeadRefreshFinish(delay);
    }
  }
}

void PullHeaderView::OnHeaderRefresh() {
    FOOTSTONE_DLOG(INFO)<<__FUNCTION__; 
  auto parentView = parent_.lock();
  if (parentView) {
    if (parentView->GetViewType() == "ListView") {
      auto listView = std::static_pointer_cast<ListView>(parentView);
      listView->ScrollToIndex(0, true);
    } else if (parentView->GetViewType() == "WaterfallView") {
      auto waterView = std::static_pointer_cast<WaterfallView>(parentView);
      waterView->OnHeadRefresh();
    }
  }
}

} // namespace native
} // namespace render
} // namespace hippy
