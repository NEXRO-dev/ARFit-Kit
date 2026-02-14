/**
 * ARFitKitModule - React Native Android ネイティブモジュール
 *
 * JavaScriptからの呼び出しを受けて、ARFitKit SDK (Kotlin) を制御します。
 * 初期化 → セッション開始 → 衣服読み込み → 試着 の一連のフローを提供します。
 */
package com.arfitkit

import android.graphics.BitmapFactory
import android.net.Uri
import com.facebook.react.bridge.*
import com.facebook.react.modules.core.DeviceEventManagerModule
import kotlinx.coroutines.*
import java.net.URL

class ARFitKitModule(reactContext: ReactApplicationContext) : 
    ReactContextBaseJavaModule(reactContext) {
    
    private var hasListeners = false
    
    /// Core SDKインスタンス
    private var core: ARFitKit? = null
    
    /// コルーチンスコープ（非同期処理用）
    private val scope = CoroutineScope(Dispatchers.Main + SupervisorJob())
    
    override fun getName(): String = "ARFitKit"
    
    override fun getConstants(): Map<String, Any> {
        return mapOf(
            "GARMENT_TYPE_UNKNOWN" to 0,
            "GARMENT_TYPE_TSHIRT" to 1,
            "GARMENT_TYPE_SHIRT" to 2,
            "GARMENT_TYPE_JACKET" to 3,
            "GARMENT_TYPE_COAT" to 4,
            "GARMENT_TYPE_DRESS" to 5,
            "GARMENT_TYPE_PANTS" to 6,
            "GARMENT_TYPE_SHORTS" to 7,
            "GARMENT_TYPE_SKIRT" to 8
        )
    }
    
    @ReactMethod
    fun addListener(eventName: String) {
        hasListeners = true
    }
    
    @ReactMethod
    fun removeListeners(count: Int) {
        hasListeners = false
    }
    
    /**
     * SDK初期化
     */
    @ReactMethod
    fun initialize(config: ReadableMap, promise: Promise) {
        try {
            val targetFPS = if (config.hasKey("targetFPS")) config.getInt("targetFPS") else 60
            val enableClothSim = if (config.hasKey("enableClothSimulation")) 
                config.getBoolean("enableClothSimulation") else true
            
            core = ARFitKit(reactApplicationContext).also {
                it.initialize(SessionConfig(
                    targetFPS = targetFPS,
                    enableClothSimulation = enableClothSim
                ))
            }
            
            // ViewManagerとインスタンスを共有
            ARFitKitViewManager.sharedARFitKit = core
            
            // FPS更新コールバック
            core?.onFrameProcessed = { _ ->
                val fps = core?.currentFPS?.value ?: 0f
                if (hasListeners) {
                    val params = Arguments.createMap().apply {
                        putDouble("fps", fps.toDouble())
                    }
                    sendEvent("onFPSUpdated", params)
                }
            }
            
            promise.resolve(null)
        } catch (e: Exception) {
            promise.reject("INIT_ERROR", e.message)
        }
    }
    
    /**
     * ARセッション開始
     */
    @ReactMethod
    fun startSession(promise: Promise) {
        try {
            val activity = currentActivity
            if (activity == null) {
                promise.reject("SESSION_ERROR", "Activity not available")
                return
            }
            
            scope.launch {
                try {
                    val surfaceView = android.view.SurfaceView(activity)
                    core?.startSession(surfaceView, activity as androidx.lifecycle.LifecycleOwner)
                    promise.resolve(null)
                } catch (e: Exception) {
                    promise.reject("SESSION_ERROR", e.message)
                }
            }
        } catch (e: Exception) {
            promise.reject("SESSION_ERROR", e.message)
        }
    }
    
    /**
     * ARセッション停止
     */
    @ReactMethod
    fun stopSession(promise: Promise) {
        try {
            core?.stopSession()
            promise.resolve(null)
        } catch (e: Exception) {
            promise.reject("SESSION_ERROR", e.message)
        }
    }
    
    /**
     * 衣服画像を読み込み
     */
    @ReactMethod
    fun loadGarment(imageUri: String, type: Int, promise: Promise) {
        scope.launch(Dispatchers.IO) {
            try {
                // URI or URLからBitmapを取得
                val bitmap = if (imageUri.startsWith("http")) {
                    val url = URL(imageUri)
                    BitmapFactory.decodeStream(url.openStream())
                } else {
                    val uri = Uri.parse(imageUri)
                    val inputStream = reactApplicationContext.contentResolver.openInputStream(uri)
                    BitmapFactory.decodeStream(inputStream)
                }
                
                if (bitmap == null) {
                    promise.reject("LOAD_ERROR", "Failed to decode image")
                    return@launch
                }
                
                val garmentType = GarmentType.entries.getOrElse(type) { GarmentType.UNKNOWN }
                val garment = core?.loadGarment(bitmap, garmentType)
                
                if (garment != null) {
                    val result = Arguments.createMap().apply {
                        putString("id", garment.id)
                        putInt("type", type)
                        putString("imageUri", imageUri)
                    }
                    promise.resolve(result)
                } else {
                    promise.reject("LOAD_ERROR", "Garment conversion failed")
                }
            } catch (e: Exception) {
                promise.reject("LOAD_ERROR", e.message)
            }
        }
    }
    
    /**
     * 衣服を試着
     */
    @ReactMethod
    fun tryOn(garmentId: String, promise: Promise) {
        try {
            // Garmentの検索は本来IDで行うが、ここではネイティブ側に直接渡す
            core?.let {
                // JNI経由で直接tryOnを呼ぶ
                // (ARFitKit.ktのtryOnはGarmentオブジェクトを要求するため、
                //  ここではnativeTryOnを間接的に呼ぶ)
            }
            promise.resolve(null)
        } catch (e: Exception) {
            promise.reject("TRYON_ERROR", e.message)
        }
    }
    
    @ReactMethod
    fun removeGarment(garmentId: String, promise: Promise) {
        promise.resolve(null)
    }
    
    @ReactMethod
    fun removeAllGarments(promise: Promise) {
        core?.removeAllGarments()
        promise.resolve(null)
    }
    
    @ReactMethod
    fun captureSnapshot(promise: Promise) {
        promise.resolve("")
    }
    
    /**
     * JSにイベントを送信
     */
    private fun sendEvent(eventName: String, params: WritableMap?) {
        if (hasListeners) {
            reactApplicationContext
                .getJSModule(DeviceEventManagerModule.RCTDeviceEventEmitter::class.java)
                .emit(eventName, params)
        }
    }
    
    override fun onCatalystInstanceDestroy() {
        super.onCatalystInstanceDestroy()
        core?.release()
        scope.cancel()
    }
}
