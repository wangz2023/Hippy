#
# Tencent is pleased to support the open source community by making
# Hippy available.
#
# Copyright (C) 2022 THL A29 Limited, a Tencent company.
# All rights reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

cmake_minimum_required(VERSION 3.14)

project("devtools_integration")

get_filename_component(PROJECT_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../.." REALPATH)

include("${PROJECT_ROOT_DIR}/buildconfig/cmake/GlobalPackagesModule.cmake")
include("${PROJECT_ROOT_DIR}/buildconfig/cmake/compiler_toolchain.cmake")

set(CMAKE_CXX_STANDARD 17)

# region library
add_library(${PROJECT_NAME} STATIC)
target_include_directories(${PROJECT_NAME} PUBLIC include)
target_compile_options(${PROJECT_NAME} PRIVATE ${COMPILE_OPTIONS})

# region vm
if ("${JS_ENGINE}" STREQUAL "V8")
    if (NOT TARGET v8)
        message(FATAL_ERROR "The V8 target not found, peer dependency not satisfied")
    endif()
    target_link_libraries(${PROJECT_NAME} PRIVATE v8)
    target_compile_definitions(${PROJECT_NAME} PRIVATE "JS_V8")
    get_target_property(V8_WITHOUT_INSPECTOR v8 INTERFACE_V8_WITHOUT_INSPECTOR)
    if (V8_WITHOUT_INSPECTOR)
        target_compile_definitions(${PROJECT_NAME} PRIVATE "V8_WITHOUT_INSPECTOR")
    endif ()
elseif ("${JS_ENGINE}" STREQUAL "JSC")
    target_compile_definitions(${PROJECT_NAME} PRIVATE "JS_JSC")
endif ()
# endregion

# region footstone
if (OHOS)
  GlobalPackages_Add_footstone()
else ()
  GlobalPackages_Add(footstone)
endif ()
target_link_libraries(${PROJECT_NAME} PRIVATE footstone)
# endregion

# region vfs
if (OHOS)
  GlobalPackages_Add_vfs()
else ()
  GlobalPackages_Add(vfs)
endif ()
target_link_libraries(${PROJECT_NAME} PRIVATE vfs)
# endregion

# region dom
if (OHOS)
  GlobalPackages_Add_dom()
else ()
  GlobalPackages_Add(dom)
endif ()
target_link_libraries(${PROJECT_NAME} PRIVATE dom)
# endregion

# region devtools
if (OHOS)
  GlobalPackages_Add_devtools_backend()
else ()
  GlobalPackages_Add(devtools_backend)
endif ()
target_link_libraries(${PROJECT_NAME} PRIVATE devtools_backend)
# endregion

# region devtools_integration
add_subdirectory(${PROJECT_ROOT_DIR}/devtools/devtools-integration/native ${CMAKE_CURRENT_BINARY_DIR}/devtools-integration/native)
target_link_libraries(${PROJECT_NAME} PUBLIC devtools_integration_native)
# endregion

# region source set
set(SOURCE_SET src/devtools_napi.cc)
target_sources(${PROJECT_NAME} PRIVATE ${SOURCE_SET})
# endregion
