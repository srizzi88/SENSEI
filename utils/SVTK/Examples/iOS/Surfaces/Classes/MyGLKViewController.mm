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

#include "svtkParametricBoy.h"
#include "svtkParametricConicSpiral.h"
#include "svtkParametricCrossCap.h"
#include "svtkParametricDini.h"
#include "svtkParametricEllipsoid.h"
#include "svtkParametricEnneper.h"
#include "svtkParametricFigure8Klein.h"
#include "svtkParametricKlein.h"
#include "svtkParametricMobius.h"
#include "svtkParametricRandomHills.h"
#include "svtkParametricRoman.h"
#include "svtkParametricSpline.h"
#include "svtkParametricSuperEllipsoid.h"
#include "svtkParametricSuperToroid.h"
#include "svtkParametricTorus.h"
#include "svtkSmartPointer.h"

#include "svtkActor.h"
#include "svtkActor2D.h"
#include "svtkCamera.h"
#include "svtkCommand.h"
#include "svtkInteractorStyleMultiTouchCamera.h"
#include "svtkMath.h"
#include "svtkParametricFunctionSource.h"
#include "svtkPoints.h"
#include "svtkPolyDataMapper.h"
#include "svtkRenderer.h"
#include "svtkTextMapper.h"
#include "svtkTextProperty.h"

#include <deque>

