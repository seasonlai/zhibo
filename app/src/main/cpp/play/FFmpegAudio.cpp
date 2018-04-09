//
// Created by season on 2018/4/2.
//

#include "FFmpegAudio.h"

int FFmpegAudio::get(AVPacket *packet) {

    pthread_mutex_lock(&mutex);
    while (isPlay) {
        if (!queue.empty()) {
            if (av_packet_ref(packet, queue.front()) < 0) {
                LOGE("get: 什么情况???");
                break;
            }
            AVPacket *pkt = queue.front();
            queue.pop();
            LOGE("取出一 个音频帧%d    线程号: %u", queue.size(), (unsigned int) pthread_self());
            av_packet_unref(pkt);
            break;
        } else {
            LOGE("阻塞等待音频帧...");
            pthread_cond_wait(&cond, &mutex);
        }
    }
//    pthread_cond_signal(&cond);
    pthread_mutex_unlock(&mutex);
    return 1;
}

int FFmpegAudio::put(AVPacket *packet) {

    AVPacket *packet1 = (AVPacket *) av_mallocz(sizeof(AVPacket));
    if (av_packet_ref(packet1, packet) < 0) {
        LOGE("put: 一帧音频数据克隆失败")
        return -1;
    }

    pthread_mutex_lock(&mutex);
//    while (queue.size()>30) {
//        LOGE("FFmpegAudio 赶紧消费啊")
//        pthread_cond_signal(&cond);
//        pthread_cond_wait(&cond, &mutex);
//    }
    queue.push(packet1);
    LOGE("压入一帧音频数据 size: %d  线程号: %u",queue.size(), (unsigned int) pthread_self());
    pthread_cond_signal(&cond);
    pthread_mutex_unlock(&mutex);
    return 1;
}

void *play_audio(void *args) {
    FFmpegAudio *audio = (FFmpegAudio *) args;
    if (audio->createPlayer() < 0) {
        audio->stop();
    }
    LOGE("创建opsles播放器线程要退出");
    pthread_exit(0);
}

int getPcm(FFmpegAudio *audio) {
    int got_frame;
    int size;
    AVPacket *packet = (AVPacket *) av_mallocz(sizeof(AVPacket));
    AVFrame *frame = av_frame_alloc();
    while (audio->isPlay) {
        size = 0;
        audio->get(packet);
        if (packet->pts != AV_NOPTS_VALUE) {
            audio->clock = av_q2d(audio->time_base) * packet->pts;
        }
//            解码  mp3   编码格式frame----pcm   frame
        LOGE("audio->get完成  codecContext: %p, packet->buff: %p, 线程号: %u",audio->codecContext,packet->buf,(unsigned int) pthread_self());
        avcodec_decode_audio4(audio->codecContext, frame, &got_frame, packet);
        if (got_frame) {
            int re = swr_convert(audio->swrContext, &audio->out_buffer, 44100 * 2,
                                 (const uint8_t **) frame->data, frame->nb_samples);
//                缓冲区的大小
            size = av_samples_get_buffer_size(NULL, audio->out_channel_nb, frame->nb_samples,
                                              AV_SAMPLE_FMT_S16, 1);
            LOGE("转换一帧数据后大小: %d", size);
            break;
        }
    }
    av_free_packet(packet);
    av_frame_free(&frame);
    return size;
}

//第一次主动调用在调用线程
//之后在新线程中回调
void bqPlayCallback(SLAndroidSimpleBufferQueueItf bq, void *context) {
    FFmpegAudio *audio = (FFmpegAudio *) context;
    int dataLen = getPcm(audio);
    LOGE("dataLen: %d", dataLen);
    if (dataLen > 0) {
        double time = dataLen / ((double) 44100 * 2 * 2);
        LOGE("数据长度%d  分母%d  值%f 通道数%d",dataLen,44100 *2 * 2,time,audio->out_channel_nb);
        audio->clock = audio->clock +time;
        LOGE("当前一帧声音时间%f   播放时间%f",time,audio->clock);
        (*bq)->Enqueue(bq, audio->out_buffer, dataLen);
        LOGE("播放 %d ", audio->queue.size());
    } else
        LOGE("解码错误");
}

