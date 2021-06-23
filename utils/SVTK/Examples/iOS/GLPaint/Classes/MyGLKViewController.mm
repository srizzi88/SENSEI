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
#import "svtkIOSRenderWindow.h"
#import "svtkIOSRenderWindowInteractor.h"
#include "svtkRenderingOpenGL2ObjectFactory.h"

#include "svtkActor.h"
#include "svtkCamera.h"
#include "svtkCommand.h"
#include "svtkConeSource.h"
#include "svtkDebugLeaks.h"
#include "svtkGlyph3D.h"
#include "svtkIOSRenderWindow.h"
#include "svtkIOSRenderWindowInteractor.h"
#include "svtkInteractorStyleMultiTouchCamera.h"
#include "svtkNew.h"
#include "svtkPolyData.h"
#include "svtkPolyDataMapper.h"
#include "svtkRenderer.h"
#include "svtkSphereSource.h"

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
  svtkRenderingOpenGL2ObjectFactory* of = svtkRenderingOpenGL2ObjectFactory::New();
  svtkObjectFactory::RegisterFactory(of);

  svtkIOSRenderWindow* renWin = svtkIOSRenderWindow::New();
  // renWin->DebugOn();
  [self setSVTKRenderWindow:renWin];

  svtkNew<svtkRenderer> renderer;
  renWin->AddRenderer(renderer.Get());

  // this example uses SVTK's built in interaction but you could choose
  // to use your own instead.
  svtkRenderWindowInteractor* iren = svtkRenderWindowInteractor::New();
  iren->SetRenderWindow(renWin);

  svtkInteractorStyleMultiTouchCamera* ismt = svtkInteractorStyleMultiTouchCamera::New();
  iren->SetInteractorStyle(ismt);
  ismt->Delete();

  svtkNew<svtkSphereSource> sphere;
  sphere->SetThetaResolution(8);
  sphere->SetPhiResolution(8);

  svtkNew<svtkPolyDataMapper> sphereMapper;
  sphereMapper->SetInputConnection(sphere->GetOutputPort());
  svtkNew<svtkActor> sphereActor;
  sphereActor->SetMapper(sphereMapper.Get());

  svtkNew<svtkConeSource> cone;
  cone->SetResolution(6);

  svtkNew<svtkGlyph3D> glyph;
  glyph->SetInputConnection(sphere->GetOutputPort());
  glyph->SetSourceConnection(cone->GetOutputPort());
  glyph->SetVectorModeToUseNormal();
  glyph->SetScaleModeToScaleByVector();
  glyph->SetScaleFactor(0.25);

  svtkNew<svtkPolyDataMapper> spikeMapper;
  spikeMapper->SetInputConnection(glyph->GetOutputPort());

  svtkNew<svtkActor> spikeActor;
  spikeActor->SetMapper(spikeMapper.Get());

  renderer->AddActor(sphereActor.Get());
  renderer->AddActor(spikeActor.Get());
  renderer->SetBackground(0.4, 0.5, 0.6);
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
  view.drawableDepthFormat = GLKViewDrawableDepthFormat24;
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
  [self getSVTKRenderWindow] -> SetSize(self.view.bounds.size.width * scale,
                              self.view.bounds.size.height * scale);
}

- (void)viewWillLayoutSubviews
{
  [self resizeView];
}

- (void)glkView:(GLKView*)view drawInRect:(CGRect)rect
{
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

  // set the position for all contacts
  NSSet* myTouches = [event touchesForView:self.view];
  for (UITouch* touch in myTouches)
  {
    // Convert touch point from UIView referential to OpenGL one (upside-down flip)
    CGPoint location = [touch locationInView:self.view];
    location.y = bounds.size.height - location.y;

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
    interactor->SetPointerIndex(index);
    interactor->LeftButtonPressEvent();
    NSLog(@"Starting left mouse");
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

  // set the position for all contacts
  int index;
  NSSet* myTouches = [event touchesForView:self.view];
  for (UITouch* touch in myTouches)
  {
    // Convert touch point from UIView referential to OpenGL one (upside-down flip)
    CGPoint location = [touch locationInView:self.view];
    location.y = bounds.size.height - location.y;

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
  //  NSLog(@"Moved left mouse");

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
  // set the position for all contacts
  NSSet* myTouches = [event touchesForView:self.view];
  for (UITouch* touch in myTouches)
  {
    // Convert touch point from UIView referential to OpenGL one (upside-down flip)
    CGPoint location = [touch locationInView:self.view];
    location.y = bounds.size.height - location.y;

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
    interactor->SetPointerIndex(index);
    interactor->LeftButtonReleaseEvent();
    interactor->ClearContact((size_t)(__bridge void*)touch);
    NSLog(@"lifting left mouse");
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
  UITouch* touch = [[event touchesForView:self.view] anyObject];
  // Convert touch point from UIView referential to OpenGL one (upside-down flip)
  CGPoint location = [touch locationInView:self.view];
  location.y = bounds.size.height - location.y;

  interactor->SetEventInformation((int)round(location.x), (int)round(location.y), 0, 0, 0, 0);
  interactor->LeftButtonReleaseEvent();
  // NSLog(@"Ended left mouse");

  // Display the buffer
  [(GLKView*)self.view display];
}

@end
