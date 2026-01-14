# Getting Started with ARfit-kit

AR試着SDKをプロジェクトに統合するためのガイドです。

## プラットフォーム別インストール

### iOS (Swift)

**Swift Package Manager**
```swift
// Package.swift
dependencies: [
    .package(url: "https://github.com/your-org/ARfit-kit.git", from: "1.0.0")
]
```

**使用例**
```swift
import ARFitKit
import SwiftUI

struct TryOnView: View {
    @StateObject private var arFitKit = ARFitKit()
    
    var body: some View {
        ARFitKitView(arFitKit: arFitKit)
            .task {
                try? arFitKit.initialize()
            }
    }
    
    func tryOnClothing(image: UIImage) async {
        let garment = try? await arFitKit.loadGarment(image: image)
        if let garment = garment {
            try? arFitKit.tryOn(garment: garment)
        }
    }
}
```

---

### Android (Kotlin)

**Gradle**
```kotlin
implementation("com.arfitkit:arfitkit:1.0.0")
```

**使用例**
```kotlin
class TryOnActivity : AppCompatActivity() {
    private lateinit var arFitKit: ARFitKit
    
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        
        arFitKit = ARFitKit(this)
        arFitKit.initialize()
        
        lifecycleScope.launch {
            arFitKit.startSession(surfaceView, this@TryOnActivity)
        }
    }
    
    suspend fun tryOnClothing(bitmap: Bitmap) {
        val garment = arFitKit.loadGarment(bitmap)
        arFitKit.tryOn(garment)
    }
}
```

---

### Web (TypeScript)

**npm**
```bash
npm install arfit-kit
```

**使用例**
```typescript
import { ARFitKit, GarmentType } from 'arfit-kit';

const arFitKit = new ARFitKit();
await arFitKit.initialize();

const canvas = document.getElementById('ar-canvas') as HTMLCanvasElement;
await arFitKit.startSession(canvas);

// 衣服を読み込んで試着
const garment = await arFitKit.loadGarment('/images/tshirt.png', GarmentType.TSHIRT);
arFitKit.tryOn(garment);
```

---

### React Native

**npm**
```bash
npm install arfit-kit-react-native
```

**使用例**
```tsx
import { ARFitKitView, useARFitKit, GarmentType } from 'arfit-kit-react-native';

function TryOnScreen() {
  const { loadGarment, tryOn, isLoading } = useARFitKit();
  
  const handleTryOn = async (imageUri: string) => {
    const garment = await loadGarment(imageUri, GarmentType.TSHIRT);
    await tryOn(garment);
  };
  
  return (
    <View style={{ flex: 1 }}>
      <ARFitKitView 
        style={{ flex: 1 }}
        onReady={() => console.log('AR Ready')}
      />
      <Button title="Try On" onPress={() => handleTryOn('path/to/image')} />
    </View>
  );
}
```

## 必要条件

| Platform | 最小バージョン | 必要な機能 |
|----------|---------------|-----------|
| iOS | 14.0+ | ARKit, カメラ |
| Android | API 24+ | ARCore, カメラ |
| Web | Chrome 94+ | WebGPU/WebGL2, WebRTC |

## 次のステップ

- [API Reference](api-reference.md) - 全APIドキュメント
- [Architecture](architecture.md) - システムアーキテクチャの詳細
