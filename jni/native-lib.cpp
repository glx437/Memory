#include <jni.h>
#include <android/log.h>
#include <vector>
#include <sstream>
#include <string>
#include "KittyMemory/KittyScanner.h"
#include "KittyMemory/KittyMemory.h"

#define LOG_TAG "NativeMem"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

static std::vector<uintptr_t> g_last_scan_results;

extern "C" {

JNIEXPORT jlongArray JNICALL
Java_memory_example_com_MainActivity_nativeScan(JNIEnv* env, jobject thiz, jint target_value) {
    g_last_scan_results.clear();
    KittyScanner::findDataAll(&target_value, sizeof(target_value), g_last_scan_results);
    jlongArray ret = env->NewLongArray((jsize)g_last_scan_results.size());
    if (ret == nullptr) return nullptr;
    if (!g_last_scan_results.empty()) {
        env->SetLongArrayRegion(ret, 0, (jsize)g_last_scan_results.size(), (const jlong*)g_last_scan_results.data());
    }
    return ret;
}

JNIEXPORT jint JNICALL
Java_memory_example_com_MainActivity_nativeRead(JNIEnv* env, jobject thiz, jlong address) {
    uintptr_t addr = (uintptr_t)address;
    if (addr == 0) return -1;
    int value = 0;
    KittyMemory::memRead(&value, (void*)addr, sizeof(int));
    return value;
}

JNIEXPORT jboolean JNICALL
Java_memory_example_com_MainActivity_nativeWrite(JNIEnv* env, jobject thiz, jlong address, jint value) {
    uintptr_t addr = (uintptr_t)address;
    if (addr == 0) return JNI_FALSE;
    if (!KittyMemory::isAddressValid((void*)addr)) {
        LOGI("write invalid address %p", (void*)addr);
        return JNI_FALSE;
    }
    KittyMemory::memWrite((void*)addr, &value, sizeof(int));
    return JNI_TRUE;
}

JNIEXPORT jint JNICALL
Java_memory_example_com_MainActivity_nativeBatchWrite(JNIEnv* env, jobject thiz, jstring values_str) {
    if (g_last_scan_results.empty()) return 0;
    const char* str = env->GetStringUTFChars(values_str, nullptr);
    if (str == nullptr) return 0;
    std::string input(str);
    env->ReleaseStringUTFChars(values_str, str);
    std::vector<int> values;
    std::stringstream ss(input);
    std::string token;
    while (std::getline(ss, token, ',')) {
        try {
            values.push_back(std::stoi(token));
        } catch (...) {
            LOGI("batch write: invalid token %s", token.c_str());
        }
    }
    int count = 0;
    size_t limit = (values.size() < g_last_scan_results.size()) ? values.size() : g_last_scan_results.size();
    for (size_t i = 0; i < limit; ++i) {
        uintptr_t addr = g_last_scan_results[i];
        if (KittyMemory::isAddressValid((void*)addr)) {
            KittyMemory::memWrite((void*)addr, &values[i], sizeof(int));
            count++;
        }
    }
    return count;
}

}
