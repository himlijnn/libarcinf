#include <jni.h>
#include "hook.hpp"

extern "C" __attribute__((visibility("default"))) jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved)
{
    (void)vm;
    (void)reserved;

    installSkillHook();
    installVideoHook();
    installAutoplayHook();
    installSettingsHook();
    installSecurityHook();

    return JNI_VERSION_1_6;
}
