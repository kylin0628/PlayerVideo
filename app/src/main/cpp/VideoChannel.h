//
// Created by 王奇 on 2020/10/24.
//
#ifndef PLAYERVIDEO_VIDEOCHANNEL_H
#define PLAYERVIDEO_VIDEOCHANNEL_H

#include "BaseChannel.h"
#include "AudioChannel.h"

typedef void (*RenderFrame)(uint8_t *, int, int, int);//创建回调接口，参数：RGBA数据，像素个数，图片宽，高

//视频处理
class VideoChannel : public BaseChannel {

public:
    VideoChannel(int id, JavaCallHelper *pHelper, AVCodecContext *pContext,
                 AVRational time_base);

    ~VideoChannel();

    virtual void paly();

    virtual void stop();

    void decodePacket();

    void synchronizeFrame();

    void setRenderFrame(RenderFrame render);

    void setFps(double fps);

    void setAudioChannel(AudioChannel *audioChannel);

private:
    pthread_t pid_video_play;//解压线程
    pthread_t pid_synchronize;//同步播放线程
    RenderFrame renderFrame;
    double fps;//帧率
    AudioChannel *audioChannel;//获取音频的当前播放时间，我们音视频同步以音频为准，因为人对于声音的敏感度要高于画面，如果声音有丢帧卡顿是很容易察觉的，相对于画面丢帧而言
};


#endif //PLAYERVIDEO_VIDEOCHANNEL_H
