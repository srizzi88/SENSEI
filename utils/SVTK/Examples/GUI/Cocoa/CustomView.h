#import <Cocoa/Cocoa.h>

#import "svtkRenderWindow.h"
#import "svtkRenderer.h"
//#import "svtkRenderWindowInteractor.h"
#import "svtkCocoaRenderWindow.h"
#import "svtkCocoaRenderWindowInteractor.h"

// This is a subclass of plain old NSView.
@interface CustomView : NSView

// Create the svtkRenderer, svtkRenderWindow, and svtkRenderWindowInteractor.
// initializeSVTKSupport/cleanUpSVTKSupport must be balanced.
- (void)initializeSVTKSupport;

// Destroy the svtkRenderer, svtkRenderWindow, and svtkRenderWindowInteractor.
// initializeSVTKSupport/cleanUpSVTKSupport must be balanced.
- (void)cleanUpSVTKSupport;

//
- (void)initializeLayerSupport;

// Accessors.
@property (readwrite, nonatomic, nullable) svtkRenderer* renderer;
@property (readwrite, nonatomic, nullable) svtkCocoaRenderWindow* renderWindow;
@property (readwrite, nonatomic, nullable) svtkRenderWindowInteractor* renderWindowInteractor;

@end
