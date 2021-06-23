#include "svtkGeoGraticule.h"

#include "svtkActor.h"
#include "svtkGeoProjection.h"
#include "svtkGeoTransform.h"
#include "svtkPNGWriter.h"
#include "svtkPolyDataMapper.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkSmartPointer.h"
#include "svtkTestUtilities.h"
#include "svtkTransformFilter.h"
#include "svtkWindowToImageFilter.h"
#include "svtkXMLPolyDataReader.h"

#define svtkCreateMacro(type, obj) svtkSmartPointer<type> obj = svtkSmartPointer<type>::New()

int TestGeoGraticule(int argc, char* argv[])
{
  int latLevel = 2;
  int lngLevel = 2;
  const char* pname = "rouss";
  svtkCreateMacro(svtkGeoGraticule, ggr);
  svtkCreateMacro(svtkGeoTransform, xfm);
  svtkCreateMacro(svtkGeoProjection, gcs);
  svtkCreateMacro(svtkGeoProjection, pcs);
  svtkCreateMacro(svtkTransformFilter, xff);
  svtkCreateMacro(svtkXMLPolyDataReader, pdr);
  svtkCreateMacro(svtkTransformFilter, xf2);
  svtkCreateMacro(svtkPolyDataMapper, mapper);
  svtkCreateMacro(svtkPolyDataMapper, mapper2);
  svtkCreateMacro(svtkActor, actor);
  svtkCreateMacro(svtkActor, actor2);

  ggr->SetGeometryType(svtkGeoGraticule::POLYLINES);
  ggr->SetLatitudeLevel(latLevel);
  ggr->SetLongitudeLevel(lngLevel);
  ggr->SetLongitudeBounds(-180, 180);
  ggr->SetLatitudeBounds(-90, 90);

  // gcs defaults to latlong.
  pcs->SetName(pname);
  pcs->SetCentralMeridian(0.);
  xfm->SetSourceProjection(gcs);
  xfm->SetDestinationProjection(pcs);
  xff->SetInputConnection(ggr->GetOutputPort());
  xff->SetTransform(xfm);
  mapper->SetInputConnection(xff->GetOutputPort());
  actor->SetMapper(mapper);

  char* input_file = svtkTestUtilities::ExpandDataFileName(argc, argv, "/Data/political.vtp");
  pdr->SetFileName(input_file);

  xf2->SetTransform(xfm);
  xf2->SetInputConnection(pdr->GetOutputPort());
  mapper2->SetInputConnection(xf2->GetOutputPort());
  actor2->SetMapper(mapper2);

  svtkCreateMacro(svtkRenderWindow, win);
  win->SetMultiSamples(0);
  svtkCreateMacro(svtkRenderer, ren);
  svtkCreateMacro(svtkRenderWindowInteractor, iren);
  win->SetInteractor(iren);
  win->AddRenderer(ren);
  ren->AddActor(actor);
  ren->AddActor(actor2);

  win->Render();
  int retVal = svtkRegressionTestImage(win);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Initialize();
    iren->Start();
  }

  delete[] input_file;

  return !retVal;
}
