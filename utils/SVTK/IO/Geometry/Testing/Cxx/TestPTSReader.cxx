#include <svtkActor.h>
#include <svtkNew.h>
#include <svtkPTSReader.h>
#include <svtkPolyData.h>
#include <svtkPolyDataMapper.h>
#include <svtkRegressionTestImage.h>
#include <svtkRenderWindow.h>
#include <svtkRenderWindowInteractor.h>
#include <svtkRenderer.h>

int TestPTSReader(int argc, char* argv[])
{
  if (argc < 2)
  {
    std::cerr << "Required parameters: <filename> maxNumberOfPoints(optional)" << endl;
    return EXIT_FAILURE;
  }

  std::string inputFilename = argv[1];

  svtkNew<svtkPTSReader> reader;
  reader->SetFileName(inputFilename.c_str());
  reader->SetLimitToMaxNumberOfPoints(true);
  reader->SetMaxNumberOfPoints(100000);

  reader->Update();

  // Visualize
  svtkNew<svtkPolyDataMapper> mapper;
  mapper->SetInputConnection(reader->GetOutputPort());

  svtkNew<svtkActor> actor;
  actor->SetMapper(mapper);

  svtkNew<svtkRenderer> renderer;
  svtkNew<svtkRenderWindow> renderWindow;
  renderWindow->AddRenderer(renderer);
  svtkNew<svtkRenderWindowInteractor> renderWindowInteractor;
  renderWindowInteractor->SetRenderWindow(renderWindow);

  renderer->AddActor(actor);
  renderer->SetBackground(.3, .6, .3); // Background color green

  renderWindow->Render();

  int retVal = svtkRegressionTestImage(renderWindow);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    renderWindowInteractor->Start();
  }

  return !retVal;
}
