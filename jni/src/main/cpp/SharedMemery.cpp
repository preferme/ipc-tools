/*
 * SharedMemery.jni
 *
 * Created on: 2023/5/15 15:31.
 *     Author: hou-lei
 */
 #include <stdint.h>
 #define __int64 int64_t

#include "com_github_preferme_ipc_SharedMemery.h"
#include <stdlib.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <zconf.h>
#include <pthread.h>
#include <string.h>
#include <errno.h>

#define LOG(format, ...)  printf(format, ##__VA_ARGS__)

typedef struct {
    bool initialized;
    // 互斥量
    pthread_mutex_t mutex;
    // 读写标记
    int reader_index;
    int writer_index;
    int capacity;
    int count;
} shared_attr;

inline void throw_runtime_exception(JNIEnv * env, const char* message) {
    if(env == NULL) return;
    jclass exception_class = env->FindClass("java/lang/RuntimeException");
    if(exception_class) {
        env->ThrowNew(exception_class, (char *)message);
    }
    env->DeleteLocalRef(exception_class);
}

/*
 * Class:     com_github_preferme_ipc_SharedMemery
 * Method:    initialize
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_com_github_preferme_ipc_SharedMemery_initialize
  (JNIEnv * env, jobject self) {
    LOG("[SharedMemery_initialize] initialize\n");
    // 1. 获取成员变量 name
    jclass klass = env->GetObjectClass(self);
    jfieldID fId_name = env->GetFieldID(klass,"name","Ljava/lang/String;");
    jstring str_name = (jstring)env->GetObjectField(self, fId_name);
    if (str_name == NULL) {
        throw_runtime_exception(env, "name can not be null.");
        return;
    }
    const char * cName = env->GetStringUTFChars(str_name, NULL);
    // 2. 打开共享内存
    int map_fd = shm_open(cName, O_RDWR | O_CREAT | O_TRUNC | O_EXCL, 0664);
    bool is_new_map = true;
    if (map_fd <= 0) {
        map_fd = shm_open(cName, O_RDWR, 0664);
        is_new_map = false;
    }
    LOG("[SharedMemery_initialize] map_fd = %d\n", map_fd);
    LOG("[SharedMemery_initialize] is_new_map = %s\n", is_new_map?"true":"false");
    if (map_fd <= 0) {
        throw_runtime_exception(env, "open map fd failed.");
        return;
    }
    LOG("[SharedMemery_initialize] name : %s\n", cName);
    env->ReleaseStringUTFChars(str_name, cName);
    // 2. 获取成员变量 length
    jfieldID fId_length = env->GetFieldID(klass,"length","I");
    jint i_length = env->GetIntField(self, fId_length);
    if (i_length == 0) {
        throw_runtime_exception(env, "length can not be zero.");
        return;
    }
    // 3. 保存 map_fd
    jfieldID fId_mapFd = env->GetFieldID(klass,"fd","I");
    env->SetIntField(self, fId_mapFd, map_fd);
    // 4. 初始化共享内存 (大小，互斥锁，读写标记)
    if (is_new_map) {
        ftruncate(map_fd, i_length);
    }
    LOG("[SharedMemery_initialize] i_length = %d\n", i_length);
    void* mapped_address = mmap(NULL, i_length, PROT_READ | PROT_WRITE, MAP_SHARED, map_fd, 0);
    if ((long)mapped_address <= 0) {
//        printf("map failed. (%d): %s\n", errno, strerror(errno));
        throw_runtime_exception(env, "map memory failed.");
        return;
    }
    // 共享内存的起始位置保存用于读写同步的互斥锁以及读写标记，后续的位置保存数据。
    shared_attr * p_shared_attr = (shared_attr *)mapped_address;
    LOG("[SharedMemery_initialize] p_shared_attr->initialized = %s\n", p_shared_attr->initialized?"true":"false");
    if (is_new_map || p_shared_attr->initialized == false) {
        printf("[SharedMemery_initialize] init shared_attr\n");
        memset(mapped_address, 0, sizeof(shared_attr));
        // 初始化互斥量 mutex
        pthread_mutexattr_t mutexattr = {0};
        pthread_mutexattr_init(&mutexattr);
        pthread_mutexattr_setpshared(&mutexattr, PTHREAD_PROCESS_SHARED);
        pthread_mutex_init(&p_shared_attr->mutex, &mutexattr);
        pthread_mutexattr_destroy(&mutexattr);
        // 初始化条件变量 cond_read, cond_write
        p_shared_attr->count = 0;
        // 设置容量
        p_shared_attr->capacity = i_length - sizeof(shared_attr);
        // 标记初始化完成
        p_shared_attr->initialized = true;
    }

    // 5. 保存映射之后的内存地址
    jfieldID fId_address = env->GetFieldID(klass,"address","J");
    LOG("[SharedMemery_initialize] mapped_address = %d\n", mapped_address);
    jlong l_mapped_addr = (jlong)mapped_address;
    env->SetLongField(self, fId_address, l_mapped_addr);

}

/*
 * Class:     com_github_preferme_ipc_SharedMemery
 * Method:    destroy
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_com_github_preferme_ipc_SharedMemery_destroy
  (JNIEnv * env, jobject self) {
    jclass klass = env->GetObjectClass(self);
    // 1. 读取映射后的地址
    jfieldID fId_address = env->GetFieldID(klass,"address","J");
    jlong l_mapped_addr = env->GetLongField(self, fId_address);
    LOG("[SharedMemery_destroy] l_mapped_addr = %d\n", l_mapped_addr);
    if (l_mapped_addr <= 0) {
        throw_runtime_exception(env, "load map address failed.");
        return;
    }
    shared_attr * p_shared_attr = (shared_attr *)l_mapped_addr;
    // 2. 清理资源
    LOG("[SharedMemery_destroy] p_shared_attr->initialized = %s\n", p_shared_attr->initialized?"true":"false");
    if (p_shared_attr->initialized == true) {
        p_shared_attr->initialized = false;
        LOG("[SharedMemery_destroy] release resources.\n");
        // 2.2 销毁 互斥量
        pthread_mutex_destroy(&p_shared_attr->mutex);
        // 2.3 重置共享属性区域
        memset(p_shared_attr, 0, sizeof(shared_attr));
        // 2.4 释放映射内存
        void * mapped_address = (void*)l_mapped_addr;
        // 2.4.1 读取映射内存的长度
        jfieldID fId_length = env->GetFieldID(klass,"length","I");
        jint i_length = env->GetIntField(self, fId_length);
        munmap(mapped_address, i_length);
        // 2.5 释放共享内存
        // 2.5.1 读取共享内存的名称
        jfieldID fId_name = env->GetFieldID(klass,"name","Ljava/lang/String;");
        jstring str_name = (jstring)env->GetObjectField(self, fId_name);
        const char * cName = env->GetStringUTFChars(str_name, NULL);
        shm_unlink(cName);
        env->ReleaseStringUTFChars(str_name, cName);
        LOG("[SharedMemery_destroy] release done.\n");
    }
}

/*
 * Class:     com_github_preferme_ipc_SharedMemery
 * Method:    writableBytes
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_com_github_preferme_ipc_SharedMemery_writableBytes
  (JNIEnv * env, jobject self) {
    // 1. 读取映射后的地址
    jclass klass = env->GetObjectClass(self);
    jfieldID fId_address = env->GetFieldID(klass,"address","J");
    jlong l_mapped_addr = env->GetLongField(self, fId_address);
    LOG("[SharedMemery_writableBytes] l_mapped_addr = %i\n", l_mapped_addr);
    if (l_mapped_addr <= 0) {
        throw_runtime_exception(env, "load map address failed.");
        return -1;
    }
    // 2. 访问共享属性
    shared_attr * p_shared_attr = (shared_attr*) l_mapped_addr;
    jint reader_index = -1, writer_index = -1, capacity = -1;
    pthread_mutex_lock(&p_shared_attr->mutex);
    reader_index = p_shared_attr->reader_index;
    writer_index = p_shared_attr->writer_index;
    capacity = p_shared_attr->capacity;
    pthread_mutex_unlock(&p_shared_attr->mutex);
    if (reader_index <= writer_index) {
        return capacity - 1 - (writer_index - reader_index);
    } else {
        return reader_index - writer_index - 1;
    }
}

/*
 * Class:     com_github_preferme_ipc_SharedMemery
 * Method:    readableBytes
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_com_github_preferme_ipc_SharedMemery_readableBytes
  (JNIEnv * env, jobject self) {
    // 1. 读取映射后的地址
    jclass klass = env->GetObjectClass(self);
    jfieldID fId_address = env->GetFieldID(klass,"address","J");
    jlong l_mapped_addr = env->GetLongField(self, fId_address);
    LOG("[SharedMemery_readableBytes] l_mapped_addr = %i\n", l_mapped_addr);
    if (l_mapped_addr <= 0) {
        throw_runtime_exception(env, "load map address failed.");
        return -1;
    }
    // 2. 访问共享属性
    shared_attr * p_shared_attr = (shared_attr*) l_mapped_addr;
    jint reader_index = -1, writer_index = -1, capacity = -1;
    pthread_mutex_lock(&p_shared_attr->mutex);
    reader_index = p_shared_attr->reader_index;
    writer_index = p_shared_attr->writer_index;
    capacity = p_shared_attr->capacity;
    pthread_mutex_unlock(&p_shared_attr->mutex);
    if (reader_index <= writer_index) {
        return writer_index - reader_index;
    } else {
        return writer_index + (capacity-reader_index);
    }
}

/*
 * Class:     com_github_preferme_ipc_SharedMemery
 * Method:    read
 * Signature: (Lcom/github/preferme/ipc/ByteBufferFactory;)Ljava/nio/ByteBuffer;
 */
