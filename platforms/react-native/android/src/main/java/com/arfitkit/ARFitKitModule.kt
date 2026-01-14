package com.arfitkit

import com.facebook.react.bridge.*
import com.facebook.react.modules.core.DeviceEventManagerModule
import java.util.UUID

class ARFitKitModule(reactContext: ReactApplicationContext) : 
    ReactContextBaseJavaModule(reactContext) {
    
    private var hasListeners = false
    // private var core: ARFitKit? = null // Native core instance
    
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
    
    @ReactMethod
    fun initialize(config: ReadableMap, promise: Promise) {
        try {
            val targetFPS = if (config.hasKey("targetFPS")) config.getInt("targetFPS") else 60
            val enableClothSim = if (config.hasKey("enableClothSimulation")) 
                config.getBoolean("enableClothSimulation") else true
            
            // TODO: Initialize C++ core via JNI
            // core = ARFitKit(reactApplicationContext)
            // core?.initialize(SessionConfig(targetFPS, enableClothSim))
            
            promise.resolve(null)
        } catch (e: Exception) {
            promise.reject("INIT_ERROR", e.message)
        }
    }
    
    @ReactMethod
    fun startSession(promise: Promise) {
        try {
            // TODO: Start AR session
            // core?.startSession(...)
            promise.resolve(null)
        } catch (e: Exception) {
            promise.reject("SESSION_ERROR", e.message)
        }
    }
    
    @ReactMethod
    fun stopSession(promise: Promise) {
        try {
            // TODO: Stop session
            // core?.stopSession()
            promise.resolve(null)
        } catch (e: Exception) {
            promise.reject("SESSION_ERROR", e.message)
        }
    }
    
    @ReactMethod
    fun loadGarment(imageUri: String, type: Int, promise: Promise) {
        try {
            // TODO: Load garment via core
            // val garment = core?.loadGarment(bitmap, GarmentType.fromInt(type))
            
            val garmentId = UUID.randomUUID().toString() // Placeholder
            
            val result = Arguments.createMap().apply {
                putString("id", garmentId)
                putInt("type", type)
                putString("imageUri", imageUri)
            }
            
            promise.resolve(result)
        } catch (e: Exception) {
            promise.reject("LOAD_ERROR", e.message)
        }
    }
    
    @ReactMethod
    fun tryOn(garmentId: String, promise: Promise) {
        try {
            // TODO: Apply garment
            // core?.tryOn(garmentId)
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
        // core?.removeAllGarments()
        promise.resolve(null)
    }
    
    @ReactMethod
    fun captureSnapshot(promise: Promise) {
        // TODO: Capture and return URI
        promise.resolve("")
    }
    
    // Helper to send events to JS
    private fun sendEvent(eventName: String, params: WritableMap?) {
        if (hasListeners) {
            reactApplicationContext
                .getJSModule(DeviceEventManagerModule.RCTDeviceEventEmitter::class.java)
                .emit(eventName, params)
        }
    }
}
