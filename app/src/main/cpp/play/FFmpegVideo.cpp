//
// Created by season on 2018/4/6.
//

#include "FFmpegVideo.h"

static void (*video_call)(AVFrame *frame);

FFmpegVideo::FFmpegVideo() {
    pthread_mutex_init(&mutex, NULL);
    pthread_cond_init(&cond, NULL);
}

FFmpegVideo::~FFmpegVideo() {

    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&cond);

}

void *play_video(void *arg) {

    FFmpegVideo *video = (FFmpegVideo *) arg;

    LOGE("开启播放线程")
    //像素数据（解码数据）
    AVFrame *frame = av_frame_alloc();

    AVFrame *rgb_frame = av_frame_alloc();
    //给缓冲区分配内存
    //只有指定了AVFrame的像素格式、画面大小才能真正分配内存
    //缓冲区分配内存
    uint8_t *out_buffer = (uint8_t *) av_mallocz(
            avpicture_get_size(AV_PIX_FMT_RGBA, video->codecContext->width,
                               video->codecContext->height));
    LOGE("宽  %d,  高  %d  ", video->codecContext->width, video->codecContext->height);
    //设置frame的缓冲区，像素格式
    int re = avpicture_fill((AVPicture *) rgb_frame, out_buffer, AV_PIX_FMT_RGBA,
                            video->codecContext->width, video->codecContext->height);
    LOGE("申请内存%d   ", re);


    //转换rgba
    SwsContext *sws_ctx = sws_getContext(
            video->codecContext->width, video->codecContext->height, video->codecContext->pix_fmt,
            video->codecContext->width, video->codecContext->height, AV_PIX_FMT_RGBA,
            SWS_BILINEAR, 0, 0, 0);

    int got_frame;
    //编码数据
    AVPacket *packet = (AVPacket *) av_mallocz(sizeof(AVPacket));
    double last_play  //上一帧的播放时间
    , play             //当前帧的播放时间
    , last_delay    // 上一次播放视频的两帧视频间隔时间
    , delay         //两帧视频间隔时间
    , audio_clock //音频轨道 实际播放时间
    , diff   //音频帧与视频帧相差时间
    , sync_threshold
    , start_time  //从第一帧开始的绝对时间
    , pts
    , actual_delay//真正需要延迟时间
    ;//两帧间隔合理间隔时间
    start_time = av_gettime() / 1000000.0;
    while (video->isPlay) {
//        消费者取到一帧数据  没有 阻塞
        video->get(packet);
        LOGE("视频解码 一帧 %d", video->queue.size());
        avcodec_decode_video2(video->codecContext, frame, &got_frame, packet);
        if (!got_frame) {
            continue;
        }

//        转码成rgb
        sws_scale(sws_ctx, (const uint8_t *const *) frame->data, frame->linesize, 0,
                  video->codecContext->height,
                  rgb_frame->data, rgb_frame->linesize);

        av_packet_unref(packet);


        if ((pts = av_frame_get_best_effort_timestamp(frame)) == AV_NOPTS_VALUE) {
            pts = 0;
        }

//        纠正时间

        play = pts * av_q2d(video->time_base);

        play = video->synchronize(frame, play);

        delay = play - last_play;
        if (delay < 0 || delay > 1) {
            delay = last_delay;
        }

        audio_clock = video->audio->clock;
        last_delay = delay;
        last_play = play;

        diff = video->clock - audio_clock;

        sync_threshold = delay > 0.01 ? 0.01 : delay;

//        if (fabs(diff) < 10) {
            if (diff <= -sync_threshold) {
                delay = 0;
            } else if (diff >= sync_threshold) {
                delay = 2 * delay;
            }
//        }

        start_time += delay;
        actual_delay = start_time - av_gettime() / 1000000.0;

        if (actual_delay < 0.01) {
            actual_delay = 0.01;
        }

        av_usleep(actual_delay * 1000000.0 + 6000);
        video_call(rgb_frame);
    }

    LOGE("free packet");
    av_freep(packet);
    LOGE("free packet ok");
    LOGE("free frame");
    av_frame_free(&frame);
    av_frame_free(&rgb_frame);
    LOGE("free frame ok");
    sws_freeContext(sws_ctx);
    LOGE("free sws_ctx ok");
    size_t size = video->queue.size();
    for (int i = 0; i < size; ++i) {
        AVPacket *pkt = video->queue.front();
        av_free_packet(pkt);
        video->queue.pop();
    }
    LOGE("VIDEO EXIT");
    pthread_exit(0);

}

