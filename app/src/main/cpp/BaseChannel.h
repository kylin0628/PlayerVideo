//
// Created by 王奇 on 2020/10/24.
//
#ifndef PLAYERVIDEO_BASECHANNEL_H
#define PLAYERVIDEO_BASECHANNEL_H

#include "safe_queue.h"
#include <pthread.h>
#include "JavaCallHelper.h"

extern "C" {
#include "include/libavcodec/avcodec.h"
#include "include/libswscale/swscale.h"
#include "include/libavutil/imgutils.h"
#include "include/libavutil/time.h"
};

class BaseChannel {
public:
    BaseChannel(int id, JavaCallHelper *callHelper, AVCodecContext *avCodecContext,
                AVRational time_base) : channelId(id),
                                        javaCallHelper(
                                                callHelper),
                                        codecContext(
                                                avCodecContext), time_base(time_base) {};

    //释放packet
    static void releaseAvPacket(AVPacket *&packet) {
        if (packet) {
            av_packet_free(&packet);
            packet = 0;
        }
    }

    //释放frame
    static void releaseAvFrame(AVFrame *&frame) {
        if (frame) {
            av_frame_free(&frame);
            frame = 0;
        }
    }

    virtual ~BaseChannel() {
        sloge(" ~BaseChannel 释放成功");
        if (codecContext) {
            avcodec_close(codecContext);
            avcodec_free_context(&codecContext);
            codecContext = 0;
        }
        DELETE(javaCallHelper);
    }

    virtual void paly() = 0;

    virtual void stop() {
        isPlaying = false;
        stopWork();
        clear();
    };

    //pkt_queue.setReleaseHandle(releaseAvPacket);
    //frame_queue.setReleaseHandle(releaseAvFrame);注意要设置释放函数，clear的时候会触发释放函数去释放帧
    void clear() {
        pkt_queue.clear();
        frame_queue.clear();
    }

    void stopWork() {
        pkt_queue.setWork(0);
        frame_queue.setWork(0);
    }

    void startWork() {
        pkt_queue.setWork(1);
        frame_queue.setWork(1);
    }

    void pause() {
        isPause = !isPause;
    }

    SafeQueue<AVPacket *> pkt_queue;//压缩包队列
    SafeQueue<AVFrame *> frame_queue;//解压数据队列

    volatile int channelId;
    volatile bool isPlaying;
    AVCodecContext *codecContext;
    JavaCallHelper *javaCallHelper;
    AVRational time_base;//相对时间基数，单位，比如1/10，每一个刻度就是1，1/0.5每一刻度就是0.1，作为pts的基数
    double clock = 0;//音频相对的时间线，走到哪里
    bool isPause;
};

#endif //PLAYERVIDEO_BASECHANNEL_H
