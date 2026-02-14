#import <Metal/Metal.h>
#import <MetalKit/MetalKit.h>
#import <React/RCTView.h>
#import <UIKit/UIKit.h>
#import "ARFitKitBridge.h"

/**
 * AR試着画面を表示するためのカスタムビュー (Metal)
 */
@interface ARFitKitView : MTKView
@end

@implementation ARFitKitView {
    id<MTLCommandQueue> _commandQueue;
    CIContext *_ciContext;
}

- (instancetype)initWithFrame:(CGRect)frame {
  self = [super initWithFrame:frame device:MTLCreateSystemDefaultDevice()];
  if (self) {
    self.colorPixelFormat = MTLPixelFormatBGRA8Unorm;
    self.depthStencilPixelFormat = MTLPixelFormatDepth32Float;
    self.paused = NO;
    self.enableSetNeedsDisplay = NO;
    
    _commandQueue = [self.device newCommandQueue];
    _ciContext = [CIContext contextWithMTLDevice:self.device];
  }
  return self;
}

/**
 * 描画ループ - ブリッジから最新の画像を取得して表示
 */
- (void)drawRect:(CGRect)rect {
    UIImage *image = [[ARFitKitBridge sharedInstance] lastRenderedImage];
    if (!image) return;
    
    id<MTLCommandBuffer> commandBuffer = [_commandQueue commandBuffer];
    id<MTLTexture> currentTexture = self.currentDrawable.texture;
    
    if (currentTexture) {
        CIImage *ciImage = [[CIImage alloc] initWithImage:image];
        
        // 画面サイズに合わせて調整
        float scaleX = self.drawableSize.width / image.size.width;
        float scaleY = self.drawableSize.height / image.size.height;
        ciImage = [ciImage imageByApplyingTransform:CGAffineTransformMakeScale(scaleX, scaleY)];
        
        [_ciContext render:ciImage toMTLTexture:currentTexture commandBuffer:commandBuffer bounds:ciImage.extent colorSpace:CGColorSpaceCreateDeviceRGB()];
        
        [commandBuffer presentDrawable:self.currentDrawable];
        [commandBuffer commit];
    }
}

@end

