#import "ARFitKitBridge.h"
#import "../../../../../core/include/arfit_kit.h" // C++ Core Header
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

@interface ARFitKitBridge () {
  std::unique_ptr<arfit::ARFitKit> _core;
}
// Keep track of C++ generic objects mapped to ObjC objects if needed
@property(nonatomic, strong) NSMutableDictionary<NSString *, id> *garmentCache;
@end

@implementation ARFitKitBridge

- (instancetype)init {
  self = [super init];
  if (self) {
    _core = std::make_unique<arfit::ARFitKit>();
    _garmentCache = [NSMutableDictionary new];
  }
  return self;
}

- (BOOL)initializeWithConfig:(NSDictionary<NSString *, id> *)config
                       error:(NSError **)error {
  arfit::SessionConfig coreConfig;

  // Convert dictionary config to C++ struct
  if (config[@"targetFPS"])
    coreConfig.targetFPS = [config[@"targetFPS"] intValue];
  if (config[@"enableClothSimulation"])
    coreConfig.enableClothSimulation =
        [config[@"enableClothSimulation"] boolValue];
  // ... other config mappings

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

- (void)startSession {
  _core->startSession();
}

- (void)stopSession {
  _core->stopSession();
}

- (void)processFrame:(ARFrame *)frame {
  // 1. Convert ARFrame (CVPixelBuffer) to arfit::CameraFrame
  CVPixelBufferRef pixelBuffer = frame.capturedImage;
  CVPixelBufferLockBaseAddress(pixelBuffer, kCVPixelBufferLock_ReadOnly);

  void *baseAddress = CVPixelBufferGetBaseAddress(pixelBuffer);
  size_t width = CVPixelBufferGetWidth(pixelBuffer);
  size_t height = CVPixelBufferGetHeight(pixelBuffer);
  size_t bytesPerRow = CVPixelBufferGetBytesPerRow(pixelBuffer);

  arfit::ImageData imgData;
  imgData.width = (int)width;
  imgData.height = (int)height;
  imgData.channels =
      4; // Assuming BGRA or similar, would need conversion actually

  // Note: This copy is expensive. Optimally pass raw pointer or use Metal
  // texture For skeleton, we just copy
  imgData.pixels.resize(height * width * 4);
  memcpy(imgData.pixels.data(), baseAddress,
         height * width * 4); // Basic copy assumption

  arfit::CameraFrame cameraFrame;
  cameraFrame.image = imgData;
  cameraFrame.timestamp = frame.timestamp;

  CVPixelBufferUnlockBaseAddress(pixelBuffer, kCVPixelBufferLock_ReadOnly);

  // 2. Process
  _core->processFrame(cameraFrame);

  // 3. Update Metrics
  if (self.onFPSUpdated) {
    self.onFPSUpdated(_core->getCurrentFPS());
  }

  // TODO: Connect Body Callback
}

- (void)loadGarment:(UIImage *)image
               type:(NSString *)type
         completion:(void (^)(BridgeGarment *_Nullable,
                              NSError *_Nullable))completion {

  // Convert UIImage to raw bytes
  // ...

  // Simulate async loading completion
  dispatch_after(
      dispatch_time(DISPATCH_TIME_NOW, (int64_t)(0.5 * NSEC_PER_SEC)),
      dispatch_get_main_queue(), ^{
        BridgeGarment *garment =
            [[BridgeGarment alloc] initWithId:[[NSUUID UUID] UUIDString]
                                         type:type];
        if (completion)
          completion(garment, nil);
      });
}

- (void)tryOnGarment:(BridgeGarment *)garment {
  // _core->tryOn(...)
}

- (void)removeGarment:(BridgeGarment *)garment {
  // _core->removeGarment(...)
}

- (void)removeAllGarments {
  _core->removeAllGarments();
}

@end