JNIEXPORT jobject JNICALL Java_com_github_preferme_ipc_SharedMemery_read
  (JNIEnv * env, jobject self, jobject factory) {
    if (factory == NULL) {
        throw_runtime_exception(env, "factory can not be null");
        return NULL;
    }
    jclass cls_factory = env->GetObjectClass(factory);
    LOG("[SharedMemery_read] cls_factory = %ld\n", cls_factory);
    jmethodID mId_create = env->GetMethodID(cls_factory, "create", "(I)Ljava/nio/ByteBuffer;");
    LOG("[SharedMemery_read] mId_create = %ld\n", mId_create);

    // 1.3. 读取映射后的地址
    jclass klass = env->GetObjectClass(self);
    jfieldID fId_address = env->GetFieldID(klass,"address","J");
    jlong l_mapped_addr = env->GetLongField(self, fId_address);
    LOG("[SharedMemery_read] l_mapped_addr = %i\n", l_mapped_addr);
    if (l_mapped_addr <= 0) {
        throw_runtime_exception(env, "load map address failed.");
        return NULL;
    }

    // 2. 访问共享属性
    shared_attr * p_shared_attr = (shared_attr*) l_mapped_addr;
    jint reader_index = -1, writer_index = -1, capacity = -1;
    pthread_mutex_lock(&p_shared_attr->mutex);
    reader_index = p_shared_attr->reader_index;
    writer_index = p_shared_attr->writer_index;
    capacity = p_shared_attr->capacity;
    pthread_mutex_unlock(&p_shared_attr->mutex);
    int readableBytes = reader_index <= writer_index
            ? writer_index - reader_index
            : writer_index + (capacity-reader_index);
    LOG("[SharedMemery_read] readableBytes = %d\n", readableBytes);
    // 3.调用函数，获取buffer对象
    jobject obj_buffer = env->CallObjectMethod(factory, mId_create, readableBytes);
    LOG("[SharedMemery_read] obj_buffer = %ld\n", obj_buffer);
    // 4.获取put函数，调用put函数
    jclass cls_buffer = env->FindClass("java/nio/ByteBuffer");
    LOG("[SharedMemery_read] cls_buffer = %ld\n", cls_buffer);
    jmethodID mId_put = env->GetMethodID(cls_buffer, "put", "([B)Ljava/nio/ByteBuffer;");
    LOG("[SharedMemery_read] mId_put = %ld\n", mId_put);


    return obj_buffer;
}

