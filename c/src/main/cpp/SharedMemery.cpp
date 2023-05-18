/*
 * SharedMemery.c
 *
 * Created on: 2023/5/15 15:31.
 *     Author: hou-lei
 */
#include "com_github_preferme_ipc_SharedMemery.h"
#include <stdlib.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <zconf.h>
#include <pthread.h>
#include <string.h>
#include <errno.h>

#define LOG(...)  printf(#__VA_ARGS__)

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
    LOG("[SharedMemery_initialize] name : %s", cName);
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
        pthread_condattr_t condattr = {0};
        pthread_condattr_init(&condattr);
        pthread_condattr_setpshared(&condattr, PTHREAD_PROCESS_SHARED);
        pthread_cond_init(&p_shared_attr->cond_read,  &condattr);
        pthread_cond_init(&p_shared_attr->cond_write, &condattr);
        pthread_condattr_destroy(&condattr);
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
    if (p_shared_attr->initialized == true) {
        LOG("[SharedMemery_destroy] release resources.");
        // 2.1 销毁 读写条件变量
        pthread_cond_destroy(&p_shared_attr->cond_read);
        pthread_cond_destroy(&p_shared_attr->cond_write);
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
    }
}


/*
 * Class:     com_github_preferme_ipc_SharedMemery
 * Method:    writerIndex
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_com_github_preferme_ipc_SharedMemery_writerIndex
  (JNIEnv * env, jobject self) {
    // 1. 读取映射后的地址
    jclass klass = env->GetObjectClass(self);
    jfieldID fId_address = env->GetFieldID(klass,"address","J");
    jlong l_mapped_addr = env->GetLongField(self, fId_address);
    LOG("[SharedMemery_writerIndex] l_mapped_addr = %d\n", l_mapped_addr);
    if (l_mapped_addr <= 0) {
        throw_runtime_exception(env, "load map address failed.");
        return -1;
    }
    // 2. 访问共享属性
    shared_attr * p_shared_attr = (shared_attr*) l_mapped_addr;
    jint writer_index = -1;
    pthread_mutex_lock(&p_shared_attr->mutex);
    writer_index = p_shared_attr->writer_index;
    pthread_mutex_unlock(&p_shared_attr->mutex);
    return writer_index;
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
    LOG("[SharedMemery_writableBytes] l_mapped_addr = %d\n", l_mapped_addr);
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
 * Method:    readerIndex
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_com_github_preferme_ipc_SharedMemery_readerIndex
  (JNIEnv * env, jobject self) {
    // 1. 读取映射后的地址
    jclass klass = env->GetObjectClass(self);
    jfieldID fId_address = env->GetFieldID(klass,"address","J");
    jlong l_mapped_addr = env->GetLongField(self, fId_address);
    LOG("[SharedMemery_readerIndex] l_mapped_addr = %d\n", l_mapped_addr);
    if (l_mapped_addr <= 0) {
        throw_runtime_exception(env, "load map address failed.");
        return -1;
    }
    // 2. 访问共享属性
    shared_attr * p_shared_attr = (shared_attr*) l_mapped_addr;
    jint reader_index = -1;
    pthread_mutex_lock(&p_shared_attr->mutex);
    reader_index = p_shared_attr->reader_index;
    pthread_mutex_unlock(&p_shared_attr->mutex);
    return reader_index;
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
    LOG("[SharedMemery_readableBytes] l_mapped_addr = %d\n", l_mapped_addr);
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
 * Method:    getBytes
 * Signature: (I[BII)Lcom/github/preferme/ipc/SharedMemery;
 */
JNIEXPORT jobject JNICALL Java_com_github_preferme_ipc_SharedMemery_getBytes
  (JNIEnv * env, jobject self, jint index, jbyteArray buffer, jint offset, jint length) {

    return self;
}

/*
 * Class:     com_github_preferme_ipc_SharedMemery
 * Method:    setBytes
 * Signature: (I[BII)Lcom/github/preferme/ipc/SharedMemery;
 */
JNIEXPORT jobject JNICALL Java_com_github_preferme_ipc_SharedMemery_setBytes
  (JNIEnv * env, jobject self, jint index, jbyteArray buffer, jint offset, jint length) {


    return self;
}

/*
 * Class:     com_github_preferme_ipc_SharedMemery
 * Method:    readBytes
 * Signature: ([BII)Lcom/github/preferme/ipc/SharedMemery;
 */
JNIEXPORT jobject JNICALL Java_com_github_preferme_ipc_SharedMemery_readBytes
  (JNIEnv *, jobject, jbyteArray, jint, jint);

/*
 * Class:     com_github_preferme_ipc_SharedMemery
 * Method:    writeBytes
 * Signature: ([BII)Lcom/github/preferme/ipc/SharedMemery;
 */
JNIEXPORT jobject JNICALL Java_com_github_preferme_ipc_SharedMemery_writeBytes
  (JNIEnv *, jobject, jbyteArray, jint, jint);
