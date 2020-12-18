#include <jni.h>
#include <string>
#include <android/log.h>
#include <android/native_window_jni.h>

#include "logutil.h"
#include "VideoFFMpeg.h"
#include "JavaCallHelper.h"
#include "Test.h"

Test *test;
ANativeWindow *window = nullptr;//最终渲染到native层的window
JavaCallHelper *javaCallHelper;
VideoFFMpeg *videoFfMpeg;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;//给渲染加把锁
//子线程要想回调java层就必须要把线程绑定jvm实例，所以接下来要回调java层的子线程都必须绑定jvm
JavaVM *javaVm = nullptr;
//通过该函数获取JVM实例，该函数写在这就会主动调用
JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved) {
    javaVm = vm;
    return JNI_VERSION_1_4;
}

//进行渲染，把VideoChannel里面解析的参数到此处，因为有窗口ANativeWindow W,H窗体宽高
void renderFrame(uint8_t *data, int linesize, int w, int h) {
    pthread_mutex_lock(&mutex);
    if (!window) {
        pthread_mutex_unlock(&mutex);
        return;
    }

    //渲染，设置窗口属性
    ANativeWindow_setBuffersGeometry(window, w, h, WINDOW_FORMAT_RGBA_8888);
    ANativeWindow_Buffer buffer;
    if (ANativeWindow_lock(window, &buffer, nullptr)) {
        ANativeWindow_release(window);
        window = nullptr;
        pthread_mutex_unlock(&mutex);
        return;
    }
    //接下来把data数据拷贝到buffer中，实现渲染,但是拷贝过程不能用for循环一个个拷贝，效率太低，所以for循环使用一行一行的拷贝的方式（不整体拷贝的原因是因为数据源和buffer宽度可能不一样）
    //获取buffer里面的缓冲区
    uint8_t *bits = static_cast<uint8_t *>(buffer.bits);
    int window_linesize = buffer.stride * 4;//stride是一行的像素，是一个字节。因为rgba所以*4
    uint8_t *src_data = data;//数据源

    for (int i = 0; i < buffer.height; ++i) {
        //拷贝每一行向下移动，并且数据源和目的地各自偏移各自的
        memcpy(bits + i * window_linesize, src_data + i * linesize,
               window_linesize);//拷贝，window的缓冲区，源数据，长度（以显示的长度目的地为准）
    }
    //释放
    ANativeWindow_unlockAndPost(window);
    pthread_mutex_unlock(&mutex);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_kylin_playervideo_VideoPlayer_native_1prepare(JNIEnv *env, jobject thiz, jstring source) {
    //获取地址
    const char *dataSource = env->GetStringUTFChars(source, nullptr);
    if (!javaCallHelper) {
        javaCallHelper = new JavaCallHelper(javaVm, env, thiz);
        //初始化控制层
        videoFfMpeg = new VideoFFMpeg(javaCallHelper, dataSource);
        //调用准备方法做FFMpeg初始化
        videoFfMpeg->prepare();
        videoFfMpeg->setRenderFrame(renderFrame);
    }
    env->ReleaseStringUTFChars(source, dataSource);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_kylin_playervideo_VideoPlayer_native_1start(JNIEnv *env, jobject thiz) {
    //开始播放状态
    if (videoFfMpeg) {
        videoFfMpeg->start();
    }
}

extern "C"
JNIEXPORT void JNICALL
Java_com_kylin_playervideo_VideoPlayer_native_1setSurface(JNIEnv *env, jobject thiz,
                                                          jobject surface) {
    //释放掉window，比如横竖屏切换的时候,全屏非全屏切换的时候需要处理
    if (window) {
        ANativeWindow_release(window);
        window = nullptr;
    }
    window = ANativeWindow_fromSurface(env, surface); //根据java层的surfaceview创建native绘制的window
}

extern "C"
JNIEXPORT jstring JNICALL
Java_com_kylin_playervideo_VideoPlayer_native_1test(JNIEnv *env, jobject thiz) {
    jobject job = env->NewGlobalRef(thiz);
    jclass jclazz = env->GetObjectClass(job);
    jmethodID testMethodID = env->GetMethodID(jclazz, "onTest", "(I)V");
    test = new Test();
    env->CallVoidMethod(job, testMethodID, test->getInt());
    return env->NewStringUTF(test->getIntString());
}

extern "C"
JNIEXPORT jint JNICALL
Java_com_kylin_playervideo_VideoPlayer_native_1duration(JNIEnv *env, jobject thiz) {
    if (videoFfMpeg) {
        return videoFfMpeg->duration;
    }
    return 0;
}

extern "C"
JNIEXPORT void JNICALL
Java_com_kylin_playervideo_VideoPlayer_native_1seek(JNIEnv *env, jobject thiz, jint progress) {
    if (videoFfMpeg) {
        videoFfMpeg->setProgress(progress);
    }
}

extern "C"
JNIEXPORT void JNICALL
Java_com_kylin_playervideo_VideoPlayer_native_1stop(JNIEnv *env, jobject thiz) {
    if (videoFfMpeg) {
        videoFfMpeg->stop();
    }
    DELETE(javaCallHelper);
    DELETE(videoFfMpeg);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_kylin_playervideo_VideoPlayer_native_1release(JNIEnv *env, jobject thiz) {
    if (window) {
        ANativeWindow_release(window);
        window = nullptr;
    }
}

extern "C"
JNIEXPORT void JNICALL
Java_com_kylin_playervideo_VideoPlayer_native_1pause(JNIEnv *env, jobject thiz) {
    if (videoFfMpeg) {
        videoFfMpeg->pause();
    }
}extern "C"
JNIEXPORT void JNICALL
Java_com_kylin_playervideo_VideoPlayer_native_1wind(JNIEnv *env, jobject thiz) {
    if (videoFfMpeg) {
        videoFfMpeg->rewind();
    }
}extern "C"
JNIEXPORT void JNICALL
Java_com_kylin_playervideo_VideoPlayer_native_1fastForward(JNIEnv *env, jobject thiz) {
    if (videoFfMpeg) {
        videoFfMpeg->fastForward();
    }
}