void FFmpegVideo::play() {
    isPlay = 1;
    pthread_create(&p_playid, 0, play_video, this);
}

void FFmpegVideo::stop() {
    LOGE("VIDEO stop");

    pthread_mutex_lock(&mutex);
    isPlay = 0;
    //因为可能卡在 deQueue
    pthread_cond_signal(&cond);
    pthread_mutex_unlock(&mutex);

    pthread_join(p_playid, 0);
    LOGE("VIDEO join pass");
    if (this->codecContext) {
        if (avcodec_is_open(this->codecContext))
            avcodec_close(this->codecContext);
        avcodec_free_context(&this->codecContext);
        this->codecContext = 0;
    }
    LOGE("VIDEO close");
}

int FFmpegVideo::put(AVPacket *packet) {

    AVPacket *pkt = (AVPacket *) av_mallocz(sizeof(AVPacket));
    if (av_packet_ref(pkt, packet) < 0) {
        LOGE("put - 复制packet出错")
        return -1;
    }
    pthread_mutex_lock(&mutex);
//        while (queue.size() > 30) {
//            LOGE("赶紧消费啊")
//            pthread_cond_signal(&cond);
//            pthread_cond_wait(&cond, &mutex);
//        }
    queue.push(pkt);
    LOGE("压入一帧视频数据 size: %d   线程号：%u", queue.size(), (unsigned int) pthread_self());
    pthread_cond_signal(&cond);
    pthread_mutex_unlock(&mutex);
    return 1;
}

int FFmpegVideo::get(AVPacket *packet) {
    pthread_mutex_lock(&mutex);
    while (isPlay) {
        if (!queue.empty()) {
            if (av_packet_ref(packet, queue.front()) < 0) {
                LOGE("get - 复制packet出错")
                break;
            }
            AVPacket *pkt = queue.front();
            queue.pop();
            LOGE("取出一帧视频数据  线程号：%u", (unsigned int) pthread_self());
            av_packet_unref(pkt);
            break;
        } else {
            pthread_cond_wait(&cond, &mutex);
        }
    }
//    pthread_cond_signal(&cond);
    pthread_mutex_unlock(&mutex);
    return 0;
}

void FFmpegVideo::setAVCodecContext(AVCodecContext *avCodecContext) {
    codecContext = avCodecContext;
}

void FFmpegVideo::setPlayCall(void (*call)(AVFrame *)) {
    video_call = call;
}

void FFmpegVideo::setFFmpegAudio(FFmpegAudio *audio1) {
    this->audio = audio1;
}

double FFmpegVideo::synchronize(AVFrame *frame, double play) {
    //clock是当前播放的时间位置
    if (play != 0)
        clock = play;
    else //pst为0 则先把pts设为上一帧时间
        play = clock;

    //可能有pts为0 则主动增加clock
    //frame->repeat_pict = 当解码时，这张图片需要要延迟多少
    //需要求出扩展延时：
    //extra_delay = repeat_pict / (2*fps) 显示这样图片需要延迟这么久来显示
    double repeat_pict = frame->repeat_pict;
    //使用AvCodecContext的而不是stream的
    double frame_delay = av_q2d(codecContext->time_base);
    //如果time_base是1,25 把1s分成25份，则fps为25
    //fps = 1/(1/25)
    double fps = 1 / frame_delay;
    //pts 加上 这个延迟 是显示时间
    double extra_delay = repeat_pict / (2 * fps);
    double delay = extra_delay + frame_delay;
//    LOGI("extra_delay:%f",extra_delay);
    clock += delay;
    return play;
}
