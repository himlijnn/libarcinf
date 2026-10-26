#include <jni.h>

#include "hook.hpp"
#include "offset.hpp"


extern "C" __attribute__((visibility("default"))) jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved)
{
    (void)vm;
    (void)reserved;

    installSkillHook();
    installVideoHook();
    installSettingsHook();

#if enableAutoplayHook
    installAutoplayHook();
#endif

#if enableSecurityHook
    installSecurityHook();
#endif

    return JNI_VERSION_1_6;
}