@interface MyGLKViewController ()
{
  std::deque<svtkSmartPointer<svtkParametricFunction> > parametricObjects;
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

- (void)initializeParametricObjects
{
  parametricObjects.push_back(svtkSmartPointer<svtkParametricBoy>::New());
  parametricObjects.push_back(svtkSmartPointer<svtkParametricConicSpiral>::New());
  parametricObjects.push_back(svtkSmartPointer<svtkParametricCrossCap>::New());
  parametricObjects.push_back(svtkSmartPointer<svtkParametricDini>::New());

  auto ellipsoid = svtkSmartPointer<svtkParametricEllipsoid>::New();
  ellipsoid->SetXRadius(0.5);
  ellipsoid->SetYRadius(2.0);
  parametricObjects.push_back(ellipsoid);

  parametricObjects.push_back(svtkSmartPointer<svtkParametricEnneper>::New());
  parametricObjects.push_back(svtkSmartPointer<svtkParametricFigure8Klein>::New());
  parametricObjects.push_back(svtkSmartPointer<svtkParametricKlein>::New());
  auto mobius = svtkSmartPointer<svtkParametricMobius>::New();
  mobius->SetRadius(2.0);
  mobius->SetMinimumV(-0.5);
  mobius->SetMaximumV(0.5);
  parametricObjects.push_back(mobius);

  svtkSmartPointer<svtkParametricRandomHills> randomHills =
    svtkSmartPointer<svtkParametricRandomHills>::New();
  randomHills->AllowRandomGenerationOff();
  parametricObjects.push_back(randomHills);

  parametricObjects.push_back(svtkSmartPointer<svtkParametricRoman>::New());

  auto superEllipsoid = svtkSmartPointer<svtkParametricSuperEllipsoid>::New();
  superEllipsoid->SetN1(0.5);
  superEllipsoid->SetN2(0.1);
  parametricObjects.push_back(superEllipsoid);

  auto superToroid = svtkSmartPointer<svtkParametricSuperToroid>::New();
  superToroid->SetN1(0.2);
  superToroid->SetN2(3.0);
  parametricObjects.push_back(superToroid);

  parametricObjects.push_back(svtkSmartPointer<svtkParametricTorus>::New());

  // The spline needs points
  svtkSmartPointer<svtkParametricSpline> spline = svtkSmartPointer<svtkParametricSpline>::New();
  svtkSmartPointer<svtkPoints> inputPoints = svtkSmartPointer<svtkPoints>::New();
  svtkMath::RandomSeed(8775070);
  for (int p = 0; p < 10; p++)
  {
    double x = svtkMath::Random(0.0, 1.0);
    double y = svtkMath::Random(0.0, 1.0);
    double z = svtkMath::Random(0.0, 1.0);
    inputPoints->InsertNextPoint(x, y, z);
  }
  spline->SetPoints(inputPoints);
  parametricObjects.push_back(spline);
}

- (void)setupPipeline
{
  // Register GL2 objects
  svtkObjectFactory::RegisterFactory(svtkRenderingOpenGL2ObjectFactory::New());

  //
  // Create the parametric objects
  //
  [self initializeParametricObjects];

  std::vector<svtkSmartPointer<svtkParametricFunctionSource> > parametricFunctionSources;
  std::vector<svtkSmartPointer<svtkRenderer> > renderers;
  std::vector<svtkSmartPointer<svtkPolyDataMapper> > mappers;
  std::vector<svtkSmartPointer<svtkActor> > actors;

  // No text mappers/actors in SVTK GL2 yet
#if 0
  //std::vector<svtkSmartPointer<svtkTextMapper> > textmappers;
  //std::vector<svtkSmartPointer<svtkActor2D> > textactors;

  // Create one text property for all
  //svtkSmartPointer<svtkTextProperty> textProperty =
  //svtkSmartPointer<svtkTextProperty>::New();
  //textProperty->SetFontSize(10);
  //textProperty->SetJustificationToCentered();
#endif

  // Create a parametric function source, renderer, mapper, and actor
  // for each object
  for (unsigned int i = 0; i < parametricObjects.size(); i++)
  {
    parametricFunctionSources.push_back(svtkSmartPointer<svtkParametricFunctionSource>::New());
    parametricFunctionSources[i]->SetParametricFunction(parametricObjects[i]);
    parametricFunctionSources[i]->Update();

    auto mapper = svtkSmartPointer<svtkPolyDataMapper>::New();
    assert(mapper);
    mappers.push_back(mapper);
    mappers[i]->SetInputConnection(parametricFunctionSources[i]->GetOutputPort());

    actors.push_back(svtkSmartPointer<svtkActor>::New());
    actors[i]->SetMapper(mappers[i]);

    // No text mappers/actors in SVTK GL2 yet
#if 0
    textmappers.push_back(svtkSmartPointer<svtkTextMapper>::New());
    textmappers[i]->SetInput(parametricObjects[i]->GetClassName());
    textmappers[i]->SetTextProperty(textProperty);

    textactors.push_back(svtkSmartPointer<svtkActor2D>::New());
    textactors[i]->SetMapper(textmappers[i]);
    textactors[i]->SetPosition(100, 16);
#endif
    renderers.push_back(svtkSmartPointer<svtkRenderer>::New());
  }
  unsigned int gridDimensions = 4;

  // Need a renderer even if there is no actor
  for (size_t i = parametricObjects.size(); i < gridDimensions * gridDimensions; i++)
  {
    renderers.push_back(svtkSmartPointer<svtkRenderer>::New());
  }

  svtkIOSRenderWindow* renWin = svtkIOSRenderWindow::New();
  // renWin->DebugOn();
  [self setSVTKRenderWindow:renWin];

  int rendererSize = 200;
  renWin->SetSize(rendererSize * gridDimensions, rendererSize * gridDimensions);

  for (int row = 0; row < static_cast<int>(gridDimensions); row++)
  {
    for (int col = 0; col < static_cast<int>(gridDimensions); col++)
    {
      int index = row * gridDimensions + col;

      // (xmin, ymin, xmax, ymax)
      double viewport[4] = {
        static_cast<double>(col) * rendererSize / (gridDimensions * rendererSize),
        static_cast<double>(gridDimensions - (row + 1)) * rendererSize /
          (gridDimensions * rendererSize),
        static_cast<double>(col + 1) * rendererSize / (gridDimensions * rendererSize),
        static_cast<double>(gridDimensions - row) * rendererSize / (gridDimensions * rendererSize)
      };

      renWin->AddRenderer(renderers[index]);
      renderers[index]->SetViewport(viewport);
      if (index > static_cast<int>(parametricObjects.size() - 1))
      {
        continue;
      }
      renderers[index]->AddActor(actors[index]);
      // renderers[index]->AddActor(textactors[index]);
      renderers[index]->SetBackground(.2, .3, .4);
      renderers[index]->ResetCamera();
      renderers[index]->GetActiveCamera()->Azimuth(30);
      renderers[index]->GetActiveCamera()->Elevation(-30);
      renderers[index]->GetActiveCamera()->Zoom(0.9);
      renderers[index]->ResetCameraClippingRange();
    }
  }

  // this example uses SVTK's built in interaction but you could choose
  // to use your own instead.
  svtkRenderWindowInteractor* iren = svtkRenderWindowInteractor::New();
  iren->SetRenderWindow(renWin);

  svtkInteractorStyleMultiTouchCamera* ismt = svtkInteractorStyleMultiTouchCamera::New();
  iren->SetInteractorStyle(ismt);
  ismt->Delete();
  iren->Start();
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

  svtkIOSRenderWindow* renWin = [self getSVTKRenderWindow];
  if (!renWin)
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
    interactor->SetPointerIndex(index);
    interactor->InvokeEvent(svtkCommand::LeftButtonPressEvent, NULL);
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

  svtkIOSRenderWindow* renWin = [self getSVTKRenderWindow];
  if (!renWin)
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
  interactor->InvokeEvent(svtkCommand::MouseMoveEvent, NULL);
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

  svtkIOSRenderWindow* renWin = [self getSVTKRenderWindow];
  if (!renWin)
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
    interactor->SetPointerIndex(index);
    interactor->InvokeEvent(svtkCommand::LeftButtonReleaseEvent, NULL);
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

  svtkIOSRenderWindow* renWin = [self getSVTKRenderWindow];
  if (!renWin)
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
  interactor->InvokeEvent(svtkCommand::LeftButtonReleaseEvent, NULL);
  // NSLog(@"Ended left mouse");

  // Display the buffer
  [(GLKView*)self.view display];
}

@end
