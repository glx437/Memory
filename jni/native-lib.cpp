#include <jni.h>
#include <android/log.h>
#include <vector>
#include <sstream>
#include <string>
#include <cstdlib>
#include <cstring>
#include "KittyMemory/KittyMemory.hpp"
#include "KittyMemory/KittyScanner.hpp"

#define LOG_TAG "NativeMem"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

static std::vector<uintptr_t> g_last_scan_results;

static bool is_app_map(const KittyMemory::ProcMap& map) {
if (map.pathname.empty()) return false;
return map.pathname.find("/data/") != std::string::npos ||
map.pathname.find("/data/user/") != std::string::npos;
}

static bool is_system_map(const KittyMemory::ProcMap& map) {
if (map.pathname.empty()) return false;
return map.pathname.find("/system/") != std::string::npos ||
map.pathname.find("/apex/") != std::string::npos ||
map.pathname.find("/vendor/") != std::string::npos;
}

static bool is_anonymous_map(const KittyMemory::ProcMap& map) {
if (map.pathname.empty()) return true;
if (map.pathname[0] != '/') return true;
if (map.pathname.find("[anon:") != std::string::npos) return true;
if (map.pathname.find("[heap]") != std::string::npos) return true;
if (map.pathname.find("[stack]") != std::string::npos) return true;
return false;
}

static bool is_scope_allowed(const KittyMemory::ProcMap& map, jint scopeMask) {
if (scopeMask == 0) return true;
bool app = is_app_map(map);
bool sys = is_system_map(map);
bool anon = is_anonymous_map(map);
if ((scopeMask & 1) && app) return true;
if ((scopeMask & 2) && sys) return true;
if ((scopeMask & 4) && anon) return true;
return false;
}

