//
// Created by season on 2018/4/2.
//

#ifndef LEARNFFMPEG_LOG_H_H
#define LEARNFFMPEG_LOG_H_H
#include <android/log.h>

#define LOGE(FORMAT,...) __android_log_print(ANDROID_LOG_ERROR,"season",FORMAT,##__VA_ARGS__);

#endif //LEARNFFMPEG_LOG_H_H
