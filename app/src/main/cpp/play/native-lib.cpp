//
// Created by season on 2018/4/2.
//

#include "jni.h"
#include "string"
#include "FFmpegAudio.h"
#include "FFmpegVideo.h"

#include <android/native_window_jni.h>


extern "C" {

#include <libavformat/avformat.h>
#include <unistd.h>

ANativeWindow *window = 0;
int isPlay = 0;
FFmpegAudio *audio = 0;
FFmpegVideo *video = 0;
pthread_t p_tid;
const char *path;
jstring jpath;
JavaVM *g_jvm = NULL;

void release(JNIEnv *env) {
    if (!jpath) {
        LOGE("没东西释放了")
        return;
    }
    LOGE("开始释放--------------")
    env->ReleaseStringUTFChars(jpath, path);
    env->DeleteGlobalRef(jpath);
    jpath = 0;
    LOGE("释放path字符串完成...")

    if (audio) {
        LOGE("开始释放audio")
        if (audio->isPlay) {
            LOGE("开始停止audio")
            audio->stop();
        }
        LOGE("开始销毁audio")
        delete (audio);
        audio = 0;
    }
    if (video) {
        LOGE("开始释放video")
        if (video->isPlay) {
            LOGE("开始停止video")
            video->stop();
        }
        LOGE("开始销毁video")
        delete (video);
        video = 0;
    }
    if (g_jvm->DetachCurrentThread() != JNI_OK) {
        LOGE("线程解绑JNIEnv失败")
    }

}


int interruptCallback(void *arg) {
    if (isPlay)
        return 0;
    return 1;
}

void *process(void *args) {

    JNIEnv *env;

    if (g_jvm->AttachCurrentThread(&env, NULL) != JNI_OK) {
        LOGE("绑定线程到JNIEnv失败");
        pthread_exit(0);
    }

    LOGE("开启解码线程");
    //1.注册组件
    av_register_all();
    avformat_network_init();

    isPlay = 1;
    AVFormatContext *pFormatContext = avformat_alloc_context();
    pFormatContext->interrupt_callback.callback = interruptCallback;
    pFormatContext->interrupt_callback.opaque = pFormatContext;
    int code;
    //2.打开输入视频文件
    if ((code = avformat_open_input(&pFormatContext, path, NULL, NULL)) != 0) {
        LOGE("错误码：%d  打开视频文件失败: %s", code, path);
        avformat_close_input(&pFormatContext);
        release(env);
        pthread_exit(0);
    }
    //3.获取视频信息
    if ((code = avformat_find_stream_info(pFormatContext, NULL)) < 0) {
        LOGE("错误码：%d  获取视频信息失败: %s", code, path);
        avformat_close_input(&pFormatContext);
        release(env);
        pthread_exit(0);
    }
    for (int i = 0; i < pFormatContext->nb_streams; ++i) {
        AVCodecContext *pCodecCtx = pFormatContext->streams[i]->codec;
        AVCodec *pCodec = avcodec_find_decoder(pCodecCtx->codec_id);
        AVCodecContext *codecCtx = avcodec_alloc_context3(pCodec);
        avcodec_copy_context(codecCtx, pCodecCtx);
        if (avcodec_open2(codecCtx, pCodec, NULL) < 0) {
            LOGE("解码器无法打开");
            continue;
        }

        if (codecCtx->codec_type == AVMEDIA_TYPE_VIDEO) {
            video->setAVCodecContext(codecCtx);
            video->index = i;
            video->time_base = pFormatContext->streams[i]->time_base;
            if (window)
                ANativeWindow_setBuffersGeometry(window, video->codecContext->width,
                                                 video->codecContext->height,
                                                 WINDOW_FORMAT_RGBA_8888);

        } else if (codecCtx->codec_type == AVMEDIA_TYPE_AUDIO) {
            audio->setAvCodecContext(codecCtx);
            audio->index = i;
            audio->time_base = pFormatContext->streams[i]->time_base;
        }
    }

    //编码数据
    AVPacket *packet = (AVPacket *) av_mallocz(sizeof(AVPacket));
//    解码完整个视频 子线程
    int ret;

    video->setFFmpegAudio(audio);
    video->play();
    audio->play();

    while (isPlay) {

        ret = av_read_frame(pFormatContext, packet);
        LOGE("读取一个数据包 结果码：%d", ret);
        if (ret == 0) {

            if (audio && audio->isPlay && packet->stream_index == audio->index) {
                audio->put(packet);
            } else if (video && video->isPlay && packet->stream_index == video->index) {
                video->put(packet);
            }

            av_packet_unref(packet);
        } else if (ret == AVERROR_EOF) {
            // 读完了
            //读取完毕 但是不一定播放完毕
            while (isPlay) {
//                vedio->queue.empty() &&
                if (video->queue.empty() && audio->queue.empty()) {
                    LOGE("全部音视频帧播放完")
                    break;
                }
                LOGE("等待播放完成");
                av_usleep(10000);
            }
            isPlay = 0;
        }
    }
    LOGE("全部音视频帧播放完,准备释放...")

    release(env);
    av_free_packet(packet);
    avformat_close_input(&pFormatContext);
    pthread_exit(0);

}

void call_video_play(AVFrame *frame) {
    if (!window) {
        LOGE("window未初始化")
        return;
    }
    ANativeWindow_Buffer window_buffer;
    if (ANativeWindow_lock(window, &window_buffer, 0) < 0) {
        LOGE("锁定window失败")
        return;
    }

    LOGE("绘制 宽%d,高%d", frame->width, frame->height);
    LOGE("绘制 宽%d,高%d  行字节 %d ", window_buffer.width, window_buffer.height, frame->linesize[0]);
    uint8_t *dst = (uint8_t *) window_buffer.bits;
    int dstStride = window_buffer.stride * 4;
    uint8_t *src = frame->data[0];
    int srcStride = frame->linesize[0];
    for (int i = 0; i < window_buffer.height; ++i) {
        memcpy(dst + i * dstStride, src + i * srcStride, srcStride);
    }
    ANativeWindow_unlockAndPost(window);
}

JNIEXPORT void JNICALL
Java_com_season_example_learnffmpeg_MyPlayer_play0(JNIEnv *env, jobject instance, jstring path_) {

    if (pthread_kill(p_tid, 0) == 0) {
        LOGE("线程已开启")
        return;
    }

    path = env->GetStringUTFChars(path_, 0);
    jpath = (jstring) env->NewGlobalRef(path_);
//        实例化对象
    video = new FFmpegVideo;
    audio = new FFmpegAudio;
    video->setPlayCall(call_video_play);
    if (g_jvm == NULL)
        env->GetJavaVM(&g_jvm);

    pthread_create(&p_tid, NULL, process, NULL);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_season_example_learnffmpeg_MyPlayer_stop0(JNIEnv *env, jobject instance) {
    if (isPlay) {
        isPlay = 0;
        pthread_join(p_tid, 0);
    } else
        release(env);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_season_example_learnffmpeg_MyPlayer_initNativeWindow(JNIEnv *env, jobject instance,
                                                              jobject surface) {

    if (window) {
        ANativeWindow_release(window);
        window = 0;
    }
    window = ANativeWindow_fromSurface(env, surface);
//    if (video && video->codecContext) {
//        ANativeWindow_setBuffersGeometry(window, video->codecContext->width,
//                                         video->codecContext->height,
//                                         WINDOW_FORMAT_RGBA_8888);
//    }
    LOGE("window初始化完成 %p", window);

}
}