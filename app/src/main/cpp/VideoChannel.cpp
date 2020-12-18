//
// Created by 王奇 on 2020/10/24.
//
#include "VideoChannel.h"


void dropFrame(queue<AVFrame *> &f) {
    //丢弃视频帧
    slogd("丢弃视频帧!");
    while (!f.empty()) {
        AVFrame *frame = f.front();
        f.pop();//丢弃
        BaseChannel::releaseAvFrame(frame);//释放
    }
}

VideoChannel::VideoChannel(int id, JavaCallHelper *pHelper, AVCodecContext *pContext,
                           AVRational time_base)
        : BaseChannel(id, pHelper, pContext, time_base) {
    pkt_queue.setReleaseHandle(releaseAvPacket);
    frame_queue.setReleaseHandle(releaseAvFrame);
    frame_queue.setSyncHandle(dropFrame);
}

void *decode(void *args) {
    auto *videoChannel = static_cast<VideoChannel *>(args);
    videoChannel->decodePacket();
    return nullptr;
}

void *synchronize(void *args) {
    auto *videoChannel = static_cast<VideoChannel *>(args);
    videoChannel->synchronizeFrame();
    return nullptr;
}

void VideoChannel::paly() {
    startWork();
    isPlaying = true;
    pthread_create(&pid_video_play, nullptr, decode, this);//创建解码AVPacket数据包线程，获取解码每一帧数据
    pthread_create(&pid_synchronize, nullptr, synchronize, this);//创建同步播放线程
}

void VideoChannel::stop() {
    isPlaying = false;
    pthread_join(pid_video_play, nullptr);
    pthread_join(pid_synchronize, nullptr);
}

void VideoChannel::decodePacket() {
    //子线程
    AVPacket *packet = nullptr;
    while (isPlaying) {
        int ret = pkt_queue.deQueue(packet);
        if (!isPlaying) { break; }
        if (!ret) {
            continue;
        }

        if (!codecContext || !packet) {
            sloge("codecContext packet 为 null");
            releaseAvPacket(packet);
            return;
        }
        ret = avcodec_send_packet(codecContext, packet);
        releaseAvPacket(packet);
        if (ret == AVERROR(EAGAIN)) {
            continue;
        } else if (ret < 0) {
            //失败
            break;
        }
        AVFrame *avFrame = av_frame_alloc();
        ret = avcodec_receive_frame(codecContext, avFrame);
        frame_queue.enQueue(avFrame);
        slogd("frame_queue = %d", frame_queue.size());
        //和packet队列一样如果frame队列大于一定大小也休眠一段时间
        while (frame_queue.size() > 100 && isPlaying) {
            av_usleep(10 * 1000);
        }
        sloge("videochannel 解码       进行中 pkt_queue : %d", pkt_queue.size());
    }
    //保险起见，以免最后一帧未被释放，可要可不要
    releaseAvPacket(packet);
}

void VideoChannel::synchronizeFrame() {
    if (!codecContext) {
        slogd("codecContext = null");
        return;
    }
    //获取转换上下文
    SwsContext *swsContext = sws_getContext(codecContext->width, codecContext->height,
                                            codecContext->pix_fmt,
                                            codecContext->width, codecContext->height,
                                            AV_PIX_FMT_RGBA, SWS_BILINEAR, nullptr, nullptr,
                                            nullptr);
    uint8_t *dst_pointers[4];
    int dst_linesizes[4];
    av_image_alloc(dst_pointers, dst_linesizes,
                   codecContext->width, codecContext->height, AV_PIX_FMT_RGBA, 1);
    AVFrame *avFrame = nullptr;
    while (isPlaying) {
        slogd("isPlaying = %d", isPlaying);
        if (isPause) {
            continue;
        }
        int ret = frame_queue.deQueue(avFrame);
        if (!isPlaying) {
            break;
        }
        if (!ret) {
            continue;
        }
        if (!swsContext || !avFrame) {
            slogd("swsContext = null");
            return;
        }
        //转换
        sws_scale(swsContext, avFrame->data, avFrame->linesize, 0, avFrame->height, dst_pointers,
                  dst_linesizes);
        //把数据回调出去，回调出去的rgb数据只需要拿到第0个，所以dst_pointers[0]
        renderFrame(dst_pointers[0], dst_linesizes[0], codecContext->width, codecContext->height);
        //做音视频同步处理，利用延时时间和视频丢帧的方式
        //延迟时间 = 编码时间 + 解码时间,把解码时间算进去，是因为配置差的手机会有差异
        clock = avFrame->pts * av_q2d(time_base);

        double frame_delay = 1.0 / fps;
        double audioClick = audioChannel->clock;
        double extra_delay = avFrame->repeat_pict / (2 * fps);
        double delay = frame_delay + extra_delay;

        double diff = clock - audioClick;
        //TODO  始终相差1，问题，待解决
        sloge("同步相差diff:%d  audioClick:%d   clock:%d", diff, audioClick, clock);
        if (clock > audioClick) {
            //视频超前
            if (diff > 1) {
                av_usleep(delay * 2 * 1000000);//多休眠一倍的时间
            } else {
                av_usleep((delay + diff) * 1000000);
            }
        } else {
            //视频延后
            if (diff > 1) {
                //不休眠
            } else if (diff >= 0.05) {
                //采用丢帧的方式视频追赶音频(采用丢弃Frame的方式，因为packet是没有解码的，并且我们渲染是frame里面的，对packet首先要考虑丢关键帧或者非关键帧，其实哪怕你丢完了packet，frame还是会导致不同步)
                releaseAvFrame(avFrame);//先丢掉当前帧
                frame_queue.sync();//再丢掉缓存
            }
        }
//        av_usleep(16 * 1000);
        //释放掉当前的frame
        releaseAvFrame(avFrame);
    }
    //清理
    av_freep(&dst_pointers[0]);
    isPlaying = false;
    releaseAvFrame(avFrame);
    sws_freeContext(swsContext);
}

void VideoChannel::setRenderFrame(RenderFrame render) {
    this->renderFrame = render;
}

void VideoChannel::setAudioChannel(AudioChannel *channel) {
    this->audioChannel = channel;
}

void VideoChannel::setFps(double videoFps) {
    this->fps = videoFps;
}

VideoChannel::~VideoChannel() {
    sloge(" ~VideoChannel 释放成功");
//    DELETE(audioChannel);
};;


