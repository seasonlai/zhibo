//
// Created by season on 2018/4/2.
//

#ifndef LEARNFFMPEG_FFMPEGAUDIO_H
#define LEARNFFMPEG_FFMPEGAUDIO_H

#include <queue>
#include <SLES/OpenSLES_Android.h>
#include <SLES/OpenSLES.h>
#include <pthread.h>

extern "C" {
#include "Log.h"
#include "libavutil/error.h"
#include <libavcodec/avcodec.h>
#include <libswresample/swresample.h>
class FFmpegAudio {

public:
    void setAvCodecContext(AVCodecContext *avCodecContext);

    int get(AVPacket *packet);

    int put(AVPacket *packet);

    void play();

    void stop();

    int createPlayer();

    FFmpegAudio();

    ~FFmpegAudio();

public:

    //是否在播放
    int isPlay = 0;
    //流索引
    int index = -1;
    //音频队列
    std::queue<AVPacket *>queue;
    //播放线程id
    pthread_t p_playId;
    //同步锁
    pthread_mutex_t mutex;
    //条件变量
    pthread_cond_t cond;

    AVCodecContext *codecContext;
    uint8_t *out_buffer=0;
    int out_channel_nb;
    double clock;
    AVRational time_base;

    SwrContext *swrContext;


    SLObjectItf engineObject;
    SLEngineItf engineEngine;
    SLEnvironmentalReverbItf outputMixEnvironmentalReverb;
    SLObjectItf outputMixObject;
    SLObjectItf bqPlayerObject;
    SLEffectSendItf bqPlayerEffectSend;
    SLVolumeItf bqPlayerVolume;
    SLPlayItf bqPlayerPlay;
    SLAndroidSimpleBufferQueueItf bqPlayerBufferQueue;

};
};

#endif //LEARNFFMPEG_FFMPEGAUDIO_H
