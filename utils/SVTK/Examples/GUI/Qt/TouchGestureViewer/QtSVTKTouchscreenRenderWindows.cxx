
#include "QtSVTKTouchscreenRenderWindows.h"
#include "ui_QtSVTKTouchscreenRenderWindows.h"

// Available interactions:
// - Tap: Randomizes background color and moves the sphere actor to the location of the tap point
//      = Touchscreen: 1 finger
// - Tap and hold: Switches camera between perspective and orthographic view and moves the cylinder
// to the location of the tap point
//      = Touchscreen and MacOS trackpad: 1 finger
// - Swipe: Changes the color of the Square/Sphere/Cylinder based on the swipe angle. Angle -> Hue
//      = Touchscreen: 3 fingers
// - Pinch : Zoom in and out the view, centered on the location of the pinch
//      = Touchscreen and MacOS trackpad: 2 fingers
// - Rotate: Rotate the view, centered on the location of the pinch
//      = Touchscreen and MacOS trackpad: 2 fingers
// - Pan: Translate the view
//      = Touchscreen: 2+ fingers
//      = MacOS trackpad: Long tap and move

#include <svtkActor.h>
#include <svtkCamera.h>
#include <svtkCollectionIterator.h>
#include <svtkCubeSource.h>
#include <svtkCylinderSource.h>
#include <svtkGenericOpenGLRenderWindow.h>
#include <svtkInteractorStyleMultiTouchCamera.h>
#include <svtkPlane.h>
#include <svtkPolyDataMapper.h>
#include <svtkProperty.h>
#include <svtkRenderer.h>
#include <svtkSphereSource.h>
#include <svtkTransform.h>

svtkNew<svtkActor> cubeActor;
svtkNew<svtkActor> sphereActor;
svtkNew<svtkActor> cylinderActor;

svtkNew<svtkSphereSource> sphereSource;
svtkNew<svtkCubeSource> cubeSource;
svtkNew<svtkCylinderSource> cylinderSource;

svtkNew<svtkTransform> sphereTransform;
svtkNew<svtkTransform> cubeTransform;
svtkNew<svtkTransform> cylinderTransform;

class svtkInteractorStyleMultiTouchCameraExample : public svtkInteractorStyleMultiTouchCamera
{
public:
  static svtkInteractorStyleMultiTouchCameraExample* New();
  svtkTypeMacro(svtkInteractorStyleMultiTouchCameraExample, svtkInteractorStyleMultiTouchCamera);

  void GetPickPosition(double pickPosition[4])
  {
    svtkCamera* camera = this->CurrentRenderer->GetActiveCamera();
    if (!camera)
    {
      return;
    }

    int pointer = this->Interactor->GetPointerIndex();

    this->FindPokedRenderer(this->Interactor->GetEventPositions(pointer)[0],
      this->Interactor->GetEventPositions(pointer)[1]);

    double* focalPointWorld = camera->GetFocalPoint();
    double focalPointDisplay[3] = { 0, 0, 0 };
    svtkInteractorObserver::ComputeWorldToDisplay(this->CurrentRenderer, focalPointWorld[0],
      focalPointWorld[1], focalPointWorld[2], focalPointDisplay);

    // New position at the center of the pinch gesture
    int* touchPositionDisplay = this->Interactor->GetEventPositions(pointer);
    double pickPoint[4] = { 0, 0, 0, 0 };
    svtkInteractorObserver::ComputeDisplayToWorld(this->CurrentRenderer, touchPositionDisplay[0],
      touchPositionDisplay[1], focalPointDisplay[2], pickPosition);
  }

  void OnLongTap() override
  {
    if (!this->CurrentRenderer)
    {
      return;
    }

    svtkCamera* camera = this->CurrentRenderer->GetActiveCamera();
    if (!camera)
    {
      return;
    }

    camera->SetParallelProjection(!camera->GetParallelProjection());

    double pickPoint[4] = { 0, 0, 0, 0 };
    this->GetPickPosition(pickPoint);
    cylinderTransform->Identity();
    cylinderTransform->Translate(pickPoint);

    this->CurrentRenderer->Render();
  }

