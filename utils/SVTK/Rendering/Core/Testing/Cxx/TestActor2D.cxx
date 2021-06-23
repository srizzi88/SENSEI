#include <svtkActor.h>
#include <svtkActor2D.h>
#include <svtkCoordinate.h>
#include <svtkLineSource.h>
#include <svtkLookupTable.h>
#include <svtkNew.h>
#include <svtkPlaneSource.h>
#include <svtkPolyDataMapper.h>
#include <svtkPolyDataMapper2D.h>
#include <svtkProperty.h>
#include <svtkProperty2D.h>
#include <svtkRenderWindow.h>
#include <svtkRenderWindowInteractor.h>
#include <svtkRenderer.h>

#include <svtkRegressionTestImage.h>
#include <svtkTestUtilities.h>

int TestActor2D(int argc, char* argv[])
{
  svtkNew<svtkLookupTable> lut;
  lut->SetNumberOfTableValues(6);
  lut->SetTableRange(0.0, 1.0);

  svtkNew<svtkPlaneSource> planeSource1;
  planeSource1->SetOrigin(0.0, 0.0, 0.0);
  planeSource1->SetPoint1(0.5, 0.0, 0.0);
  planeSource1->SetPoint2(0.0, 0.5, 0.0);

  svtkNew<svtkPolyDataMapper> mapper1;
  mapper1->SetInputConnection(planeSource1->GetOutputPort());
  mapper1->ScalarVisibilityOn();
  mapper1->SetLookupTable(lut);
  mapper1->UseLookupTableScalarRangeOn();
  mapper1->SetScalarModeToUsePointFieldData();
  mapper1->ColorByArrayComponent("TextureCoordinates", 0);
  mapper1->InterpolateScalarsBeforeMappingOn();

  svtkNew<svtkActor> actor1;
  actor1->SetMapper(mapper1);
  actor1->GetProperty()->SetColor(1.0, 0.0, 0.0);

  svtkNew<svtkPlaneSource> planeSource2;
  planeSource2->SetOrigin(-0.5, 0.0, 0.0);
  planeSource2->SetPoint1(0.0, 0.0, 0.0);
  planeSource2->SetPoint2(-0.5, 0.5, 0.0);

  svtkNew<svtkCoordinate> pCoord;
  pCoord->SetCoordinateSystemToWorld();

  svtkNew<svtkCoordinate> coord;
  coord->SetCoordinateSystemToNormalizedViewport();
  coord->SetReferenceCoordinate(pCoord);

  svtkNew<svtkPolyDataMapper2D> mapper2;
  mapper2->SetInputConnection(planeSource2->GetOutputPort());
  mapper2->SetLookupTable(lut);
  mapper2->ScalarVisibilityOff();
  mapper2->SetTransformCoordinate(coord);

  svtkNew<svtkActor2D> actor2;
  actor2->SetMapper(mapper2);
  actor2->GetProperty()->SetColor(1.0, 1.0, 0.0);

  svtkNew<svtkRenderer> renderer;
  renderer->AddActor(actor1);
  renderer->AddActor(actor2);

  svtkNew<svtkRenderWindow> renWin;
  renWin->AddRenderer(renderer);

  svtkNew<svtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);
  renWin->Render();
  renderer->ResetCamera();
  renderer->SetBackground(1.0, 0.0, 0.0);
  renWin->SetSize(300, 300);

  int retVal = svtkRegressionTestImage(renWin);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
