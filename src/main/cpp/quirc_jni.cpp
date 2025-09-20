#define LOGV(...) __android_log_print(ANDROID_LOG_VERBOSE, "quirc_jni", __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, "quirc_jni", __VA_ARGS__)

#include <jni.h>
#ifndef ANDROID
#define JNIEXPORT
#endif
#include <string>
#include <vector>
#include <android/log.h>
#include "quirc.h"

extern "C" {

// Создаём контекст quirc и возвращаем указатель как jlong
JNIEXPORT jlong JNICALL
Java_com_example_quirc_QuircNative_nativeInit(JNIEnv *env, jclass clazz) {
    struct quirc *q = quirc_new();
    if (!q) {
        LOGE("quirc_new() failed");
        return 0;
    }
    return reinterpret_cast<jlong>(q);
}

// Освобождаем
JNIEXPORT void JNICALL
Java_com_example_quirc_QuircNative_nativeDestroy(JNIEnv *env, jclass clazz, jlong ctx) {
    struct quirc *q = reinterpret_cast<struct quirc *>(ctx);
    if (q) quirc_destroy(q);
}

// Сканирование: передаётся байтовый массив со значениями яркости (grayscale), ширина и высота
// Возвращаем массив объектов QuircResult
JNIEXPORT jobject JNICALL
Java_com_example_quirc_QuircNative_nativeScan(JNIEnv *env, jclass clazz,
                                              jlong ctx, jbyteArray grayBytes,
                                              jint width, jint height) {
    struct quirc *q = reinterpret_cast<struct quirc *>(ctx);
    if (!q) return nullptr;

    // Проверки размеров
    jsize len = env->GetArrayLength(grayBytes);
    if (len < width * height) {
        LOGE("grayBytes too small: %d < %d", (int)len, width*height);
        return nullptr;
    }

    // Подготовить буфер quirc
    if (quirc_resize(q, width, height) < 0) {
        LOGE("quirc_resize failed");
        return nullptr;
    }

    unsigned char *image = quirc_begin(q, nullptr, nullptr);
    if (!image) {
        LOGE("quirc_begin returned NULL");
        return nullptr;
    }

    // Копируем байты из java-массива в quirc image
    env->GetByteArrayRegion(grayBytes, 0, width * height, reinterpret_cast<jbyte *>(image));

    quirc_end(q);

    int count = quirc_count(q);
    // LOGV("Found regions: %d", count);

    // Подготовим класс результата java: com.example.quirc.QuircNative$QuircResult
    jclass resultClass = env->FindClass("com/example/quirc/QuircNative$QuircResult");
    if (!resultClass) {
        LOGE("Result class not found");
        return nullptr;
    }

    jmethodID resultCtor = env->GetMethodID(resultClass, "<init>", "(Ljava/lang/String;I[I)V");
    if (!resultCtor) {
        LOGE("Result ctor not found");
        return nullptr;
    }

    // jobjectArray resultArray = env->NewObjectArray(count, resultClass, nullptr);

    for (int i = 0; i < count; ++i) {
        struct quirc_code code;
        struct quirc_data data;

        quirc_extract(q, i, &code);

        auto err = quirc_decode(&code, &data);
        if (err) {
            // LOGV("decode error %d", err);
            continue;
            // можно пропустить, но создадим пустой объект с пустой строкой
            // jstring payload = env->NewStringUTF("");
            // jint ecc = 0;
            // jintArray corners = env->NewIntArray(8); // 4 точек x,y
            // env->SetObjectArrayElement(resultArray, i, env->NewObject(resultClass, resultCtor, payload, ecc, corners));
            // continue;
        }

        // payload
        jstring payload = env->NewStringUTF(reinterpret_cast<const char *>(data.payload));
        jint ecc = data.ecc_level;

        // corners: 4 точек (x,y) => 8 ints: x0,y0,x1,y1,x2,y2,x3,y3
        jint corners_buf[8];
        for (int p = 0; p < 4; ++p) {
            corners_buf[p*2 + 0] = code.corners[p].x;
            corners_buf[p*2 + 1] = code.corners[p].y;
        }
        jintArray corners_arr = env->NewIntArray(8);
        env->SetIntArrayRegion(corners_arr, 0, 8, corners_buf);

        jobject resultObj = env->NewObject(resultClass, resultCtor, payload, ecc, corners_arr);
        return resultObj;
    }

    return NULL;
}

} // extern "C"