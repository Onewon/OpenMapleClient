// BeiDou v83 wire strings use GBK (CharsetConstants); game/UI strings use UTF-8.
// AGPL-3.0-or-later.
#pragma once
#include <algorithm>
#include <string>
#ifdef __ANDROID__
#include <jni.h>
#include <android/native_activity.h>
#include <glfm.h>
#else
#include <iconv.h>
#endif
namespace ms::wire_text {
inline std::string convert(const std::string &text,const char *from,const char *to) {
    if(std::all_of(text.begin(),text.end(),[](unsigned char c){return c<128;}))return text;
#ifdef __ANDROID__
    auto activity=glfmAndroidGetActivity();if(!activity || !activity->vm)return text;
    JNIEnv *env=nullptr;bool attached=false;
    if(activity->vm->GetEnv(reinterpret_cast<void**>(&env),JNI_VERSION_1_6)!=JNI_OK) {
        if(activity->vm->AttachCurrentThread(&env,nullptr)!=JNI_OK)return text;attached=true;
    }
    std::string result=text;
    if(env->PushLocalFrame(8)==JNI_OK) {
        auto cls=env->FindClass("java/lang/String");
        auto ctor=cls?env->GetMethodID(cls,"<init>","([BLjava/lang/String;)V"):nullptr;
        auto bytesMethod=cls?env->GetMethodID(cls,"getBytes","(Ljava/lang/String;)[B"):nullptr;
        if(ctor && bytesMethod) {
            auto input=env->NewByteArray(text.size());env->SetByteArrayRegion(input,0,text.size(),reinterpret_cast<const jbyte*>(text.data()));
            auto fromName=env->NewStringUTF(from);auto toName=env->NewStringUTF(to);
            auto str=env->NewObject(cls,ctor,input,fromName);
            auto output=str?static_cast<jbyteArray>(env->CallObjectMethod(str,bytesMethod,toName)):nullptr;
            if(output && !env->ExceptionCheck()) {result.resize(env->GetArrayLength(output));env->GetByteArrayRegion(output,0,result.size(),reinterpret_cast<jbyte*>(result.data()));}
        }
        if(env->ExceptionCheck())env->ExceptionClear();env->PopLocalFrame(nullptr);
    } else if(env->ExceptionCheck())env->ExceptionClear();
    if(attached)activity->vm->DetachCurrentThread();return result;
#else
    auto codec=iconv_open(to,from);if(codec==reinterpret_cast<iconv_t>(-1))return text;
    std::string result(text.size()*4+8,'\0');char *input=const_cast<char*>(text.data()),*output=result.data();size_t in=text.size(),out=result.size();
    const auto status=iconv(codec,&input,&in,&output,&out);iconv_close(codec);
    if(status==size_t(-1))return text;result.resize(result.size()-out);return result;
#endif
}
inline std::string decode(const std::string &text){return convert(text,"GBK","UTF-8");}
inline std::string encode(const std::string &text){return convert(text,"UTF-8","GBK");}
}