extern "C" {

JNIEXPORT jlongArray JNICALL
Java_memory_example_com_Starter_nativeScanInt(JNIEnv* env, jclass clazz, jint target_value, jint permMask, jint scopeMask, jint limit) {
g_last_scan_results.clear();
KittyScanner::setPatternScanSafeMode(true);
auto maps = KittyMemory::getAllMaps();
for (const auto& map : maps) {
if (g_last_scan_results.size() >= (size_t)limit) break;
if (!map.isValid() || !map.readable) continue;
if (!is_scope_allowed(map, scopeMask)) continue;
bool allowed = false;
if ((permMask & 1) && map.writeable && !map.executable) allowed = true;
if ((permMask & 2) && !map.writeable && !map.executable) allowed = true;
if ((permMask & 4) && map.executable) allowed = true;
if ((permMask & 8) && map.readable && !map.writeable && !map.executable) allowed = true;
if (!allowed) continue;
auto results = KittyScanner::findDataAll(map.startAddress, map.endAddress, &target_value, sizeof(target_value));
for (auto addr : results) {
if (g_last_scan_results.size() >= (size_t)limit) break;
g_last_scan_results.push_back(addr);
}
}
jlongArray ret = env->NewLongArray((jsize)g_last_scan_results.size());
if (ret == nullptr) return nullptr;
if (!g_last_scan_results.empty()) {
env->SetLongArrayRegion(ret, 0, (jsize)g_last_scan_results.size(), (const jlong*)g_last_scan_results.data());
}
return ret;
}

JNIEXPORT jlongArray JNICALL
Java_memory_example_com_Starter_nativeScanString(JNIEnv* env, jclass clazz, jstring target_value, jint permMask, jint scopeMask, jint limit) {
g_last_scan_results.clear();
KittyScanner::setPatternScanSafeMode(true);
const char* str = env->GetStringUTFChars(target_value, nullptr);
if (str == nullptr) return nullptr;
std::string target(str);
env->ReleaseStringUTFChars(target_value, str);
auto maps = KittyMemory::getAllMaps();
for (const auto& map : maps) {
if (g_last_scan_results.size() >= (size_t)limit) break;
if (!map.isValid() || !map.readable) continue;
if (!is_scope_allowed(map, scopeMask)) continue;
bool allowed = false;
if ((permMask & 1) && map.writeable && !map.executable) allowed = true;
if ((permMask & 2) && !map.writeable && !map.executable) allowed = true;
if ((permMask & 4) && map.executable) allowed = true;
if ((permMask & 8) && map.readable && !map.writeable && !map.executable) allowed = true;
if (!allowed) continue;
auto results = KittyScanner::findDataAll(map.startAddress, map.endAddress, target.c_str(), target.length());
for (auto addr : results) {
if (g_last_scan_results.size() >= (size_t)limit) break;
g_last_scan_results.push_back(addr);
}
}
jlongArray ret = env->NewLongArray((jsize)g_last_scan_results.size());
if (ret == nullptr) return nullptr;
if (!g_last_scan_results.empty()) {
env->SetLongArrayRegion(ret, 0, (jsize)g_last_scan_results.size(), (const jlong*)g_last_scan_results.data());
}
return ret;
}

JNIEXPORT jlongArray JNICALL
Java_memory_example_com_Starter_nativeScanUtf16(JNIEnv* env, jclass clazz, jstring target_value, jint permMask, jint scopeMask, jint limit) {
g_last_scan_results.clear();
KittyScanner::setPatternScanSafeMode(true);
const char* str = env->GetStringUTFChars(target_value, nullptr);
if (str == nullptr) return nullptr;
std::string target(str);
env->ReleaseStringUTFChars(target_value, str);
std::vector<char> utf16;
for (size_t i = 0; i < target.length(); i++) {
utf16.push_back(target[i]);
utf16.push_back(0);
}
auto maps = KittyMemory::getAllMaps();
for (const auto& map : maps) {
if (g_last_scan_results.size() >= (size_t)limit) break;
if (!map.isValid() || !map.readable) continue;
if (!is_scope_allowed(map, scopeMask)) continue;
bool allowed = false;
if ((permMask & 1) && map.writeable && !map.executable) allowed = true;
if ((permMask & 2) && !map.writeable && !map.executable) allowed = true;
if ((permMask & 4) && map.executable) allowed = true;
if ((permMask & 8) && map.readable && !map.writeable && !map.executable) allowed = true;
if (!allowed) continue;
auto results = KittyScanner::findDataAll(map.startAddress, map.endAddress, utf16.data(), utf16.size());
for (auto addr : results) {
if (g_last_scan_results.size() >= (size_t)limit) break;
g_last_scan_results.push_back(addr);
}
}
jlongArray ret = env->NewLongArray((jsize)g_last_scan_results.size());
if (ret == nullptr) return nullptr;
if (!g_last_scan_results.empty()) {
env->SetLongArrayRegion(ret, 0, (jsize)g_last_scan_results.size(), (const jlong*)g_last_scan_results.data());
}
return ret;
}

JNIEXPORT jint JNICALL
Java_memory_example_com_Starter_nativeReadInt(JNIEnv* env, jclass clazz, jlong address) {
uintptr_t addr = (uintptr_t)address;
if (addr == 0) return -1;
int value = 0;
if (!KittyMemory::memRead(addr, &value, sizeof(int))) return -1;
return value;
}

JNIEXPORT jboolean JNICALL
Java_memory_example_com_Starter_nativeWriteInt(JNIEnv* env, jclass clazz, jlong address, jint value) {
uintptr_t addr = (uintptr_t)address;
if (addr == 0) return JNI_FALSE;
bool ok = KittyMemory::memWrite(addr, &value, sizeof(int));
return ok ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jstring JNICALL
Java_memory_example_com_Starter_nativeReadString(JNIEnv* env, jclass clazz, jlong address, jint length) {
uintptr_t addr = (uintptr_t)address;
if (addr == 0 || length <= 0) return env->NewStringUTF("");
std::vector<char> buf(length + 1, 0);
if (!KittyMemory::memRead(addr, buf.data(), length)) return env->NewStringUTF("");
buf[length] = '\0';
return env->NewStringUTF(buf.data());
}

JNIEXPORT jboolean JNICALL
Java_memory_example_com_Starter_nativeWriteString(JNIEnv* env, jclass clazz, jlong address, jstring value) {
uintptr_t addr = (uintptr_t)address;
if (addr == 0) return JNI_FALSE;
const char* str = env->GetStringUTFChars(value, nullptr);
if (str == nullptr) return JNI_FALSE;
bool ok = KittyMemory::memWrite(addr, str, strlen(str) + 1);
env->ReleaseStringUTFChars(value, str);
return ok ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jboolean JNICALL
Java_memory_example_com_Starter_nativeIsWritable(JNIEnv* env, jclass clazz, jlong address) {
uintptr_t addr = (uintptr_t)address;
if (addr == 0) return JNI_FALSE;
auto map = KittyMemory::getAddressMap(addr);
if (!map.isValid()) return JNI_FALSE;
return map.writeable ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jlong JNICALL
Java_memory_example_com_Starter_nativeGetModuleBase(JNIEnv* env, jclass clazz, jstring moduleName) {
const char* name = env->GetStringUTFChars(moduleName, nullptr);
if (name == nullptr) return 0;
std::string modName(name);
env->ReleaseStringUTFChars(moduleName, name);
auto elf = KittyScanner::ElfScanner::findElf(modName);
if (elf.isValid()) {
return (jlong)elf.base();
}
return 0;
}

JNIEXPORT jstring JNICALL
Java_memory_example_com_Starter_nativeGetModuleForAddress(JNIEnv* env, jclass clazz, jlong address) {
uintptr_t addr = (uintptr_t)address;
if (addr == 0) return env->NewStringUTF("");
auto map = KittyMemory::getAddressMap(addr);
if (!map.isValid()) return env->NewStringUTF("");
std::string path = map.pathname;
if (path.empty() || path[0] != '/') return env->NewStringUTF("");
size_t pos = path.find_last_of('/');
std::string name = (pos != std::string::npos) ? path.substr(pos + 1) : path;
return env->NewStringUTF(name.c_str());
}

JNIEXPORT jlong JNICALL
Java_memory_example_com_Starter_nativeGetOffset(JNIEnv* env, jclass clazz, jlong address) {
uintptr_t addr = (uintptr_t)address;
if (addr == 0) return -1;
auto map = KittyMemory::getAddressMap(addr);
if (!map.isValid()) return -1;
std::string path = map.pathname;
if (path.empty() || path[0] != '/') return -1;
size_t pos = path.find_last_of('/');
std::string name = (pos != std::string::npos) ? path.substr(pos + 1) : path;
auto elf = KittyScanner::ElfScanner::findElf(name);
if (!elf.isValid()) return -1;
return (jlong)(addr - elf.base());
}

JNIEXPORT jobjectArray JNICALL
Java_memory_example_com_Starter_nativeGetAllModules(JNIEnv* env, jclass clazz) {
auto maps = KittyMemory::getAllMaps();
std::vector<std::string> modules;
for (const auto& map : maps) {
if (!map.isValid() || map.pathname.empty()) continue;
if (map.pathname[0] != '/') continue;
if (map.pathname.find(".so") == std::string::npos) continue;
std::string path = map.pathname;
size_t pos = path.find_last_of('/');
std::string name = (pos != std::string::npos) ? path.substr(pos + 1) : path;
bool found = false;
for (const auto& m : modules) {
if (m == name) { found = true; break; }
}
if (!found) modules.push_back(name);
}
jclass stringClass = env->FindClass("java/lang/String");
jobjectArray ret = env->NewObjectArray(modules.size(), stringClass, nullptr);
for (size_t i = 0; i < modules.size(); i++) {
env->SetObjectArrayElement(ret, i, env->NewStringUTF(modules[i].c_str()));
}
return ret;
}

}
