#include <svtkActor.h>
#include <svtkCallbackCommand.h>
#include <svtkCommand.h>
#include <svtkInteractorStyleTrackballCamera.h>
#include <svtkPolyData.h>
#include <svtkPolyDataMapper.h>
#include <svtkProperty.h>
#include <svtkRenderWindow.h>
#include <svtkRenderWindowInteractor.h>
#include <svtkRenderer.h>
#include <svtkSliderRepresentation3D.h>
#include <svtkSliderWidget.h>
#include <svtkSmartPointer.h>
#include <svtkSphereSource.h>
#include <svtkWidgetEvent.h>
#include <svtkWidgetEventTranslator.h>

// The callback does the work.
// The callback keeps a pointer to the sphere whose resolution is
// controlled. After constructing the callback, the program sets the
// SphereSource of the callback to
// the object to be controlled.
class svtkSliderCallback : public svtkCommand
{
public:
  static svtkSliderCallback* New() { return new svtkSliderCallback; }
  void Execute(svtkObject* caller, unsigned long, void*) override
  {
    svtkSliderWidget* sliderWidget = reinterpret_cast<svtkSliderWidget*>(caller);
    int value = static_cast<int>(
      static_cast<svtkSliderRepresentation*>(sliderWidget->GetRepresentation())->GetValue());
    this->SphereSource->SetPhiResolution(value / 2);
    this->SphereSource->SetThetaResolution(value);
  }
  svtkSliderCallback()
    : SphereSource(nullptr)
  {
  }
  svtkSphereSource* SphereSource;
};

int main(int, char*[])
{
  // A sphere
  svtkSmartPointer<svtkSphereSource> sphereSource = svtkSmartPointer<svtkSphereSource>::New();
  sphereSource->SetCenter(0.0, 0.0, 0.0);
  sphereSource->SetRadius(4.0);
  sphereSource->SetPhiResolution(4);
  sphereSource->SetThetaResolution(8);

  svtkSmartPointer<svtkPolyDataMapper> mapper = svtkSmartPointer<svtkPolyDataMapper>::New();
  mapper->SetInputConnection(sphereSource->GetOutputPort());

  svtkSmartPointer<svtkActor> actor = svtkSmartPointer<svtkActor>::New();
  actor->SetMapper(mapper);
  actor->GetProperty()->SetInterpolationToFlat();

  // A renderer and render window
  svtkSmartPointer<svtkRenderer> renderer = svtkSmartPointer<svtkRenderer>::New();
  svtkSmartPointer<svtkRenderWindow> renderWindow = svtkSmartPointer<svtkRenderWindow>::New();
  renderWindow->AddRenderer(renderer);

  // An interactor
  svtkSmartPointer<svtkRenderWindowInteractor> renderWindowInteractor =
    svtkSmartPointer<svtkRenderWindowInteractor>::New();
  renderWindowInteractor->SetRenderWindow(renderWindow);

  // Add the actors to the scene
  renderer->AddActor(actor);

  // Render an image (lights and cameras are created automatically)
  renderWindow->Render();

  svtkSmartPointer<svtkSliderRepresentation3D> sliderRep =
    svtkSmartPointer<svtkSliderRepresentation3D>::New();
  sliderRep->SetMinimumValue(3.0);
  sliderRep->SetMaximumValue(50.0);
  sliderRep->SetValue(sphereSource->GetThetaResolution());
  sliderRep->SetTitleText("Sphere Resolution");
  sliderRep->GetPoint1Coordinate()->SetCoordinateSystemToWorld();
  sliderRep->GetPoint1Coordinate()->SetValue(-4, 6, 0);
  sliderRep->GetPoint2Coordinate()->SetCoordinateSystemToWorld();
  sliderRep->GetPoint2Coordinate()->SetValue(4, 6, 0);
  sliderRep->SetSliderLength(0.075);
  sliderRep->SetSliderWidth(0.05);
  sliderRep->SetEndCapLength(0.05);

  svtkSmartPointer<svtkSliderWidget> sliderWidget = svtkSmartPointer<svtkSliderWidget>::New();
  sliderWidget->SetInteractor(renderWindowInteractor);
  sliderWidget->SetRepresentation(sliderRep);
  sliderWidget->SetAnimationModeToAnimate();
  sliderWidget->EnabledOn();

  svtkSmartPointer<svtkSliderCallback> callback = svtkSmartPointer<svtkSliderCallback>::New();
  callback->SphereSource = sphereSource;

  sliderWidget->AddObserver(svtkCommand::InteractionEvent, callback);

  renderWindowInteractor->Initialize();
  renderWindow->Render();

  renderWindowInteractor->Start();

  return EXIT_SUCCESS;
}
