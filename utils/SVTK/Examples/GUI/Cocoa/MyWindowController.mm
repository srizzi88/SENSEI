#import "MyWindowController.h"

#import "BasicSVTKView.h"
#import "CustomView.h"

#import "svtkCocoaRenderWindowInteractor.h"
#import "svtkConeSource.h"
#import "svtkCylinderSource.h"
#import "svtkInteractorStyleSwitch.h"
#import "svtkPolyDataMapper.h"
#import "svtkSmartPointer.h"
#import "svtkTextActor.h"
#import "svtkTextProperty.h"

// Private Interface
@interface MyWindowController ()
@property (readwrite, weak, nonatomic) IBOutlet BasicSVTKView* leftView;
@property (readwrite, weak, nonatomic) IBOutlet BasicSVTKView* middleView;
@property (readwrite, weak, nonatomic) IBOutlet CustomView* rightView;
@end

@implementation MyWindowController

// ----------------------------------------------------------------------------
// Private helper method to get the path to the system font appropriate for
// the given font and font size.
+ (nullable NSURL*)fontPathForString:(nullable NSString*)inString size:(CGFloat)inSize
{
  NSURL* fontUrl = nil;

  if (inString)
  {
    NSFont* startFont = [NSFont systemFontOfSize:inSize];
    CTFontRef font = CTFontCreateForString((__bridge CTFontRef)startFont,
      (__bridge CFStringRef)inString, CFRangeMake(0, [inString length]));
    if (font)
    {
      NSFontDescriptor* fontDesc = [(__bridge NSFont*)font fontDescriptor];
      fontUrl = [fontDesc objectForKey:(__bridge NSString*)kCTFontURLAttribute];

      CFRelease(font);
    }
  }

  return fontUrl;
}

// ----------------------------------------------------------------------------
- (void)setupLeftView
{
  BasicSVTKView* thisView = [self leftView];

  // Explicitly enable HiDPI/Retina (this is the default anyway).
  [thisView setWantsBestResolutionOpenGLSurface:YES];

  [thisView initializeSVTKSupport];

  // 'smart pointers' are used because they are very similar to reference counting in Cocoa.

  // Personal Taste Section. I like to use a trackball interactor
  svtkSmartPointer<svtkInteractorStyleSwitch> intStyle =
    svtkSmartPointer<svtkInteractorStyleSwitch>::New();
  intStyle->SetCurrentStyleToTrackballCamera();
  [thisView getInteractor] -> SetInteractorStyle(intStyle);

  // Create a cone, see the "SVTK User's Guide" for details
  svtkSmartPointer<svtkConeSource> cone = svtkSmartPointer<svtkConeSource>::New();
  cone->SetHeight(3.0);
  cone->SetRadius(1.0);
  cone->SetResolution(100);

  svtkSmartPointer<svtkPolyDataMapper> coneMapper = svtkSmartPointer<svtkPolyDataMapper>::New();
  coneMapper->SetInputConnection(cone->GetOutputPort());

  svtkSmartPointer<svtkActor> coneActor = svtkSmartPointer<svtkActor>::New();
  coneActor->SetMapper(coneMapper);

  [thisView getRenderer] -> AddActor(coneActor);

  // Create a text actor.
  NSString* string = @"日本語";
  int fontSize = 30;
  svtkSmartPointer<svtkTextActor> textActor = svtkSmartPointer<svtkTextActor>::New();
  textActor->SetPickable(false);
  textActor->SetInput([string UTF8String]);
  svtkTextProperty* properties = textActor->GetTextProperty();
  NSURL* fontURL = [[self class] fontPathForString:string size:fontSize];
  properties->SetFontFile([fontURL fileSystemRepresentation]);
  properties->SetFontFamily(SVTK_FONT_FILE);
  properties->SetFontSize(fontSize);
  svtkCoordinate* coord = textActor->GetPositionCoordinate();
  coord->SetCoordinateSystemToWorld();
  coord->SetValue(0.0, 0.5, 0.0);
  [thisView getRenderer] -> AddViewProp(textActor);

  // Tell the system that the view needs to be redrawn
  [thisView setNeedsDisplay:YES];
}

// ----------------------------------------------------------------------------
- (void)setupMiddleView
{
  BasicSVTKView* thisView = [self middleView];

  // Explicitly disable HiDPI/Retina as a demonstration of the difference.
  // One might want to disable it to reduce memory usage / increase performance.
  [thisView setWantsBestResolutionOpenGLSurface:NO];

  [thisView initializeSVTKSupport];

  // 'smart pointers' are used because they are very similar to reference counting in Cocoa.

  // Personal Taste Section. I like to use a trackball interactor
  svtkSmartPointer<svtkInteractorStyleSwitch> intStyle =
    svtkSmartPointer<svtkInteractorStyleSwitch>::New();
  intStyle->SetCurrentStyleToTrackballCamera();
  [thisView getInteractor] -> SetInteractorStyle(intStyle);

  // Create a cylinder, see the "SVTK User's Guide" for details
  svtkSmartPointer<svtkCylinderSource> cylinder = svtkSmartPointer<svtkCylinderSource>::New();
  cylinder->SetResolution(100);

  svtkSmartPointer<svtkPolyDataMapper> cylinderMapper = svtkSmartPointer<svtkPolyDataMapper>::New();
  cylinderMapper->SetInputConnection(cylinder->GetOutputPort());

  svtkSmartPointer<svtkActor> cylinderActor = svtkSmartPointer<svtkActor>::New();
  cylinderActor->SetMapper(cylinderMapper);

  [thisView getRenderer] -> AddActor(cylinderActor);

  // Create a text actor.
  NSString* string = @"日本語";
  int fontSize = 30;
  svtkSmartPointer<svtkTextActor> textActor = svtkSmartPointer<svtkTextActor>::New();
  textActor->SetPickable(false);
  textActor->SetInput([string UTF8String]);
  svtkTextProperty* properties = textActor->GetTextProperty();
  NSURL* fontURL = [[self class] fontPathForString:string size:fontSize];
  properties->SetFontFile([fontURL fileSystemRepresentation]);
  properties->SetFontFamily(SVTK_FONT_FILE);
  properties->SetFontSize(fontSize);
  svtkCoordinate* coord = textActor->GetPositionCoordinate();
  coord->SetCoordinateSystemToWorld();
  coord->SetValue(0.3, 0.5, 0.0);
  [thisView getRenderer] -> AddViewProp(textActor);

  // Tell the system that the view needs to be redrawn
  [thisView setNeedsDisplay:YES];
}

