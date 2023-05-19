#ifndef MYOBJECT_H
#define MYOBJECT_H

#include <napi.h>
#include <string>

class SharedMemory : public Napi::ObjectWrap<SharedMemory> {
public:
    // 初始化函数
    static Napi::Object Init(Napi::Env env, Napi::Object exports);
    // 工厂函数
    static Napi::Object NewInstance(Napi::Env env, Napi::Value arg1, Napi::Value arg2);
    // 构造函数
    SharedMemory(const Napi::CallbackInfo& info);

private:
    std::string name;
    int length;
    int fd;
    long address;
private: // native call
    void initialize(const Napi::CallbackInfo& info);
    void destroy(const Napi::CallbackInfo& info);
    Napi::Value writableBytes(const Napi::CallbackInfo& info);
    Napi::Value readableBytes(const Napi::CallbackInfo& info);
//    SharedMemery readBytes(byte[] buffer, int offset, int length);
//    SharedMemery writeBytes(byte[] buffer, int offset, int length);
};

#endif