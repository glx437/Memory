#include <jni.h>
#include <android/log.h>
#include <vector>
#include <sstream>
#include <string>
#include <cstdlib>
#include "KittyMemory/KittyMemory.hpp"
#include "KittyMemory/KittyScanner.hpp"

#define LOG_TAG "NativeMem"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

static std::vector<uintptr_t> g_last_scan_results;

extern "C" {

JNIEXPORT jlongArray JNICALL
Java_memory_example_com_NativeBridge_nativeScan(JNIEnv* env, jclass clazz, jint target_value, jint permMask) {
g_last_scan_results.clear();
KittyScanner::setPatternScanSafeMode(true);
auto maps = KittyMemory::getAllMaps();
for (const auto& map : maps) {
if (!map.isValid() || !map.readable) continue;
bool allowed = false;
if ((permMask & 1) && map.writeable && !map.executable) allowed = true;
if ((permMask & 2) && !map.writeable && !map.executable) allowed = true;
if ((permMask & 4) && map.executable) allowed = true;
if ((permMask & 8) && map.readable && !map.writeable && !map.executable) allowed = true;
if (!allowed) continue;
auto results = KittyScanner::findDataAll(map.startAddress, map.endAddress, &target_value, sizeof(target_value));
g_last_scan_results.insert(g_last_scan_results.end(), results.begin(), results.end());
}
jlongArray ret = env->NewLongArray((jsize)g_last_scan_results.size());
if (ret == nullptr) return nullptr;
if (!g_last_scan_results.empty()) {
env->SetLongArrayRegion(ret, 0, (jsize)g_last_scan_results.size(), (const jlong*)g_last_scan_results.data());
}
return ret;
}

JNIEXPORT jint JNICALL
Java_memory_example_com_NativeBridge_nativeRead(JNIEnv* env, jclass clazz, jlong address) {
uintptr_t addr = (uintptr_t)address;
if (addr == 0) return -1;
int value = 0;
if (!KittyMemory::memRead(addr, &value, sizeof(int))) return -1;
return value;
}

JNIEXPORT jboolean JNICALL
Java_memory_example_com_NativeBridge_nativeWrite(JNIEnv* env, jclass clazz, jlong address, jint value) {
uintptr_t addr = (uintptr_t)address;
if (addr == 0) return JNI_FALSE;
bool ok = KittyMemory::memWrite(addr, &value, sizeof(int));
return ok ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jint JNICALL
Java_memory_example_com_NativeBridge_nativeBatchWrite(JNIEnv* env, jclass clazz, jstring values_str) {
if (g_last_scan_results.empty()) return 0;
const char* str = env->GetStringUTFChars(values_str, nullptr);
if (str == nullptr) return 0;
std::string input(str);
env->ReleaseStringUTFChars(values_str, str);
std::vector<int> values;
std::stringstream ss(input);
std::string token;
while (std::getline(ss, token, ',')) {
if (token.empty()) continue;
int v = (int)strtol(token.c_str(), nullptr, 10);
values.push_back(v);
}
int count = 0;
size_t limit = (values.size() < g_last_scan_results.size()) ? values.size() : g_last_scan_results.size();
for (size_t i = 0; i < limit; ++i) {
uintptr_t addr = g_last_scan_results[i];
if (addr != 0 && KittyMemory::memWrite(addr, &values[i], sizeof(int))) {
count++;
}
}
return count;
}

JNIEXPORT jboolean JNICALL
Java_memory_example_com_NativeBridge_nativeIsWritable(JNIEnv* env, jclass clazz, jlong address) {
uintptr_t addr = (uintptr_t)address;
if (addr == 0) return JNI_FALSE;
auto map = KittyMemory::getAddressMap(addr);
if (!map.isValid()) return JNI_FALSE;
return map.writeable ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jintArray JNICALL
Java_memory_example_com_NativeBridge_nativeReadBatch(JNIEnv* env, jclass clazz, jlongArray addresses) {
jsize len = env->GetArrayLength(addresses);
jlong* addrs = env->GetLongArrayElements(addresses, nullptr);
jintArray ret = env->NewIntArray(len);
if (ret == nullptr) {
env->ReleaseLongArrayElements(addresses, addrs, JNI_ABORT);
return nullptr;
}
jint* vals = env->GetIntArrayElements(ret, nullptr);
for (jsize i = 0; i < len; i++) {
int value = -1;
if (addrs[i] != 0) {
KittyMemory::memRead((uintptr_t)addrs[i], &value, sizeof(int));
}
vals[i] = value;
}
env->ReleaseIntArrayElements(ret, vals, 0);
env->ReleaseLongArrayElements(addresses, addrs, JNI_ABORT);
return ret;
}

}
