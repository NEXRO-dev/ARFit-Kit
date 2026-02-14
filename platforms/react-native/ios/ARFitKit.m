#import "ARFitKit.h"
#import <React/RCTLog.h>
#import "ARFitKitBridge.h"

@implementation ARFitKit

RCT_EXPORT_MODULE();

/**
 * 初期化
 */
RCT_EXPORT_METHOD(initialize : (NSDictionary *)config resolver : (
    RCTPromiseResolveBlock)resolve rejecter : (RCTPromiseRejectBlock)reject) {
  RCTLogInfo(@"[ARFitKit] Initializing with config: %@", config);

  NSError *error = nil;
  BOOL success = [[ARFitKitBridge sharedInstance] initializeWithConfig:config error:&error];

  if (success) {
    resolve(nil);
  } else {
    reject(@"init_failed", error.localizedDescription, error);
  }
}

/**
 * セッション開始
 */
RCT_EXPORT_METHOD(startSession : (RCTPromiseResolveBlock)resolve rejecter : (
    RCTPromiseResolveBlock)reject) {
  RCTLogInfo(@"[ARFitKit] Starting session");
  [[ARFitKitBridge sharedInstance] startSession];
  resolve(nil);
}

/**
 * セッション停止
 */
RCT_EXPORT_METHOD(stopSession : (RCTPromiseResolveBlock)resolve rejecter : (
    RCTPromiseRejectBlock)reject) {
  RCTLogInfo(@"[ARFitKit] Stopping session");
  [[ARFitKitBridge sharedInstance] stopSession];
  resolve(nil);
}

/**
 * 衣服の読み込み
 */
RCT_EXPORT_METHOD(loadGarment : (NSString *)imageUri resolver : (
    RCTPromiseResolveBlock)resolve rejecter : (RCTPromiseRejectBlock)reject) {
  RCTLogInfo(@"[ARFitKit] Loading garment from URI: %@", imageUri);

  // 簡易的なローカルファイル読み込み
  NSURL *url = [NSURL URLWithString:imageUri];
  NSData *data = [NSData dataWithContentsOfURL:url];
  UIImage *image = [UIImage imageWithData:data];

  if (!image) {
    reject(@"invalid_image", @"画像を読み込めませんでした", nil);
    return;
  }

  [[ARFitKitBridge sharedInstance] loadGarment:image type:@"tshirt" completion:^(BridgeGarment * _Nullable garment, NSError * _Nullable error) {
    if (error) {
      reject(@"load_failed", error.localizedDescription, error);
    } else {
      resolve(@{
        @"id" : garment.uuid,
        @"type" : garment.type,
        @"imageUri" : imageUri
      });
    }
  }];
}

/**
 * 試着
 */
RCT_EXPORT_METHOD(tryOn : (NSString *)garmentId resolver : (
    RCTPromiseResolveBlock)resolve rejecter : (RCTPromiseRejectBlock)reject) {
  RCTLogInfo(@"[ARFitKit] Trying on garment: %@", garmentId);

  BridgeGarment *garment = [[BridgeGarment alloc] initWithId:garmentId type:@"unknown"];
  [[ARFitKitBridge sharedInstance] tryOnGarment:garment];

  resolve(nil);
}

/**
 * 衣服の削除
 */
RCT_EXPORT_METHOD(removeGarment : (NSString *)garmentId resolver : (
    RCTPromiseResolveBlock)resolve rejecter : (RCTPromiseRejectBlock)reject) {
  RCTLogInfo(@"[ARFitKit] Removing garment: %@", garmentId);

  BridgeGarment *garment = [[BridgeGarment alloc] initWithId:garmentId type:@"unknown"];
  [[ARFitKitBridge sharedInstance] removeGarment:garment];

  resolve(nil);
}

/**
 * すべての衣服の削除
 */
RCT_EXPORT_METHOD(removeAllGarments : (RCTPromiseResolveBlock)resolve rejecter : (
    RCTPromiseRejectBlock)reject) {
  RCTLogInfo(@"[ARFitKit] Removing all garments");
  [[ARFitKitBridge sharedInstance] removeAllGarments];
  resolve(nil);
}

/**
 * スナップショットの作成
 */
RCT_EXPORT_METHOD(captureSnapshot : (RCTPromiseResolveBlock)
                      resolve rejecter : (RCTPromiseRejectBlock)reject) {
  // TODO: スナップショット機能を実装
  resolve(@"");
}

@end
