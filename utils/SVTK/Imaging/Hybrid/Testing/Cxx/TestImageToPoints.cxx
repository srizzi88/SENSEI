#include "svtkActor.h"
#include "svtkCamera.h"
#include "svtkGlyph3DMapper.h"
#include "svtkImageData.h"
#include "svtkImageMapToColors.h"
#include "svtkImageReader2.h"
#include "svtkImageToImageStencil.h"
#include "svtkImageToPoints.h"
#include "svtkProperty.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkScalarsToColors.h"
#include "svtkSmartPointer.h"
#include "svtkSphereSource.h"
#include "svtkTestUtilities.h"

int TestImageToPoints(int argc, char* argv[])
{
  // Test svtkImageToPoints by converting a few slices of the headsq
  // data set into points and glyphing them.

  char* fname = svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/headsq/quarter");
  std::string filename = fname;
  delete[] fname;

  int extent[6] = { 0, 63, 0, 63, 0, 3 };
  double origin[3] = { 0.0, 0.0, 0.0 };
  double spacing[3] = { 3.2, 3.2, 1.5 };
  double center[3] = { 0.5 * 3.2 * 63, 0.5 * 3.2 * 63, 0.5 * 1.5 * 3 };

  svtkSmartPointer<svtkImageReader2> reader = svtkSmartPointer<svtkImageReader2>::New();
  reader->SetDataByteOrderToLittleEndian();
  reader->SetDataExtent(extent);
  reader->SetDataOrigin(origin);
  reader->SetDataSpacing(spacing);
  reader->SetFileNameSliceOffset(40);
  reader->SetFilePrefix(filename.c_str());

  // convert the image into color scalars
  svtkSmartPointer<svtkScalarsToColors> table = svtkSmartPointer<svtkScalarsToColors>::New();
  table->SetRange(0, 2000);

  svtkSmartPointer<svtkImageMapToColors> colors = svtkSmartPointer<svtkImageMapToColors>::New();
  colors->SetInputConnection(reader->GetOutputPort());
  colors->SetLookupTable(table);
  colors->SetOutputFormatToRGB();

  // generate a stencil by thresholding the image
  svtkSmartPointer<svtkImageToImageStencil> stencil = svtkSmartPointer<svtkImageToImageStencil>::New();
  stencil->SetInputConnection(reader->GetOutputPort());
  stencil->ThresholdBetween(800, 4000);

  // generate a point set
  svtkSmartPointer<svtkImageToPoints> imageToPointSet = svtkSmartPointer<svtkImageToPoints>::New();
  imageToPointSet->SetInputConnection(colors->GetOutputPort());
  imageToPointSet->SetStencilConnection(stencil->GetOutputPort());
  imageToPointSet->SetOutputPointsPrecision(svtkAlgorithm::SINGLE_PRECISION);
  imageToPointSet->Update();

  // generate a sphere for each point
  svtkSmartPointer<svtkSphereSource> sphere = svtkSmartPointer<svtkSphereSource>::New();
  sphere->SetRadius(1.5);

  // display the points as glyphs
  svtkSmartPointer<svtkGlyph3DMapper> mapper = svtkSmartPointer<svtkGlyph3DMapper>::New();
  mapper->ScalingOff();
  mapper->SetInputConnection(imageToPointSet->GetOutputPort());
  mapper->SetSourceConnection(sphere->GetOutputPort());

  svtkSmartPointer<svtkActor> actor = svtkSmartPointer<svtkActor>::New();
  actor->SetMapper(mapper);

  svtkSmartPointer<svtkRenderWindow> renWin = svtkSmartPointer<svtkRenderWindow>::New();
  renWin->SetSize(256, 256);

  svtkSmartPointer<svtkRenderWindowInteractor> iren =
    svtkSmartPointer<svtkRenderWindowInteractor>::New();
  iren->SetRenderWindow(renWin);

  svtkSmartPointer<svtkRenderer> renderer = svtkSmartPointer<svtkRenderer>::New();
  renderer->AddViewProp(actor);
  renWin->AddRenderer(renderer);

  svtkCamera* camera = renderer->GetActiveCamera();
  camera->SetFocalPoint(center);
  camera->SetPosition(center[0], center[1], center[2] - 400.0);

  iren->Initialize();
  renWin->Render();
  iren->Start();

  return EXIT_SUCCESS;
}
