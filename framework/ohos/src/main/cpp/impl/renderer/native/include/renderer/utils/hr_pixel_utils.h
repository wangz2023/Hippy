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

namespace hippy {
inline namespace render {
inline namespace native {

class HRPixelUtils {
public:
  inline static void InitDensity(double density) { density_ = static_cast<float>(density); }
  
  inline static float DpToPx(float dp) { return dp * density_; }
  inline static float PxToDp(float px) { return px / density_; }
  
private:
  static float density_;
};

} // namespace native
} // namespace render
} // namespace hippy
