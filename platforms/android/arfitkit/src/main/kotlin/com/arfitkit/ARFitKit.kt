/**
 * ARFitKit - Android SDK for AR Virtual Try-On
 *
 * オープンソースのAR試着SDK
 * KotlinでネイティブなARCore統合を提供
 */

package com.arfitkit

import android.content.Context
import android.graphics.Bitmap
import android.view.SurfaceView
import androidx.annotation.MainThread
import androidx.lifecycle.LifecycleOwner
import com.google.ar.core.*
import kotlinx.coroutines.*
import kotlinx.coroutines.flow.*

/**
 * 衣服の種類
 */
enum class GarmentType {
    UNKNOWN,
    TSHIRT,
    SHIRT,
    JACKET,
    COAT,
    DRESS,
    PANTS,
    SHORTS,
    SKIRT
}

/**
 * エラーコード
 */
sealed class ARFitKitError(message: String) : Exception(message) {
    object InitializationFailed : ARFitKitError("Initialization failed")
    object CameraAccessDenied : ARFitKitError("Camera access denied")
    object GpuNotAvailable : ARFitKitError("GPU not available")
    object ModelLoadFailed : ARFitKitError("Failed to load model")
    object GarmentConversionFailed : ARFitKitError("Garment conversion failed")
    object InvalidImage : ARFitKitError("Invalid image")
    object SessionNotStarted : ARFitKitError("Session not started")
    object NetworkError : ARFitKitError("Network error")
    object ARCoreNotAvailable : ARFitKitError("ARCore not available")
}

/**
 * セッション設定
 */
data class SessionConfig(
    val targetFPS: Int = 60,
    val enableClothSimulation: Boolean = true,
    val enableShadows: Boolean = true,
    val maxGarments: Int = 3,
    val serverEndpoint: String = "",
    val useHybridProcessing: Boolean = true
)

/**
 * 衣服データ
 */
data class Garment(
    val id: String = java.util.UUID.randomUUID().toString(),
    val type: GarmentType,
    val image: Bitmap,
    var isLoading: Boolean = false,
    var conversionProgress: Float = 0f,
    internal var nativeGarment: Long = 0
)

/**
 * ボディポーズデータ
 */
data class BodyPose(
    val landmarks: List<FloatArray>,
    val visibility: List<Float>,
    val confidence: Float,
    val timestamp: Long
)

/**
 * ARFitKit メインクラス
 * AR試着機能を提供するプライマリインターフェース
 */
class ARFitKit(private val context: Context) {
    
    // State flows
    private val _isSessionActive = MutableStateFlow(false)
    val isSessionActive: StateFlow<Boolean> = _isSessionActive.asStateFlow()
    
    private val _currentFPS = MutableStateFlow(0f)
    val currentFPS: StateFlow<Float> = _currentFPS.asStateFlow()
    
    private val _currentPose = MutableStateFlow<BodyPose?>(null)
    val currentPose: StateFlow<BodyPose?> = _currentPose.asStateFlow()
    
    private val _activeGarments = MutableStateFlow<List<Garment>>(emptyList())
    val activeGarments: StateFlow<List<Garment>> = _activeGarments.asStateFlow()
    
    // Private properties
    private var config: SessionConfig = SessionConfig()
    private var arSession: Session? = null
    private var surfaceView: SurfaceView? = null
    
    private val processingScope = CoroutineScope(Dispatchers.Default + SupervisorJob())
    
    // Callbacks
    var onFrameProcessed: ((Bitmap) -> Unit)? = null
    var onPoseUpdated: ((BodyPose) -> Unit)? = null
    var onError: ((ARFitKitError) -> Unit)? = null
    
    // MARK: - Native Methods
    
    private external fun nativeInitialize(targetFPS: Int, enableClothSimulation: Boolean)
    private external fun nativeStartSession()
    private external fun nativeStopSession()
    private external fun nativeGetCurrentFPS(): Float
    private external fun nativeProcessFrame(bitmap: Bitmap, timestampNs: Long)
    private external fun nativeLoadGarment(bitmap: Bitmap, type: Int): String?
    private external fun nativeTryOn(garmentId: String)
    private external fun nativeRemoveAllGarments()

