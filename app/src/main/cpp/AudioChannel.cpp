//
// Created by 王奇 on 2020/10/24.
//
#include "AudioChannel.h"

AudioChannel::AudioChannel(int i, JavaCallHelper *pHelper, AVCodecContext *pContext,
                           AVRational time_base) : BaseChannel(
        i, pHelper, pContext, time_base) {
    pkt_queue.setReleaseHandle(releaseAvPacket);
    frame_queue.setReleaseHandle(releaseAvFrame);
    //根据布局获取通道数
    out_channels = av_get_channel_layout_nb_channels(AV_CH_LAYOUT_STEREO);
    out_simplesize = av_get_bytes_per_sample(AV_SAMPLE_FMT_S16);
    out_simple_rate = 44100;
    //CD音频标准 44100 双声道 2字节(out_simplesize)
    buffer = static_cast<uint8_t *>(malloc(out_simple_rate * out_simplesize * out_channels));
}

AudioChannel::~AudioChannel() {
    sloge(" ~AudioChannel 释放成功");
    free(buffer);
    buffer = nullptr;
    if (swrContext) {
        swr_close(swrContext);
        swr_free(&swrContext);
        swrContext = nullptr;
    }
}

void *audioPlay(void *args) {
    AudioChannel *audio = static_cast<AudioChannel *>(args);
    audio->initOpenSL();
    return nullptr;
}

void *audioDecode(void *args) {
    AudioChannel *audio = static_cast<AudioChannel *>(args);
    audio->decode();
    return nullptr;
}


void AudioChannel::paly() {
    if (!codecContext) {
        sloge("codecContext paly 为 null");
        return;
    }
    //输出是指定的，输入是可变的
    swrContext = swr_alloc_set_opts(nullptr, AV_CH_LAYOUT_STEREO, AV_SAMPLE_FMT_S16,
                                    out_simple_rate,//输出
                                    codecContext->channel_layout,//输入
                                    codecContext->sample_fmt,
                                    codecContext->sample_rate, 0, nullptr);
    if (!codecContext) {
        sloge("swrContext paly 为 null");
        return;
    }
    swr_init(swrContext);
    pkt_queue.setWork(1);
    frame_queue.setWork(1);
    isPlaying = true;

    //创建初始化OpenSL ES线程
    pthread_create(&pid_audio_play, nullptr, audioPlay, this);
    //    创建初始化音频解码线程
    pthread_create(&pid_audio_Deccode, nullptr, audioDecode, this);
}

void AudioChannel::stop() {
    isPlaying = false;
    pthread_join(pid_audio_play, nullptr);
    pthread_join(pid_audio_Deccode, nullptr);
}

void bqPlayerCallback(SLAndroidSimpleBufferQueueItf bq, void *context) {
    auto *audioChannel = static_cast<AudioChannel *>(context);
    int datalen = audioChannel->getPcm();
    //放入缓冲队列播放
    if (datalen > 0) {
        (*bq)->Enqueue(bq, audioChannel->buffer, datalen);
    }
}