/*
 * Class:     com_github_preferme_ipc_SharedMemery
 * Method:    readBytes
 * Signature: ([BII)Lcom/github/preferme/ipc/SharedMemery;
 */
JNIEXPORT jobject JNICALL Java_com_github_preferme_ipc_SharedMemery_readBytes
  (JNIEnv * env, jobject self, jbyteArray buffer, jint offset, jint length) {
    // 1.1. 校验 offset, length 是否为非负数
    if (offset < 0) {
        throw_runtime_exception(env, "offset cannot be negative.");
        return self;
    }
    if (length < 0) {
        throw_runtime_exception(env, "length cannot be negative.");
        return self;
    }
    // 1.2. 校验 buffer 的长度与 offset,length 是否合适
    jsize buff_size = env->GetArrayLength(buffer);
    if (offset + length > buff_size) {
        throw_runtime_exception(env, "offset and length out of buffer size.");
        return self;
    }
    // 1.3. 读取映射后的地址
    jclass klass = env->GetObjectClass(self);
    jfieldID fId_address = env->GetFieldID(klass,"address","J");
    jlong l_mapped_addr = env->GetLongField(self, fId_address);
    LOG("[SharedMemery_getBytes] l_mapped_addr = %i\n", l_mapped_addr);
    if (l_mapped_addr <= 0) {
        throw_runtime_exception(env, "load map address failed.");
        return self;
    }
    // 1.3.1. 访问共享属性
    shared_attr * p_shared_attr = (shared_attr*) l_mapped_addr;
    jint reader_index = -1, writer_index = -1, capacity = -1;
    pthread_mutex_lock(&p_shared_attr->mutex);
    reader_index = p_shared_attr->reader_index;
    writer_index = p_shared_attr->writer_index;
    capacity = p_shared_attr->capacity;
    // 1.4 校验可读数据量
    int readableBytes =  reader_index <= writer_index
                         ? writer_index - reader_index
                         : writer_index + (capacity-reader_index);
    if (length > readableBytes) {
        throw_runtime_exception(env, "not enough bytes to be read.");
        pthread_mutex_unlock(&p_shared_attr->mutex);
        return self;
    }
    // 2. 读取数据
    jint index = p_shared_attr->reader_index;
    const jbyte *buf = reinterpret_cast<const jbyte *>(l_mapped_addr + sizeof(shared_attr) + index);
    if (index + length <= capacity) {
        // 2.1 读取中间部分的数据
        env->SetByteArrayRegion(buffer, offset, length, buf);
        p_shared_attr->reader_index += length;
    } else {
        // 2.2 读取前后两部分数据
        jsize second_part = index + length - capacity;
        jsize first_part = length - second_part;
        if (first_part < 0) {
            throw_runtime_exception(env, "not enough bytes to be read.");
            pthread_mutex_unlock(&p_shared_attr->mutex);
            return self;
        }
        if (first_part > 0) {
            env->SetByteArrayRegion(buffer, offset, first_part, buf);
            p_shared_attr->reader_index += first_part;
        }
        if (second_part > 0) {
            const jbyte *buf_second = reinterpret_cast<const jbyte *>(l_mapped_addr + sizeof(shared_attr));
            env->SetByteArrayRegion(buffer, offset + first_part, second_part, buf_second);
            p_shared_attr->reader_index = second_part;
        }
    }
    pthread_mutex_unlock(&p_shared_attr->mutex);
    return self;
}

