package com.arfitkit

import com.facebook.react.uimanager.SimpleViewManager
import com.facebook.react.uimanager.ThemedReactContext
import android.view.SurfaceView
import android.view.ViewGroup

class ARFitKitViewManager : SimpleViewManager<SurfaceView>() {
    override fun getName() = "ARFitKitView"

    override fun createViewInstance(reactContext: ThemedReactContext): SurfaceView {
        val view = SurfaceView(reactContext)
        // In a real implementation:
        // 1. Initialize ARCore session attached to this surface
        // 2. Pass surface to native render pipeline
        return view
    }
}
