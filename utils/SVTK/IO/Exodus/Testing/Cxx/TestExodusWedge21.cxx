#include "svtkActor.h"
#include "svtkCamera.h"
#include "svtkCellData.h"
#include "svtkCompositePolyDataMapper.h"
#include "svtkDataArray.h"
#include "svtkDataSet.h"
#include "svtkDataSetSurfaceFilter.h"
#include "svtkExodusIIReader.h"
#include "svtkMultiBlockDataSet.h"
#include "svtkNew.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkTestUtilities.h"

int TestExodusWedge21(int argc, char* argv[])
{
  char* fname = svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/wedge21.g");
  if (!fname)
  {
    cout << "Could not obtain filename for test data.\n";
    return 1;
  }

  svtkNew<svtkExodusIIReader> rdr;
  if (!rdr->CanReadFile(fname))
  {
    cout << "Cannot read \"" << fname << "\"\n";
    return 1;
  }
  rdr->SetFileName(fname);
  rdr->Update();
  svtkIndent ind;

  svtkNew<svtkDataSetSurfaceFilter> surface;
  svtkNew<svtkCompositePolyDataMapper> mapper;
  svtkNew<svtkActor> actor;
  svtkNew<svtkRenderWindow> renWin;
  svtkNew<svtkRenderer> ren;
  svtkNew<svtkRenderWindowInteractor> iren;

  surface->SetInputConnection(rdr->GetOutputPort());
  mapper->SetInputConnection(surface->GetOutputPort());
  actor->SetMapper(mapper);
  renWin->AddRenderer(ren);
  iren->SetRenderWindow(renWin);

  ren->AddActor(actor);
  ren->SetBackground(1, 1, 1);
  renWin->SetSize(300, 300);
  auto cam = ren->GetActiveCamera();
  cam->SetPosition(10., 10., 5.);
  cam->SetViewUp(0., 0.4, 1.);
  ren->ResetCamera();
  renWin->Render();

  int retVal = svtkRegressionTestImage(renWin);

  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  delete[] fname;
  return !retVal;
}