// ----------------------------------------------------------------------------
- (void)setupRightView
{
  CustomView* thisView = [self rightView];

  // Explicitly enable HiDPI/Retina (this is required when using CAOpenGLLayer, otherwise the view
  // will be 1/4 size on Retina).
  [thisView setWantsBestResolutionOpenGLSurface:YES];

  [thisView initializeSVTKSupport];
  [thisView initializeLayerSupport];

  // 'smart pointers' are used because they are very similar to reference counting in Cocoa.

  // Personal Taste Section. I like to use a trackball interactor
  svtkSmartPointer<svtkInteractorStyleSwitch> intStyle =
    svtkSmartPointer<svtkInteractorStyleSwitch>::New();
  intStyle->SetCurrentStyleToTrackballCamera();
  [thisView renderWindowInteractor] -> SetInteractorStyle(intStyle);

  // Create a cylinder, see the "SVTK User's Guide" for details
  svtkSmartPointer<svtkCylinderSource> cylinder = svtkSmartPointer<svtkCylinderSource>::New();
  cylinder->SetResolution(100);

  svtkSmartPointer<svtkPolyDataMapper> cylinderMapper = svtkSmartPointer<svtkPolyDataMapper>::New();
  cylinderMapper->SetInputConnection(cylinder->GetOutputPort());

  svtkSmartPointer<svtkActor> cylinderActor = svtkSmartPointer<svtkActor>::New();
  cylinderActor->SetMapper(cylinderMapper);

  [thisView renderer] -> AddActor(cylinderActor);

  // Create a text actor.
  NSString* string = @"日本語";
  int fontSize = 30;
  svtkSmartPointer<svtkTextActor> textActor = svtkSmartPointer<svtkTextActor>::New();
  textActor->SetPickable(false);
  textActor->SetInput([string UTF8String]);
  svtkTextProperty* properties = textActor->GetTextProperty();
  NSURL* fontURL = [[self class] fontPathForString:string size:fontSize];
  properties->SetFontFile([fontURL fileSystemRepresentation]);
  properties->SetFontFamily(SVTK_FONT_FILE);
  properties->SetFontSize(fontSize);
  svtkCoordinate* coord = textActor->GetPositionCoordinate();
  coord->SetCoordinateSystemToWorld();
  coord->SetValue(0.3, 0.5, 0.0);
  [thisView renderer] -> AddViewProp(textActor);

  // Tell the system that the view needs to be redrawn
  [thisView setNeedsDisplay:YES];
}

#pragma mark -

// ----------------------------------------------------------------------------
// Designated initializer
- (instancetype)init
{
  // Call the superclass' designated initializer, giving it the name of the nib file.
  self = [super initWithWindowNibName:@"MyWindow"];

  return self;
}

// ----------------------------------------------------------------------------
// Called once when the window is loaded.
- (void)windowDidLoad
{
  [self setupLeftView];
  [self setupMiddleView];
  [self setupRightView];
}

// ----------------------------------------------------------------------------
// Called once when the window is closed.
- (void)windowWillClose:(NSNotification*)inNotification
{
  // Releases memory allocated in initializeSVTKSupport.
  [[self leftView] cleanUpSVTKSupport];
  [[self middleView] cleanUpSVTKSupport];
  [[self rightView] cleanUpSVTKSupport];
}

#pragma mark -

// ----------------------------------------------------------------------------
- (IBAction)handleLeftButton:(id)sender
{
  // Do anything you want with the view.
  NSBeep();

  // Here we just clean it up and remove it.
  [[self leftView] cleanUpSVTKSupport];
  [[self leftView] removeFromSuperview];
}

// ----------------------------------------------------------------------------
- (IBAction)handleMiddleButton:(id)sender
{
  // Do anything you want with the view.
  NSBeep();

  // Here we just clean it up and remove it.
  [[self middleView] cleanUpSVTKSupport];
  [[self middleView] removeFromSuperview];
}

// ----------------------------------------------------------------------------
- (IBAction)handleRightButton:(id)sender
{
  // Do anything you want with the view.
  NSBeep();

  // Here we just clean it up and remove it.
  [[self rightView] cleanUpSVTKSupport];
  [[self rightView] removeFromSuperview];
}

@end
