/**
 * ARFitKitViewManager - React Native用のAndroidネイティブビューマネージャー
 *
 * ARCoreのカメラフレームにCore C++エンジンで衣服合成した結果を
 * ImageViewに描画して表示するビューを提供します。
 */
package com.arfitkit

import android.graphics.Bitmap
import android.widget.ImageView
import com.facebook.react.uimanager.SimpleViewManager
import com.facebook.react.uimanager.ThemedReactContext

class ARFitKitViewManager : SimpleViewManager<ImageView>() {
    
    companion object {
        const val REACT_CLASS = "ARFitKitView"
        
        // ARFitKitインスタンスへの参照（React Nativeモジュールから設定される）
        var sharedARFitKit: ARFitKit? = null
    }
    
    override fun getName() = REACT_CLASS

    /**
     * React Nativeがビューを生成する際に呼ばれる。
     * ImageViewを生成し、ARFitKitの処理結果をリアルタイムで表示する。
     */
    override fun createViewInstance(reactContext: ThemedReactContext): ImageView {
        val imageView = ImageView(reactContext).apply {
            scaleType = ImageView.ScaleType.FIT_CENTER
            // 背景を黒に設定（カメラフレームが届くまでの間）
            setBackgroundColor(android.graphics.Color.BLACK)
        }
        
        // ARFitKitのonFrameProcessedコールバックを接続
        // Core C++エンジンで処理された各フレーム（カメラ映像＋衣服合成）を
        // このImageViewに描画する
        sharedARFitKit?.onFrameProcessed = { bitmap: Bitmap ->
            imageView.post {
                imageView.setImageBitmap(bitmap)
            }
        }
        
        return imageView
    }
}
