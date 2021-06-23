
#include "svtkActor.h"
#include "svtkConeSource.h"
#include "svtkPolyDataMapper.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkSmartPointer.h"
#include "svtkTestUtilities.h"

#define SVTK_CREATE(type, name) svtkSmartPointer<type> name = svtkSmartPointer<type>::New()

int TestGradientBackground(int argc, char* argv[])
{
  SVTK_CREATE(svtkRenderWindow, win);
  SVTK_CREATE(svtkRenderWindowInteractor, iren);
  SVTK_CREATE(svtkRenderer, ren);
  SVTK_CREATE(svtkConeSource, cone);
  SVTK_CREATE(svtkPolyDataMapper, map);
  SVTK_CREATE(svtkActor, act);

  map->SetInputConnection(cone->GetOutputPort());
  act->SetMapper(map);
  ren->AddActor(act);
  ren->GradientBackgroundOn();
  ren->SetBackground(0.8, 0.4, 0.1);
  ren->SetBackground2(0.1, 0.4, 0.8);
  win->AddRenderer(ren);
  win->SetInteractor(iren);
  win->Render();
  iren->Initialize();

  int retVal = svtkRegressionTestImage(win);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }
  return !retVal;
}
