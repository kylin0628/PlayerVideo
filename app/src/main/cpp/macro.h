//
// Created by 王奇 on 2020/10/24.
//
#ifndef PLAYERVIDEO_MACRO_H
#define PLAYERVIDEO_MACRO_H


#define DELETE(obj) if(obj){delete obj; obj = 0;}

//标记线程，因为子线程需要attach绑定
#define THREAD_MAIN 1
#define THREAD_CHILD 2

//错误码
#define FFMPEG_CAN_NOT_OPEN_URL 1    //打不开视频
#define FFMPEG_CAN_NOT_FIND_STREAMS 2    //找不到流媒体
#define FFMPEG_FIND_DECODER_FAIL 3    //找不到解码器
#define FFMPEG_ALLOC_CODEC_CONTEXT_FAIL 4    //无法根据解码器创建上下文
#define FFMPEG_CODE_CONTEXT_PARAMETERS_FAIL 6    //根据流信息，配置上下文参数失败
#define FFMPEG_OPEN_DECODER_FAIL 7    //打开解码器失败
#define FFMPEG_NOMEDIA 8    //没有音视频


#endif //PLAYERVIDEO_MACRO_H
