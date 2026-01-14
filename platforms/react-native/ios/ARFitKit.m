#import "ARFitKit.h"
#import <React/RCTLog.h>

// Import the C++ bridge (assuming it's built as part of the app)
// #import "ARFitKitBridge.h"

@implementation ARFitKit {
  bool hasListeners;
  // ARFitKitBridge *bridge; // Actual C++ bridge instance
}

RCT_EXPORT_MODULE()

- (NSArray<NSString *> *)supportedEvents {
  return @[ @"onPoseUpdated", @"onFPSUpdated", @"onError" ];
}

- (void)startObserving {
  hasListeners = YES;
}

- (void)stopObserving {
  hasListeners = NO;
}

RCT_EXPORT_METHOD(initialize : (NSDictionary *)config resolver : (
    RCTPromiseResolveBlock)resolve rejecter : (RCTPromiseRejectBlock)reject) {
  RCTLogInfo(@"[ARFitKit] Initializing with config: %@", config);

  // TODO: Initialize C++ core via bridge
  // bridge = [[ARFitKitBridge alloc] init];
  // [bridge initializeWithConfig:config error:nil];

  resolve(nil);
}

RCT_EXPORT_METHOD(startSession : (RCTPromiseResolveBlock)
                      resolve rejecter : (RCTPromiseRejectBlock)reject) {
  RCTLogInfo(@"[ARFitKit] Starting session");

  // TODO: Start AR session
  // [bridge startSession];

  resolve(nil);
}

RCT_EXPORT_METHOD(stopSession : (RCTPromiseResolveBlock)
                      resolve rejecter : (RCTPromiseRejectBlock)reject) {
  RCTLogInfo(@"[ARFitKit] Stopping session");

  // TODO: Stop AR session
  // [bridge stopSession];

  resolve(nil);
}

RCT_EXPORT_METHOD(loadGarment : (NSString *)imageUri type : (NSInteger)
                      type resolver : (RCTPromiseResolveBlock)
                          resolve rejecter : (RCTPromiseRejectBlock)reject) {
  RCTLogInfo(@"[ARFitKit] Loading garment from: %@", imageUri);

  // TODO: Load image and convert to garment
  // NSString *garmentId = [bridge loadGarmentFromUri:imageUri type:type];

  NSString *garmentId = [[NSUUID UUID] UUIDString]; // Placeholder

  NSDictionary *garment =
      @{@"id" : garmentId,
        @"type" : @(type),
        @"imageUri" : imageUri};

  resolve(garment);
}

RCT_EXPORT_METHOD(tryOn : (NSString *)garmentId resolver : (
    RCTPromiseResolveBlock)resolve rejecter : (RCTPromiseRejectBlock)reject) {
  RCTLogInfo(@"[ARFitKit] Trying on garment: %@", garmentId);

  // TODO: Apply garment
  // [bridge tryOn:garmentId];

  resolve(nil);
}

RCT_EXPORT_METHOD(removeGarment : (NSString *)garmentId resolver : (
    RCTPromiseResolveBlock)resolve rejecter : (RCTPromiseRejectBlock)reject) {
  // TODO: Remove garment
  resolve(nil);
}

RCT_EXPORT_METHOD(removeAllGarments : (RCTPromiseResolveBlock)
                      resolve rejecter : (RCTPromiseRejectBlock)reject) {
  // TODO: Remove all
  resolve(nil);
}

RCT_EXPORT_METHOD(captureSnapshot : (RCTPromiseResolveBlock)
                      resolve rejecter : (RCTPromiseRejectBlock)reject) {
  // TODO: Capture and return image URI
  resolve(@"");
}

// Helper to send events to JS
- (void)sendPoseUpdate:(NSDictionary *)pose {
  if (hasListeners) {
    [self sendEventWithName:@"onPoseUpdated" body:pose];
  }
}

- (void)sendFPSUpdate:(NSNumber *)fps {
  if (hasListeners) {
    [self sendEventWithName:@"onFPSUpdated" body:fps];
  }
}

@end
