#include "svtkActor.h"
#include "svtkAppendPolyData.h"
#include "svtkCallbackCommand.h"
#include "svtkCamera.h"
#include "svtkCellData.h"
#include "svtkColorTransferFunction.h"
#include "svtkDepthSortPolyData.h"
#include "svtkPolyDataMapper.h"
#include "svtkProperty.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkSmartPointer.h"
#include "svtkSphereSource.h"

int TestDepthSortPolyData(int argc, char* argv[])
{
  svtkRenderer* ren = svtkRenderer::New();
  ren->SetBackground(1.0, 1.0, 1.0);

  svtkRenderWindow* renWin = svtkRenderWindow::New();
  renWin->SetSize(400, 400);
  renWin->AddRenderer(ren);
  ren->Delete();

  svtkRenderWindowInteractor* iren = svtkRenderWindowInteractor::New();
  iren->SetRenderWindow(renWin);
  renWin->Delete();

  // generate some geometry for each mode and dir
  int sortMode[] = { svtkDepthSortPolyData::SVTK_SORT_FIRST_POINT,
    svtkDepthSortPolyData::SVTK_SORT_BOUNDS_CENTER,
    svtkDepthSortPolyData::SVTK_SORT_PARAMETRIC_CENTER };

  int sortDir[] = { svtkDepthSortPolyData::SVTK_DIRECTION_BACK_TO_FRONT,
    svtkDepthSortPolyData::SVTK_DIRECTION_FRONT_TO_BACK,
    svtkDepthSortPolyData::SVTK_DIRECTION_SPECIFIED_VECTOR };

  svtkCamera* cam = svtkCamera::New();
  cam->SetPosition(1, 2, 0);
  cam->SetFocalPoint(1, 1, 0);

  for (size_t j = 0; j < sizeof(sortMode) / sizeof(int); ++j)
  {
    for (size_t i = 0; i < sizeof(sortDir) / sizeof(int); ++i)
    {
      svtkSphereSource* ss = svtkSphereSource::New();
      ss->SetThetaResolution(64);
      ss->SetPhiResolution(64);
      ss->SetRadius(0.25);
      ss->SetCenter(j, i, 0.0);
      ss->Update();

      svtkDepthSortPolyData* ds = svtkDepthSortPolyData::New();
      ds->SetDirection(sortDir[i]);
      ds->SetDepthSortMode(sortMode[j]);
      ds->SortScalarsOn();
      ds->SetInputConnection(ss->GetOutputPort(0));
      if (i == svtkDepthSortPolyData::SVTK_DIRECTION_SPECIFIED_VECTOR)
      {
        ds->SetOrigin(0.0, 0.0, 0.0);
        ds->SetVector(0.5, 0.5, 0.125);
      }
      else
      {
        ds->SetCamera(cam);
      }
      ss->Delete();

      svtkPolyDataMapper* pdm = svtkPolyDataMapper::New();
      pdm->SetInputConnection(ds->GetOutputPort(0));
      ds->Delete();

      svtkIdType nc = ss->GetOutput()->GetNumberOfCells();
      svtkColorTransferFunction* lut = svtkColorTransferFunction::New();
      lut->SetColorSpaceToRGB();
      lut->AddRGBPoint(0.0, 0.0, 0.0, 1.0);
      lut->AddRGBPoint(nc, 1.0, 0.0, 0.0);
      lut->SetColorSpaceToDiverging();
      lut->Build();
      pdm->SetLookupTable(lut);
      pdm->SetScalarVisibility(1);
      pdm->SelectColorArray("sortedCellIds");
      pdm->SetUseLookupTableScalarRange(1);
      pdm->SetScalarMode(SVTK_SCALAR_MODE_USE_CELL_FIELD_DATA);
      lut->Delete();

      svtkActor* act = svtkActor::New();
      act->SetMapper(pdm);
      pdm->Delete();

      ren->AddActor(act);
      act->Delete();
    }
  }

  cam->Delete();
  cam = ren->GetActiveCamera();
  cam->SetPosition(1, 1, 10);
  ren->ResetCamera();
  cam->Zoom(1.25);

  iren->Initialize();
  renWin->Render();

  int retVal = svtkRegressionTestImage(renWin);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  iren->Delete();
  return !retVal;
}
