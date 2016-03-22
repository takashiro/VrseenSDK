/*
 * logcat_cpp.h
 *
 *  Created on: 2016年3月6日
 *      Author: yangkai
 */
#pragma once
#include <android/log.h>
#define _LOGD(LOG_TAG, ...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define _LOGV(LOG_TAG, ...) __android_log_print(ANDROID_LOG_VERBOSE,  LOG_TAG, __VA_ARGS__)
#define _LOGI(LOG_TAG, ...) __android_log_print(ANDROID_LOG_INFO  ,  LOG_TAG, __VA_ARGS__)
#define _LOGW(LOG_TAG, ...) __android_log_print(ANDROID_LOG_WARN  ,  LOG_TAG, __VA_ARGS__)
#define _LOGE(LOG_TAG, ...) __android_log_print(ANDROID_LOG_ERROR  ,  LOG_TAG, __VA_ARGS__)
