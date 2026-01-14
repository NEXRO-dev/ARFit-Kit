#import <Metal/Metal.h>
#import <MetalKit/MetalKit.h>
#import <React/RCTView.h>
#import <UIKit/UIKit.h>

@interface ARFitKitView : MTKView

@end

@implementation ARFitKitView

- (instancetype)init {
  self = [super initWithFrame:CGRectZero device:MTLCreateSystemDefaultDevice()];
  if (self) {
    self.colorPixelFormat = MTLPixelFormatBGRA8Unorm;
    self.depthStencilPixelFormat = MTLPixelFormatDepth32Float;
    // In a real implementation, you would attach the renderer here
  }
  return self;
}

@end
