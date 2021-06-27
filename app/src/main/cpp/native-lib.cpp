#include <jni.h>
#include <string>
#include "unzip.h"

extern "C" JNIEXPORT jboolean JNICALL
Java_com_huawei_zip_MainActivity_unzip(
        JNIEnv* env,
        jobject /* this */,jstring zipPath,jstring fileName, jstring targetDir) {
    const char* zip_path = env->GetStringUTFChars(zipPath, NULL);
    const char* target_dir = env->GetStringUTFChars(targetDir, NULL);
    const char* file_name = env->GetStringUTFChars(fileName, NULL);
    extractFileFromZip(zip_path, file_name, target_dir);
    env->ReleaseStringUTFChars(zipPath, zip_path);
    env->ReleaseStringUTFChars(targetDir, target_dir);
    env->ReleaseStringUTFChars(fileName, file_name);
    return JNI_TRUE;
}