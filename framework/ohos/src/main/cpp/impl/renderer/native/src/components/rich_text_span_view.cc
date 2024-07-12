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

#include "renderer/components/rich_text_span_view.h"
#include "renderer/dom_node/hr_node_props.h"
#include "renderer/utils/hr_text_convert_utils.h"
#include "renderer/utils/hr_value_utils.h"

namespace hippy {
inline namespace render {
inline namespace native {

RichTextSpanView::RichTextSpanView(std::shared_ptr<NativeRenderContext> &ctx) : BaseView(ctx) {
}

RichTextSpanView::~RichTextSpanView() {}

SpanNode &RichTextSpanView::GetLocalRootArkUINode() {
  return spanNode_;
}

bool RichTextSpanView::SetProp(const std::string &propKey, const HippyValue &propValue) {
  if (propKey == "text") {
    std::string value = HRValueUtils::GetString(propValue);
    if (!text_.has_value() || value != text_) {
      GetLocalRootArkUINode().SetSpanContent(value);
      text_ = value;
    }
    return true;
  } else if (propKey == HRNodeProps::COLOR) {
    uint32_t value = HRValueUtils::GetUint32(propValue);
    if (!color_.has_value() || value != color_) {
      GetLocalRootArkUINode().SetFontColor(value);
      color_ = value;
    }
    return true;
  } else if (propKey == HRNodeProps::FONT_FAMILY) {
    std::string value = HRValueUtils::GetString(propValue);
    if (!fontFamily_.has_value() || value != fontFamily_) {
      GetLocalRootArkUINode().SetFontFamily(value);
      fontFamily_ = value;
    }
    return true;
  } else if (propKey == HRNodeProps::FONT_SIZE) {
    float value = HRValueUtils::GetFloat(propValue);
    if (!fontSize_.has_value() || value != fontSize_) {
      GetLocalRootArkUINode().SetFontSize(value);
      fontSize_ = value;
    }
    return true;
  } else if (propKey == HRNodeProps::FONT_STYLE) {
    std::string value = HRValueUtils::GetString(propValue);
    int32_t style = HRTextConvertUtils::FontStyleToArk(value);
    if (!fontStyle_.has_value() || style != fontStyle_) {
      GetLocalRootArkUINode().SetFontStyle(style);
      fontStyle_ = style;
    }
    return true;
  } else if (propKey == HRNodeProps::FONT_WEIGHT) {
    std::string value = HRValueUtils::GetString(propValue);
    ArkUI_FontWeight weight = HRTextConvertUtils::FontWeightToArk(value);
    if (!fontWeight_.has_value() || weight != fontWeight_) {
      GetLocalRootArkUINode().SetFontWeight(weight);
      fontWeight_ = weight;
    }
    return true;
  } else if (propKey == HRNodeProps::LETTER_SPACING) {
    float value = HRValueUtils::GetFloat(propValue);
    if (!letterSpacing_.has_value() || value != letterSpacing_) {
      GetLocalRootArkUINode().SetTextLetterSpacing(value);
      letterSpacing_ = value;
    }
    return true;
  } else if (propKey == HRNodeProps::LINE_HEIGHT) {
    float value = HRValueUtils::GetFloat(propValue);
    if (!lineHeight_.has_value() || value != lineHeight_) {
      GetLocalRootArkUINode().SetTextLineHeight(value);
      lineHeight_ = value;
    }
    return true;
  } else if (propKey == HRNodeProps::TEXT_DECORATION_LINE) {
    std::string value = HRValueUtils::GetString(propValue);
    decorationType_ = HRTextConvertUtils::TextDecorationTypeToArk(value);
    toSetTextDecoration_ = true;
    return true;
  } else if (propKey == HRNodeProps::TEXT_DECORATION_COLOR) {
    decorationColor_ = HRValueUtils::GetUint32(propValue);
    toSetTextDecoration_ = true;
    return true;
  } else if (propKey == HRNodeProps::TEXT_DECORATION_STYLE) {
    std::string value = HRValueUtils::GetString(propValue);
    decorationStyle_ = HRTextConvertUtils::TextDecorationStyleToArk(value);
    toSetTextDecoration_ = true;
    return true;
  } else if (propKey == HRNodeProps::TEXT_SHADOW_COLOR) {
    textShadowColor_ = HRValueUtils::GetUint32(propValue);
    toSetTextShadow = true;
    return true;
  } else if (propKey == HRNodeProps::TEXT_SHADOW_OFFSET) {
    HippyValueObjectType m;
    if (propValue.ToObject(m)) {
      textShadowOffsetX_ = HRValueUtils::GetFloat(m["width"]);
      textShadowOffsetY_ = HRValueUtils::GetFloat(m["height"]);
    }
    toSetTextShadow = true;
    return true;
  } else if (propKey == HRNodeProps::TEXT_SHADOW_RADIUS) {
    textShadowRadius_ = HRValueUtils::GetFloat(propValue);
    toSetTextShadow = true;
    return true;
  } else if (propKey == HRNodeProps::BACKGROUND_COLOR) {
    uint32_t value = HRValueUtils::GetUint32(propValue);
    GetLocalRootArkUINode().SetSpanTextBackgroundStyle(value);
    return true;
  } else {
    bool handled = SetEventProp(propKey, propValue);
    return handled;
  }
  
  // Not to set some attributes for text span.
  // For example: NODE_BACKGROUND_COLOR will return ARKUI_ERROR_CODE_ATTRIBUTE_OR_EVENT_NOT_SUPPORTED (106102)
}

void RichTextSpanView::OnSetPropsEnd() {
  if (toSetTextDecoration_) {
    toSetTextDecoration_ = false;
    GetLocalRootArkUINode().SetTextDecoration(decorationType_, decorationColor_, decorationStyle_);
  }
  if (toSetTextShadow) {
    toSetTextShadow = false;
    GetLocalRootArkUINode().SetTextShadow(textShadowRadius_, ARKUI_SHADOW_TYPE_COLOR, textShadowColor_, textShadowOffsetX_, textShadowOffsetY_);
  }
  BaseView::OnSetPropsEnd();
}

void RichTextSpanView::UpdateRenderViewFrame(const HRRect &frame, const HRPadding &padding) {
  // Nothing to set for text span.
  // NODE_POSITION / NODE_WIDTH will return ARKUI_ERROR_CODE_ATTRIBUTE_OR_EVENT_NOT_SUPPORTED (106102)
}

} // namespace native
} // namespace render
} // namespace hippy