    companion object {
        init {
            try {
                System.loadLibrary("arfit_jni")
                android.util.Log.d("ARFitKit", "Native library loaded successfully")
            } catch (e: UnsatisfiedLinkError) {
                android.util.Log.e("ARFitKit", "Failed to load native library: ${e.message}")
            }
        }
    }
    
    /**
     * SDKを初期化
     * @param config セッション設定
     */
    fun initialize(config: SessionConfig = SessionConfig()) {
        this.config = config
        
        // Native initialize
        nativeInitialize(config.targetFPS, config.enableClothSimulation)
        
        android.util.Log.d("ARFitKit", "Initialized with config: $config")
    }
    
    /**
     * ARセッションを開始
     * @param surfaceView AR表示用のSurfaceView
     * @param lifecycleOwner ライフサイクルオーナー
     */
    @MainThread
    suspend fun startSession(surfaceView: SurfaceView, lifecycleOwner: LifecycleOwner) {
        if (_isSessionActive.value) return
        
        // Check ARCore availability
        val availability = ArCoreApk.getInstance().checkAvailability(context)
        if (availability == ArCoreApk.Availability.UNSUPPORTED_DEVICE_NOT_CAPABLE) {
            throw ARFitKitError.ARCoreNotAvailable
        }
        
        // Request ARCore installation if needed
        val installStatus = ArCoreApk.getInstance().requestInstall(
            context as android.app.Activity?,
            true
        )
        
        if (installStatus == ArCoreApk.InstallStatus.INSTALL_REQUESTED) {
            return // Will resume after install
        }
        
        // Create AR session
        arSession = Session(context).apply {
            val arConfig = Config(this).apply {
                updateMode = Config.UpdateMode.LATEST_CAMERA_IMAGE
                focusMode = Config.FocusMode.AUTO
            }
            configure(arConfig)
        }
        
        this.surfaceView = surfaceView
        _isSessionActive.value = true
        
        // Start native session
        nativeStartSession()
        
        android.util.Log.d("ARFitKit", "Session started")
        
        // Start processing loop
        processingScope.launch {
            processingLoop()
        }
    }
    
    /**
     * ARセッションを停止
     */
    fun stopSession() {
        nativeStopSession()
        arSession?.pause()
        arSession?.close()
        arSession = null
        surfaceView = null
        _isSessionActive.value = false
        _activeGarments.value = emptyList()
        nativeRemoveAllGarments()
        
        android.util.Log.d("ARFitKit", "Session stopped")
    }
    
    /**
     * 衣服を読み込む
     * @param bitmap 衣服の画像
     * @param type 衣服の種類（省略時は自動検出）
     * @return 読み込まれた衣服
     */
    suspend fun loadGarment(bitmap: Bitmap, type: GarmentType = GarmentType.UNKNOWN): Garment = withContext(Dispatchers.IO) {
        val garment = Garment(type = type, image = bitmap, isLoading = true)
        
        val id = nativeLoadGarment(bitmap, type.ordinal)
        if (id == null) {
             throw ARFitKitError.GarmentConversionFailed
        }
        
        val loadedGarment = garment.copy(id = id, isLoading = false, conversionProgress = 1f)
        return@withContext loadedGarment
    }
    
    /**
     * 衣服を試着
     * @param garment 試着する衣服
     */
    fun tryOn(garment: Garment) {
        if (!_isSessionActive.value) {
            throw ARFitKitError.SessionNotStarted
        }
        
        val currentList = _activeGarments.value.toMutableList()
        
        // Check max garments
        if (currentList.size >= config.maxGarments) {
            currentList.removeAt(0)
        }
        
        currentList.add(garment)
        _activeGarments.value = currentList
        
        nativeTryOn(garment.id)
        
        android.util.Log.d("ARFitKit", "Trying on garment: ${garment.type}")
    }
    
