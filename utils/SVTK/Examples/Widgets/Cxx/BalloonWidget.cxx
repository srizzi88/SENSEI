#include <svtkActor.h>
#include <svtkBalloonRepresentation.h>
#include <svtkBalloonWidget.h>
#include <svtkCommand.h>
#include <svtkInteractorStyleTrackball.h>
#include <svtkInteractorStyleTrackballCamera.h>
#include <svtkPolyData.h>
#include <svtkPolyDataMapper.h>
#include <svtkRegularPolygonSource.h>
#include <svtkRenderWindow.h>
#include <svtkRenderWindowInteractor.h>
#include <svtkRenderer.h>
#include <svtkSmartPointer.h>
#include <svtkSphereSource.h>

int main(int, char*[])
{
  // Sphere
  svtkSmartPointer<svtkSphereSource> sphereSource = svtkSmartPointer<svtkSphereSource>::New();
  sphereSource->SetCenter(-4.0, 0.0, 0.0);
  sphereSource->SetRadius(4.0);

  svtkSmartPointer<svtkPolyDataMapper> sphereMapper = svtkSmartPointer<svtkPolyDataMapper>::New();
  sphereMapper->SetInputConnection(sphereSource->GetOutputPort());

  svtkSmartPointer<svtkActor> sphereActor = svtkSmartPointer<svtkActor>::New();
  sphereActor->SetMapper(sphereMapper);

  // Regular Polygon
  svtkSmartPointer<svtkRegularPolygonSource> regularPolygonSource =
    svtkSmartPointer<svtkRegularPolygonSource>::New();
  regularPolygonSource->SetCenter(4.0, 0.0, 0.0);
  regularPolygonSource->SetRadius(4.0);

  svtkSmartPointer<svtkPolyDataMapper> regularPolygonMapper =
    svtkSmartPointer<svtkPolyDataMapper>::New();
  regularPolygonMapper->SetInputConnection(regularPolygonSource->GetOutputPort());

  svtkSmartPointer<svtkActor> regularPolygonActor = svtkSmartPointer<svtkActor>::New();
  regularPolygonActor->SetMapper(regularPolygonMapper);

  // A renderer and render window
  svtkSmartPointer<svtkRenderer> renderer = svtkSmartPointer<svtkRenderer>::New();
  svtkSmartPointer<svtkRenderWindow> renderWindow = svtkSmartPointer<svtkRenderWindow>::New();
  renderWindow->AddRenderer(renderer);

  // An interactor
  svtkSmartPointer<svtkRenderWindowInteractor> renderWindowInteractor =
    svtkSmartPointer<svtkRenderWindowInteractor>::New();
  renderWindowInteractor->SetRenderWindow(renderWindow);

  // Create the widget
  svtkSmartPointer<svtkBalloonRepresentation> balloonRep =
    svtkSmartPointer<svtkBalloonRepresentation>::New();
  balloonRep->SetBalloonLayoutToImageRight();

  svtkSmartPointer<svtkBalloonWidget> balloonWidget = svtkSmartPointer<svtkBalloonWidget>::New();
  balloonWidget->SetInteractor(renderWindowInteractor);
  balloonWidget->SetRepresentation(balloonRep);
  balloonWidget->AddBalloon(sphereActor, "This is a sphere", nullptr);
  balloonWidget->AddBalloon(regularPolygonActor, "This is a regular polygon", nullptr);

  // Add the actors to the scene
  renderer->AddActor(sphereActor);
  renderer->AddActor(regularPolygonActor);

  // Render an image (lights and cameras are created automatically)
  renderWindow->Render();
  balloonWidget->EnabledOn();

  // Begin mouse interaction
  renderWindowInteractor->Start();

  return EXIT_SUCCESS;
}