/*
 * Class:     com_github_preferme_ipc_SharedMemery
 * Method:    write
 * Signature: (Ljava/nio/ByteBuffer;)I
 */
JNIEXPORT jint JNICALL Java_com_github_preferme_ipc_SharedMemery_write
  (JNIEnv * env, jobject self, jobject buffer) {
    // 1. 调用 buffer.remaining()
    jclass cls_buffer = env->GetObjectClass(buffer);
    LOG("[SharedMemery_write] cls_buffer = %ld\n", cls_buffer);
    jmethodID mid_remaining = env->GetMethodID(cls_buffer, "remaining", "()I");
    LOG("[SharedMemery_write] mid_remaining = %ld\n", mid_remaining);
    jint remain = env->CallIntMethod(buffer, mid_remaining);
    LOG("[SharedMemery_write] remain = %d\n", remain);
    if (remain <= 0) {
        return 0;
    }
    // 2. 读取 buffer 数据
    jmethodID mid_get = env->GetMethodID(cls_buffer, "get", "([B)Ljava/nio/ByteBuffer;");
    jbyteArray ba_data = env->NewByteArray(remain);
    env->CallObjectMethod(buffer, mid_get, ba_data);

    //env->GetByteArrayRegion(ba_buffer, 0, remain, jbyte *);

    // 2. 映射共享缓存
    // 1.3. 读取映射后的地址
    jclass cls_self = env->GetObjectClass(self);
    jfieldID fid_address = env->GetFieldID(cls_self,"address","J");
    jlong l_mapped_addr = env->GetLongField(self, fid_address);
    LOG("[SharedMemery_write] l_mapped_addr = %i\n", l_mapped_addr);
    if (l_mapped_addr <= 0) {
        throw_runtime_exception(env, "load map address failed.");
        return 0;
    }
    // 1.3.1. 访问共享属性
    shared_attr * p_shared_attr = (shared_attr*) l_mapped_addr;
    jint reader_index = -1, writer_index = -1, capacity = -1;
    pthread_mutex_lock(&p_shared_attr->mutex);
    reader_index = p_shared_attr->reader_index;
    writer_index = p_shared_attr->writer_index;
    capacity = p_shared_attr->capacity;
    // 1.4 校验可写入的数据量
    jsize writableBytes = reader_index <= writer_index
                          ? capacity - 1 - (writer_index - reader_index)
                          : reader_index - writer_index - 1;
    int length = remain + 4;
    if (length > writableBytes) {
        throw_runtime_exception(env, "not enough capacity to be write.");
        pthread_mutex_unlock(&p_shared_attr->mutex);
        return 0;
    }
    // 3. 写入缓存数据
    jint index = writer_index;
    jbyte * buf = reinterpret_cast<jbyte *>(l_mapped_addr + sizeof(shared_attr) + writer_index);
    if (index + length <= capacity) {
        // 2.1 写入中间部分的数据
        *((int*)buf) = remain;
        env->GetByteArrayRegion(ba_data, 0, remain, buf + 4);
        p_shared_attr->writer_index += length;
    } else {
        // 2.2 写入前后两部分数据
        jsize second_part = index + length - capacity;
        jsize first_part = length - second_part;
        if (first_part < 0) {
            throw_runtime_exception(env, "not enough bytes to be write.");
            pthread_mutex_unlock(&p_shared_attr->mutex);
            return 0;
        }
        if (first_part > 0) {

            if (first_part>4) {
                env->GetByteArrayRegion(ba_data, 0, first_part, buf);
            }
            p_shared_attr->writer_index += first_part;
        }
        if (second_part > 0) {
            jbyte *buf_second = reinterpret_cast<jbyte *>(l_mapped_addr + sizeof(shared_attr));
            env->GetByteArrayRegion(ba_data, first_part, second_part, buf_second);
            p_shared_attr->writer_index = second_part;
        }
    }
    pthread_mutex_unlock(&p_shared_attr->mutex);
    return 0;
}

