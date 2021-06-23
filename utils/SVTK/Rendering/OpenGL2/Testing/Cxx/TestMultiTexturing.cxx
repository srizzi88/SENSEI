#include "svtkActor.h"
#include "svtkCamera.h"
#include "svtkFloatArray.h"
#include "svtkJPEGReader.h"
#include "svtkNew.h"
#include "svtkOpenGLRenderWindow.h"
#include "svtkPointData.h"
#include "svtkPolyDataMapper.h"
#include "svtkProperty.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkTIFFReader.h"
#include "svtkTestUtilities.h"
#include "svtkTexture.h"
#include "svtkTexturedSphereSource.h"

//----------------------------------------------------------------------------
int TestMultiTexturing(int argc, char* argv[])
{
  svtkNew<svtkRenderer> renderer;
  renderer->SetBackground(0.5, 0.5, 0.5);
  svtkNew<svtkRenderWindow> renderWindow;
  renderWindow->SetSize(300, 300);
  renderWindow->AddRenderer(renderer);
  svtkNew<svtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renderWindow);

  svtkNew<svtkTexturedSphereSource> sphere;
  sphere->SetThetaResolution(64);
  sphere->SetPhiResolution(32);
  sphere->Update();
  svtkPolyData* pd = sphere->GetOutput();

  svtkFloatArray* tcoord = svtkFloatArray::SafeDownCast(pd->GetPointData()->GetTCoords());
  svtkNew<svtkFloatArray> tcoord2;
  tcoord2->SetNumberOfComponents(2);
  tcoord2->SetNumberOfTuples(tcoord->GetNumberOfTuples());
  for (int i = 0; i < tcoord->GetNumberOfTuples(); ++i)
  {
    float tmp[2];
    tcoord->GetTypedTuple(i, tmp);
    // mess with the tcoords to make sure
    // this array is getting used
    tcoord2->SetTuple2(i, tmp[0], tmp[1] * 2.0);
  }
  tcoord2->SetName("tcoord2");
  pd->GetPointData()->AddArray(tcoord2);

  svtkNew<svtkPolyDataMapper> mapper;
  mapper->SetInputData(pd);
  svtkNew<svtkActor> actor;
  renderer->AddActor(actor);
  actor->SetMapper(mapper);

  const char* file1 = svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/GIS/raster.tif");
  svtkNew<svtkTIFFReader> reader1;
  reader1->SetFileName(file1);
  delete[] file1;

  svtkNew<svtkTexture> tex1;
  tex1->InterpolateOn();
  tex1->SetInputConnection(reader1->GetOutputPort());
  actor->GetProperty()->SetTexture("earth_color", tex1);

  const char* file2 = svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/clouds.jpeg");
  svtkNew<svtkJPEGReader> reader2;
  reader2->SetFileName(file2);
  delete[] file2;

  svtkNew<svtkTexture> tex2;
  tex2->InterpolateOn();
  tex2->SetBlendingMode(svtkTexture::SVTK_TEXTURE_BLENDING_MODE_MODULATE);
  tex2->SetBlendingMode(svtkTexture::SVTK_TEXTURE_BLENDING_MODE_ADD);
  tex2->SetInputConnection(reader2->GetOutputPort());
  actor->GetProperty()->SetTexture("skyclouds", tex2);

  mapper->MapDataArrayToMultiTextureAttribute(
    "skyclouds", "tcoord2", svtkDataObject::FIELD_ASSOCIATION_POINTS);

  renderWindow->SetMultiSamples(0);
  renderer->ResetCamera();
  renderer->GetActiveCamera()->Elevation(-45);
  renderer->GetActiveCamera()->OrthogonalizeViewUp();
  renderer->GetActiveCamera()->Zoom(1.5);
  renderer->ResetCameraClippingRange();
  renderWindow->Render();

  int retVal = svtkRegressionTestImage(renderWindow);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
