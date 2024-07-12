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

#include "renderer/components/list_view.h"
#include "renderer/components/list_item_view.h"
#include "renderer/utils/hr_event_utils.h"
#include "renderer/utils/hr_value_utils.h"

// #define LIST_VIEW_DEBUG_LOG

namespace hippy {
inline namespace render {
inline namespace native {

ListView::ListView(std::shared_ptr<NativeRenderContext> &ctx) : BaseView(ctx) {
  stackNode_.AddChild(listNode_);
  listNode_.SetNodeDelegate(this);
  listNode_.SetSizePercent(HRSize(1.f, 1.f));
  listNode_.SetScrollBarDisplayMode(ARKUI_SCROLL_BAR_DISPLAY_MODE_OFF);
  listNode_.SetListCachedCount(4);
  listNode_.SetScrollNestedScroll(ARKUI_SCROLL_NESTED_MODE_SELF_FIRST, ARKUI_SCROLL_NESTED_MODE_SELF_FIRST);
  adapter_ = std::make_shared<ListItemAdapter>(children_);
  listNode_.SetLazyAdapter(adapter_->GetHandle());
}

ListView::~ListView() {
  ctx_->GetNativeRender().lock()->RemoveEndBatchCallback(ctx_->GetRootId(), end_batch_callback_id_);
  if (!children_.empty()) {
    for (const auto &child : children_) {
      listNode_.RemoveChild(child->GetLocalRootArkUINode());
    }
    children_.clear();
  }
  stackNode_.RemoveChild(listNode_);
  adapter_.reset();
}

void ListView::Init() {
  BaseView::Init();
  auto weak_view = weak_from_this();
  end_batch_callback_id_ = ctx_->GetNativeRender().lock()->AddEndBatchCallback(ctx_->GetRootId(), [weak_view]() {
    auto view = weak_view.lock();
    if (view) {
      auto listView = std::static_pointer_cast<ListView>(view);
      listView->HandleOnChildrenUpdated();
      listView->CheckInitOffset();
      
      // TODO(hot): rowShouldSticky
    }
  });
}

StackNode &ListView::GetLocalRootArkUINode() { return stackNode_; }

bool ListView::SetProp(const std::string &propKey, const HippyValue &propValue) {
  if (propKey == "nestedScrollTopPriority") {
    ArkUI_ScrollNestedMode scrollForward = ARKUI_SCROLL_NESTED_MODE_SELF_FIRST;
    ArkUI_ScrollNestedMode scrollBackward = ARKUI_SCROLL_NESTED_MODE_SELF_FIRST;
    auto value = HRValueUtils::GetString(propValue);
    if (value == "parent") {
      scrollForward = ARKUI_SCROLL_NESTED_MODE_PARENT_FIRST;
    } else if (value == "self") {
      scrollForward = ARKUI_SCROLL_NESTED_MODE_SELF_FIRST;
    }
    listNode_.SetScrollNestedScroll(scrollForward, scrollBackward);
    return true;
  } else if (propKey == "horizontal") {
    auto value = HRValueUtils::GetBool(propValue, false);
    isVertical_ = !value;
    listNode_.SetListDirection(isVertical_);
    return true;
  } else if (propKey == "scrollEnabled") {
    auto value = HRValueUtils::GetBool(propValue, true);
    listNode_.SetEnableScrollInteraction(value);
    return true;
  } else if (propKey == "initialContentOffset") {
    initialOffset_ = HRValueUtils::GetFloat(propValue);
    return true;
  } else if (propKey == "itemViewCacheSize") {
    auto value = HRValueUtils::GetInt32(propValue);
    listNode_.SetListCachedCount(value);
    return true;
  } else if (propKey == "scrollEventThrottle") {
    scrollEventThrottle_ = HRValueUtils::GetInt32(propValue, 30);
    return true;
  } else if (propKey == "preloadItemNumber") {
    preloadItemNumber_ = HRValueUtils::GetInt32(propValue);
    return true;
  } else if (propKey == "exposureEventEnabled") {
    exposureEventEnabled_ = HRValueUtils::GetBool(propValue, false);
    return true;
  } else if (propKey == "rowShouldSticky") {
    rowShouldSticky_ = HRValueUtils::GetBool(propValue, false);
    return true;
  } else if (propKey == "bounces") {
    bool b = HRValueUtils::GetBool(propValue, true);
    listNode_.SetScrollEdgeEffect(b);
    return true;
  } else if (propKey == "scrollbegindrag") {
    scrollBeginDragEventEnable_ = HRValueUtils::GetBool(propValue, false);
    return true;
  } else if (propKey == "scrollenddrag") {
    scrollEndDragEventEnable_ = HRValueUtils::GetBool(propValue, false);
    return true;
  } else if (propKey == "momentumscrollbegin") {
    momentumScrollBeginEventEnable_ = HRValueUtils::GetBool(propValue, false);
    return true;
  } else if (propKey == "momentumscrollend") {
    momentumScrollEndEventEnable_ = HRValueUtils::GetBool(propValue, false);
    return true;
  } else if (propKey == "scroll") {
    onScrollEventEnable_ = HRValueUtils::GetBool(propValue, false);
    return true;
  }
  return BaseView::SetProp(propKey, propValue);
}

void ListView::Call(const std::string &method, const std::vector<HippyValue> params,
                    std::function<void(const HippyValue &result)> callback) {
  FOOTSTONE_DLOG(INFO) << "ListView call: method " << method << ", params: " << params.size();
  if (method == "scrollToIndex") {
    auto xIndex = HRValueUtils::GetInt32(params[0]);
    auto yIndex = HRValueUtils::GetInt32(params[1]);
    auto animated = HRValueUtils::GetBool(params[2], false);
    auto index = isVertical_ ? yIndex : xIndex;
    listNode_.ScrollToIndex(hasPullHeader_ ? index + 1 : index, animated, true);
  } else if (method == "scrollToContentOffset") {
    auto xOffset = HRValueUtils::GetFloat(params[0]);
    auto yOffset = HRValueUtils::GetFloat(params[1]);
    auto animated = HRValueUtils::GetBool(params[2], false);
    if (isVertical_) {
      yOffset += pullHeaderHeight_;
    } else {
      xOffset += pullHeaderHeight_;
    }
    listNode_.ScrollTo(xOffset, yOffset, animated);
  } else if (method == "scrollToTop") {
    listNode_.ScrollToIndex(hasPullHeader_ ? 1 : 0, true, true);
  } else {
    BaseView::Call(method, params, callback);
  }
}

void ListView::OnChildInserted(std::shared_ptr<BaseView> const &childView, int32_t index) {
  BaseView::OnChildInserted(childView, index);

#ifdef LIST_VIEW_DEBUG_LOG
  FOOTSTONE_DLOG(INFO) << "hippy ListView - on child inserted: " << index;
#endif

  if (index == 0 && childView->GetViewType() == PULL_HEADER_VIEW_TYPE) {
    listNode_.SetListInitialIndex(1);
  }
  
  adapter_->InsertItem(index);
}

void ListView::OnChildRemoved(std::shared_ptr<BaseView> const &childView, int32_t index) {
  BaseView::OnChildRemoved(childView, index);
  adapter_->RemoveItem(index);
}

void ListView::UpdateRenderViewFrame(const HRRect &frame, const HRPadding &padding) {
  BaseView::UpdateRenderViewFrame(frame, padding);
  width_ = frame.width;
  height_ = frame.height;
}

void ListView::ScrollToIndex(int32_t index, bool animated) {
  listNode_.ScrollToIndex(index, animated, true);
}

void ListView::SetScrollNestedMode(ArkUI_ScrollNestedMode scrollForward, ArkUI_ScrollNestedMode scrollBackward) {
  listNode_.SetScrollNestedScroll(scrollForward, scrollBackward);
}

void ListView::OnAppear() {
  
}

void ListView::OnDisappear() {
  
}

void ListView::OnScrollIndex(int32_t firstIndex, int32_t lastIndex, int32_t centerIndex) {
#ifdef LIST_VIEW_DEBUG_LOG
  FOOTSTONE_DLOG(INFO) << "hippy ListView - on scroll index, first: " << firstIndex << ", last: " << lastIndex;
#endif
  CheckSendReachEndEvent(lastIndex);
}

void ListView::OnWillScroll(float offset, ArkUI_ScrollState state) {
  if (offset > 0) {
    if (footerView_) {
      footerView_->Show(true);
    }
  }
}

void ListView::OnScroll(float scrollOffsetX, float scrollOffsetY) {
  CheckSendOnScrollEvent();
  CheckPullOnScroll();
}

void ListView::OnScrollStart() {
#ifdef LIST_VIEW_DEBUG_LOG
  FOOTSTONE_DLOG(INFO) << "hippy ListView on scroll start";
#endif
}

void ListView::OnScrollStop() {
#ifdef LIST_VIEW_DEBUG_LOG
  FOOTSTONE_DLOG(INFO) << "hippy ListView on scroll stop";
#endif
  if (momentumScrollEndEventEnable_) {
    EmitScrollEvent(HREventUtils::EVENT_SCROLLER_MOMENTUM_END);
  }
  if (onScrollEventEnable_) {
    EmitScrollEvent(HREventUtils::EVENT_SCROLLER_ON_SCROLL);
  }
}

void ListView::OnReachStart() {
  FOOTSTONE_DLOG(INFO) << "ListView onReachStart";
}

void ListView::OnReachEnd() {
  FOOTSTONE_DLOG(INFO) << "ListView onReachEnd";
  SendOnReachedEvent();
}

void ListView::OnTouch(int32_t actionType) {
  if (actionType == UI_TOUCH_EVENT_ACTION_DOWN || actionType == UI_TOUCH_EVENT_ACTION_MOVE) {
    CheckBeginDrag();
  } else if (actionType == UI_TOUCH_EVENT_ACTION_UP || actionType == UI_TOUCH_EVENT_ACTION_CANCEL) {
    CheckEndDrag();
  }
}

void ListView::OnItemVisibleAreaChange(int32_t index, bool isVisible, float currentRatio) {
#ifdef LIST_VIEW_DEBUG_LOG
  FOOTSTONE_DLOG(INFO) << "hippy ListView - on item visible area change, index: " << index
    << ", isVisible: " << isVisible << ", currentRatio: " << currentRatio
    << ", lastIndex: " << static_cast<int32_t>(children_.size()) - 1;
#endif
  
  CheckPullOnItemVisibleAreaChange(index, isVisible, currentRatio);
  if (rowShouldSticky_) {
    CheckStickyOnItemVisibleAreaChange(index, isVisible, currentRatio);
  }
  if (exposureEventEnabled_) {
    if (index >= 0 && index < static_cast<int32_t>(children_.size())) {
      auto &view = children_[static_cast<uint32_t>(index)];
      if (view->GetViewType() == LIST_VIEW_ITEM_TYPE) {
        auto itemView = std::static_pointer_cast<ListItemView>(view);
        itemView->CheckExposureView(isVisible ? currentRatio : 0.0);
      }
    }
  }
}

void ListView::HandleOnChildrenUpdated() {
  auto childrenCount = children_.size();
  if (childrenCount > 0) {
    // Index must be recalculated.
    for (uint32_t i = 0; i < childrenCount; i++) {
      auto itemView = std::static_pointer_cast<ListItemView>(children_[i]);
      itemView->GetLocalRootArkUINode().SetNodeDelegate(this);
      itemView->GetLocalRootArkUINode().SetItemIndex((int32_t)i);
    }
    
    if (children_[0]->GetViewType() == PULL_HEADER_VIEW_TYPE) {
      headerView_ = std::static_pointer_cast<PullHeaderView>(children_[0]);
      hasPullHeader_ = true;
      pullHeaderHeight_ = headerView_->GetHeight();
    }
    if (children_[childrenCount - 1]->GetViewType() == PULL_FOOTER_VIEW_TYPE) {
      footerView_ = std::static_pointer_cast<PullFooterView>(children_[childrenCount - 1]);
      footerView_->Show(false);
    }
  }
  
  stickyArray_.clear();
  for (uint32_t i = 0; i < childrenCount; i++) {
    auto &view = children_[i];
    if (view->GetViewType() == LIST_VIEW_ITEM_TYPE) {
      auto itemView = std::static_pointer_cast<ListItemView>(view);
      if (itemView->IsSticky()) {
        stickyArray_.push_back(static_cast<int32_t>(i));
      }
    }
  }
}

void ListView::EmitScrollEvent(const std::string &eventName) {
  if (!HREventUtils::CheckRegisteredEvent(ctx_, tag_, eventName)) {
    return;
  }

  HippyValueObjectType contentInset;
  contentInset["top"] = HippyValue(0);
  contentInset["bottom"] = HippyValue(0);
  contentInset["left"] = HippyValue(0);
  contentInset["right"] = HippyValue(0);

  auto offset = listNode_.GetScrollOffset();
  
  HippyValueObjectType contentOffset;
  contentOffset["x"] = HippyValue(offset.x);
  contentOffset["y"] = HippyValue(offset.y);
  
  float contentWidth = width_;
  float contentHeight = height_;
  if (children_.size() > 0) {
    auto view = std::static_pointer_cast<ListItemView>(children_[0]);
    contentWidth = view->GetWidth();
    contentHeight = view->GetHeight();
  }

  HippyValueObjectType contentSize;
  contentSize["width"] = HippyValue(contentWidth);
  contentSize["height"] = HippyValue(contentHeight);

  HippyValueObjectType layoutMeasurement;
  contentSize["width"] = HippyValue(width_);
  contentSize["height"] = HippyValue(height_);

  HippyValueObjectType params;
  params["contentInset"] = contentInset;
  params["contentOffset"] = contentOffset;
  params["contentSize"] = contentSize;
  params["layoutMeasurement"] = layoutMeasurement;
  
  HREventUtils::SendComponentEvent(ctx_, tag_, eventName, std::make_shared<HippyValue>(params));
}

void ListView::CheckSendOnScrollEvent() {
  if (onScrollEventEnable_) {
    auto currentTime = GetTimeMilliSeconds();
    if (currentTime - lastScrollTime_ >= scrollEventThrottle_) {
      lastScrollTime_ = currentTime;
      EmitScrollEvent(HREventUtils::EVENT_SCROLLER_ON_SCROLL);
    }
  }
}

void ListView::CheckSendReachEndEvent(int32_t lastIndex) {
  bool isThisTimeReachEnd = IsReachEnd(lastIndex);
  if (!isLastTimeReachEnd_ && isThisTimeReachEnd) {
    SendOnReachedEvent();
  }
  isLastTimeReachEnd_ = isThisTimeReachEnd;
}

bool ListView::IsReachEnd(int32_t lastIndex) {
  if (preloadItemNumber_ > 0 && lastIndex >= (static_cast<int32_t>(children_.size()) - preloadItemNumber_)) {
    return true;
  } else {
    return false;
  }
}

void ListView::SendOnReachedEvent() {
  HREventUtils::SendComponentEvent(ctx_, tag_, HREventUtils::EVENT_RECYCLER_END_REACHED, nullptr);
  HREventUtils::SendComponentEvent(ctx_, tag_, HREventUtils::EVENT_RECYCLER_LOAD_MORE, nullptr);
}

void ListView::CheckBeginDrag() {
  if (!isDragging_) {
    isDragging_ = true;
    if (scrollBeginDragEventEnable_) {
      EmitScrollEvent(HREventUtils::EVENT_SCROLLER_BEGIN_DRAG);
    }
  }
}

void ListView::CheckEndDrag() {
  if (isDragging_) {
    isDragging_ = false;
    if (scrollEndDragEventEnable_) {
      EmitScrollEvent(HREventUtils::EVENT_SCROLLER_END_DRAG);
    }
    if (momentumScrollBeginEventEnable_) {
      EmitScrollEvent(HREventUtils::EVENT_SCROLLER_MOMENTUM_BEGIN);
    }

    if (headerView_ && pullAction_ == ScrollAction::PullHeader) {
      if (headerViewFullVisible_) {
        HREventUtils::SendComponentEvent(headerView_->GetCtx(), headerView_->GetTag(),
                                         HREventUtils::EVENT_PULL_HEADER_RELEASED, nullptr);
      } else {
        listNode_.ScrollToIndex(1, true, true);
      }
      pullAction_ = ScrollAction::None;
    } else if (footerView_ && pullAction_ == ScrollAction::PullFooter) {
      if (footerViewFullVisible_) {
        HREventUtils::SendComponentEvent(footerView_->GetCtx(), footerView_->GetTag(),
                                         HREventUtils::EVENT_PULL_FOOTER_RELEASED, nullptr);
      } else {
        auto lastIndex = static_cast<int32_t>(children_.size()) - 1;
        listNode_.ScrollToIndex(lastIndex - 1, true, false);
      }
      pullAction_ = ScrollAction::None;
    }
  }
}

void ListView::CheckPullOnItemVisibleAreaChange(int32_t index, bool isVisible, float currentRatio) {
  auto lastIndex = static_cast<int32_t>(children_.size()) - 1;
  if (headerView_ && index == 0) {
    if (isVisible) {
      if (isDragging_) {
        pullAction_ = ScrollAction::PullHeader;
        if (currentRatio >= 1.0) {
          headerViewFullVisible_ = true;
        } else {
          headerViewFullVisible_ = false;
        }
      } else {
        listNode_.ScrollToIndex(1, true, true);
      }
    } else {
      headerViewFullVisible_ = false;
      if (currentRatio <= 0.0) {
        pullAction_ = ScrollAction::None;
      }
    }
  } else if (footerView_ && index == lastIndex) {
    if (isVisible) {
      if (isDragging_) {
        pullAction_ = ScrollAction::PullFooter;
        if (currentRatio >= 1.0) {
          footerViewFullVisible_ = true;
        } else {
          footerViewFullVisible_ = false;
        }
      } else {
        listNode_.ScrollToIndex(lastIndex - 1, true, false);
      }
    } else {
      footerViewFullVisible_ = false;
      if (currentRatio <= 0.0) {
        pullAction_ = ScrollAction::None;
      }
    }
  } else if (footerView_ && index == lastIndex - 1) {
    if (isVisible && currentRatio >= 1.0) {
      lastItemFullVisibleYOffset_ = listNode_.GetScrollOffset().y;
    }
  }
}

void ListView::CheckPullOnScroll() {
  auto offset = listNode_.GetScrollOffset();
  auto yOff = offset.y;

  if (headerView_ && pullAction_ == ScrollAction::PullHeader) {
    HippyValueObjectType params;
    params[CONTENT_OFFSET] = -yOff + headerView_->GetHeight();
    HREventUtils::SendComponentEvent(headerView_->GetCtx(), headerView_->GetTag(),
                                     HREventUtils::EVENT_PULL_HEADER_PULLING, std::make_shared<HippyValue>(params));
  } else if (footerView_ && pullAction_ == ScrollAction::PullFooter) {
    HippyValueObjectType params;
    params[CONTENT_OFFSET] = yOff - lastItemFullVisibleYOffset_;
    HREventUtils::SendComponentEvent(footerView_->GetCtx(), footerView_->GetTag(),
                                     HREventUtils::EVENT_PULL_FOOTER_PULLING, std::make_shared<HippyValue>(params));
  }
}

void ListView::CheckStickyOnItemVisibleAreaChange(int32_t index, bool isVisible, float currentRatio) {
  auto moveUp = false;
  auto offset = listNode_.GetScrollOffset();
  if (lastMoveY_ != 0 && offset.y > lastMoveY_) {
    moveUp = true;
    lastMoveY_ = offset.y;
  } else {
    lastMoveY_ = offset.y;
  }

  if (!isVisible && moveUp) {
    if (stickyIndex_ != index && std::find(stickyArray_.begin(), stickyArray_.end(), index) != stickyArray_.end()) {
      stickyStack_.push_back(index);
      stickyIndex_ = index;
    }
  }

  if (isVisible && currentRatio >= 1.0) {
    if (stickyStack_.size() > 0 && stickyStack_[stickyStack_.size() - 1] == index) {
      stickyStack_.pop_back();
    }
    if (stickyStack_.size() > 0) {
      stickyIndex_ = stickyStack_[stickyStack_.size() - 1];
    } else {
      stickyIndex_ = INVALID_STICKY_INDEX;
    }
  }
}

void ListView::CheckInitOffset() {
  if (!initOffsetUsed_) {
    initOffsetUsed_ = true;
    
    if (headerView_) {
      listNode_.ScrollToIndex(1, false, true); // TODO(hot): delete when NODE_LIST_INITIAL_INDEX is supported
    }
    
    if (initialOffset_ > 0) {
      float y = 0;
      if (headerView_ != nullptr) {
        y = headerView_->GetHeight();
      }
      y += initialOffset_;
      listNode_.ScrollTo(0, y, true);
    }
  }
}

} // namespace native
} // namespace render
} // namespace hippy
