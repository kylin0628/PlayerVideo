//
// Created by 王奇 on 2020/10/24.
//
#ifndef PLAYERVIDEO_AUDIOCHANNEL_H
#define PLAYERVIDEO_AUDIOCHANNEL_H

#include "BaseChannel.h"
#include <SLES/OpenSLES_Android.h>

extern "C" {
#include "include/libswresample/swresample.h"
};

//音频处理
class AudioChannel : public BaseChannel {

public:
    AudioChannel(int i, JavaCallHelper *pHelper, AVCodecContext *pContext,AVRational time_base);

    ~AudioChannel();

    virtual void paly();

    virtual void stop();

    void initOpenSL();

    void decode();

    int getPcm();

    uint8_t *buffer;
private:
    pthread_t pid_audio_play;//openSL ES初始化线程
    pthread_t pid_audio_Deccode;//音频解码线程

    SwrContext *swrContext = nullptr;//音频转换上下文，（SwsContext是视频的）
    int out_channels;//通道数
    int out_simplesize;//采样位数
    int out_simple_rate;//采样率

};


#endif //PLAYERVIDEO_AUDIOCHANNEL_H
