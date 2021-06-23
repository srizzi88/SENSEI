#import "CustomLayer.h"

#import "svtkCocoaRenderWindow.h"
#import "svtkCocoaRenderWindowInteractor.h"
#import "svtkRenderWindow.h"
#import "svtkRenderWindowInteractor.h"
#import "svtkRenderer.h"

@implementation CustomLayer

// ---------------------------------------------------------------------------------------------------------------
// This is CAOpenGLLayer's entry point for drawing. The OS calls it when it's time to draw.
- (void)drawInCGLContext:(CGLContextObj)inCGLContext
             pixelFormat:(CGLPixelFormatObj)inPixelFormat
            forLayerTime:(CFTimeInterval)inTimeInterval
             displayTime:(nullable const CVTimeStamp*)inTimeStamp
{
  assert(inCGLContext);
  assert(inPixelFormat);

  // Get the related view.
  CustomView* customView = [self customView];
  assert(customView);

  // Tell SVTK to render.
  assert([customView renderWindowInteractor] -> GetInitialized());
  svtkCocoaRenderWindow* renderWindow = [customView renderWindow];
  if (renderWindow && renderWindow->GetMapped())
  {
    renderWindow->InitializeFromCurrentContext();
    renderWindow->Render();
  }

  // Generally, subclasses should call the superclass implementation of the method to flush the
  // context after rendering. But SVTK itself flushes, so it's probably not necessary to call super.
#if 0
  [super drawInCGLContext:inCGLContext
              pixelFormat:inPixelFormat
             forLayerTime:inTimeInterval
              displayTime:inTimeStamp];
#endif
}

// ---------------------------------------------------------------------------------------------------------------
// CAOpenGLLayer triggers this function when a context is needed by the receiver. Here we return the
// SVTK context.
- (CGLContextObj)copyCGLContextForPixelFormat:(CGLPixelFormatObj)inPixelFormat
{
  assert(inPixelFormat);
  (void)inPixelFormat;

  // Get the related view.
  CustomView* customView = [self customView];
  assert(customView);

  // Fetch the rendering window's context.
  svtkCocoaRenderWindow* renderWindow = [customView renderWindow];
  assert(renderWindow);

  // Get the OpenGL context from SVTK.
  assert([customView renderWindowInteractor] -> GetInitialized());
  NSOpenGLContext* openGLContext = (__bridge NSOpenGLContext*)(renderWindow->GetContextId());
  assert(openGLContext);

  // Convert to CGLContextObj.
  CGLContextObj cglContext = (CGLContextObj)[openGLContext CGLContextObj];
  assert(cglContext);

  return cglContext;
}

// ---------------------------------------------------------------------------------------------------------------
- (void)releaseCGLContext:(CGLContextObj)inContext
{
  (void)inContext;
}

// ---------------------------------------------------------------------------------------------------------------
- (void)releaseCGLPixelFormat:(CGLPixelFormatObj)inPixelFormat
{
  (void)inPixelFormat;
}

@end