int FFmpegAudio::createPlayer() {

    SLresult lresult;
    //创建引擎
    lresult = slCreateEngine(&engineObject, 0, NULL, 0, NULL, NULL);
    if (SL_RESULT_SUCCESS != lresult) {
        LOGE("engineObject创建失败");
        return -1;
    }
    // 实现引擎engineObject
    lresult = (*engineObject)->Realize(engineObject, SL_BOOLEAN_FALSE);
    if (SL_RESULT_SUCCESS != lresult) {
        LOGE("engineObject实现引擎失败");
        return -1;
    }
    // 获取引擎接口engineEngine
    lresult = (*engineObject)->GetInterface(engineObject, SL_IID_ENGINE, &engineEngine);
    if (SL_RESULT_SUCCESS != lresult) {
        LOGE("获取引擎接口engineEngine失败");
        return -1;
    }
    // 创建混音器outputMixObject
    lresult = (*engineEngine)->CreateOutputMix(engineEngine, &outputMixObject, 0, 0, 0);
    if (SL_RESULT_SUCCESS != lresult) {
        LOGE("创建混音器outputMixObject失败,错误码：%d", lresult);
        return -1;
    }
    lresult = (*outputMixObject)->Realize(outputMixObject, SL_BOOLEAN_FALSE);
    if (SL_RESULT_SUCCESS != lresult) {
        LOGE("混音器实现失败");
        return -1;
    }
    lresult = (*outputMixObject)->GetInterface(outputMixObject, SL_IID_ENVIRONMENTALREVERB,
                                               &outputMixEnvironmentalReverb);
    const SLEnvironmentalReverbSettings settings = SL_I3DL2_ENVIRONMENT_PRESET_DEFAULT;
    //这里貌似获取不了啊
    if (SL_RESULT_SUCCESS == lresult) {
        LOGE("支持3D环绕");
        (*outputMixEnvironmentalReverb)->SetEnvironmentalReverbProperties(
                outputMixEnvironmentalReverb,
                &settings);
    }
    //======================
    SLDataLocator_AndroidBufferQueue androidBufferQueue =
            {SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE, 2};
    SLDataFormat_PCM pcm = {SL_DATAFORMAT_PCM, 2, SL_SAMPLINGRATE_44_1, SL_PCMSAMPLEFORMAT_FIXED_16,
                            SL_PCMSAMPLEFORMAT_FIXED_16,
                            SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT,
                            SL_BYTEORDER_LITTLEENDIAN};
//   新建一个数据源 将上述配置信息放到这个数据源中
    SLDataSource slDataSource = {&androidBufferQueue, &pcm};
    //设置混音器
    SLDataLocator_OutputMix outputMix = {SL_DATALOCATOR_OUTPUTMIX, outputMixObject};

    SLDataSink audioSink = {&outputMix, NULL};
    const SLInterfaceID ids[3] = {SL_IID_BUFFERQUEUE, SL_IID_EFFECTSEND, SL_IID_VOLUME};
    const SLboolean req[3] = {SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE};
    lresult = (*engineEngine)->CreateAudioPlayer(engineEngine, &bqPlayerObject, &slDataSource,
                                                 &audioSink, 2,
                                                 ids, req);
    if (lresult != SL_RESULT_SUCCESS) {
        LOGE("创建播放器失败，错误码%d", lresult);
        return -1;
    }
    //初始化播放器
    lresult = (*bqPlayerObject)->Realize(bqPlayerObject, SL_BOOLEAN_FALSE);
    LOGE("bqPlayerObject->Realize，结果码 %d", lresult);

//    得到接口后调用  获取Player接口
    lresult = (*bqPlayerObject)->GetInterface(bqPlayerObject, SL_IID_PLAY, &bqPlayerPlay);
    LOGE("bqPlayerObject->GetInterface(bqPlayerPlay)，结果码 %d", lresult);

//    注册回调缓冲区 //获取缓冲队列接口
    lresult = (*bqPlayerObject)->GetInterface(bqPlayerObject, SL_IID_BUFFERQUEUE,
                                              &bqPlayerBufferQueue);
    LOGE("bqPlayerObject->GetInterface(bqPlayerBufferQueue)，结果码 %d", lresult);
    lresult = (*bqPlayerBufferQueue)->RegisterCallback(bqPlayerBufferQueue, bqPlayCallback, this);
    LOGE("bqPlayerBufferQueue->RegisterCallback(bqPlayCallback)，结果码 %d", lresult);
    //    获取音量接口
    lresult = (*bqPlayerObject)->GetInterface(bqPlayerObject, SL_IID_VOLUME, &bqPlayerVolume);
    LOGE("bqPlayerObject->GetInterface(bqPlayerVolume)，结果码 %d", lresult);

//    获取播放状态接口
    lresult = (*bqPlayerPlay)->SetPlayState(bqPlayerPlay, SL_PLAYSTATE_PLAYING);
    LOGE("bqPlayerPlay->SetPlayState(SL_PLAYSTATE_PLAYING)，结果码 %d", lresult);

    bqPlayCallback(bqPlayerBufferQueue, this);
    return 1;
}

