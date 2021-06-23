#import "BasicSVTKView.h"

#import "svtkCocoaRenderWindow.h"
#import "svtkCocoaRenderWindowInteractor.h"
#import "svtkRenderWindow.h"
#import "svtkRenderWindowInteractor.h"
#import "svtkRenderer.h"

@implementation BasicSVTKView

// ----------------------------------------------------------------------------
// Designated initializer
- (instancetype)initWithFrame:(NSRect)frameRect
{
  self = [super initWithFrame:frameRect];
  if (self)
  {
    // nothing to do... add something if you need to
  }

  return self;
}

// ----------------------------------------------------------------------------
// Designated initializer
- (nullable instancetype)initWithCoder:(NSCoder*)coder
{
  self = [super initWithCoder:coder];
  if (self)
  {
    // nothing to do... add something if you need to
  }

  return self;
}

// ----------------------------------------------------------------------------
// We are going to override the super class here to do some last minute
// setups. We need to do this because if we initialize in the constructor or
// even later, in say an NSDocument's windowControllerDidLoadNib, then
// we will get a warning about "Invalid Drawable" because the OpenGL Context
// is trying to be set and rendered into an NSView that most likely is not
// on screen yet. This is a way to defer that initialization until the NSWindow
// that contains our NSView subclass is actually on screen and ready to be drawn.
- (void)drawRect:(NSRect)theRect
{
  // Check for a valid svtkWindowInteractor and then initialize it. Technically we
  // do not need to do this, but what happens is that the window that contains
  // this object will not immediately render it so you end up with a big empty
  // space in your gui where this NSView subclass should be. This may or may
  // not be what is wanted. If you allow this code then what you end up with is the
  // typical empty black OpenGL view which seems more 'correct' or at least is
  // more soothing to the eye.
  svtkRenderWindowInteractor* renWinInt = [self getInteractor];
  if (renWinInt && (renWinInt->GetInitialized() == NO))
  {
    renWinInt->Initialize();
  }

  // Let the svtkCocoaGLView do its regular drawing
  [super drawRect:theRect];
}

// ----------------------------------------------------------------------------
- (void)initializeSVTKSupport
{
  // The usual svtk object creation.
  svtkRenderer* ren = svtkRenderer::New();
  svtkRenderWindow* renWin = svtkRenderWindow::New();
  svtkRenderWindowInteractor* renWinInt = svtkRenderWindowInteractor::New();

  // The cast should never fail, but we do it anyway, as
  // it's more correct to do so.
  svtkCocoaRenderWindow* cocoaRenWin = svtkCocoaRenderWindow::SafeDownCast(renWin);

  if (ren && cocoaRenWin && renWinInt)
  {
    // This is special to our usage of svtk.  To prevent svtk
    // from creating an NSWindow and NSView automatically (its
    // default behaviour) we tell svtk that they exist already.
    // The APIs names are a bit misleading, due to the cross
    // platform nature of svtk, but this usage is correct.
    NSWindow* parentWindow = [self window];
    assert(parentWindow);
    cocoaRenWin->SetRootWindow((__bridge void*)parentWindow);
    cocoaRenWin->SetWindowId((__bridge void*)self);

    // The usual svtk connections.
    cocoaRenWin->AddRenderer(ren);
    renWinInt->SetRenderWindow(cocoaRenWin);

    // This is special to our usage of svtk.  svtkCocoaGLView
    // keeps track of the renderWindow, and has a get
    // accessor if you ever need it.
    [self setSVTKRenderWindow:cocoaRenWin];

    // Likewise, BasicSVTKView keeps track of the renderer.
    [self setRenderer:ren];
  }
}

// ----------------------------------------------------------------------------
- (void)cleanUpSVTKSupport
{
  svtkRenderer* ren = [self getRenderer];
  svtkRenderWindow* renWin = [self getSVTKRenderWindow];
  svtkRenderWindowInteractor* renWinInt = [self getInteractor];

  if (ren)
  {
    ren->Delete();
  }
  if (renWin)
  {
    renWin->Delete();
  }
  if (renWinInt)
  {
    renWinInt->Delete();
  }

  [self setRenderer:nil];
  [self setSVTKRenderWindow:nil];

  // There is no setter accessor for the render window
  // interactor, that's ok.
}

@end
