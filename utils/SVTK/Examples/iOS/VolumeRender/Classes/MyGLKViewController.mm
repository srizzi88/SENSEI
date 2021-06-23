/*=========================================================================

Program:   Visualization Toolkit

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#import "MyGLKViewController.h"
#include "svtk/svtkIOSRenderWindow.h"
#include "svtk/svtkIOSRenderWindowInteractor.h"
#include "svtk/svtkRenderingOpenGL2ObjectFactory.h"

#include "svtk/svtkNew.h"

#include "svtk/svtkActor.h"
#include "svtk/svtkCamera.h"
#include "svtk/svtkColorTransferFunction.h"
#include "svtk/svtkConeSource.h"
#include "svtk/svtkDebugLeaks.h"
#include "svtk/svtkGlyph3D.h"
#include "svtk/svtkImageCast.h"
#include "svtk/svtkImageData.h"
#include "svtk/svtkNrrdReader.h"
#include "svtk/svtkOpenGLGPUVolumeRayCastMapper.h"
#include "svtk/svtkPiecewiseFunction.h"
#include "svtk/svtkPointData.h"
#include "svtk/svtkPolyData.h"
#include "svtk/svtkPolyDataMapper.h"
#include "svtk/svtkRTAnalyticSource.h"
#include "svtk/svtkRenderWindow.h"
#include "svtk/svtkRenderer.h"
#include "svtk/svtkSmartPointer.h"
#include "svtk/svtkSphereSource.h"
#include "svtk/svtkTextActor.h"
#include "svtk/svtkTextProperty.h"
#include "svtk/svtkVolume.h"
#include "svtk/svtkVolumeProperty.h"

#include "svtk/svtkActor.h"
#include "svtk/svtkActor2D.h"
#include "svtk/svtkCamera.h"
#include "svtk/svtkCommand.h"
#include "svtk/svtkInteractorStyleMultiTouchCamera.h"
#include "svtk/svtkMath.h"
#include "svtk/svtkPoints.h"
#include "svtk/svtkPolyDataMapper.h"
#include "svtk/svtkRenderer.h"
#include "svtk/svtkTextMapper.h"
#include "svtk/svtkTextProperty.h"

#include <deque>

@interface MyGLKViewController ()
{
}

@property (strong, nonatomic) EAGLContext* context;
- (void)tearDownGL;
@end

@implementation MyGLKViewController

//----------------------------------------------------------------------------
- (svtkIOSRenderWindow*)getSVTKRenderWindow
{
  return _mySVTKRenderWindow;
}

//----------------------------------------------------------------------------
- (void)setSVTKRenderWindow:(svtkIOSRenderWindow*)theSVTKRenderWindow
{
  _mySVTKRenderWindow = theSVTKRenderWindow;
}

//----------------------------------------------------------------------------
- (svtkIOSRenderWindowInteractor*)getInteractor
{
  if (_mySVTKRenderWindow)
  {
    return (svtkIOSRenderWindowInteractor*)_mySVTKRenderWindow->GetInteractor();
  }
  else
  {
    return NULL;
  }
}

- (void)setupPipeline
{
  // Register GL2 objects
  svtkObjectFactory::RegisterFactory(svtkRenderingOpenGL2ObjectFactory::New());

  svtkIOSRenderWindow* renWin = svtkIOSRenderWindow::New();
  // renWin->DebugOn();
  [self setSVTKRenderWindow:renWin];

  // this example uses SVTK's built in interaction but you could choose
  // to use your own instead.
  svtkRenderWindowInteractor* iren = svtkRenderWindowInteractor::New();
  svtkNew<svtkInteractorStyleMultiTouchCamera> ismt;
  ismt->DebugOn();
  iren->SetInteractorStyle(ismt.Get());
  iren->SetRenderWindow(renWin);

  svtkNew<svtkRenderer> renderer;
  renWin->AddRenderer(renderer.Get());

  svtkNew<svtkOpenGLGPUVolumeRayCastMapper> volumeMapper;

#if 0
    svtkNew<svtkRTAnalyticSource> wavelet;
    wavelet->SetWholeExtent(-127, 128,
                            -127, 128,
                            -127, 128);
    wavelet->SetCenter(0.0, 0.0, 0.0);

    svtkNew<svtkImageCast> ic;
    ic->SetInputConnection(wavelet->GetOutputPort());
    ic->SetOutputScalarTypeToUnsignedChar();
    volumeMapper->SetInputConnection(ic->GetOutputPort());
#else
  NSArray* paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
  NSString* basePath = paths.firstObject;
  std::string fname([basePath UTF8String]);
  fname += "/CT-chest-quantized.nrrd";
  svtkNew<svtkNrrdReader> mi;
  mi->SetFileName(fname.c_str());
  mi->Update();

  double range[2];
  mi->GetOutput()->GetPointData()->GetScalars()->GetRange(range);

  volumeMapper->SetInputConnection(mi->GetOutputPort());
#endif

  volumeMapper->SetAutoAdjustSampleDistances(1);
  volumeMapper->SetSampleDistance(0.5);

  svtkNew<svtkVolumeProperty> volumeProperty;
  volumeProperty->SetShade(1);
  volumeProperty->SetInterpolationTypeToLinear();

  svtkNew<svtkColorTransferFunction> ctf;
  // ctf->AddRGBPoint(90, 0.2, 0.29, 1);
  // ctf->AddRGBPoint(157.091, 0.87, 0.87, 0.87);
  // ctf->AddRGBPoint(250, 0.7, 0.015, 0.15);

  ctf->AddRGBPoint(0, 0, 0, 0);
  ctf->AddRGBPoint(255 * 67.0106 / 3150.0, 0.54902, 0.25098, 0.14902);
  ctf->AddRGBPoint(255 * 251.105 / 3150.0, 0.882353, 0.603922, 0.290196);
  ctf->AddRGBPoint(255 * 439.291 / 3150.0, 1, 0.937033, 0.954531);
  ctf->AddRGBPoint(255 * 3071 / 3150.0, 0.827451, 0.658824, 1);

  // svtkNew<svtkPiecewiseFunction> pwf;
  // pwf->AddPoint(0, 0.0);
  // pwf->AddPoint(7000, 1.0);

  double tweak = 80.0;
  svtkNew<svtkPiecewiseFunction> pwf;
  pwf->AddPoint(0, 0);
  pwf->AddPoint(255 * (67.0106 + tweak) / 3150.0, 0);
  pwf->AddPoint(255 * (251.105 + tweak) / 3150.0, 0.3);
  pwf->AddPoint(255 * (439.291 + tweak) / 3150.0, 0.5);
  pwf->AddPoint(255 * 3071 / 3150.0, 0.616071);

  volumeProperty->SetColor(ctf.GetPointer());
  volumeProperty->SetScalarOpacity(pwf.GetPointer());

  svtkNew<svtkVolume> volume;
  volume->SetMapper(volumeMapper.GetPointer());
  volume->SetProperty(volumeProperty.GetPointer());

  renderer->SetBackground2(0.2, 0.3, 0.4);
  renderer->SetBackground(0.1, 0.1, 0.1);
  renderer->GradientBackgroundOn();
  renderer->AddVolume(volume.GetPointer());
  renderer->ResetCamera();
  renderer->GetActiveCamera()->Zoom(1.4);
}

- (void)viewDidLoad
{
  [super viewDidLoad];

  self.context = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES3];

  if (!self.context)
  {
    NSLog(@"Failed to create ES context");
  }

  GLKView* view = (GLKView*)self.view;
  view.context = self.context;
  view.drawableDepthFormat = GLKViewDrawableDepthFormat16;
  // view.drawableMultisample = GLKViewDrawableMultisample4X;

  // setup the vis pipeline
  [self setupPipeline];

  [EAGLContext setCurrentContext:self.context];
  [self resizeView];
  [self getSVTKRenderWindow] -> Render();
}

- (void)dealloc
{
  [self tearDownGL];

  if ([EAGLContext currentContext] == self.context)
  {
    [EAGLContext setCurrentContext:nil];
  }
}

- (void)didReceiveMemoryWarning
{
  [super didReceiveMemoryWarning];

  if ([self isViewLoaded] && ([[self view] window] == nil))
  {
    self.view = nil;

    [self tearDownGL];

    if ([EAGLContext currentContext] == self.context)
    {
      [EAGLContext setCurrentContext:nil];
    }
    self.context = nil;
  }

  // Dispose of any resources that can be recreated.
}

- (void)tearDownGL
{
  [EAGLContext setCurrentContext:self.context];

  // free GL resources
  // ...
}

- (void)resizeView
{
  double scale = self.view.contentScaleFactor;
  double newWidth = scale * self.view.bounds.size.width;
  double newHeight = scale * self.view.bounds.size.height;
  [self getSVTKRenderWindow] -> SetSize(newWidth, newHeight);
}

- (void)viewWillLayoutSubviews
{
  [self resizeView];
}

- (void)glkView:(GLKView*)view drawInRect:(CGRect)rect
{
  // std::cout << [self getSVTKRenderWindow]->ReportCapabilities() << std::endl;
  [self getSVTKRenderWindow] -> Render();
}

//=================================================================
// this example uses SVTK's built in interaction but you could choose
// to use your own instead. The remaining methods forward touch events
// to SVTKs interactor.

// Handles the start of a touch
- (void)touchesBegan:(NSSet*)touches withEvent:(UIEvent*)event
{
  svtkIOSRenderWindowInteractor* interactor = [self getInteractor];
  if (!interactor)
  {
    return;
  }

  CGRect bounds = [self.view bounds];
  double scale = self.view.contentScaleFactor;

  // set the position for all contacts
  NSSet* myTouches = [event touchesForView:self.view];
  for (UITouch* touch in myTouches)
  {
    // Convert touch point from UIView referential to OpenGL one (upside-down flip)
    CGPoint location = [touch locationInView:self.view];
    location.y = bounds.size.height - location.y;

    // Account for the content scaling factor
    location.x *= scale;
    location.y *= scale;

    int index = interactor->GetPointerIndexForContact((size_t)(__bridge void*)touch);
    if (index < SVTKI_MAX_POINTERS)
    {
      interactor->SetEventInformation(
        (int)round(location.x), (int)round(location.y), 0, 0, 0, 0, 0, index);
    }
  }

  // handle begin events
  for (UITouch* touch in touches)
  {
    int index = interactor->GetPointerIndexForContact((size_t)(__bridge void*)touch);
    svtkGenericWarningMacro("down touch  " << (size_t)(__bridge void*)touch << " index " << index);
    interactor->SetPointerIndex(index);
    interactor->LeftButtonPressEvent();
    // NSLog(@"Starting left mouse");
  }

  // Display the buffer
  [(GLKView*)self.view display];
}

// Handles the continuation of a touch.
- (void)touchesMoved:(NSSet*)touches withEvent:(UIEvent*)event
{
  svtkIOSRenderWindowInteractor* interactor = [self getInteractor];
  if (!interactor)
  {
    return;
  }

  CGRect bounds = [self.view bounds];
  double scale = self.view.contentScaleFactor;

  // set the position for all contacts
  int index;
  NSSet* myTouches = [event touchesForView:self.view];
  for (UITouch* touch in myTouches)
  {
    // Convert touch point from UIView referential to OpenGL one (upside-down flip)
    CGPoint location = [touch locationInView:self.view];
    location.y = bounds.size.height - location.y;

    // Account for the content scaling factor
    location.x *= scale;
    location.y *= scale;

    index = interactor->GetPointerIndexForContact((size_t)(__bridge void*)touch);
    if (index < SVTKI_MAX_POINTERS)
    {
      interactor->SetEventInformation(
        (int)round(location.x), (int)round(location.y), 0, 0, 0, 0, 0, index);
    }
  }

  // fire move event on last index
  interactor->SetPointerIndex(index);
  interactor->MouseMoveEvent();
  NSLog(@"Moved left mouse");

  // Display the buffer
  [(GLKView*)self.view display];
}

// Handles the end of a touch event when the touch is a tap.
- (void)touchesEnded:(NSSet*)touches withEvent:(UIEvent*)event
{
  svtkIOSRenderWindowInteractor* interactor = [self getInteractor];
  if (!interactor)
  {
    return;
  }

  CGRect bounds = [self.view bounds];
  double scale = self.view.contentScaleFactor;

  // set the position for all contacts
  NSSet* myTouches = [event touchesForView:self.view];
  for (UITouch* touch in myTouches)
  {
    // Convert touch point from UIView referential to OpenGL one (upside-down flip)
    CGPoint location = [touch locationInView:self.view];
    location.y = bounds.size.height - location.y;

    // Account for the content scaling factor
    location.x *= scale;
    location.y *= scale;

    int index = interactor->GetPointerIndexForContact((size_t)(__bridge void*)touch);
    if (index < SVTKI_MAX_POINTERS)
    {
      interactor->SetEventInformation(
        (int)round(location.x), (int)round(location.y), 0, 0, 0, 0, 0, index);
    }
  }

  // handle begin events
  for (UITouch* touch in touches)
  {
    int index = interactor->GetPointerIndexForContact((size_t)(__bridge void*)touch);
    svtkGenericWarningMacro("up touch  " << (size_t)(__bridge void*)touch << " index " << index);
    interactor->SetPointerIndex(index);
    interactor->LeftButtonReleaseEvent();
    interactor->ClearContact((size_t)(__bridge void*)touch);
    // NSLog(@"lifting left mouse");
  }

  // Display the buffer
  [(GLKView*)self.view display];
}

// Handles the end of a touch event.
- (void)touchesCancelled:(NSSet*)touches withEvent:(UIEvent*)event
{
  svtkIOSRenderWindowInteractor* interactor = [self getInteractor];
  if (!interactor)
  {
    return;
  }

  CGRect bounds = [self.view bounds];
  double scale = self.view.contentScaleFactor;

  UITouch* touch = [[event touchesForView:self.view] anyObject];
  // Convert touch point from UIView referential to OpenGL one (upside-down flip)
  CGPoint location = [touch locationInView:self.view];
  location.y = bounds.size.height - location.y;

  // Account for the content scaling factor
  location.x *= scale;
  location.y *= scale;

  interactor->SetEventInformation((int)round(location.x), (int)round(location.y), 0, 0, 0, 0);
  interactor->LeftButtonReleaseEvent();
  // NSLog(@"Ended left mouse");

  // Display the buffer
  [(GLKView*)self.view display];
}

@end