/*
 * Class:     com_github_preferme_ipc_SharedMemery
 * Method:    writeBytes
 * Signature: ([BII)Lcom/github/preferme/ipc/SharedMemery;
 */
JNIEXPORT jobject JNICALL Java_com_github_preferme_ipc_SharedMemery_writeBytes
  (JNIEnv * env, jobject self, jbyteArray buffer, jint offset, jint length) {
    // 1.1. 校验 offset, length 是否为非负数
    if (offset < 0) {
        throw_runtime_exception(env, "offset cannot be negative.");
        return self;
    }
    if (length < 0) {
        throw_runtime_exception(env, "length cannot be negative.");
        return self;
    }
    // 1.2. 校验 buffer 的长度与 offset,length 是否合适
    jsize buff_size = env->GetArrayLength(buffer);
    if (offset + length > buff_size) {
        throw_runtime_exception(env, "offset and length out of buffer size.");
        return self;
    }
    // 1.3. 读取映射后的地址
    jclass klass = env->GetObjectClass(self);
    jfieldID fId_address = env->GetFieldID(klass,"address","J");
    jlong l_mapped_addr = env->GetLongField(self, fId_address);
    LOG("[SharedMemery_setBytes] l_mapped_addr = %i\n", l_mapped_addr);
    if (l_mapped_addr <= 0) {
        throw_runtime_exception(env, "load map address failed.");
        return self;
    }
    // 1.3.1. 访问共享属性
    shared_attr * p_shared_attr = (shared_attr*) l_mapped_addr;
    jint reader_index = -1, writer_index = -1, capacity = -1;
    pthread_mutex_lock(&p_shared_attr->mutex);
    reader_index = p_shared_attr->reader_index;
    writer_index = p_shared_attr->writer_index;
    capacity = p_shared_attr->capacity;
    // 1.4 校验可写入的数据量
    jsize writableBytes = reader_index <= writer_index
                          ? capacity - 1 - (writer_index - reader_index)
                          : reader_index - writer_index - 1;
    if (length > writableBytes) {
        throw_runtime_exception(env, "not enough capacity to be write.");
        pthread_mutex_unlock(&p_shared_attr->mutex);
        return self;
    }
    // 2. 写入数据
    jint index = writer_index;
    jbyte * buf = reinterpret_cast<jbyte *>(l_mapped_addr + sizeof(shared_attr) + index);
    if (index + length <= capacity) {
        // 2.1 写入中间部分的数据
        env->GetByteArrayRegion(buffer, offset, length, buf);
        p_shared_attr->writer_index += length;
    } else {
        // 2.2 写入前后两部分数据
        jsize second_part = index + length - capacity;
        jsize first_part = length - second_part;
        if (first_part < 0) {
            throw_runtime_exception(env, "not enough bytes to be write.");
            pthread_mutex_unlock(&p_shared_attr->mutex);
            return self;
        }
        if (first_part > 0) {
            env->GetByteArrayRegion(buffer, offset, first_part, buf);
            p_shared_attr->writer_index += first_part;
        }
        if (second_part > 0) {
            jbyte *buf_second = reinterpret_cast<jbyte *>(l_mapped_addr + sizeof(shared_attr));
            env->GetByteArrayRegion(buffer, offset + first_part, second_part, buf_second);
            p_shared_attr->writer_index = second_part;
        }
    }
    pthread_mutex_unlock(&p_shared_attr->mutex);
    return self;
}
