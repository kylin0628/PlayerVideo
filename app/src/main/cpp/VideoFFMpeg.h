//
// Created by 王奇 on 2020/10/23.
//
#ifndef PLAYERVIDEO_VIDEOFFMPEG_H
#define PLAYERVIDEO_VIDEOFFMPEG_H

#include "pthread.h"
#include "android/native_window.h"
#include "JavaCallHelper.h"
#include "AudioChannel.h"
#include "VideoChannel.h"
#include "macro.h"

extern "C" {
#include "include/libavformat/avformat.h"
#include "include/libavutil/time.h"
};

// 控制层
class VideoFFMpeg {

public:
    //构造方法（和析构方法成对出现）
    VideoFFMpeg(JavaCallHelper *javaCallHelper, const char *dataSource);

    //析构方法（相当于java中inDestory，用于释放）
    ~VideoFFMpeg();

    //准备初始化
    void prepare();

    void prepareFFMpeg();

    void start();

    void play();

    void setRenderFrame(RenderFrame frame);

    int duration;

    void setProgress(int progress);

    void stop();

    void pause();

    void rewind();

    void fastForward();

    bool isPalying{};//播放状态
    bool isPause{};//暂停播放
    char *url;//输入的视频地址（可以是网络地址或者本地路径）
    pthread_t pid_prepare{}; //FFMpeg准备操作线程引用,准备完成销毁
    pthread_t pid_play{}; //FFMpeg播放线程引用，整个播放过程存在
    pthread_t pid_stop{}; //FFMpeg释放停止线程
    AVFormatContext *formatContext{}; //总上下文
    JavaCallHelper *javaCallHelper;
    AudioChannel *audioChannel{};//音频处理类实例
    VideoChannel *videoChannel{};//视频处理类实例
    RenderFrame renderFrame;
    int seek = 0;//是否拖动
    pthread_mutex_t seekMutex;//线程锁
    AVRational display_aspect_ratio;
};


#endif //PLAYERVIDEO_VIDEOFFMPEG_H
