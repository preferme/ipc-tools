#include <napi.h>
#include "shared_memory.h"

Napi::Object MakeSharedMemory(const Napi::CallbackInfo& info) {
    if (info.Length() != 2) {
        Napi::Error::New(info.Env(), "Expected exactly two argument (name, length)")
                .ThrowAsJavaScriptException();
        return info.Env().Undefined().ToObject();
    }
    if (!info[0].IsString()) {
        Napi::Error::New(info.Env(), "The First Parameter Expected an String")
                .ThrowAsJavaScriptException();
        return info.Env().Undefined().ToObject();
    }
    if (!info[1].IsNumber()) {
        Napi::Error::New(info.Env(), "The Second Parameter Expected an Number")
                .ThrowAsJavaScriptException();
        return info.Env().Undefined().ToObject();
    }

  return SharedMemory::NewInstance(info.Env(), info[0], info[1]);
}

/**
 * 插件初始化函数。
 */
Napi::Object InitAll(Napi::Env env, Napi::Object exports) {
    // 声明 SharedMemory 类（函数形式）
    SharedMemory::Init(env, exports);
    // 为插件对象添加工厂函数 `makeSharedMemory`
    exports.Set(Napi::String::New(env, "makeSharedMemory"),
                Napi::Function::New(env, MakeSharedMemory));

  return exports;
}

NODE_API_MODULE(addon, InitAll)