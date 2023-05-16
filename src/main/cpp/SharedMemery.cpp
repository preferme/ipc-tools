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

inline void throw_runtime_exception(JNIEnv * env, const char* message) {
    if(!env) return;
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
  (JNIEnv * env, jobject obj) {
    // 1. 获取成员变量 name
    jclass klass = env->GetObjectClass(obj);
    jfieldID fId_name = env->GetFieldID(klass,"name","Ljava/lang/String;");
    jstring str_name = (jstring)env->GetObjectField(obj, fId_name);
    if (str_name == NULL) {
        throw_runtime_exception(env, "name can not be null.");
        return;
    }
    const char * cName = env->GetStringUTFChars(str_name, NULL);
    int map_fd = shm_open(cName, O_RDWR, 0664);
    bool is_new_map = false;
    if (map_fd <= 0) {
        map_fd = shm_open(cName, O_RDWR | O_CREAT | O_TRUNC, 0664);
        is_new_map = true;
    }
    if (map_fd <= 0) {
        throw_runtime_exception(env, "open map fd failed.");
        return;
    }
    printf("SharedMemery_initialize name : %s", cName);
    env->ReleaseStringUTFChars(str_name, cName);
    // 2. 获取成员变量 length
    jfieldID fId_length = env->GetFieldID(klass,"length","I");
    jint i_length = env->GetIntField(obj, fId_length);
    if (i_length == 0) {
        throw_runtime_exception(env, "length can not be zero.");
        return;
    }
    if (is_new_map) {
        ftruncate(map_fd, i_length);
        // TODO mutex, indexes
    }
    // 3. 设置 fd



}

/*
 * Class:     com_github_preferme_ipc_SharedMemery
 * Method:    destroy
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_com_github_preferme_ipc_SharedMemery_destroy
  (JNIEnv *, jobject) {


}


/*
 * Class:     com_github_preferme_ipc_SharedMemery
 * Method:    writerIndex
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_com_github_preferme_ipc_SharedMemery_writerIndex
  (JNIEnv *, jobject);

/*
 * Class:     com_github_preferme_ipc_SharedMemery
 * Method:    writableBytes
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_com_github_preferme_ipc_SharedMemery_writableBytes
  (JNIEnv *, jobject);

/*
 * Class:     com_github_preferme_ipc_SharedMemery
 * Method:    readerIndex
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_com_github_preferme_ipc_SharedMemery_readerIndex
  (JNIEnv *, jobject);

/*
 * Class:     com_github_preferme_ipc_SharedMemery
 * Method:    readableBytes
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_com_github_preferme_ipc_SharedMemery_readableBytes
  (JNIEnv *, jobject);

/*
 * Class:     com_github_preferme_ipc_SharedMemery
 * Method:    getBytes
 * Signature: (I[BII)Lcom/github/preferme/ipc/SharedMemery;
 */
JNIEXPORT jobject JNICALL Java_com_github_preferme_ipc_SharedMemery_getBytes
  (JNIEnv *, jobject, jint, jbyteArray, jint, jint);

/*
 * Class:     com_github_preferme_ipc_SharedMemery
 * Method:    setBytes
 * Signature: (I[BII)Lcom/github/preferme/ipc/SharedMemery;
 */
JNIEXPORT jobject JNICALL Java_com_github_preferme_ipc_SharedMemery_setBytes
  (JNIEnv *, jobject, jint, jbyteArray, jint, jint);

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
