/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestGPURayCastMultiVolumeOverlapping.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * Tests rendering 3 overlapping volumes as inputs in svtkGPUVolumeRCMapper
 * svtkMultiVolume.
 */
#include "svtkAxesActor.h"
#include "svtkCamera.h"
#include "svtkColorTransferFunction.h"
#include "svtkCommand.h"
#include "svtkConeSource.h"
#include "svtkGPUVolumeRayCastMapper.h"
#include "svtkImageResample.h"
#include "svtkImageResize.h"
#include "svtkInteractorStyleTrackballCamera.h"
#include "svtkMultiVolume.h"
#include "svtkNew.h"
#include "svtkPiecewiseFunction.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkTestUtilities.h"
#include "svtkVolume16Reader.h"
#include "svtkVolumeProperty.h"
#include "svtkXMLImageDataReader.h"

#include "svtkAbstractMapper.h"
#include "svtkImageData.h"
#include "svtkOutlineFilter.h"
#include "svtkPolyDataMapper.h"

#include "svtkMath.h"
#include <chrono>

namespace
{
class MoveRotateCommand : public svtkCommand
{
public:
  static MoveRotateCommand* New() { return new MoveRotateCommand; }

  void Execute(svtkObject* caller, unsigned long eventId, void* /*data*/) override
  {
    switch (eventId)
    {
      case svtkCommand::KeyPressEvent:
      {
        auto interactor = svtkRenderWindowInteractor::SafeDownCast(caller);
        const std::string key = interactor->GetKeySym();

        double times[3] = { 0, 0, 0 };
        double timesAngle[3] = { 0, 0, 0 };
        // Translations
        if (key == "Left")
        {
          times[0] = -1;
        }
        else if (key == "Right")
        {
          times[0] = 1;
        }
        else if (key == "Down")
        {
          times[1] = -1;
        }
        else if (key == "Up")
        {
          times[1] = 1;
        }
        else if (key == "n")
        {
          times[2] = -1;
        }
        else if (key == "m")
        {
          times[2] = 1;
        }
        // Rotations
        else if (key == "a")
        {
          timesAngle[0] = -1;
        }
        else if (key == "s")
        {
          timesAngle[0] = 1;
        }
        else if (key == "z")
        {
          timesAngle[1] = -1;
        }
        else if (key == "x")
        {
          timesAngle[1] = 1;
        }
        else if (key == "c")
        {
          timesAngle[2] = -1;
        }
        else if (key == "v")
        {
          timesAngle[2] = 1;
        }
        else
          break;

        const double deltaX = times[0] * Delta;
        const double deltaY = times[1] * Delta;
        const double deltaZ = times[2] * Delta;
        const auto pos = this->Volume->GetPosition();
        this->Volume->SetPosition(pos[0] + deltaX, pos[1] + deltaY, pos[2] + deltaZ);

        const double rotX = timesAngle[0] * DeltaAngle;
        this->Volume->RotateX(rotX);
        const double rotY = timesAngle[1] * DeltaAngle;
        this->Volume->RotateY(rotY);
        const double rotZ = timesAngle[2] * DeltaAngle;
        this->Volume->RotateZ(rotZ);
      }
      break;
    }

    this->RenderWindow->Render();
  };

  void SetVolume(svtkProp3D* vol) { this->Volume = vol; }

  svtkRenderWindow* RenderWindow = nullptr;
  svtkProp3D* Volume = nullptr;
  double Delta = 10.0;
  double DeltaAngle = 5.0;
};
}

