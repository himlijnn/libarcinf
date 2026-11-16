#include <dlfcn.h>
#include <jni.h>

#include "hook.hpp"
#include "offset.hpp"
#include "trampoline.hpp"

uintptr_t g_base = 0;

static bool safeModuleBase()
{
    (void)dlopen(TARGET, RTLD_NOW | RTLD_GLOBAL);
    g_base = getModuleBase(TARGET);
    return g_base != 0;
}

extern "C" __attribute__((visibility("default"))) jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved)
{
    (void)vm;
    (void)reserved;

    if (!safeModuleBase())
        return JNI_VERSION_1_6;

    installSkillHook();
    installVideoHook();
    installSettingsHook();

#if enableAutoplayHook
    installAutoplayHook();
#if enableSecurityHook
    installSecurityHook();
#endif
#endif

    return JNI_VERSION_1_6;
}
