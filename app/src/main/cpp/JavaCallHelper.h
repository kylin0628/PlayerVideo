//
// Created by 王奇 on 2020/10/24.
//
#ifndef PLAYERVIDEO_JAVACALLHELPER_H
#define PLAYERVIDEO_JAVACALLHELPER_H

#include "macro.h"
#include <jni.h>
#include <string>

//native层反射获取java层方法的辅助类
class JavaCallHelper {
public:
    JavaCallHelper(JavaVM *_javaVm, JNIEnv *_env, jobject &_jobj);

    ~JavaCallHelper();

    void onPrepare(int thread);

    void onProgress(int thread, int progress);

    void onError(int thread, int errorCode);

    void adaptiveSize(int thread, int den, int num);

private:
    JavaVM *javaVm;
    JNIEnv *env;
    jobject jobj;
    jmethodID jmid_prepare;
    jmethodID jmid_progress;
    jmethodID jmid_error;
    jmethodID jmid_size;

};


#endif //PLAYERVIDEO_JAVACALLHELPER_H
