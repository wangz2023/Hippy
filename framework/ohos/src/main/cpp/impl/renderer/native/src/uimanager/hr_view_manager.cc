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

#include <arkui/native_node_napi.h>
#include "renderer/uimanager/hr_view_manager.h"
#include "oh_napi/ark_ts.h"
#include "oh_napi/oh_napi_object.h"
#include "oh_napi/oh_napi_object_builder.h"
#include "renderer/api/hippy_view_provider.h"
#include "renderer/components/custom_ts_view.h"
#include "renderer/components/custom_view.h"
#include "renderer/components/hippy_render_view_creator.h"
#include "renderer/native_render_context.h"
#include "footstone/logging.h"

namespace hippy {
inline namespace render {
inline namespace native {

HRViewManager::HRViewManager(uint32_t instance_id, uint32_t root_id, std::shared_ptr<NativeRender> &native_render,
    napi_env ts_env, napi_ref ts_render_provider_ref,
    std::set<std::string> &custom_views, std::map<std::string, std::string> &mapping_views)
  : serializer_(std::make_shared<footstone::value::Serializer>()) {
  root_id_ = root_id;
  ctx_ = std::make_shared<NativeRenderContext>(instance_id, root_id, native_render);
  root_view_ = std::make_shared<RootView>(ctx_);
  root_view_->SetTag(root_id);
  std::string root_view_type = "RootView";
  root_view_->SetViewType(root_view_type);
  view_registry_[root_id] = root_view_;
  
  mapping_render_views_ = mapping_views;
  custom_ts_render_views_ = custom_views;
  ts_env_ = ts_env;
  ts_render_provider_ref_ = ts_render_provider_ref;
}

void HRViewManager::BindNativeRoot(ArkUI_NodeContentHandle contentHandle, uint32_t node_id) {
  bool isRoot = (node_id == 0);
  uint32_t current_id = isRoot ? root_id_ : node_id;
  ArkUI_NodeContentHandle savedHandle = nullptr;
  auto it = nodeContentMap_.find(current_id);
  if (it != nodeContentMap_.end()) {
    savedHandle = it->second;
  }
  if (contentHandle == savedHandle) {
    return;
  }
  
  auto viewIt = view_registry_.find(current_id);
  if (viewIt == view_registry_.end()) {
    return;
  }
  auto view = viewIt->second;
  
  nodeContentMap_[current_id] = contentHandle;
  OH_ArkUI_NodeContent_RegisterCallback(contentHandle, nullptr);
  OH_ArkUI_NodeContent_AddNode(contentHandle, view->GetLocalRootArkUINode().GetArkUINodeHandle());
}

void HRViewManager::UnbindNativeRoot(uint32_t node_id) {
  bool isRoot = (node_id == 0);
  uint32_t current_id = isRoot ? root_id_ : node_id;
  auto it = nodeContentMap_.find(current_id);
  if (it == nodeContentMap_.end()) {
    return;
  }
  ArkUI_NodeContentHandle savedHandle = it->second;
  auto viewIt = view_registry_.find(current_id);
  if (viewIt == view_registry_.end()) {
    return;
  }
  auto view = viewIt->second;
  OH_ArkUI_NodeContent_RemoveNode(savedHandle, view->GetLocalRootArkUINode().GetArkUINodeHandle());
  nodeContentMap_.erase(current_id);
}

void HRViewManager::AddMutations(std::shared_ptr<HRMutation> &m) {
  mutations_.push_back(m);
}

void HRViewManager::ApplyMutations() {
  for (auto it = mutations_.begin(); it != mutations_.end(); it++) {
    ApplyMutation(*it);
  }
  mutations_.clear();
}

void HRViewManager::ApplyMutation(std::shared_ptr<HRMutation> &m) {
  if (m->type_ == HRMutationType::CREATE) {
    auto tm = std::static_pointer_cast<HRCreateMutation>(m);
    auto view = CreateRenderView(tm->tag_, tm->view_name_, tm->is_parent_text_);
    if (view) {
      InsertSubRenderView(tm->parent_tag_, view, tm->index_);
      UpdateProps(view, tm->props_);
    }
  } else if (m->type_ == HRMutationType::UPDATE) {
    auto tm = std::static_pointer_cast<HRUpdateMutation>(m);
    UpdateProps(tm->tag_, tm->props_, tm->delete_props_);
  } else if (m->type_ == HRMutationType::MOVE) {
    auto tm = std::static_pointer_cast<HRMoveMutation>(m);
    MoveRenderView(tm->node_infos_, tm->parent_tag_);
  } else if (m->type_ == HRMutationType::MOVE2) {
    auto tm = std::static_pointer_cast<HRMove2Mutation>(m);
    Move2RenderView(tm->tags_, tm->to_parent_tag_, tm->from_parent_tag_, tm->index_);
  } else if (m->type_ == HRMutationType::DELETE) {
    auto tm = std::static_pointer_cast<HRDeleteMutation>(m);
    RemoveRenderView(tm->tag_);
  } else if (m->type_ == HRMutationType::UPDATE_LAYOUT) {
    auto tm = std::static_pointer_cast<HRUpdateLayoutMutation>(m);
    SetRenderViewFrame(
      tm->tag_,
      HRRect(tm->left_, tm->top_, tm->width_, tm->height_),
      HRPadding(tm->padding_left_, tm->padding_top_, tm->padding_right_, tm->padding_bottom_)
    );
  } else if (m->type_ == HRMutationType::UPDATE_EVENT_LISTENER) {
    auto tm = std::static_pointer_cast<HRUpdateEventListenerMutation>(m);
    UpdateProps(tm->tag_, tm->props_);
    UpdateEventListener(tm->tag_, tm->props_);
  }
}

std::shared_ptr<BaseView> HRViewManager::CreateRenderView(uint32_t tag, std::string &view_name, bool is_parent_text) {
  // custom ts view
  if (IsCustomTsRenderView(view_name)) {
    return CreateCustomTsRenderView(tag, view_name, is_parent_text);
  }
  
  // custom cpp view
  auto custom_view = CreateCustomRenderView(tag, view_name, is_parent_text);
  if (custom_view) {
    return custom_view;
  }
  
  // build-in view
  auto it = mapping_render_views_.find(view_name);
  auto real_view_name = it != mapping_render_views_.end() ? it->second : view_name;
  auto view = HippyCreateRenderView(real_view_name, is_parent_text, ctx_);
  if (view) {
    view->SetTag(tag);
    view->SetViewType(real_view_name);
    view_registry_[tag] = view;
    return view;
  } else {
    FOOTSTONE_DLOG(INFO) << "CreateRenderView failed, " << view_name;
  }
  return nullptr;
}

void HRViewManager::RemoveRenderView(uint32_t tag) {
  auto it = view_registry_.find(tag);
  std::shared_ptr<BaseView> renderView = it != view_registry_.end() ? it->second : nullptr;
  if (renderView) {
    renderView->RemoveFromParentView();
    RemoveFromRegistry(renderView);
  }
}

void HRViewManager::RemoveFromRegistry(std::shared_ptr<BaseView> &renderView) {
  auto &children = renderView->GetChildren();
  for (uint32_t i = 0; i < children.size(); i++) {
    RemoveFromRegistry(children[i]);
  }
  view_registry_.erase(renderView->GetTag());
}

void HRViewManager::InsertSubRenderView(uint32_t parentTag, std::shared_ptr<BaseView> &childView, int32_t index) {
  auto it = view_registry_.find(parentTag);
  std::shared_ptr<BaseView> parentView = it != view_registry_.end() ? it->second : nullptr;
  if (parentView && childView) {
    parentView->AddSubRenderView(childView, index);
  } else {
    FOOTSTONE_DLOG(INFO) << "InsertSubRenderView parentTag:" << parentTag << ", child:" << childView->GetTag();
  }
}

static bool SortMoveNodeInfo(const HRMoveNodeInfo &lhs, const HRMoveNodeInfo &rhs) {
  return lhs.index_ < rhs.index_;
}

void HRViewManager::MoveRenderView(std::vector<HRMoveNodeInfo> nodeInfos, uint32_t parentTag) {
  auto it = view_registry_.find(parentTag);
  std::shared_ptr<BaseView> parentView = it != view_registry_.end() ? it->second : nullptr;
  if (!parentView) {
    FOOTSTONE_LOG(WARNING) << "MoveRenderView fail";
    return;
  }
  
  std::sort(nodeInfos.begin(), nodeInfos.end(), SortMoveNodeInfo);
  for (uint32_t i = 0; i < nodeInfos.size(); i++) {
    auto &info = nodeInfos[i];
    auto childIt = view_registry_.find(info.tag_);
    if (childIt != view_registry_.end()) {
      auto &child = childIt->second;
      child->RemoveFromParentView();
      parentView->AddSubRenderView(child, info.index_);
    }
  }
}

void HRViewManager::Move2RenderView(std::vector<uint32_t> tags, uint32_t newParentTag, uint32_t oldParentTag, int index) {
  auto it1 = view_registry_.find(oldParentTag);
  std::shared_ptr<BaseView> oldParent = it1 != view_registry_.end() ? it1->second : nullptr;
  auto it2 = view_registry_.find(newParentTag);
  std::shared_ptr<BaseView> newParent = it2 != view_registry_.end() ? it2->second : nullptr;
  if (!oldParent || !newParent) {
    FOOTSTONE_LOG(WARNING) << "Move2RenderView fail, oldParent=" << oldParentTag << ", newParent=" << newParentTag;
    return;
  }
  
  for (uint32_t i = 0; i < tags.size(); i++) {
    auto childIt = view_registry_.find(tags[i]);
    if (childIt != view_registry_.end()) {
      auto &child = childIt->second;
      child->RemoveFromParentView();
      newParent->AddSubRenderView(child, (int)i + index);
    }
  }
}

void HRViewManager::UpdateProps(std::shared_ptr<BaseView> &view, const HippyValueObjectType &props, const std::vector<std::string> &deleteProps) {
  if (view) {
    // custom ts view
    if (IsCustomTsRenderView(view->GetViewType())) {
      UpdateCustomTsProps(view, props, deleteProps);
      return;
    }
    
    // build-in view
    if (props.size() > 0) {
      for (auto it = props.begin(); it != props.end(); it++) {
        // value maybe empty string / false / 0
        auto &key = it->first;
        if (key.length() > 0) {
          view->SetProp(key, it->second);
        }
      }
    }
    if (deleteProps.size() > 0) {
      for (auto it = deleteProps.begin(); it != deleteProps.end(); it++) {
        auto &key = *it;
        if (key.length() > 0) {
          view->SetProp(key, HippyValue::Null());
        }
      }
    }
    view->OnSetPropsEnd();
  }
}

void HRViewManager::UpdateProps(uint32_t tag, const HippyValueObjectType &props, const std::vector<std::string> &deleteProps) {
  auto it = view_registry_.find(tag);
  std::shared_ptr<BaseView> renderView = it != view_registry_.end() ? it->second : nullptr;
  UpdateProps(renderView, props, deleteProps);
}

void HRViewManager::UpdateEventListener(uint32_t tag, HippyValueObjectType &props) {
  auto it = view_registry_.find(tag);
  std::shared_ptr<BaseView> renderView = it != view_registry_.end() ? it->second : nullptr;
  if (renderView) {
    // custom ts view
    if (IsCustomTsRenderView(renderView->GetViewType())) {
      UpdateCustomTsEventListener(tag, props);
      return;
    }
    
    // build-in view
    renderView->UpdateEventListener(props);
  }
}

bool HRViewManager::CheckRegisteredEvent(uint32_t tag, std::string &eventName) {
  auto it = view_registry_.find(tag);
  std::shared_ptr<BaseView> renderView = it != view_registry_.end() ? it->second : nullptr;
  if (renderView) {
    return renderView->CheckRegisteredEvent(eventName);
  }
  return false;
}

void HRViewManager::SetRenderViewFrame(uint32_t tag, const HRRect &frame, const HRPadding &padding) {
  auto it = view_registry_.find(tag);
  std::shared_ptr<BaseView> renderView = it != view_registry_.end() ? it->second : nullptr;
  if (renderView) {
    // custom ts view
    if (IsCustomTsRenderView(renderView->GetViewType())) {
      renderView->SetRenderViewFrame(frame, padding);
      SetCustomTsRenderViewFrame(tag, frame, padding);
      return;
    }
    
    // build-in view
    renderView->SetRenderViewFrame(frame, padding);
  }
}

void HRViewManager::CallViewMethod(uint32_t tag, const std::string &method, const std::vector<HippyValue> params,
                    std::function<void(const HippyValue &result)> callback) {
  auto it = view_registry_.find(tag);
  std::shared_ptr<BaseView> renderView = it != view_registry_.end() ? it->second : nullptr;
  if (renderView) {
    renderView->Call(method, params, callback);
  }
}

LayoutSize HRViewManager::CallCustomMeasure(uint32_t tag,
    float width, LayoutMeasureMode width_measure_mode,
    float height, LayoutMeasureMode height_measure_mode) {
  auto it = view_registry_.find(tag);
  std::shared_ptr<BaseView> renderView = it != view_registry_.end() ? it->second : nullptr;
  if (renderView) {
    auto customView = std::static_pointer_cast<CustomView>(renderView);
    return customView->CustomMeasure(width, width_measure_mode, height, height_measure_mode);
  }
  return {0, 0};
}

uint64_t HRViewManager::AddEndBatchCallback(const EndBatchCallback &cb) {
  ++end_batch_callback_id_count_;
  end_batch_callback_map_[end_batch_callback_id_count_] = std::move(cb);
  return end_batch_callback_id_count_;
}

void HRViewManager::RemoveEndBatchCallback(uint64_t cbId) {
  end_batch_callback_map_.erase(cbId);
}

void HRViewManager::NotifyEndBatchCallbacks() {
  for (const auto &callback : end_batch_callback_map_) {
    auto &cb = callback.second;
    cb();
  }
}

bool HRViewManager::GetViewParent(uint32_t node_id, uint32_t &parent_id, std::string &parent_view_type) {
  auto viewIt = view_registry_.find(node_id);
  if (viewIt == view_registry_.end()) {
    return false;
  }
  auto view = viewIt->second;
  auto parentView = view->GetParent().lock();
  if (parentView) {
    parent_id = parentView->GetTag();
    parent_view_type = parentView->GetViewType();
    return true;
  }
  return false;
}

bool HRViewManager::GetViewChildren(uint32_t node_id, std::vector<uint32_t> &children_ids, std::vector<std::string> &children_view_types) {
  auto viewIt = view_registry_.find(node_id);
  if (viewIt == view_registry_.end()) {
    return false;
  }
  auto view = viewIt->second;
  auto childrenViews = view->GetChildren();
  for (int i = 0; i < (int)childrenViews.size(); i++) {
    auto &child = childrenViews[(size_t)i];
    children_ids.push_back(child->GetTag());
    children_view_types.push_back(child->GetViewType());
  }
  return children_ids.size() > 0;
}

bool HRViewManager::IsCustomTsRenderView(std::string &view_name) {
  // custom ts view or WebView (no c-api for WebView)
  return custom_ts_render_views_.find(view_name) != custom_ts_render_views_.end() || view_name == "WebView";
}

std::shared_ptr<BaseView> HRViewManager::CreateCustomTsRenderView(uint32_t tag, std::string &view_name, bool is_parent_text) {
  napi_handle_scope scope = nullptr;
  napi_open_handle_scope(ts_env_, &scope);
  if (scope == nullptr) {
    return nullptr;
  }
  
  ArkTS arkTs(ts_env_);
  auto params_builder = arkTs.CreateObjectBuilder();
  params_builder.AddProperty("rootTag", ctx_->GetRootId());
  params_builder.AddProperty("tag", tag);
  params_builder.AddProperty("viewName", view_name);
  
  std::vector<napi_value> args = {
    params_builder.Build()
  };
  
  auto delegateObject = arkTs.GetObject(ts_render_provider_ref_);
  napi_value tsNode = delegateObject.Call("createRenderViewForCApi", args);
  
  napi_valuetype type = arkTs.GetType(tsNode);
  if (type == napi_null) {
    // TODO(hot): check tsNode not null
    FOOTSTONE_LOG(ERROR) << "create ts view error, tsNode null";
    return nullptr;
  }
  
  ArkUI_NodeHandle nodeHandle = nullptr;
  auto status = OH_ArkUI_GetNodeHandleFromNapiValue(ts_env_, tsNode, &nodeHandle);
  if (status != ARKUI_ERROR_CODE_NO_ERROR) {
    // TODO(hot):
    FOOTSTONE_LOG(ERROR) << "create ts view error, nodeHandle fail, status: " << status << ", nodeHandle: " << nodeHandle;
    return nullptr;
  }
  
  napi_close_handle_scope(ts_env_, scope);
  
  auto view = std::make_shared<CustomTsView>(ctx_, nodeHandle);
  view->Init();
  view->SetTag(tag);
  view->SetViewType(view_name);
  view_registry_[tag] = view;
  return view;
}

void HRViewManager::UpdateCustomTsProps(std::shared_ptr<BaseView> &view, const HippyValueObjectType &props, const std::vector<std::string> &deleteProps) {
  ArkTS arkTs(ts_env_);
  
  serializer_->Release();
  serializer_->WriteHeader();
  serializer_->WriteValue(HippyValue(props));
  std::pair<uint8_t *, size_t> props_buffer_pair = serializer_->Release();
  
  std::vector<napi_value> delete_props_array;
  for (auto delete_prop : deleteProps) {
    delete_props_array.push_back(arkTs.CreateString(delete_prop));
  }

  auto params_builder = arkTs.CreateObjectBuilder();
  params_builder.AddProperty("rootTag", ctx_->GetRootId());
  params_builder.AddProperty("tag", view->GetTag());
  params_builder.AddProperty("props", arkTs.CreateExternalArrayBuffer(props_buffer_pair.first, props_buffer_pair.second));
  params_builder.AddProperty("deleteProps", arkTs.CreateArray(delete_props_array));
  
  std::vector<napi_value> args = {
    params_builder.Build()
  };
  
  auto delegateObject = arkTs.GetObject(ts_render_provider_ref_);
  delegateObject.Call("updatePropsForCApi", args);
}

void HRViewManager::UpdateCustomTsEventListener(uint32_t tag, HippyValueObjectType &props) {
  ArkTS arkTs(ts_env_);
    
  serializer_->Release();
  serializer_->WriteHeader();
  serializer_->WriteValue(HippyValue(props));
  std::pair<uint8_t *, size_t> props_buffer_pair = serializer_->Release();
  
  auto params_builder = arkTs.CreateObjectBuilder();
  params_builder.AddProperty("rootTag", ctx_->GetRootId());
  params_builder.AddProperty("tag", tag);
  params_builder.AddProperty("props", arkTs.CreateExternalArrayBuffer(props_buffer_pair.first, props_buffer_pair.second));
  
  std::vector<napi_value> args = {
    params_builder.Build()
  };
  
  auto delegateObject = arkTs.GetObject(ts_render_provider_ref_);
  delegateObject.Call("updateEventListenerForCApi", args);
}

void HRViewManager::SetCustomTsRenderViewFrame(uint32_t tag, const HRRect &frame, const HRPadding &padding) {
  ArkTS arkTs(ts_env_);
  auto params_builder = arkTs.CreateObjectBuilder();
  params_builder.AddProperty("rootTag", ctx_->GetRootId());
  params_builder.AddProperty("tag", tag);
  params_builder.AddProperty("left", frame.x);
  params_builder.AddProperty("top", frame.y);
  params_builder.AddProperty("width", frame.width);
  params_builder.AddProperty("height", frame.height);
  
  std::vector<napi_value> args = {
    params_builder.Build()
  };
  
  auto delegateObject = arkTs.GetObject(ts_render_provider_ref_);
  delegateObject.Call("setRenderViewFrameForCApi", args);
}

std::shared_ptr<BaseView> HRViewManager::CreateCustomRenderView(uint32_t tag, std::string &view_name, bool is_parent_text) {
  auto creator_map = HippyViewProvider::GetCustomViewCreatorMap();
  auto it = creator_map.find(view_name);
  if (it != creator_map.end()) {
    auto view = it->second(ctx_);
    if (view) {
      view->Init();
      view->SetTag(tag);
      view->SetViewType(view_name);
      view_registry_[tag] = view;
      return view;
    }
  }
  return nullptr;
}

} // namespace native
} // namespace render
} // namespace hippy
