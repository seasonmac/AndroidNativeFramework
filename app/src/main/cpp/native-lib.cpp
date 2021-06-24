#include <jni.h>
#include <string>
#include "ZipFile.h"
extern "C" JNIEXPORT jstring JNICALL
Java_com_huawei_zip_MainActivity_stringFromJNI(
        JNIEnv* env,
        jobject /* this */) {
    std::string hello = "Hello from C++";
    std::string path{""};
    hms::ZipFile zip(path);
    zip.unCompress(nullptr);
    return env->NewStringUTF(hello.c_str());
}
