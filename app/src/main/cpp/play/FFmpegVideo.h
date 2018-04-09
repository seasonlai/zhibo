//
// Created by season on 2018/4/3.
//

#ifndef LEARNFFMPEG_FFMPEGVIDEO_H
#define LEARNFFMPEG_FFMPEGVIDEO_H

#include "FFmpegAudio.h"
extern "C"{
#include <libavutil/time.h>
#include <libswscale/swscale.h>

class FFmpegVideo{

public:
    FFmpegVideo();
    ~FFmpegVideo();

    void play();
    void stop();

    int put(AVPacket *);
    int get(AVPacket *);

    void setAVCodecContext(AVCodecContext *);

    void setPlayCall(void (*call)(AVFrame* frame));

    double synchronize(AVFrame *, double);
    void setFFmpegAudio(FFmpegAudio *);
public:
    std::queue<AVPacket *> queue;

    AVCodecContext *codecContext;

    int isPlay = 0;

    int index = -1;
//    SwsContext *swsContext;

    pthread_t p_playid;

//    同步锁
    pthread_mutex_t mutex;
//    条件变量
    pthread_cond_t cond;

    FFmpegAudio* audio;
    AVRational time_base;
    double  clock;
};

};
#endif //LEARNFFMPEG_FFMPEGVIDEO_H
