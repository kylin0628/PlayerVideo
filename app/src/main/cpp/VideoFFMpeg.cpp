//
// Created by 王奇 on 2020/10/23.
//
#include "VideoFFMpeg.h"
#include "logutil.h"


void *prepareFFMpeg_(void *arg) {
    //此处无法直接访问对象的this
    auto *videoFfMpeg = static_cast<VideoFFMpeg *>(arg);
    videoFfMpeg->prepareFFMpeg();
    return nullptr;
}


VideoFFMpeg::VideoFFMpeg(JavaCallHelper *javaCallHelper, const char *dataSource) {
//因为传入DataSource之后外面会env->ReleaseStringUTFChars(source, dataSource);执行被释放，所以要赋值一份地址保存
    url = new char[strlen(dataSource) + 1];
    strcpy(url, dataSource);
    this->javaCallHelper = javaCallHelper;
    pthread_mutex_init(&seekMutex, 0);
}

VideoFFMpeg::~VideoFFMpeg() {
    sloge("~VideoFFMpeg");
};

void VideoFFMpeg::prepare() {
    //FFMpeg初始化是个耗时操作,所以需要子线程
    //开辟线程
    pthread_create(&pid_prepare, nullptr, prepareFFMpeg_, this);
}

//该方法就执行在子线程pid_prepare，并且能访问到对象this的属性
void VideoFFMpeg::prepareFFMpeg() {
    avformat_network_init();//1. 初始化网络
    if (formatContext) {
        avformat_close_input(&formatContext);
        avformat_free_context(formatContext);
    }
    formatContext = avformat_alloc_context();//2. 初始化总上下文

    //3. 设置打开文件的一些参数没比如timeout文件打开超时时间
    AVDictionary *options = nullptr;
    av_dict_set(&options, "timeout", "3000000", 0);
    int ret = avformat_open_input(&formatContext, url, nullptr,
                                  &options);//打开文件，函数返回值int判断函数是否执行正确 0代表成功
    if (ret && javaCallHelper) {
        javaCallHelper->onError(THREAD_CHILD, ret);
        return;
    }
    //. 4查找流，如果找不到直接回调java层通知
    if (avformat_find_stream_info(formatContext, nullptr) < 0) {
        if (javaCallHelper) {
            javaCallHelper->onError(THREAD_CHILD, FFMPEG_CAN_NOT_FIND_STREAMS);
        }
        return;
    }
    //formatContext->nb_streams流的个数
    int nbstrems = formatContext->nb_streams;
    duration = formatContext->duration / 1000000;//获取视频时长
    for (int i = 0; i < nbstrems; ++i) {
        AVStream *stream = formatContext->streams[i];
        AVCodecParameters *codecpar = stream->codecpar;//获取流解码的参数
        AVCodec *dec = avcodec_find_decoder(codecpar->codec_id);//根据id找到解码器
        if (!dec) {
            if (javaCallHelper) {
                javaCallHelper->onError(THREAD_CHILD, FFMPEG_FIND_DECODER_FAIL);
            }
            return;
        }
        //5. 创建解码器上下文
        AVCodecContext *codecContext = avcodec_alloc_context3(dec);
        if (!codecContext) {
            if (javaCallHelper) {
                javaCallHelper->onError(THREAD_CHILD, FFMPEG_ALLOC_CODEC_CONTEXT_FAIL);
            }
            return;
        }

        //6. 复制参数
        if (avcodec_parameters_to_context(codecContext, codecpar) < 0) {
            if (javaCallHelper) {
                javaCallHelper->onError(THREAD_CHILD, FFMPEG_CODE_CONTEXT_PARAMETERS_FAIL);
            }
            return;
        }

        //7. 打开解码器
        if (avcodec_open2(codecContext, dec, nullptr) < 0) {
            if (javaCallHelper) {
                javaCallHelper->onError(THREAD_CHILD, FFMPEG_OPEN_DECODER_FAIL);
            }
            return;
        }


        //8. 音频处理
        if (codecpar->codec_type == AVMEDIA_TYPE_AUDIO && javaCallHelper && codecContext) {
            audioChannel = new AudioChannel(i, javaCallHelper, codecContext, stream->time_base);
        } else if (codecpar->codec_type == AVMEDIA_TYPE_VIDEO && javaCallHelper &&
                   codecContext) {//视频处理
            av_reduce(&display_aspect_ratio.num, &display_aspect_ratio.den,
                      codecpar->width * (int64_t) stream->sample_aspect_ratio.num,
                      codecpar->height * (int64_t) stream->sample_aspect_ratio.den,
                      1024 * 1024);

            sloge("display_aspect_ratio： %d:%d",
                  display_aspect_ratio.den,
                  display_aspect_ratio.num);
            if (javaCallHelper) {
                javaCallHelper->adaptiveSize(THREAD_CHILD, display_aspect_ratio.den,
                                             display_aspect_ratio.num);
            }
            videoChannel = new VideoChannel(i, javaCallHelper, codecContext, stream->time_base);
            videoChannel->setRenderFrame(renderFrame);
            AVRational frame_rate = stream->avg_frame_rate;//获取帧率
            videoChannel->setFps(av_q2d(frame_rate));
        }
    }
    videoChannel->setAudioChannel(audioChannel);
    //音视频都没有
    if (!audioChannel && !videoChannel) {
        if (javaCallHelper) {
            javaCallHelper->onError(THREAD_CHILD, FFMPEG_NOMEDIA);
        }
        return;
    }
    if (javaCallHelper) {
        javaCallHelper->onPrepare(THREAD_CHILD);
    }

}

