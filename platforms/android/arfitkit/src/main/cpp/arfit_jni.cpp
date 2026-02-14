#include <android/bitmap.h> // Bitmap処理用
#include <jni.h>
#include <memory>
#include <string>
#include <vector>

#include "../../../../../core/include/arfit_kit.h"
#include "../../../../../core/include/types.h"

/**
 * ARFitKitのグローバルインスタンス
 * デモ用に単純化してグローバルとして保持しています。
 * 本番用では、nativeHandle (ポインタ) を介してインスタンスごとに管理するのが一般的です。
 */
static std::unique_ptr<arfit::ARFitKit> g_arFitKit;

extern "C" JNIEXPORT void JNICALL Java_com_arfitkit_ARFitKit_nativeInitialize(
    JNIEnv *env, jobject /* this */, jint targetFPS,
    jboolean enableClothSimulation) {

  // ARFitKitの初期化
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

/**
 * フレーム処理（画像を渡して試着結果を書き戻す）
 */
extern "C" JNIEXPORT void JNICALL Java_com_arfitkit_ARFitKit_nativeProcessFrame(
    JNIEnv *env, jobject /* this */, jobject bitmap, jlong timestamp) {

  if (!g_arFitKit || !bitmap) return;

  AndroidBitmapInfo info;
  void *pixels;
  int ret;

  // Bitmap情報の取得とピクセルロック
  if ((ret = AndroidBitmap_getInfo(env, bitmap, &info)) < 0) return;
  // RGBA_8888形式のみをサポート
  if (info.format != ANDROID_BITMAP_FORMAT_RGBA_8888) return;

  if ((ret = AndroidBitmap_lockPixels(env, bitmap, &pixels)) < 0) return;

  // BitmapからImageDataを構築
  // ここではpixelsポインタを直接利用し、ImageDataはコピーを持つ
  arfit::ImageData imgData;
  imgData.width = info.width;
  imgData.height = info.height;
  imgData.channels = 4; // RGBA_8888は4チャンネル
  imgData.pixels.assign((uint8_t *)pixels, (uint8_t *)pixels + info.height * info.width * 4);

  arfit::CameraFrame frame;
  frame.image = imgData;
  frame.timestamp = (double)timestamp / 1e9; // nsを秒に変換

  // Core処理の実行
  // processFrameは処理結果のImageDataを返す
  auto result = g_arFitKit->processFrame(frame);

  if (result.isSuccess()) {
      // 処理結果（背景＋衣服）を元のBitmapに書き戻す
      // Coreエンジンからの結果がRGBA_8888形式であることを前提とする
      memcpy(pixels, result.value.pixels.data(), result.value.pixels.size());
  }

  // ピクセルロックを解除
  AndroidBitmap_unlockPixels(env, bitmap);
}

/**
 * 衣服の読み込み
 */
extern "C" JNIEXPORT jstring JNICALL
Java_com_arfitkit_ARFitKit_nativeLoadGarment(JNIEnv *env, jobject /* this */,
                                             jobject bitmap, jint type) {
  
  if (!g_arFitKit || !bitmap) return nullptr;

  AndroidBitmapInfo info;
  void *pixels;
  
  // Bitmap情報の取得とロック
  if (AndroidBitmap_getInfo(env, bitmap, &info) < 0) return nullptr;
  if (info.format != ANDROID_BITMAP_FORMAT_RGBA_8888) return nullptr;
  if (AndroidBitmap_lockPixels(env, bitmap, &pixels) < 0) return nullptr;

  // BitmapからImageDataを構築
  arfit::ImageData imgData;
  imgData.width = info.width;
  imgData.height = info.height;
  imgData.channels = 4;
  imgData.pixels.assign((uint8_t *)pixels, (uint8_t *)pixels + info.height * info.width * 4);

  // ロック解除
  AndroidBitmap_unlockPixels(env, bitmap);

  // 衣服の読み込み実行
  auto result = g_arFitKit->loadGarment(imgData, static_cast<arfit::GarmentType>(type));

  if (result.isSuccess()) {
    // 生成された衣服IDをJavaのStringとして返す
    return env->NewStringUTF(result.value.c_str());
  }

  return nullptr;
}

extern "C" JNIEXPORT void JNICALL Java_com_arfitkit_ARFitKit_nativeTryOn(
    JNIEnv *env, jobject /* this */, jstring garmentId) {
  
  if (!g_arFitKit || !garmentId) return;

  // jstringをstd::stringに変換
  const char *nativeId = env->GetStringUTFChars(garmentId, nullptr);
  std::string idString(nativeId);
  env->ReleaseStringUTFChars(garmentId, nativeId);

  // CoreのtryOnを呼び出す (IDベースで検索される)
  g_arFitKit->tryOn(idString);
}

extern "C" JNIEXPORT void JNICALL
Java_com_arfitkit_ARFitKit_nativeRemoveAllGarments(JNIEnv *env,
                                                   jobject /* this */) {
  if (g_arFitKit) {
    g_arFitKit->removeAllGarments();
  }
}

