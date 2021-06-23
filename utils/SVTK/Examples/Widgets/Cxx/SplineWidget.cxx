#include "svtkActor.h"
#include "svtkCamera.h"
#include "svtkCommand.h"
#include "svtkImageData.h"
#include "svtkImagePlaneWidget.h"
#include "svtkInteractorEventRecorder.h"
#include "svtkKochanekSpline.h"
#include "svtkParametricSpline.h"
#include "svtkPlaneSource.h"
#include "svtkPointData.h"
#include "svtkPolyData.h"
#include "svtkPolyDataMapper.h"
#include "svtkProperty.h"
#include "svtkProperty2D.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkSplineWidget.h"
#include "svtkTextProperty.h"

// Callback for the spline widget interaction
class svtkSplineWidgetCallback : public svtkCommand
{
public:
  static svtkSplineWidgetCallback* New() { return new svtkSplineWidgetCallback; }
  virtual void Execute(svtkObject* caller, unsigned long, void*)
  {
    svtkSplineWidget* spline = reinterpret_cast<svtkSplineWidget*>(caller);
    spline->GetPolyData(Poly);
  }
  svtkSplineWidgetCallback()
    : Poly(0)
  {
  }
  svtkPolyData* Poly;
};

int main(int, char*[])
{
  svtkRenderer* ren1 = svtkRenderer::New();
  svtkRenderWindow* renWin = svtkRenderWindow::New();
  renWin->AddRenderer(ren1);

  svtkRenderWindowInteractor* iren = svtkRenderWindowInteractor::New();
  iren->SetRenderWindow(renWin);

  svtkPlaneSource* planeSource = svtkPlaneSource::New();
  planeSource->Update();

  svtkPolyDataMapper* planeSourceMapper = svtkPolyDataMapper::New();
  planeSourceMapper->SetInput(planeSource->GetOutput());
  svtkActor* planeSourceActor = svtkActor::New();
  planeSourceActor->SetMapper(planeSourceMapper);

  svtkSplineWidget* spline = svtkSplineWidget::New();
  spline->SetInteractor(iren);
  spline->SetInput(planeSource->GetOutput());
  spline->SetPriority(1.0);
  spline->KeyPressActivationOff();
  spline->PlaceWidget();
  spline->ProjectToPlaneOn();
  spline->SetProjectionNormal(0);
  spline->SetProjectionPosition(102.4); // initial plane oriented position
  spline->SetProjectionNormal(3);       // allow arbitrary oblique orientations
  spline->SetPlaneSource(planeSource);

  // Specify the type of spline (change from default svtkCardinalSpline)
  svtkKochanekSpline* xspline = svtkKochanekSpline::New();
  svtkKochanekSpline* yspline = svtkKochanekSpline::New();
  svtkKochanekSpline* zspline = svtkKochanekSpline::New();

  svtkParametricSpline* para = spline->GetParametricSpline();

  para->SetXSpline(xspline);
  para->SetYSpline(yspline);
  para->SetZSpline(zspline);

  svtkPolyData* poly = svtkPolyData::New();
  spline->GetPolyData(poly);

  svtkSplineWidgetCallback* swcb = svtkSplineWidgetCallback::New();
  swcb->Poly = poly;

  spline->AddObserver(svtkCommand::InteractionEvent, swcb);

  ren1->SetBackground(0.1, 0.2, 0.4);
  ren1->AddActor(planeSourceActor);

  renWin->SetSize(600, 300);
  renWin->Render();

  spline->On();
  spline->SetNumberOfHandles(4);
  spline->SetNumberOfHandles(5);
  spline->SetResolution(399);

  // Set up an interesting viewpoint
  svtkCamera* camera = ren1->GetActiveCamera();

  // Render the image
  iren->Initialize();
  renWin->Render();

  return EXIT_SUCCESS;
}
