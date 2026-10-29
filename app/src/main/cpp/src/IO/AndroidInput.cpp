#include "AndroidInput.h"
#include <jni.h>
extern "C" JNIEXPORT jboolean JNICALL Java_openMapleClient_NativeInput_takeConnectionLost(JNIEnv *, jobject) {
    return ms::AndroidInput::connection_lost.exchange(false);
}

extern "C" JNIEXPORT void JNICALL Java_openMapleClient_NativeInput_update(JNIEnv *, jobject, jboolean keyboard, jboolean visible) {
    const unsigned next = (keyboard ? 1u : 0u) | (visible ? 2u : 0u);
    if (ms::AndroidInput::state.exchange(next) != next) {
        ++ms::AndroidInput::release_generation;
    }
}
extern "C" JNIEXPORT void JNICALL Java_openMapleClient_NativeInput_releaseKeys(JNIEnv *, jobject) {
    ++ms::AndroidInput::release_generation;
}
extern "C" JNIEXPORT jboolean JNICALL Java_openMapleClient_NativeInput_takeSettingsRequest(JNIEnv *, jobject) {
    return ms::AndroidInput::settings_requested.exchange(false);
}

extern "C" JNIEXPORT void JNICALL Java_openMapleClient_NativeInput_setControlsOpacity(JNIEnv *, jobject, jint opacity) {
    ms::AndroidInput::controls_opacity.store(opacity < 10 ? 10 : opacity > 100 ? 100 : opacity);
}
