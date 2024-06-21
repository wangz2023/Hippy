#include "driver/scope.h"
#include "footstone/string_view_utils.h"
#include "napi/native_api.h"

#include "connector/dom_manager_napi.h"
#include <js_native_api.h>
#include <js_native_api_types.h>
#include "oh_napi/data_holder.h"
#include "oh_napi/oh_napi_register.h"
#include "oh_napi/ark_ts.h"
#include "dom/dom_manager.h"
#include "dom/root_node.h"
#include "footstone/check.h"
#include "footstone/logging.h"
#include "footstone/persistent_object_map.h"
#include "footstone/task_runner.h"
#include "footstone/worker_impl.h"
#include "connector/wangz.h"

#include "devtools/devtools_data_source.h"
// #include "devtools/vfs/devtools_handler.h"

namespace hippy {
inline namespace framework {
inline namespace connector {
inline namespace wangz {

using WorkerManager = footstone::runner::WorkerManager;
using string_view = footstone::stringview::string_view;
using StringViewUtils = footstone::stringview::StringViewUtils;
constexpr uint32_t kPoolSize = 1;
constexpr char kDevToolsTag[] = "DevTools-Napi:";
std::shared_ptr<WorkerManager> worker_manager;
#ifdef ENABLE_INSPECTOR
using DevtoolsDataSource = hippy::devtools::DevtoolsDataSource;
#endif

static napi_value Add(napi_env env, napi_callback_info info){
//     size_t requireArgc = 2;
    size_t argc = 2;
    napi_value args[2] = {nullptr};

    napi_get_cb_info(env, info, &argc, args , nullptr, nullptr);

    napi_valuetype valuetype0;
    napi_typeof(env, args[0], &valuetype0);

    napi_valuetype valuetype1;
    napi_typeof(env, args[1], &valuetype1);

    double value0;
    napi_get_value_double(env, args[0], &value0);

    double value1;
    napi_get_value_double(env, args[1], &value1);

    napi_value sum;
    napi_create_double(env, value0 + value1, &sum);

    return sum;

}

// EXTERN_C_START
// static napi_value Init(napi_env env, napi_value exports)
// {
//     napi_property_descriptor desc[] = {
//         { "add", nullptr, Add, nullptr, nullptr, nullptr, napi_default, nullptr }
//     };
//     napi_define_properties(env, exports, sizeof(desc) / sizeof(desc[0]), desc);
//     return exports;
// }
// EXTERN_C_END
//
// static napi_module demoModule = {
//     .nm_version = 1,
//     .nm_flags = 0,
//     .nm_filename = nullptr,
//     .nm_register_func = Init,
//     .nm_modname = "wangzapp",
//     .nm_priv = ((void*)0),
//     .reserved = { 0 },
// };
//
// extern "C" __attribute__((constructor)) void RegisterWangzAppModule(void)
// {
//     napi_module_register(&demoModule);
// }

static napi_value OnCreateDevtools(napi_env env, napi_callback_info info) {
    //     size_t requireArgc = 2;
    ArkTS arkTs(env);
    size_t argc = 2;
    napi_value args[2] = {nullptr};

    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    std::vector<napi_value> argVector = arkTs.GetCallbackArgs(info);
    FOOTSTONE_DLOG(INFO) << "wangz::OnCreateDevtools::" << argVector.size();

    if (argc != 2) {
      napi_throw_error(env, nullptr, " Requires 2 arguments: data directory and WebSocket URL.");
      return nullptr;
    }

    napi_valuetype dirType = arkTs.GetType(args[0]);
    napi_valuetype wsType = arkTs.GetType(args[1]);
    if (!(dirType == napi_string && wsType == napi_string)) {
      napi_throw_error(env, nullptr, "Both arguments must be strings.");
      return nullptr;
    }

    std::string data_dir = arkTs.GetString(args[0]);
    std::string ws_url = arkTs.GetString(args[1]);

    worker_manager = std::make_shared<WorkerManager>(kPoolSize);

    DevtoolsDataSource::SetFileCacheDir(data_dir);

    auto devtools_data_source =
        std::make_shared<hippy::devtools::DevtoolsDataSource>(ws_url, worker_manager);
    uint32_t id = devtools::DevtoolsDataSource::Insert(devtools_data_source);

    napi_value result = arkTs.CreateInt(id);
    
    return result;
}

static napi_value OnDestroyDevtools(napi_env env, napi_callback_info info) {
    ArkTS arkTs(env);
    size_t argc = 2;
    napi_value args[2] = {nullptr};

    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    std::vector<napi_value> argVector = arkTs.GetCallbackArgs(info);
    FOOTSTONE_DLOG(INFO) << "wangz::OnDestroyDevtools::" << argVector.size();

    if (argc != 2) {
      napi_throw_error(env, nullptr, " Requires 2 arguments: data directory and WebSocket URL.");
      return nullptr;
    }

    napi_valuetype devtoolsIdType = arkTs.GetType(args[0]);
    napi_valuetype isReloadType = arkTs.GetType(args[1]);
    if (!(devtoolsIdType == napi_number && isReloadType == napi_boolean)) {
      napi_throw_error(env, nullptr, "Both arguments must be strings.");
      return nullptr;
    }

    int devtools_id = arkTs.GetInteger(args[0]);
    bool is_reload = arkTs.GetBoolean(args[1]);
    std::shared_ptr<DevtoolsDataSource> devtools_data_source = DevtoolsDataSource::Find(devtools_id);
    devtools_data_source->Destroy(is_reload);
    bool flag = DevtoolsDataSource::Erase(devtools_id);
    FOOTSTONE_DLOG(INFO) << kDevToolsTag << "OnDestroyDevtools devtools_id=" << devtools_id << ",flag=" << flag;
    FOOTSTONE_DCHECK(flag);
    worker_manager->Terminate();
    napi_value result = arkTs.GetUndefined();
    return result;
}


static napi_value OnBindDevtools(napi_env env, napi_callback_info info) {
    ArkTS arkTs(env);
    size_t argc = 4;
    napi_value args[4] = {nullptr};

    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    std::vector<napi_value> argVector = arkTs.GetCallbackArgs(info);
    FOOTSTONE_DLOG(INFO) << "wangz::OnBindDevtools::" << argVector.size();

    if (argc != 4) {
      napi_throw_error(env, nullptr, " Requires 4 arguments: data directory and WebSocket URL.");
      return nullptr;
    }

    napi_valuetype devtoolsIdType = arkTs.GetType(args[0]);
    //     napi_valuetype driverIdType = arkTs.GetType(args[1]);
    napi_valuetype domIdType = arkTs.GetType(args[2]);
    //     napi_valuetype renderIdType = arkTs.GetType(args[3]);
    if (!(devtoolsIdType == napi_number && domIdType == napi_number)) {
      napi_throw_error(env, nullptr, "Both arguments must be strings.");
      return nullptr;
    }

    int devtools_id = arkTs.GetInteger(args[0]);
    //     int driver_id = arkTs.GetInteger(args[1]);
    int dom_id = arkTs.GetInteger(args[2]);
    //     int render_id = arkTs.GetInteger(args[3]);
    std::shared_ptr<DevtoolsDataSource> devtools_data_source =
        DevtoolsDataSource::Find(devtools_id);
    std::any dom_manager;
    auto flag = hippy::global_data_holder.Find(dom_id, dom_manager);
    FOOTSTONE_CHECK(flag);
    auto dom_manager_object = std::any_cast<std::shared_ptr<DomManager>>(dom_manager);
    devtools_data_source->Bind(dom_manager_object);

    napi_value result = arkTs.GetUndefined();
    return result;
}

static napi_value OnAttachToRoot(napi_env env, napi_callback_info info) {
    ArkTS arkTs(env);
    size_t argc = 2;
    napi_value args[2] = {nullptr};

    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    std::vector<napi_value> argVector = arkTs.GetCallbackArgs(info);
    FOOTSTONE_DLOG(INFO) << "wangz::OnAttachToRoot::" << argVector.size();

    if (argc != 2) {
      napi_throw_error(env, nullptr, " Requires 2 arguments: data directory and WebSocket URL.");
      return nullptr;
    }

    napi_valuetype devtoolsIdType = arkTs.GetType(args[0]);
    napi_valuetype rootIdType = arkTs.GetType(args[1]);
    if (!(devtoolsIdType == napi_number && rootIdType == napi_number)) {
      napi_throw_error(env, nullptr, "Both arguments must be number.");
      return nullptr;
    }

    int devtools_id = arkTs.GetInteger(args[0]);
    int root_id = arkTs.GetInteger(args[1]);

    auto &root_map = RootNode::PersistentMap();
    std::shared_ptr<RootNode> root_node;
    auto ret = root_map.Find(root_id, root_node);
    if (!ret) {
      FOOTSTONE_DLOG(WARNING) << kDevToolsTag << "OnAttachToRoot root_node is nullptr";
      return nullptr;
    }

    FOOTSTONE_DLOG(INFO) << kDevToolsTag << "OnAttachToRoot root_id=" << root_id;
    std::shared_ptr<DevtoolsDataSource> devtools_data_source =
        DevtoolsDataSource::Find(devtools_id);
    devtools_data_source->SetRootNode(root_node);
    napi_value result = arkTs.GetUndefined();
    return result;
}

REGISTER_OH_NAPI("Wangz", "Wangz_Add", Add)
REGISTER_OH_NAPI("Wangz", "Wangz_OnCreateDevtools", OnCreateDevtools)
REGISTER_OH_NAPI("Wangz", "Wangz_OnDestroyDevtools", OnDestroyDevtools)
REGISTER_OH_NAPI("Wangz", "Wangz_OnBindDevtools", OnBindDevtools)
REGISTER_OH_NAPI("Wangz", "Wangz_OnAttachToRoot", OnAttachToRoot)

} // namespace wangz
} // namespace connector
} // namespace framework
} // namespace hippy