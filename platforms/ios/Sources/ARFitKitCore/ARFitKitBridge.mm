#import "ARFitKitBridge.h"
#import "../../../../../core/include/arfit_kit.h"
#import "../../../../../core/include/types.h"

#include <memory>

@implementation BridgeGarment
- (instancetype)initWithId:(NSString *)uuid type:(NSString *)type {
  self = [super init];
  if (self) {
    _uuid = uuid;
    _type = type;
  }
  return self;
}
@end

@interface ARFitKitBridge () <ARSessionDelegate> {
  std::unique_ptr<arfit::ARFitKit> _core;
  UIImage *_lastRenderedImage;
  ARSession *_session; // ARKitのセッション管理用
}
@end


@implementation ARFitKitBridge

/**
 * シングルトンインスタンスの取得
 */
+ (instancetype)sharedInstance {
  static ARFitKitBridge *sharedInstance = nil;
  static dispatch_once_t onceToken;
  dispatch_once(&onceToken, ^{
    sharedInstance = [[self alloc] init];
  });
  return sharedInstance;
}

- (instancetype)init {
  self = [super init];
  if (self) {
    _core = std::make_unique<arfit::ARFitKit>();
  }
  return self;
}

- (UIImage *)lastRenderedImage {
    return _lastRenderedImage;
}

/**
 * 初期化
 */
- (BOOL)initializeWithConfig:(NSDictionary<NSString *, id> *)config
                       error:(NSError **)error {
  arfit::SessionConfig coreConfig;

  if (config[@"targetFPS"])
    coreConfig.targetFPS = [config[@"targetFPS"] intValue];
  if (config[@"enableClothSimulation"])
    coreConfig.enableClothSimulation =
        [config[@"enableClothSimulation"] boolValue];

  auto result = _core->initialize(coreConfig);
  if (!result) {
    if (error) {
      *error = [NSError
          errorWithDomain:@"com.arfitkit"
                     code:static_cast<int>(result.error)
                 userInfo:@{
                   NSLocalizedDescriptionKey :
                       [NSString stringWithUTF8String:result.message.c_str()]
                 }];
    }
    return NO;
  }
  return YES;
}

/**
 * ARセッションを開始し、ARKitのフレーム処理を開始します。
 */
- (void)startSession {
  if (!_session) {
      _session = [[ARSession alloc] init];
      _session.delegate = self;
  }
  
  ARBodyTrackingConfiguration *config = [[ARBodyTrackingConfiguration alloc] init];
  config.automaticSkeletonScaleEstimationEnabled = YES;
  [_session runWithConfiguration:config];
  
  _core->startSession();
}

/**
 * ARセッションを停止します。
 */
- (void)stopSession {
  [_session pause];
  _core->stopSession();
}

#pragma mark - ARSessionDelegate

/**
 * ARSessionDelegateメソッド。ARKitから新しいフレームが利用可能になったときに呼び出されます。
 */
- (void)session:(ARSession *)session didUpdateFrame:(ARFrame *)frame {
    [self processFrame:frame];
}

/**
 * フレーム処理（内部的に呼び出される）
 */
- (void)processFrame:(ARFrame *)frame {
  CVPixelBufferRef pixelBuffer = frame.capturedImage;
  
  // ARFrameは通常YUV形式。CIImageを使ってRGBAに変換するのが確実。
  CIImage *ciImage = [CIImage imageWithCVPixelBuffer:pixelBuffer];
  
  // 処理効率のため、一定サイズにリサイズ
  // (Coreエンジン内での計算負荷を軽減するため、実アプリでは解像度を調整します)
  size_t width = CVPixelBufferGetWidth(pixelBuffer);
  size_t height = CVPixelBufferGetHeight(pixelBuffer);
  
  static CIContext *ctx = nil;
  if (!ctx) ctx = [CIContext contextWithOptions:nil];
  
  arfit::ImageData imgData;
  imgData.width = (int)width;
  imgData.height = (int)height;
  imgData.pixels.resize(width * height * 4);
  
  [ctx render:ciImage 
      toBitmap:imgData.pixels.data() 
      rowBytes:width * 4 
      bounds:ciImage.extent 
      format:kCIFormatRGBA8 
      colorSpace:CGColorSpaceCreateDeviceRGB()];

  arfit::CameraFrame cameraFrame;
  cameraFrame.image = imgData;
  cameraFrame.timestamp = frame.timestamp;

  // Core処理の実行
  auto result = _core->processFrame(cameraFrame);

  if (result.isSuccess()) {
      // 処理結果をUIImageに変換して保持
      _lastRenderedImage = [self imageFromImageData:result.value];
  }

  if (self.onFPSUpdated) {
    self.onFPSUpdated(_core->getCurrentFPS());
  }
}


/**
 * ImageData (RGBA) を UIImage に変換
 */
- (UIImage *)imageFromImageData:(const arfit::ImageData &)data {
    CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
    CGContextRef context = CGBitmapContextCreate((void *)data.pixels.data(), data.width, data.height, 8, data.width * 4, colorSpace, kCGImageAlphaPremultipliedLast | kCGBitmapByteOrder32Big);
    
    CGImageRef cgImage = CGBitmapContextCreateImage(context);
    UIImage *uiImage = [UIImage imageWithCGImage:cgImage];
    
    CGImageRelease(cgImage);
    CGContextRelease(context);
    CGColorSpaceRelease(colorSpace);
    
    return uiImage;
}

- (void)loadGarment:(UIImage *)image
               type:(NSString *)type
         completion:(void (^)(BridgeGarment *_Nullable,
                              NSError *_Nullable))completion {

  CGImageRef cgImage = image.CGImage;
  size_t width = CGImageGetWidth(cgImage);
  size_t height = CGImageGetHeight(cgImage);
  
  arfit::ImageData imgData;
  imgData.width = (int)width;
  imgData.height = (int)height;
  imgData.channels = 4;
  imgData.pixels.resize(width * height * 4);
  
  CGContextRef context = CGBitmapContextCreate(imgData.pixels.data(), width, height, 8, width * 4, CGImageGetColorSpace(cgImage), kCGImageAlphaPremultipliedLast);
  CGContextDrawImage(context, CGRectMake(0, 0, width, height), cgImage);
  CGContextRelease(context);

  auto result = _core->loadGarment(imgData, arfit::GarmentType::UNKNOWN);

  if (result) {
    NSString *garmentId = [NSString stringWithUTF8String:result.value.c_str()];
    BridgeGarment *garment = [[BridgeGarment alloc] initWithId:garmentId type:type];
    if (completion) completion(garment, nil);
  } else {
    if (completion) {
        NSError *error = [NSError errorWithDomain:@"com.arfitkit" code:500 userInfo:@{NSLocalizedDescriptionKey: @"衣服の変換に失敗しました"}];
        completion(nil, error);
    }
  }
}

- (void)tryOnGarment:(BridgeGarment *)garment {
  if (garment.uuid) {
    _core->tryOn([garment.uuid UTF8String]);
  }
}

- (void)removeGarment:(BridgeGarment *)garment {
  if (garment.uuid) {
    _core->removeGarment([garment.uuid UTF8String]);
  }
}

- (void)removeAllGarments {
  _core->removeAllGarments();
}

@end


