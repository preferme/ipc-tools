#include "shared_memory.h"
#include <napi.h>
#include <uv.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <zconf.h>
#include <pthread.h>
#include <string.h>
#include <errno.h>

#define LOG(format, ...)  printf(format, ##__VA_ARGS__)

using namespace Napi;

typedef struct {
    bool initialized;
    // 互斥量
    pthread_mutex_t mutex;
    // 读写条件变量
    pthread_cond_t cond_read;
    pthread_cond_t cond_write;
    // 读写标记
    int reader_index;
    int writer_index;
    int capacity;
} shared_attr;

inline void throw_js_exception(Napi::Env env, const char* message) {
    Napi::Error::New(env, message).ThrowAsJavaScriptException();
}


Napi::Object SharedMemory::Init(Napi::Env env, Napi::Object exports) {
    // 定义 SharedMemory 类
    Napi::Function func = DefineClass(env, "SharedMemory", {
//        InstanceMethod("initialize", &SharedMemory::initialize),
        InstanceMethod("destroy", &SharedMemory::destroy),
        InstanceMethod("writableBytes", &SharedMemory::writableBytes),
        InstanceMethod("readableBytes", &SharedMemory::readableBytes)
    });

  Napi::FunctionReference* constructor = new Napi::FunctionReference();
  *constructor = Napi::Persistent(func);
  env.SetInstanceData(constructor);

  exports.Set("SharedMemory", func);
  return exports;
}

// 构造函数
SharedMemory::SharedMemory(const Napi::CallbackInfo& info)
    : Napi::ObjectWrap<SharedMemory>(info) {
    this->name   = info[0].As<Napi::String>().Utf8Value();
    this->length = info[1].As<Napi::Number>().Int32Value();
    this->initialize(info);
};

Napi::Object SharedMemory::NewInstance(Napi::Env env, Napi::Value arg1, Napi::Value arg2) {
    unsigned long length = arg2.As<Napi::Number>().Int64Value();
    if (length <= sizeof(shared_attr)) {
        throw_js_exception(env, "Need a larger shared memory space");
        return env.Undefined().ToObject();
    }
  Napi::EscapableHandleScope scope(env);
  Napi::Object obj = env.GetInstanceData<Napi::FunctionReference>()->New({arg1, arg2});
  return scope.Escape(napi_value(obj)).ToObject();
}

void SharedMemory::initialize(const Napi::CallbackInfo& info) {
    LOG("[SharedMemory::initialize] name = %s, length = %d\n", this->name.c_str(), this->length);
    // 2. 打开共享内存
    int map_fd = shm_open(this->name.c_str(), O_RDWR | O_CREAT | O_TRUNC | O_EXCL, 0664);
    bool is_new_map = true;
    if (map_fd <= 0) {
        map_fd = shm_open(this->name.c_str(), O_RDWR, 0664);
        is_new_map = false;
    }
    LOG("[SharedMemory::initialize] map_fd = %d\n", map_fd);
    LOG("[SharedMemory::initialize] is_new_map = %s\n", is_new_map?"true":"false");
    if (map_fd <= 0) {
        throw_js_exception(info.Env(), "open map fd failed.");
        return;
    }
    // 3. 保存 map_fd
    this->fd = map_fd;
    // 4. 初始化共享内存 (大小，互斥锁，读写标记)
    if (is_new_map) {
        ftruncate(map_fd, this->length);
    }
    // 映射内存
    void* mapped_address = mmap(NULL, this->length, PROT_READ | PROT_WRITE, MAP_SHARED, map_fd, 0);
    if ((long)mapped_address <= 0) {
//        printf("map failed. (%d): %s\n", errno, strerror(errno));
        throw_js_exception(info.Env(), "map memory failed.");
        return;
    }
    // 共享内存的起始位置保存用于读写同步的互斥锁以及读写标记，后续的位置保存数据。
    shared_attr * p_shared_attr = (shared_attr *)mapped_address;
    LOG("[SharedMemory::initialize] p_shared_attr->initialized = %s\n", p_shared_attr->initialized?"true":"false");
    if (is_new_map || p_shared_attr->initialized == false) {
        LOG("[SharedMemory::initialize] init shared_attr\n");
        memset(mapped_address, 0, sizeof(shared_attr));
        // 初始化互斥量 mutex
        pthread_mutexattr_t mutexattr;
        pthread_mutexattr_init(&mutexattr);
        pthread_mutexattr_setpshared(&mutexattr, PTHREAD_PROCESS_SHARED);
        pthread_mutex_init(&p_shared_attr->mutex, &mutexattr);
        pthread_mutexattr_destroy(&mutexattr);
        // 初始化条件变量 cond_read, cond_write
        pthread_condattr_t condattr;
        pthread_condattr_init(&condattr);
        pthread_condattr_setpshared(&condattr, PTHREAD_PROCESS_SHARED);
        pthread_cond_init(&p_shared_attr->cond_read,  &condattr);
        pthread_cond_init(&p_shared_attr->cond_write, &condattr);
        pthread_condattr_destroy(&condattr);
        // 设置容量
        p_shared_attr->capacity = this->length - sizeof(shared_attr);
        // 标记初始化完成
        p_shared_attr->initialized = true;
    }

    // 5. 保存映射之后的内存地址
    this->address = (long)mapped_address;
    LOG("[SharedMemory::initialize] mapped_address = %ld\n", this->address);

}

