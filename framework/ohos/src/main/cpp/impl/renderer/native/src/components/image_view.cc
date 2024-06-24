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

#include "renderer/components/image_view.h"
#include "renderer/dom_node/hr_node_props.h"
#include "renderer/utils/hr_event_utils.h"
#include "renderer/utils/hr_url_utils.h"
#include "renderer/utils/hr_value_utils.h"

namespace hippy {
inline namespace render {
inline namespace native {

static const std::string BASE64_IMAGE_PREFIX = "data:image";
static const std::string RAW_IMAGE_PREFIX = "hpfile://";
static const std::string ASSETS_PREFIX = "assets://";
static const std::string INTERNET_IMAGE_PREFIX = "http";

ImageView::ImageView(std::shared_ptr<NativeRenderContext> &ctx) : BaseView(ctx) {
  imageNode_.SetNodeDelegate(this);
  GetLocalRootArkUINode().SetDraggable(false);
}

ImageView::~ImageView() {}

ImageNode &ImageView::GetLocalRootArkUINode() {
  return imageNode_;
}

bool ImageView::SetProp(const std::string &propKey, const HippyValue &propValue) {
  if (propKey == "src") {
    auto value = HRValueUtils::GetString(propValue);
    if (value != src_) {
      src_ = value;
      fetchImage(value);
    }
    return true;
  } else if (propKey == "resizeMode") {
    auto value = HRValueUtils::GetInt32(propValue);
    const HRImageResizeMode mode = (HRImageResizeMode)value;
    GetLocalRootArkUINode().SetResizeMode(mode);
    return true;
  } else if (propKey == "defaultSource") {
    auto value = HRValueUtils::GetString(propValue);
    GetLocalRootArkUINode().SetAlt(value);
    return true;
  } else if (propKey == "tintColor") {
    uint32_t value = HRValueUtils::GetUint32(propValue);
    GetLocalRootArkUINode().SetTintColor(value);
    return true;
  } else if (propKey == "tintColorBlendMode") {
    auto value = HRValueUtils::GetInt32(propValue);
    GetLocalRootArkUINode().SetTintColorBlendMode(value);
    return true;
  } else if (propKey == "capInsets") {
    HippyValueObjectType m;
    if (propValue.ToObject(m)) {
      auto left = HRValueUtils::GetFloat(m["left"]);
      auto top = HRValueUtils::GetFloat(m["top"]);
      auto right = HRValueUtils::GetFloat(m["right"]);
      auto bottom = HRValueUtils::GetFloat(m["bottom"]);
      GetLocalRootArkUINode().SetResizeable(left, top, right, bottom);
    } else {
      return false;
    }
	} else if (propKey == "blur") {
		auto value = HRValueUtils::GetFloat(propValue);
    GetLocalRootArkUINode().SetBlur(value);
	} else if (propKey == "draggable") {
		auto value = HRValueUtils::GetBool(propValue, false);
    GetLocalRootArkUINode().SetDraggable(value);
	}
	return BaseView::SetProp(propKey, propValue);
}

void ImageView::UpdateRenderViewFrame(const HRRect &frame, const HRPadding &padding) {
  BaseView::UpdateRenderViewFrame(frame, padding);
}

void ImageView::fetchImage(const std::string &imageUrl) {
  if (imageUrl.size() > 0) {
    if (imageUrl.find(BASE64_IMAGE_PREFIX) == 0) {
      GetLocalRootArkUINode().SetSources(imageUrl);
      return;
		} else if (imageUrl.find(RAW_IMAGE_PREFIX) == 0) {
			std::string convertUrl = ConvertToLocalPathIfNeeded(imageUrl);
      GetLocalRootArkUINode().SetSources(convertUrl);
      return;
		} else if (HRUrlUtils::isWebUrl(imageUrl)) {
			GetLocalRootArkUINode().SetSources(imageUrl);
      return;
		}
    
    // TODO(hot):
	}
}

void ImageView::OnClick() {
  if (eventClick_) {
    eventClick_();
  }
}

void ImageView::OnComplete(float width, float height) {
  HREventUtils::SendComponentEvent(ctx_, tag_, HREventUtils::EVENT_IMAGE_ON_LOAD, nullptr);
  HippyValueObjectType paramsObj;
  paramsObj.insert_or_assign("success", 1);
  HippyValueObjectType imageSizeObj;
  imageSizeObj.insert_or_assign("width", width);
  imageSizeObj.insert_or_assign("height", height);
  paramsObj.insert_or_assign("image", imageSizeObj);
  std::shared_ptr<HippyValue> params = std::make_shared<HippyValue>(paramsObj);
  HREventUtils::SendComponentEvent(ctx_, tag_, HREventUtils::EVENT_IMAGE_LOAD_END, params);
}

void ImageView::OnError(int32_t errorCode) {
  FOOTSTONE_DLOG(INFO) << tag_ << "ImageView onErrorCode :" << errorCode;
  HREventUtils::SendComponentEvent(ctx_, tag_, HREventUtils::EVENT_IMAGE_LOAD_ERROR, nullptr);
  HippyValueObjectType paramsObj;
  paramsObj.insert_or_assign("success", 0);
  paramsObj.insert_or_assign("errorCode", errorCode);
  std::shared_ptr<HippyValue> params = std::make_shared<HippyValue>(paramsObj);
  HREventUtils::SendComponentEvent(ctx_, tag_, HREventUtils::EVENT_IMAGE_LOAD_END, params);
}

} // namespace native
} // namespace render
} // namespace hippy
