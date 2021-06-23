#include "svtkInteractorStyleTrackballCamera.h"
#include "svtkObjectFactory.h"
#include "svtkPolyDataMapper.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkRendererCollection.h"
#include "svtkSphereSource.h"
#include "svtkStringArray.h"
#include "svtkTestUtilities.h"
#include "svtkXMLPolyDataReader.h"

class svtkTestDragInteractorStyle : public svtkInteractorStyleTrackballCamera
{
public:
  static svtkTestDragInteractorStyle* New();
  svtkTypeMacro(svtkTestDragInteractorStyle, svtkInteractorStyleTrackballCamera);

  void OnDropLocation(double* position) override
  {
    this->Location[0] = position[0];
    this->Location[1] = position[1];
    this->Location[2] = 0.0;
  }

  void OnDropFiles(svtkStringArray* filePaths) override
  {
    svtkRenderWindowInteractor* rwi = this->GetInteractor();

    const char* path = filePaths->GetValue(0);

    svtkNew<svtkXMLPolyDataReader> reader;
    reader->SetFileName(path);

    svtkNew<svtkPolyDataMapper> mapper;
    mapper->SetInputConnection(reader->GetOutputPort());

    svtkNew<svtkActor> actor;
    actor->SetMapper(mapper);

    svtkRenderer* ren = rwi->GetRenderWindow()->GetRenderers()->GetFirstRenderer();
    ren->AddActor(actor);

    // move actor to location
    ren->SetDisplayPoint(this->Location);
    ren->DisplayToWorld();
    actor->SetPosition(ren->GetWorldPoint());

    rwi->GetRenderWindow()->Render();
  }

  double Location[3];
};

svtkStandardNewMacro(svtkTestDragInteractorStyle);

int TestDragEvent(int argc, char* argv[])
{
  svtkNew<svtkRenderer> renderer;
  svtkNew<svtkRenderWindow> renWin;
  renWin->SetMultiSamples(0);
  renWin->AddRenderer(renderer);

  svtkNew<svtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  svtkNew<svtkTestDragInteractorStyle> style;
  iren->SetInteractorStyle(style);

  svtkNew<svtkSphereSource> sphere;
  sphere->SetRadius(5.0);
  svtkNew<svtkPolyDataMapper> mapper;
  mapper->SetInputConnection(sphere->GetOutputPort());

  svtkNew<svtkActor> actor;
  actor->SetMapper(mapper);

  renderer->AddActor(actor);

  renWin->Render();

  char* fname = svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/cow.vtp");

  svtkNew<svtkStringArray> pathArray;
  pathArray->InsertNextValue(fname);

  delete[] fname;

  // Manually invoke drag and drop events for this test
  // These events are usually invoked when a file is dropped on the render window
  // from a file manager
  double loc[2] = { 100.0, 250.0 };
  iren->InvokeEvent(svtkCommand::UpdateDropLocationEvent, loc);
  iren->InvokeEvent(svtkCommand::DropFilesEvent, pathArray);

  renWin->Render();

  // Compare image
  int retVal = svtkRegressionTestImage(renWin);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