void FFmpegAudio::play() {
    isPlay = 1;
    pthread_create(&p_playId, NULL, play_audio, this);
}

void FFmpegAudio::stop() {
    LOGE("声音暂停");
    pthread_mutex_lock(&mutex);
    isPlay = 0;
    pthread_cond_signal(&cond);
    pthread_mutex_unlock(&mutex);
    pthread_join(p_playId, 0);
    if (bqPlayerPlay) {
        (*bqPlayerPlay)->SetPlayState(bqPlayerPlay, SL_PLAYSTATE_STOPPED);
        bqPlayerPlay = 0;
    }
    if (bqPlayerObject) {
        (*bqPlayerObject)->Destroy(bqPlayerObject);
        bqPlayerObject = 0;
        bqPlayerBufferQueue = 0;
        bqPlayerVolume = 0;
    }
    if (outputMixObject) {
        (*outputMixObject)->Destroy(engineObject);
        engineObject = 0;
        engineEngine = 0;
    }
    if (swrContext)
        swr_free(&swrContext);
    if (codecContext) {
        if (avcodec_is_open(codecContext)) {
            avcodec_close(codecContext);
        }
        avcodec_free_context(&codecContext);
        codecContext = 0;
    }
    LOGE("AUDIO 释放内存完毕");
}


FFmpegAudio::FFmpegAudio() {
    clock=0;
    isPlay=0;
    pthread_mutex_init(&mutex, NULL);
    pthread_cond_init(&cond, NULL);
}

FFmpegAudio::~FFmpegAudio() {
    LOGE("~FFmpegAudio")
    if (out_buffer) {
        free(out_buffer);
    }
    for (int i = 0; i < queue.size(); ++i) {
        AVPacket *pPacket = queue.front();
        queue.pop();
        LOGE("销毁音频帧%d", i);
        av_free_packet(pPacket);
    }
    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&cond);
}


int createFFmpeg(FFmpegAudio *audio) {
//    mp3  里面所包含的编码格式   转换成  pcm   SwcContext
    audio->swrContext = swr_alloc();
//    44100*2
    audio->out_buffer = (uint8_t *) av_mallocz(44100 * 2);
    uint64_t out_ch_layout = AV_CH_LAYOUT_STEREO;
//    输出采样位数  16位
    enum AVSampleFormat out_format = AV_SAMPLE_FMT_S16;
//输出的采样率必须与输入相同
    int out_sample_rate = audio->codecContext->sample_rate;


    swr_alloc_set_opts(audio->swrContext, out_ch_layout, out_format, out_sample_rate,
                       audio->codecContext->channel_layout, audio->codecContext->sample_fmt,
                       audio->codecContext->sample_rate, 0,
                       NULL);

    swr_init(audio->swrContext);
    LOGE("swr_is_initialized: %d", swr_is_initialized(audio->swrContext));

//    获取通道数  2
    audio->out_channel_nb = av_get_channel_layout_nb_channels(AV_CH_LAYOUT_STEREO);
    LOGE("------>通道数%d  ", audio->out_channel_nb);
    return 1;
}

void FFmpegAudio::setAvCodecContext(AVCodecContext *avCodecContext) {
    codecContext = avCodecContext;
    LOGE("获取视频编码器上下文 %p  ", codecContext);
//    加密的用不了
    AVCodec *pCodex = avcodec_find_decoder(codecContext->codec_id);
    LOGE("获取视频编码 %p", pCodex);
    if (avcodec_open2(codecContext, pCodex, NULL) < 0) {
        LOGE("FFmpegAudio解码器打开失败");
    }
    createFFmpeg(this);
}