  void OnTap() override
  {
    if (!this->CurrentRenderer)
    {
      return;
    }

    this->CurrentRenderer->SetBackground(
      (double)rand() / RAND_MAX, (double)rand() / RAND_MAX, (double)rand() / RAND_MAX);

    double pickPoint[4] = { 0, 0, 0, 0 };
    this->GetPickPosition(pickPoint);
    sphereTransform->Identity();
    sphereTransform->Translate(pickPoint);

    this->CurrentRenderer->Render();
  }

  bool IsSwiping = false;
  void OnStartSwipe() override
  {
    this->IsSwiping = true;
    this->StartGesture();
  }

  void OnEndSwipe() override
  {
    this->IsSwiping = false;
    this->EndGesture();
  }

  void OnSwipe() override
  {
    if (!this->CurrentRenderer)
    {
      return;
    }

    double hsv[3] = { this->Interactor->GetRotation() / 360.0, 1.0, 1.0 };
    double rgb[3];
    svtkMath::HSVToRGB(hsv, rgb);

    cubeActor->GetProperty()->SetColor(rgb);
    sphereActor->GetProperty()->SetColor(rgb);
    cylinderActor->GetProperty()->SetColor(rgb);

    this->CurrentRenderer->Render();
  }

  void OnPinch() override
  {
    if (this->IsSwiping)
    {
      return;
    }
    Superclass::OnPinch();
  }

  void OnRotate() override
  {
    if (this->IsSwiping)
    {
      return;
    }
    Superclass::OnRotate();
  }

  void OnPan() override
  {
    if (this->IsSwiping)
    {
      return;
    }
    Superclass::OnPan();
  }
};
svtkStandardNewMacro(svtkInteractorStyleMultiTouchCameraExample);

//----------------------------------------------------------------------------
QtSVTKTouchscreenRenderWindows::QtSVTKTouchscreenRenderWindows(int svtkNotUsed(argc), char* argv[])
{
  this->ui = new Ui_QtSVTKTouchscreenRenderWindows;
  this->ui->setupUi(this);

  svtkSmartPointer<svtkGenericOpenGLRenderWindow> renderWindow =
    svtkSmartPointer<svtkGenericOpenGLRenderWindow>::New();
  this->ui->view->setRenderWindow(renderWindow);

  svtkSmartPointer<svtkRenderer> renderer = svtkSmartPointer<svtkRenderer>::New();
  this->ui->view->renderWindow()->AddRenderer(renderer);

  svtkRenderWindowInteractor* interactor = this->ui->view->interactor();
  svtkSmartPointer<svtkInteractorStyleMultiTouchCameraExample> interactorStyle =
    svtkSmartPointer<svtkInteractorStyleMultiTouchCameraExample>::New();
  interactor->SetInteractorStyle(interactorStyle);
  renderWindow->SetInteractor(interactor);

  // Create a cube.
  cubeSource->SetXLength(0.5);
  cubeSource->SetYLength(0.5);
  cubeSource->SetZLength(0.5);

  // Create a mapper and actor.
  svtkSmartPointer<svtkPolyDataMapper> mapper = svtkSmartPointer<svtkPolyDataMapper>::New();
  mapper->SetInputConnection(cubeSource->GetOutputPort());
  cubeActor->SetMapper(mapper);
  renderer->AddActor(cubeActor);

  // Create a sphere.
  sphereSource->SetRadius(0.125);

  // Create a mapper and actor.
  svtkSmartPointer<svtkPolyDataMapper> sphereMapper = svtkSmartPointer<svtkPolyDataMapper>::New();
  sphereMapper->SetInputConnection(sphereSource->GetOutputPort());
  sphereActor->SetMapper(sphereMapper);
  sphereActor->SetUserTransform(sphereTransform);
  renderer->AddActor(sphereActor);

  // Create a cylinder.
  cylinderSource->SetRadius(0.125);
  cylinderSource->SetHeight(0.25);

  // Create a mapper and actor.
  svtkSmartPointer<svtkPolyDataMapper> cylinderMapper = svtkSmartPointer<svtkPolyDataMapper>::New();
  cylinderMapper->SetInputConnection(cylinderSource->GetOutputPort());
  cylinderActor->SetMapper(cylinderMapper);
  cylinderActor->SetUserTransform(cylinderTransform);
  renderer->AddActor(cylinderActor);

  renderer->SetBackground(0.1, 0.2, 0.4);
};
