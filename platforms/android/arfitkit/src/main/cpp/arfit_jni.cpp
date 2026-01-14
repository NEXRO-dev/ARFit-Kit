#include <android/bitmap.h> // For processing Bitmap
#include <jni.h>
#include <memory>
#include <string>
#include <vector>

#include "../../../../../core/include/arfit_kit.h"
#include "../../../../../core/include/types.h"

// Global global ARFitKit instance for simplicity in this demo
// In production, handle instance per object via nativeHandle pointer
static std::unique_ptr<arfit::ARFitKit> g_arFitKit;

extern "C" JNIEXPORT void JNICALL Java_com_arfitkit_ARFitKit_nativeInitialize(
    JNIEnv *env, jobject /* this */, jint targetFPS,
    jboolean enableClothSimulation) {

  g_arFitKit = std::make_unique<arfit::ARFitKit>();

  arfit::SessionConfig config;
  config.targetFPS = targetFPS;
  config.enableClothSimulation = enableClothSimulation;

  g_arFitKit->initialize(config);
}

extern "C" JNIEXPORT void JNICALL
Java_com_arfitkit_ARFitKit_nativeStartSession(JNIEnv *env, jobject /* this */) {
  if (g_arFitKit) {
    g_arFitKit->startSession();
  }
}

extern "C" JNIEXPORT void JNICALL
Java_com_arfitkit_ARFitKit_nativeStopSession(JNIEnv *env, jobject /* this */) {
  if (g_arFitKit) {
    g_arFitKit->stopSession();
    // Don't reset unique_ptr strictly if we want to reuse, but usually stop
    // clears state
  }
}

extern "C" JNIEXPORT jfloat JNICALL
Java_com_arfitkit_ARFitKit_nativeGetCurrentFPS(JNIEnv *env,
                                               jobject /* this */) {
  if (g_arFitKit) {
    return g_arFitKit->getCurrentFPS();
  }
  return 0.0f;
}

extern "C" JNIEXPORT void JNICALL Java_com_arfitkit_ARFitKit_nativeProcessFrame(
    JNIEnv *env, jobject /* this */, jobject bitmap, jlong timestamp) {

  if (!g_arFitKit)
    return;

  // Lock Bitmap pixels
  AndroidBitmapInfo info;
  void *pixels;
  int ret;

  if ((ret = AndroidBitmap_getInfo(env, bitmap, &info)) < 0)
    return;
  if (info.format != ANDROID_BITMAP_FORMAT_RGBA_8888)
    return;

  if ((ret = AndroidBitmap_lockPixels(env, bitmap, &pixels)) < 0)
    return;

  // Create CameraFrame wrapper
  // Copying data is safer for JNI but slow. In prod use hardware buffer.
  arfit::ImageData imgData;
  imgData.width = info.width;
  imgData.height = info.height;
  imgData.channels = 4;
  imgData.pixels.resize(info.width * info.height * 4);
  memcpy(imgData.pixels.data(), pixels, info.width * info.height * 4);

  AndroidBitmap_unlockPixels(env, bitmap);

  arfit::CameraFrame frame;
  frame.image = imgData;
  frame.timestamp = (double)timestamp / 1000000000.0; // ns to sec

  g_arFitKit->processFrame(frame);
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_arfitkit_ARFitKit_nativeLoadGarment(JNIEnv *env, jobject /* this */,
                                             jobject bitmap, jint type) {

  if (!g_arFitKit)
    return nullptr;

  // Convert Bitmap to ImageData
  AndroidBitmapInfo info;
  void *pixels;
  AndroidBitmap_getInfo(env, bitmap, &info);
  AndroidBitmap_lockPixels(env, bitmap, &pixels);

  arfit::ImageData imgData;
  imgData.width = info.width;
  imgData.height = info.height;
  imgData.channels = 4;
  imgData.pixels.resize(info.width * info.height * 4);
  memcpy(imgData.pixels.data(), pixels, info.width * info.height * 4);

  AndroidBitmap_unlockPixels(env, bitmap);

  auto result =
      g_arFitKit->loadGarment(imgData, static_cast<arfit::GarmentType>(type));

  if (result && result.value) {
    return env->NewStringUTF(result.value->id.c_str());
  }

  return nullptr;
}

extern "C" JNIEXPORT void JNICALL Java_com_arfitkit_ARFitKit_nativeTryOn(
    JNIEnv *env, jobject /* this */, jstring garmentId) {
  // In this simplified JNI, we assume the C++ side manages loaded garments by
  // ID Since our C++ API returns shared_ptr, we really should be storing them
  // in a map in native layer For now, assume single garment flow or adapt C++
  // API slightly if needed. Let's assume the Last Loaded Garment for this demo
  // or handle ID lookups.

  // TODO: Implement proper ID to object mapping in native layer
}

extern "C" JNIEXPORT void JNICALL
Java_com_arfitkit_ARFitKit_nativeRemoveAllGarments(JNIEnv *env,
                                                   jobject /* this */) {
  if (g_arFitKit) {
    g_arFitKit->removeAllGarments();
  }
}
