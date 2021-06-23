#import <Cocoa/Cocoa.h>

#import "svtkCocoaGLView.h"

#import "svtkRenderer.h"

// This is a subclass of SVTK's svtkCocoaGLView.
@interface BasicSVTKView : svtkCocoaGLView

// Create the svtkRenderer, svtkRenderWindow, and svtkRenderWindowInteractor.
// initializeSVTKSupport/cleanUpSVTKSupport must be balanced.
- (void)initializeSVTKSupport;

// Destroy the svtkRenderer, svtkRenderWindow, and svtkRenderWindowInteractor.
// initializeSVTKSupport/cleanUpSVTKSupport must be balanced.
- (void)cleanUpSVTKSupport;

// Accessors for the svtkRenderer.
@property (readwrite, nonatomic, nullable, getter=getRenderer) svtkRenderer* renderer;

@end
