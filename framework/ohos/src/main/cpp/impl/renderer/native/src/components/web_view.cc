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

#include "renderer/components/web_view.h"
#include "renderer/utils/hr_value_utils.h"

namespace hippy {
inline namespace render {
inline namespace native {

WebView::WebView(std::shared_ptr<NativeRenderContext> &ctx) : BaseView(ctx) {}

WebView::~WebView() {}

WebNode &WebView::GetLocalRootArkUINode() { return webNode_; }

bool WebView::SetProp(const std::string &propKey, const HippyValue &propValue) {
  if (propKey == "url") {
    return true;
  } else if (propKey == "source") {
    return true;
  }
  return BaseView::SetProp(propKey, propValue);
}

} // namespace native
} // namespace render
} // namespace hippy
