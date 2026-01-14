#import "ARFitKitView.h"
#import <React/RCTUIManager.h>
#import <React/RCTViewManager.h>

@interface ARFitKitViewManager : RCTViewManager
@end

@implementation ARFitKitViewManager

RCT_EXPORT_MODULE(ARFitKitView)

- (UIView *)view {
  return [[ARFitKitView alloc] init];
}

@end