void *statThread(void *args) {
    auto *videoFfMpeg = static_cast<VideoFFMpeg *>(args);
    videoFfMpeg->play();
    return nullptr;
}


void VideoFFMpeg::start() {
    isPalying = true;
    //播放过程的方法是音视频都有，所以抽象base，处理音视频各自的play
    if (videoChannel) {
        videoChannel->paly();
    }
    if (audioChannel) {
        audioChannel->paly();
    }
    //读取保存AVPacket
    pthread_create(&pid_play, nullptr, statThread, this);
}

void VideoFFMpeg::play() {
    //子线程
    int ret = 0;
    while (isPalying) {
        if (isPause) {
            continue;
        }
        //因为渲染的速度明显会大于读取的速度，所以我们在当队列达到一定值的时候让其休眠等待一定时间10ms
        if (audioChannel && audioChannel->pkt_queue.size() > 100) {
            av_usleep(1000 * 10);
            continue;
        }
        if (videoChannel && videoChannel->pkt_queue.size() > 100) {
            av_usleep(1000 * 10);
            continue;
        }
        //读取包
        AVPacket *avPacket = av_packet_alloc();
        if (!avPacket || !formatContext) {
            return;
        }
        //从媒体中读取音视频包
        ret = av_read_frame(formatContext, avPacket);
        if (ret == 0) {
            if (videoChannel && avPacket->stream_index == videoChannel->channelId) {
                //保存视频packet
                videoChannel->pkt_queue.enQueue(avPacket);
            } else if (audioChannel && avPacket->stream_index == audioChannel->channelId) {
                //保存音频packet
                audioChannel->pkt_queue.enQueue(avPacket);
            }
        } else if (AVERROR_EOF == ret) {
            //读取完毕，电脑上不一定播放完成
            if (videoChannel && audioChannel && videoChannel->pkt_queue.empty() &&
                audioChannel->pkt_queue.empty() &&
                videoChannel->frame_queue.empty() && audioChannel->frame_queue.empty()) {
                slogd("播放完毕！");
                av_free_packet(avPacket);
                break;
            } else {
                if (javaCallHelper) {
                    javaCallHelper->onError(THREAD_CHILD, FFMPEG_NOMEDIA);
                }
            }
        } else {
            break;
        }
        sloge("videoFFMpeg 解码进行中 ret:%d",ret);
    }
    sloge("videoFFMpeg 跳出while循环");
    isPalying = false;
    audioChannel->stop();
    videoChannel->stop();
}

void VideoFFMpeg::setRenderFrame(RenderFrame frame) {
    this->renderFrame = frame;
}

void VideoFFMpeg::setProgress(int progress) {
    //拖动范围是否在时间内
    if (progress >= 0 && progress <= duration && formatContext) {
        pthread_mutex_lock(&seekMutex);
        seek = 1;//拖动中
        int seekes = progress * 1000000;//单位要转换成微妙
        //-1代表音视频一起拖动，AVSEEK_FLAG_BACKWARD代表异步拖动 av_seek_frame调用之前会把已经解码的给清掉，调用之后就是最新的数据
        av_seek_frame(formatContext, -1, seekes, AVSEEK_FLAG_BACKWARD);
        //清空队列，保证拖动没有延迟
        if (videoChannel) {
            videoChannel->stopWork();
            videoChannel->clear();
            videoChannel->startWork();
        }
        if (audioChannel) {
            audioChannel->stopWork();
            audioChannel->clear();
            audioChannel->startWork();
        }
        pthread_mutex_unlock(&seekMutex);
        seek = 0;
    }
}

void *asyn_stop(void *args) {
    auto *videoFfMpeg = static_cast<VideoFFMpeg *>(args);
    videoFfMpeg->isPalying = false;
    videoFfMpeg->duration = 0;
    pthread_mutex_destroy(&videoFfMpeg->seekMutex);
//    pthread_join(videoFfMpeg->pid_prepare, nullptr);
//    pthread_join(videoFfMpeg->pid_play, nullptr);
//    if (videoFfMpeg->formatContext) {
//        avformat_close_input(&videoFfMpeg->formatContext);
//        avformat_free_context(videoFfMpeg->formatContext);
//        videoFfMpeg->formatContext = nullptr;
//    }
//    DELETE(videoFfMpeg->javaCallHelper);
//    DELETE(videoFfMpeg->url);
//    DELETE(videoFfMpeg->audioChannel);
//    DELETE(videoFfMpeg->videoChannel);
//    DELETE(videoFfMpeg);

    sloge("asyn_stop");
    return nullptr;
}

void VideoFFMpeg::stop() {
    //释放也耗时间
    pthread_create(&pid_stop, nullptr, asyn_stop, this);
}

void VideoFFMpeg::pause() {
    isPause = !isPause;
    videoChannel->pause();
    audioChannel->pause();
}

void VideoFFMpeg::rewind() {
    setProgress(audioChannel->clock - 30);
}

void VideoFFMpeg::fastForward() {
    setProgress(audioChannel->clock + 30);
}