// Napi::Value
// return info.Env().Undefined();
void SharedMemory::destroy(const Napi::CallbackInfo& info) {
    // 1. 读取映射后的地址
    shared_attr * p_shared_attr = (shared_attr *)this->address;
    // 2. 清理资源
    LOG("[SharedMemory::destroy] p_shared_attr->initialized = %s\n", p_shared_attr->initialized?"true":"false");
    if (p_shared_attr->initialized == true) {
        p_shared_attr->initialized = false;
        LOG("[SharedMemory::destroy] release resources.\n");
        // 2.1 销毁 读写条件变量
        pthread_cond_destroy(&p_shared_attr->cond_read);
        pthread_cond_destroy(&p_shared_attr->cond_write);
        // 2.2 销毁 互斥量
        pthread_mutex_destroy(&p_shared_attr->mutex);
        // 2.3 重置共享属性区域
        memset(p_shared_attr, 0, sizeof(shared_attr));
        // 2.4 释放映射内存
        void * mapped_address = (void*)this->address;
        // 2.4.1 读取映射内存的长度
        munmap(mapped_address, this->length);
        // 2.5 释放共享内存
        // 2.5.1 读取共享内存的名称
        const char * cName = this->name.c_str();
        shm_unlink(cName);
        this->address = NULL;
        close(this->fd);
        this->fd = -1;
        LOG("[SharedMemory::destroy] release done.\n");
    }
}

Napi::Value SharedMemory::writableBytes(const Napi::CallbackInfo& info) {
    shared_attr * p_shared_attr = (shared_attr*) this->address;
    int reader_index = -1, writer_index = -1, capacity = -1, writableBytes = 0;
    pthread_mutex_lock(&p_shared_attr->mutex);
    reader_index = p_shared_attr->reader_index;
    writer_index = p_shared_attr->writer_index;
    capacity = p_shared_attr->capacity;
    pthread_mutex_unlock(&p_shared_attr->mutex);
    LOG("[SharedMemory::writableBytes] reader_index = %d, writer_index = %d, capacity = %d\n", reader_index, writer_index, capacity);
    if (reader_index <= writer_index) {
        writableBytes = capacity - 1 - (writer_index - reader_index);
    } else {
        writableBytes = reader_index - writer_index - 1;
    }
    return Napi::Number::New(info.Env(), writableBytes);
}

Napi::Value SharedMemory::readableBytes(const Napi::CallbackInfo& info) {
    shared_attr * p_shared_attr = (shared_attr*) this->address;
    int reader_index = -1, writer_index = -1, capacity = -1, readableBytes = 0;
    pthread_mutex_lock(&p_shared_attr->mutex);
    reader_index = p_shared_attr->reader_index;
    writer_index = p_shared_attr->writer_index;
    capacity = p_shared_attr->capacity;
    pthread_mutex_unlock(&p_shared_attr->mutex);
    LOG("[SharedMemory::readableBytes] reader_index = %d, writer_index = %d, capacity = %d\n", reader_index, writer_index, capacity);
    if (reader_index <= writer_index) {
        readableBytes = writer_index - reader_index;
    } else {
        readableBytes = writer_index + (capacity-reader_index);
    }
    return Napi::Number::New(info.Env(), readableBytes);
}