void AudioChannel::initOpenSL() {
    //1. 音频引擎
    SLEngineItf engineInterface = nullptr;
    // 2. engine interfaces 音频对象
    SLObjectItf engineObject = nullptr;
    // 3. output mix interfaces 混音器
    SLObjectItf outputMixObject = nullptr;
    //4.  buffer queue player interfaces 播放器
    SLObjectItf bqPlayerObject = nullptr;
    //   5.  回调接口
    SLPlayItf bqPlayerInterface;
    //  6.  缓冲队列
    SLAndroidSimpleBufferQueueItf bqPlayerBufferQueue;

    //---------------------------初始化播放引擎 start---------------------------------
    SLresult result;
    result = slCreateEngine(&engineObject, 0, nullptr, 0, nullptr, nullptr);
    assert(SL_RESULT_SUCCESS == result);
    (void) result;

    result = (*engineObject)->Realize(engineObject, SL_BOOLEAN_FALSE);
    assert(SL_RESULT_SUCCESS == result);
    (void) result;

    // get the engine interface, which is needed in order to create other objects 获取到音频接口相当于SurfaceHolder
    result = (*engineObject)->GetInterface(engineObject, SL_IID_ENGINE, &engineInterface);
    assert(SL_RESULT_SUCCESS == result);
    (void) result;
    //---------------------------初始化播放引擎 end---------------------------------

    //---------------------------初始化混音器 start---------------------------------
    // create output mix, with environmental reverb specified as a non-required interface
    const SLInterfaceID idss[1] = {SL_IID_ENVIRONMENTALREVERB};
    const SLboolean reqs[1] = {SL_BOOLEAN_FALSE};
    result = (*engineInterface)->CreateOutputMix(engineInterface, &outputMixObject, 1, idss, reqs);
    // 初始化混音器outputMixObject
    result = (*outputMixObject)->Realize(outputMixObject, SL_BOOLEAN_FALSE);
    assert(SL_RESULT_SUCCESS == result);
    (void) result;
    //---------------------------初始化混音器 end---------------------------------

    //---------------------------初始化音频对象 start---------------------------------
    SLDataLocator_AndroidSimpleBufferQueue loc_bufq = {SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE, 2};
    //TODO 声道数为1的时候报错，不是很理解
    SLDataFormat_PCM format_pcm = {SL_DATAFORMAT_PCM,//播放PCM的数据
                                   2,//声道数量（立体声）
                                   SL_SAMPLINGRATE_44_1, //44100hz的频率
                                   SL_PCMSAMPLEFORMAT_FIXED_16,//位数
                                   SL_PCMSAMPLEFORMAT_FIXED_16,//和上面位数保持一致就可以
                                   SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT,//声音环绕参数(此处立体声)，
                                   SL_BYTEORDER_LITTLEENDIAN //小端模式
    }; //参数设置

    SLDataSource audioSrc = {&loc_bufq, &format_pcm};

    SLDataLocator_OutputMix loc_outmix = {SL_DATALOCATOR_OUTPUTMIX, outputMixObject};
    SLDataSink audioSnk = {&loc_outmix, nullptr};
    const SLInterfaceID ids[3] = {SL_IID_BUFFERQUEUE, SL_IID_VOLUME, SL_IID_EFFECTSEND};
    const SLboolean req[3] = {SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE};

    result = (*engineInterface)->CreateAudioPlayer(engineInterface,//音频引擎
                                                   &bqPlayerObject,//播放器
                                                   &audioSrc,//播放器的参数，缓冲队列，播放格式
                                                   &audioSnk,//播放缓冲区
                                                   1, //播放接口回调个数
                                                   ids,//设置播放队列ID
                                                   req //是否采取内置播放器队列
    );
    assert(SL_RESULT_SUCCESS == result);
    (void) result;

    // realize the player初始化播放器
    result = (*bqPlayerObject)->Realize(bqPlayerObject, SL_BOOLEAN_FALSE);
    assert(SL_RESULT_SUCCESS == result);
    (void) result;
    // get the play interface  得到接口后调用 获取Player接口
    result = (*bqPlayerObject)->GetInterface(bqPlayerObject, SL_IID_PLAY, &bqPlayerInterface);
    assert(SL_RESULT_SUCCESS == result);
    (void) result;
    // get the buffer queue interface  获得播放接口
    result = (*bqPlayerObject)->GetInterface(bqPlayerObject, SL_IID_BUFFERQUEUE,
                                             &bqPlayerBufferQueue);
    assert(SL_RESULT_SUCCESS == result);
    (void) result;

    // register callback on the buffer queue
    result = (*bqPlayerBufferQueue)->RegisterCallback(bqPlayerBufferQueue, bqPlayerCallback, this);
    assert(SL_RESULT_SUCCESS == result);
    (void) result;
    //设置播放状态
    (*bqPlayerInterface)->SetPlayState(bqPlayerInterface, SL_PLAYSTATE_PLAYING);
    slogd("手动调用播放 packet：%d", pkt_queue.size());
    //手动调用播放
    bqPlayerCallback(bqPlayerBufferQueue, this);
}

void AudioChannel::decode() {
    //解码过程和视频一样
    AVPacket *packet = nullptr;
    while (isPlaying) {
        int ret = pkt_queue.deQueue(packet);
        if (!isPlaying) {
            break;
        }
        if (!ret) {
            continue;
        }
        if (!codecContext || !packet) {
            slogd("codecContext = null");
            return;
        }
        ret = avcodec_send_packet(codecContext, packet);
        releaseAvPacket(packet);
        if (ret == AVERROR(EAGAIN)) {
            continue;
        } else if (ret < 0) {
            break;
        }
        AVFrame *avFrame = av_frame_alloc();
        ret = avcodec_receive_frame(codecContext, avFrame);
        if (ret == AVERROR(EAGAIN)) {
            continue;
        } else if (ret < 0) {
            break;
        }
        while (frame_queue.size() > 100 && isPlaying) {
            av_usleep(10 * 1000);
        }
        frame_queue.enQueue(avFrame);
        if (isPlaying) {
            sloge("audiochannel 解码进行中 isPlaying : true %d", pkt_queue.size());
        } else {
            sloge("audiochannel 解码进行中 isPlaying : false");
        }
    }
}

int AudioChannel::getPcm() {
    AVFrame *avFrame = nullptr;
    int data_size = 0;//转换后的数据大小
    while (isPlaying) {
        if (isPause) {
            continue;
        }
        int ret = frame_queue.deQueue(avFrame);
        if (!isPlaying) { break; }
        if (!ret) { continue; }
        if (!swrContext || !avFrame) {
            sloge("swrContext = null");
            return 0;
        }
        uint64_t dst_nb_samples = av_rescale_rnd(
                swr_get_delay(swrContext, avFrame->sample_rate) + avFrame->nb_samples,
                out_simple_rate, avFrame->sample_rate, AV_ROUND_UP);
        //转换，返回值为转换后sample的个数
        int nb = swr_convert(swrContext, &buffer, dst_nb_samples, (const uint8_t **) avFrame->data,
                             avFrame->nb_samples);

        data_size = nb * out_simplesize * out_channels;
        //播放之前来获取clock,音频pts和dts相同
        clock = avFrame->pts * av_q2d(time_base);
        if (javaCallHelper) {
            (*javaCallHelper).onProgress(THREAD_CHILD, clock);
        }
        break;
    }
    releaseAvFrame(avFrame);
    return data_size;
}
