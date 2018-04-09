//
// Created by season on 2018/4/8.
//
#include <jni.h>
#include "../play/Log.h"

extern "C" {
/**
 * 设置视频
 */
extern "C"
JNIEXPORT void JNICALL
Java_com_season_example_learnffmpeg_pusher_PusherNative_setVideoOptions(JNIEnv *env,
                                                                        jobject instance,
                                                                        jint width, jint height,
                                                                        jint bitrate, jint fps) {




}

/**
 * 接受视频流
 */
extern "C"
JNIEXPORT void JNICALL
Java_com_season_example_learnffmpeg_pusher_PusherNative_fireVideo(JNIEnv *env, jobject instance,
                                                                  jbyteArray buffer_) {
    jbyte *buffer = env->GetByteArrayElements(buffer_, NULL);


    env->ReleaseByteArrayElements(buffer_, buffer, 0);
}

/**
 * 推流
 */
extern "C"
JNIEXPORT jboolean JNICALL
Java_com_season_example_learnffmpeg_pusher_PusherNative_startPush(JNIEnv *env, jobject instance,
                                                                  jstring url_) {
    const char *url = env->GetStringUTFChars(url_, 0);

    LOGE("开始推流：%s",url);

    env->ReleaseStringUTFChars(url_, url);
    return 1;
}

/**
 * 设置音频
 */
extern "C"
JNIEXPORT void JNICALL
Java_com_season_example_learnffmpeg_pusher_PusherNative_setAudioOptions(JNIEnv *env,
                                                                        jobject instance,
                                                                        jint sampleRate,
                                                                        jint channel) {


}

/**
 * 接受音频流
 */
extern "C"
JNIEXPORT void JNICALL
Java_com_season_example_learnffmpeg_pusher_PusherNative_fireAudio(JNIEnv *env, jobject instance,
                                                                  jbyteArray buffer_, jint len) {
    jbyte *buffer = env->GetByteArrayElements(buffer_, NULL);


    env->ReleaseByteArrayElements(buffer_, buffer, 0);
}

extern "C"
JNIEXPORT jint JNICALL
Java_com_season_example_learnffmpeg_pusher_PusherNative_getInputSamples(JNIEnv *env,
                                                                        jobject instance) {


    return 0;
}


/**
 * 停止推流
 */
extern "C"
JNIEXPORT void JNICALL
Java_com_season_example_learnffmpeg_pusher_PusherNative_stopPush(JNIEnv *env, jobject instance) {

    // TODO

}

/**
 * 释放内存
 */
extern "C"
JNIEXPORT void JNICALL
Java_com_season_example_learnffmpeg_pusher_PusherNative_release(JNIEnv *env, jobject instance) {

    // TODO

}

}