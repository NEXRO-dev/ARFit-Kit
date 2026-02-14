#import <ARKit/ARKit.h>
#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>

NS_ASSUME_NONNULL_BEGIN

/**
 * 衣服のブリッジデータ
 */
@interface BridgeGarment : NSObject
@property(nonatomic, readonly) NSString *uuid;
@property(nonatomic, readonly) NSString *type;

- (instancetype)initWithId:(NSString *)uuid type:(NSString *)type;
@end

/**
 * ARFitKitのコアエンジンとネイティブ(iOS)を繋ぐブリッジクラス
 */
@interface ARFitKitBridge : NSObject

// シングルトンインスタンス
+ (instancetype)sharedInstance;

// 初期化
- (BOOL)initializeWithConfig:(NSDictionary<NSString *, id> *)config
                       error:(NSError **)error;

// セッション制御
- (void)startSession;
- (void)stopSession;

// フレーム処理
- (void)processFrame:(ARFrame *)frame;

// 最新のレンダリング済み画像
@property(nonatomic, readonly, nullable) UIImage *lastRenderedImage;

// 衣服操作
- (void)loadGarment:(UIImage *)image
                type:(NSString *)type
          completion:(void (^)(BridgeGarment *_Nullable garment,
                               NSError *_Nullable error))completion;

- (void)tryOnGarment:(BridgeGarment *)garment;
- (void)removeGarment:(BridgeGarment *)garment;
- (void)removeAllGarments;

// コールバック
@property(nonatomic, copy, nullable) void (^onFPSUpdated)(float fps);
@property(nonatomic, copy, nullable) void (^onBodyTracked)
    (NSArray<NSNumber *> *landmarks, float confidence);

@end

NS_ASSUME_NONNULL_END

