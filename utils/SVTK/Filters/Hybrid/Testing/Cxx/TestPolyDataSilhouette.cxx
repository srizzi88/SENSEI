#include <svtkActor.h>
#include <svtkCleanPolyData.h>
#include <svtkPolyData.h>
#include <svtkPolyDataMapper.h>
#include <svtkPolyDataSilhouette.h>
#include <svtkProperty.h>
#include <svtkRenderWindow.h>
#include <svtkRenderWindowInteractor.h>
#include <svtkRenderer.h>
#include <svtkSmartPointer.h>
#include <svtkSphereSource.h>
#include <svtkTestUtilities.h>
#include <svtkXMLPolyDataReader.h>

int TestPolyDataSilhouette(int argc, char* argv[])
{
  svtkSmartPointer<svtkPolyData> polyData;
  char* fname(nullptr);
  if (argc < 2)
  {
    svtkSmartPointer<svtkSphereSource> sphereSource = svtkSmartPointer<svtkSphereSource>::New();
    sphereSource->Update();

    polyData = sphereSource->GetOutput();
  }
  else
  {
    fname = svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/cow.vtp");
    svtkSmartPointer<svtkXMLPolyDataReader> reader = svtkSmartPointer<svtkXMLPolyDataReader>::New();
    reader->SetFileName(fname);

    svtkSmartPointer<svtkCleanPolyData> clean = svtkSmartPointer<svtkCleanPolyData>::New();
    clean->SetInputConnection(reader->GetOutputPort());
    clean->Update();

    polyData = clean->GetOutput();
  }

  // create mapper and actor for original model
  svtkSmartPointer<svtkPolyDataMapper> mapper = svtkSmartPointer<svtkPolyDataMapper>::New();
  mapper->SetInputData(polyData);

  svtkSmartPointer<svtkActor> actor = svtkSmartPointer<svtkActor>::New();
  actor->SetMapper(mapper);
  actor->GetProperty()->SetInterpolationToFlat();

  // create renderer and renderWindow
  svtkSmartPointer<svtkRenderer> renderer = svtkSmartPointer<svtkRenderer>::New();
  svtkSmartPointer<svtkRenderWindow> renderWindow = svtkSmartPointer<svtkRenderWindow>::New();
  renderWindow->AddRenderer(renderer);

  renderer->AddActor(actor); // view the original model

  // Compute the silhouette
  svtkSmartPointer<svtkPolyDataSilhouette> silhouette = svtkSmartPointer<svtkPolyDataSilhouette>::New();
  silhouette->SetInputData(polyData);
  silhouette->SetCamera(renderer->GetActiveCamera());
  silhouette->SetEnableFeatureAngle(0);

  // create mapper and actor for silouette
  svtkSmartPointer<svtkPolyDataMapper> mapper2 = svtkSmartPointer<svtkPolyDataMapper>::New();
  mapper2->SetInputConnection(silhouette->GetOutputPort());

  svtkSmartPointer<svtkActor> actor2 = svtkSmartPointer<svtkActor>::New();
  actor2->SetMapper(mapper2);
  actor2->GetProperty()->SetColor(1.0, 0.3882, 0.2784); // tomato
  actor2->GetProperty()->SetLineWidth(5);
  renderer->AddActor(actor2);
  renderer->SetBackground(.1, .2, .3);
  renderer->ResetCamera();

  // you MUST NOT call renderWindow->Render() before
  // iren->SetRenderWindow(renderWindow);
  svtkSmartPointer<svtkRenderWindowInteractor> iren =
    svtkSmartPointer<svtkRenderWindowInteractor>::New();
  iren->SetRenderWindow(renderWindow);

  // render and interact
  renderWindow->Render();
  iren->Start();

  delete[] fname;
  return EXIT_SUCCESS;
}