////////////////////////////////////////////////////////////////////////////////
int TestGPURayCastMultiVolumeOverlapping(int argc, char* argv[])
{
  // Load data
  svtkNew<svtkVolume16Reader> reader;
  reader->SetDataDimensions(64, 64);
  reader->SetImageRange(1, 93);
  reader->SetDataByteOrderToLittleEndian();
  char* fname = svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/headsq/quarter");
  reader->SetFilePrefix(fname);
  delete[] fname;
  reader->SetDataSpacing(3.2, 3.2, 1.5);

  svtkNew<svtkXMLImageDataReader> vaseSource;
  const char* volumeFile = svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/vase_1comp.vti");
  vaseSource->SetFileName(volumeFile);
  delete[] volumeFile;

  svtkSmartPointer<svtkXMLImageDataReader> xmlReader = svtkSmartPointer<svtkXMLImageDataReader>::New();
  char* filename = svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/hncma-atlas.vti");
  xmlReader->SetFileName(filename);
  xmlReader->Update();
  delete[] filename;
  filename = nullptr;

  // Geometry
  svtkNew<svtkConeSource> coneSource;
  coneSource->SetRadius(30.);
  coneSource->SetHeight(50.);
  coneSource->SetResolution(40);
  svtkNew<svtkPolyDataMapper> coneMapper;
  coneMapper->SetInputConnection(coneSource->GetOutputPort());
  svtkNew<svtkActor> cone;
  cone->SetMapper(coneMapper);
  cone->RotateX(35.);
  cone->RotateY(-245.);
  cone->SetPosition(160., 90., -30.);

  svtkNew<svtkAxesActor> axis;
  axis->SetTotalLength(100., 100., 100.);
  axis->SetNormalizedTipLength(0.1, 0.1, 0.1);
  axis->SetNormalizedShaftLength(1., 1., 1.);
  axis->AxisLabelsOff();
  axis->SetConeRadius(0.5);

  // Volume 0 (upsampled headmr)
  // ---------------------------
  svtkNew<svtkImageResize> headmrSource;
  headmrSource->SetInputConnection(reader->GetOutputPort());
  headmrSource->SetResizeMethodToOutputDimensions();
  headmrSource->SetOutputDimensions(128, 128, 128);
  headmrSource->Update();

  svtkNew<svtkColorTransferFunction> ctf;
  ctf->AddRGBPoint(0, 0.0, 0.0, 0.0);
  ctf->AddRGBPoint(500, 1.0, 0.5, 0.3);
  ctf->AddRGBPoint(1000, 1.0, 0.5, 0.3);
  ctf->AddRGBPoint(1150, 1.0, 1.0, 0.9);

  svtkNew<svtkPiecewiseFunction> pf;
  pf->AddPoint(0, 0.00);
  pf->AddPoint(500, 0.15);
  pf->AddPoint(1000, 0.15);
  pf->AddPoint(1150, 0.85);

  svtkNew<svtkPiecewiseFunction> gf;
  gf->AddPoint(0, 0.0);
  gf->AddPoint(90, 0.1);
  gf->AddPoint(100, 0.7);

  svtkNew<svtkVolume> vol;
  vol->GetProperty()->SetScalarOpacity(pf);
  vol->GetProperty()->SetColor(ctf);
  vol->GetProperty()->SetGradientOpacity(gf);
  vol->GetProperty()->SetInterpolationType(SVTK_LINEAR_INTERPOLATION);
  // Note: Shading is currently not supported with multi-volume active
  //->ShadeOn();

  // Volume 1 (vase)
  // -----------------------------
  svtkNew<svtkColorTransferFunction> ctf1;
  ctf1->AddRGBPoint(0, 0.0, 0.0, 0.0);
  ctf1->AddRGBPoint(500, 0.1, 1.0, 0.3);
  ctf1->AddRGBPoint(1000, 0.1, 1.0, 0.3);
  ctf1->AddRGBPoint(1150, 1.0, 1.0, 0.9);

  svtkNew<svtkPiecewiseFunction> pf1;
  pf1->AddPoint(0, 0.0);
  pf1->AddPoint(500, 1.0);

  svtkNew<svtkPiecewiseFunction> gf1;
  gf1->AddPoint(0, 0.0);
  gf1->AddPoint(550, 1.0);

  svtkNew<svtkVolume> vol1;
  vol1->GetProperty()->SetScalarOpacity(pf1);
  vol1->GetProperty()->SetColor(ctf1);
  vol1->GetProperty()->SetGradientOpacity(gf1);
  vol1->GetProperty()->SetInterpolationType(SVTK_LINEAR_INTERPOLATION);

  vol1->RotateX(-55.);
  vol1->SetPosition(80., 50., 130.);

  // Volume 2 (brain)
  // -----------------------------
  svtkNew<svtkPiecewiseFunction> pf2;
  pf1->AddPoint(0, 0.0);
  pf1->AddPoint(5022, 0.09);

  svtkNew<svtkColorTransferFunction> ctf2;
  ctf2->AddRGBPoint(0, 1.0, 0.3, 0.2);
  ctf2->AddRGBPoint(2511, 0.3, 0.2, 0.9);
  ctf2->AddRGBPoint(5022, 0.5, 0.6, 1.0);

  svtkNew<svtkPiecewiseFunction> gf2;
  gf2->AddPoint(0, 0.0);
  gf2->AddPoint(550, 0.5);

  svtkNew<svtkVolume> vol2;
  vol2->GetProperty()->SetScalarOpacity(pf2);
  vol2->GetProperty()->SetColor(ctf2);
  // vol2->GetProperty()->SetGradientOpacity(gf2);
  vol2->GetProperty()->SetInterpolationType(SVTK_LINEAR_INTERPOLATION);

  vol2->SetScale(0.8, 0.8, 0.8);
  vol2->SetPosition(210., 200., -90.);
  vol2->RotateX(90.);
  vol2->RotateY(-95.);
  vol2->RotateZ(-5.);

  // Multi volume instance
  // ---------------------
  // Create an overlapping volume prop (add specific properties to each
  // entity).
  svtkNew<svtkMultiVolume> overlappingVol;
  svtkNew<svtkGPUVolumeRayCastMapper> mapper;
  mapper->UseJitteringOn();
  overlappingVol->SetMapper(mapper);

  mapper->SetInputConnection(0, headmrSource->GetOutputPort());
  overlappingVol->SetVolume(vol, 0);

  mapper->SetInputConnection(2, vaseSource->GetOutputPort());
  overlappingVol->SetVolume(vol1, 2);

  mapper->SetInputConnection(4, xmlReader->GetOutputPort());
  overlappingVol->SetVolume(vol2, 4);

  // Rendering context
  svtkNew<svtkRenderWindow> renWin;
  renWin->SetSize(512, 512);
  renWin->SetMultiSamples(0);

  svtkNew<svtkRenderer> ren;
  renWin->AddRenderer(ren);
  ren->SetBackground(1.0, 1.0, 1.0);

  ren->AddActor(axis);
  ren->AddActor(cone);
  ren->AddVolume(overlappingVol);

  svtkNew<svtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  svtkNew<MoveRotateCommand> moveComm;
  moveComm->SetVolume(vol1);
  moveComm->RenderWindow = renWin;
  iren->AddObserver(svtkCommand::KeyPressEvent, moveComm);

  svtkNew<svtkInteractorStyleTrackballCamera> style;
  iren->SetInteractorStyle(style);

  auto cam = ren->GetActiveCamera();
  cam->SetFocalPoint(41.9596, -17.9662, 78.5903);
  cam->SetPosition(373.891, 619.954, -53.5932);
  cam->SetViewUp(-0.0358384, -0.184856, -0.982112);
  renWin->Render();

  int retVal = svtkTesting::Test(argc, argv, renWin, 90);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !((retVal == svtkTesting::PASSED) || (retVal == svtkTesting::DO_INTERACTOR));
}
