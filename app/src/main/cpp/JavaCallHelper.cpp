//
// Created by 王奇 on 2020/10/24.
//
#include "JavaCallHelper.h"
#include <jni.h>

//初始化赋值
JavaCallHelper::JavaCallHelper(JavaVM *_javaVm, JNIEnv *_env, jobject &_jobj) : javaVm(_javaVm),
                                                                                env(_env) {
    //因为方法执行完jobject就被销毁了，所以全局保存一下，扩大生命周期
    jobj = env->NewGlobalRef(_jobj);
    jclass jclazz = env->GetObjectClass(jobj);
    //反射得到相应的methodid，实际获取的就是artMethod结构体，（阿里热修复也是通过artMethod结构体替换的方式）
    jmid_prepare = env->GetMethodID(jclazz, "onPrepare", "()V");
    jmid_progress = env->GetMethodID(jclazz, "onProgress", "(I)V");
    jmid_error = env->GetMethodID(jclazz, "onError", "(I)V");
    jmid_size = env->GetMethodID(jclazz, "adaptiveSize", "(II)V");
}

JavaCallHelper::~JavaCallHelper() {

}

void JavaCallHelper::onPrepare(int thread) {
    if (thread == THREAD_CHILD) {
        JNIEnv *jniEnv;
        //绑定子线程到jvm
        if (javaVm->AttachCurrentThread(&jniEnv, 0) != JNI_OK) {
            return;
        }
        //注意此处是使用绑定之后的jniEnv调用，而不是默认的
        jniEnv->CallVoidMethod(jobj, jmid_prepare);
        //解除绑定
        javaVm->DetachCurrentThread();
    } else {
        env->CallVoidMethod(jobj, jmid_prepare);
    }
}

void JavaCallHelper::onProgress(int thread, int progress) {
    if (thread == THREAD_CHILD) {
        JNIEnv *jniEnv;
        //绑定子线程到jvm
        if (javaVm->AttachCurrentThread(&jniEnv, 0) != JNI_OK) {
            return;
        }
        //注意此处是使用绑定之后的jniEnv调用，而不是默认的
        jniEnv->CallVoidMethod(jobj, jmid_progress, progress);
        //解除绑定
        javaVm->DetachCurrentThread();
    } else {
        env->CallVoidMethod(jobj, jmid_progress, progress);
    }
}

void JavaCallHelper::onError(int thread, int errorCode) {
    if (thread == THREAD_CHILD) {
        JNIEnv *jniEnv;
        //绑定子线程到jvm
        if (javaVm->AttachCurrentThread(&jniEnv, 0) != JNI_OK) {
            return;
        }
        //注意此处是使用绑定之后的jniEnv调用，而不是默认的
        jniEnv->CallVoidMethod(jobj, jmid_error, errorCode);
        //解除绑定
        javaVm->DetachCurrentThread();
    } else {
        env->CallVoidMethod(jobj, jmid_error, errorCode);
    }
}

void JavaCallHelper::adaptiveSize(int thread, int den, int num) {
    if (thread == THREAD_CHILD) {
        JNIEnv *jniEnv;
        //绑定子线程到jvm
        if (javaVm->AttachCurrentThread(&jniEnv, 0) != JNI_OK) {
            return;
        }
        //注意此处是使用绑定之后的jniEnv调用，而不是默认的
        jniEnv->CallVoidMethod(jobj, jmid_size, den, num);
        //解除绑定
        javaVm->DetachCurrentThread();
    } else {
        env->CallVoidMethod(jobj, jmid_size, den, num);
    }
}




