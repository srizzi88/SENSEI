#include "svtkActor.h"
#include "svtkCellData.h"
#include "svtkCompositePolyDataMapper2.h"
#include "svtkExpandMarkedElements.h"
#include "svtkMultiBlockDataSet.h"
#include "svtkNew.h"
#include "svtkPolyData.h"
#include "svtkProperty.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkSignedCharArray.h"
#include "svtkSmartPointer.h"
#include "svtkSphereSource.h"

namespace
{

svtkSmartPointer<svtkDataSet> GetSphere(int part, int num_parts)
{
  svtkNew<svtkSphereSource> sphere;
  sphere->SetPhiResolution(6);
  sphere->SetPhiResolution(6);
  sphere->SetStartTheta(360.0 * part / num_parts);
  sphere->SetEndTheta(360.0 * (part + 1) / num_parts);
  sphere->Update();
  auto ds = sphere->GetOutput();

  svtkNew<svtkSignedCharArray> selectedCells;
  selectedCells->SetName("MarkedCells");
  selectedCells->SetNumberOfTuples(ds->GetNumberOfCells());
  selectedCells->FillComponent(0, 0);
  selectedCells->SetTypedComponent(20, 0, 1);
  ds->GetCellData()->AddArray(selectedCells);
  return ds;
}
}
int TestExpandMarkedElements(int argc, char* argv[])
{
  svtkNew<svtkMultiBlockDataSet> mb;
  for (int cc = 0; cc < 3; ++cc)
  {
    mb->SetBlock(cc, ::GetSphere(cc, 3));
  }

  svtkNew<svtkExpandMarkedElements> filter;
  filter->SetInputDataObject(mb);
  filter->SetInputArrayToProcess(0, 0, 0, svtkDataObject::FIELD_ASSOCIATION_CELLS, "MarkedCells");

  svtkNew<svtkCompositePolyDataMapper2> mapper;
  mapper->SetInputConnection(filter->GetOutputPort());
  mapper->SetScalarModeToUseCellFieldData();
  mapper->SelectColorArray("MarkedCells");

  svtkNew<svtkActor> actor;
  actor->SetMapper(mapper);
  actor->GetProperty()->EdgeVisibilityOn();

  svtkNew<svtkRenderer> renderer;
  renderer->AddActor(actor);

  svtkNew<svtkRenderWindow> renWin;
  svtkNew<svtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);
  renWin->AddRenderer(renderer);

  int retVal = svtkRegressionTestImage(renWin);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