    /**
     * 衣服を削除
     * @param garment 削除する衣服
     */
    fun removeGarment(garment: Garment) {
        _activeGarments.value = _activeGarments.value.filter { it.id != garment.id }
        nativeRemoveGarment(garment.id)
    }
    
    /**
     * すべての衣服を削除
     */
    fun removeAllGarments() {
        _activeGarments.value = emptyList()
        nativeRemoveAllGarments()
    }
    
    /**
     * リソースを解放
     */
    fun release() {
        stopSession()
        processingScope.cancel()
    }
    
    /**
     * ARCore フレーム取得・処理ループ
     * セッションがアクティブな間、カメラフレームを取得し
     * Core C++ エンジンで処理（物理・レンダリング）した結果をコールバックに渡す
     */
    private suspend fun processingLoop() {
        // 処理結果の転送用Bitmap（毎フレーム再生成を避けるためキャッシュ）
        var outputBitmap: Bitmap? = null
        
        while (isActive && _isSessionActive.value) {
            try {
                val session = arSession ?: break
                
                // ARCoreから最新フレームを取得
                val frame = session.update()
                val camera = frame.camera
                
                // カメラのトラッキング状態を確認
                if (camera.trackingState != TrackingState.TRACKING) {
                    delay(16)
                    continue
                }
                
                // カメラ画像を取得してBitmapに変換
                val image = frame.acquireCameraImage()
                val width = image.width
                val height = image.height
                
                // 出力Bitmapのサイズが異なる場合は再作成
                if (outputBitmap == null || outputBitmap.width != width || outputBitmap.height != height) {
                    outputBitmap?.recycle()
                    outputBitmap = Bitmap.createBitmap(width, height, Bitmap.Config.ARGB_8888)
                }
                
                // YUV → RGBA変換してBitmapにコピー
                val yBuffer = image.planes[0].buffer
                val uBuffer = image.planes[1].buffer
                val vBuffer = image.planes[2].buffer
                
                val ySize = yBuffer.remaining()
                val uSize = uBuffer.remaining()
                val vSize = vBuffer.remaining()
                
                val nv21 = ByteArray(ySize + uSize + vSize)
                yBuffer.get(nv21, 0, ySize)
                vBuffer.get(nv21, ySize, vSize)
                uBuffer.get(nv21, ySize + vSize, uSize)
                
                val yuvImage = android.graphics.YuvImage(
                    nv21, android.graphics.ImageFormat.NV21, width, height, null
                )
                val out = java.io.ByteArrayOutputStream()
                yuvImage.compressToJpeg(android.graphics.Rect(0, 0, width, height), 90, out)
                val jpegBytes = out.toByteArray()
                val decodedBitmap = android.graphics.BitmapFactory.decodeByteArray(jpegBytes, 0, jpegBytes.size)
                
                // decodedBitmapをoutputBitmapにコピー
                val canvas = android.graphics.Canvas(outputBitmap!!)
                canvas.drawBitmap(decodedBitmap, 0f, 0f, null)
                decodedBitmap.recycle()
                
                image.close()
                
                // JNI経由でCore C++エンジンにフレームを処理させる
                // outputBitmapは in-place で書き換えられる
                nativeProcessFrame(outputBitmap, frame.timestamp)
                
                // FPSの更新
                val fps = nativeGetCurrentFPS()
                _currentFPS.value = fps
                
                // 処理済みフレームをコールバック経由でUIに配信
                withContext(Dispatchers.Main) {
                    onFrameProcessed?.invoke(outputBitmap!!)
                }
                
            } catch (e: com.google.ar.core.exceptions.NotYetAvailableException) {
                // フレームがまだ利用不可（ARCore初期化中）
                delay(16)
            } catch (e: Exception) {
                android.util.Log.e("ARFitKit", "Processing error: ${e.message}")
                delay(16)
            }
        }
        
        // ループ終了時にBitmapを解放
        outputBitmap?.recycle()
    }
    
    // ネイティブメソッド: 個別の衣服を削除
    private external fun nativeRemoveGarment(garmentId: String)
}

