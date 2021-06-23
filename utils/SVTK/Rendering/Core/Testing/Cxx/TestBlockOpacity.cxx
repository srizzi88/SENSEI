#include "svtkActor.h"
#include "svtkArrayCalculator.h"
#include "svtkColorTransferFunction.h"
#include "svtkCompositeDataDisplayAttributes.h"
#include "svtkCompositePolyDataMapper2.h"
#include "svtkDataArray.h"
#include "svtkLookupTable.h"
#include "svtkMultiBlockDataGroupFilter.h"
#include "svtkPointData.h"
#include "svtkPolyData.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkSmartPointer.h"
#include "svtkSphereSource.h"

int TestBlockOpacity(int argc, char* argv[])
{
  // Standard rendering classes
  svtkSmartPointer<svtkRenderer> renderer = svtkSmartPointer<svtkRenderer>::New();
  svtkSmartPointer<svtkRenderWindow> renWin = svtkSmartPointer<svtkRenderWindow>::New();
  renWin->SetMultiSamples(0);
  renWin->SetAlphaBitPlanes(1);
  renWin->AddRenderer(renderer);
  svtkSmartPointer<svtkRenderWindowInteractor> iren =
    svtkSmartPointer<svtkRenderWindowInteractor>::New();
  iren->SetRenderWindow(renWin);

  // We create a multiblock dataset with one block (a sphere) and set the
  // block opacity to .75

  svtkSmartPointer<svtkSphereSource> sphere = svtkSmartPointer<svtkSphereSource>::New();
  sphere->SetRadius(0.5);
  sphere->SetCenter(0.0, 0.0, 0.0);
  sphere->SetThetaResolution(8);
  sphere->SetPhiResolution(8);
  sphere->Update();
  //  sphere->GetOutput()->GetPointData()->SetActiveScalars(name);

  svtkSmartPointer<svtkArrayCalculator> calc = svtkSmartPointer<svtkArrayCalculator>::New();
  calc->SetInputConnection(sphere->GetOutputPort());
  calc->AddCoordinateScalarVariable("x", 0);
  calc->AddCoordinateScalarVariable("y", 1);
  calc->AddCoordinateScalarVariable("z", 2);
  calc->SetFunction("(x-y)*z");
  calc->SetResultArrayName("result");
  calc->Update();

  double range[2];

  svtkDataSet::SafeDownCast(calc->GetOutput())->GetPointData()->GetScalars()->GetRange(range);

  svtkSmartPointer<svtkMultiBlockDataGroupFilter> groupDatasets =
    svtkSmartPointer<svtkMultiBlockDataGroupFilter>::New();
  groupDatasets->SetInputConnection(calc->GetOutputPort());
  groupDatasets->Update();

  svtkSmartPointer<svtkCompositePolyDataMapper2> mapper =
    svtkSmartPointer<svtkCompositePolyDataMapper2>::New();
  mapper->SetInputConnection(groupDatasets->GetOutputPort(0));
  mapper->SetColorModeToMapScalars();
  mapper->SetScalarModeToUsePointData();
  mapper->ScalarVisibilityOn();

  svtkSmartPointer<svtkCompositeDataDisplayAttributes> attrs =
    svtkSmartPointer<svtkCompositeDataDisplayAttributes>::New();
  mapper->SetCompositeDataDisplayAttributes(attrs);
  mapper->SetBlockOpacity(1, 0.5);

  svtkSmartPointer<svtkColorTransferFunction> lut = svtkSmartPointer<svtkColorTransferFunction>::New();
  // This creates a blue to red lut.
  lut->AddHSVPoint(range[0], 0.667, 1, 1);
  lut->AddHSVPoint(range[1], 0, 1, 1);
  lut->SetColorSpaceToDiverging();
  lut->SetVectorModeToMagnitude();
  mapper->SetLookupTable(lut);
  mapper->SetInterpolateScalarsBeforeMapping(1);

  svtkSmartPointer<svtkActor> actor = svtkSmartPointer<svtkActor>::New();
  actor->SetMapper(mapper);
  renderer->AddActor(actor);

  renderer->SetUseDepthPeeling(1);
  // reasonable depth peeling settings
  // no more than 50 layers of transluceny
  renderer->SetMaximumNumberOfPeels(50);
  // stop when less than 2 in 1000 pixels changes
  renderer->SetOcclusionRatio(0.002);

  // Standard testing code.
  renderer->SetBackground(0.5, 0.5, 0.5);
  renWin->SetSize(300, 300);
  renWin->Render();

  if (renderer->GetLastRenderingUsedDepthPeeling())
  {
    cout << "depth peeling was used" << endl;
  }
  else
  {
    cout << "depth peeling was not used (alpha blending instead)" << endl;
  }
  int retVal = svtkRegressionTestImage(renWin);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
