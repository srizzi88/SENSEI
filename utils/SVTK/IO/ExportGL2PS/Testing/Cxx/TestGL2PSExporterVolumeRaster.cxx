/*=========================================================================

  Program:   Visualization Toolkit
  Module:    GL2PSExporterVolumeRaster.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkGL2PSExporter.h"
#include "svtkRegressionTestImage.h"
#include "svtkTestUtilities.h"

#include "svtkCamera.h"
#include "svtkColorTransferFunction.h"
#include "svtkCone.h"
#include "svtkCubeAxesActor2D.h"
#include "svtkDataArray.h"
#include "svtkImageData.h"
#include "svtkImageShiftScale.h"
#include "svtkNew.h"
#include "svtkPiecewiseFunction.h"
#include "svtkPointData.h"
#include "svtkProperty.h"
#include "svtkProperty2D.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkSampleFunction.h"
#include "svtkSmartPointer.h"
#include "svtkSmartVolumeMapper.h"
#include "svtkTestingInteractor.h"
#include "svtkVolume.h"
#include "svtkVolumeProperty.h"

#include <string>

int TestGL2PSExporterVolumeRaster(int, char*[])
{
  svtkNew<svtkCone> coneFunction;
  svtkNew<svtkSampleFunction> coneSample;
  coneSample->SetImplicitFunction(coneFunction);
  coneSample->SetOutputScalarTypeToFloat();
  coneSample->SetSampleDimensions(127, 127, 127);
  coneSample->SetModelBounds(-1.0, 1.0, -1.0, 1.0, -1.0, 1.0);
  coneSample->SetCapping(false);
  coneSample->SetComputeNormals(false);
  coneSample->SetScalarArrayName("volume");
  coneSample->Update();

  svtkNew<svtkImageShiftScale> coneShift;
  double range[2];
  coneSample->GetOutput()->GetPointData()->GetScalars("volume")->GetRange(range);
  coneShift->SetInputConnection(coneSample->GetOutputPort());
  coneShift->SetShift(-range[0]);
  double mag = range[1] - range[0];
  if (mag == 0.0)
  {
    mag = 1.0;
  }
  coneShift->SetScale(255.0 / mag);
  coneShift->SetOutputScalarTypeToUnsignedChar();
  coneShift->Update();

  svtkNew<svtkSmartVolumeMapper> coneMapper;
  coneMapper->SetInputConnection(coneShift->GetOutputPort());
  coneMapper->SetBlendModeToComposite();
  coneMapper->SetRequestedRenderModeToRayCast();

  svtkNew<svtkVolumeProperty> volProp;
  volProp->ShadeOff();
  volProp->SetInterpolationTypeToLinear();

  svtkNew<svtkPiecewiseFunction> opacity;
  opacity->AddPoint(0.0, 0.9);
  opacity->AddPoint(20.0, 0.9);
  opacity->AddPoint(40.0, 0.3);
  opacity->AddPoint(90.0, 0.8);
  opacity->AddPoint(100.1, 0.0);
  opacity->AddPoint(255.0, 0.0);
  volProp->SetScalarOpacity(opacity);

  svtkNew<svtkColorTransferFunction> color;
  color->AddRGBPoint(0.0, 0.0, 0.0, 1.0);
  color->AddRGBPoint(20.0, 0.0, 1.0, 1.0);
  color->AddRGBPoint(40.0, 0.5, 0.0, 1.0);
  color->AddRGBPoint(80.0, 1.0, 0.2, 0.3);
  color->AddRGBPoint(255.0, 1.0, 1.0, 1.0);
  volProp->SetColor(color);

  svtkNew<svtkVolume> coneVolume;
  coneVolume->SetMapper(coneMapper);
  coneVolume->SetProperty(volProp);

  svtkNew<svtkCubeAxesActor2D> axes;
  axes->SetInputConnection(coneShift->GetOutputPort());
  axes->SetFontFactor(2.0);
  axes->SetCornerOffset(0.0);
  axes->GetProperty()->SetColor(0.0, 0.0, 0.0);

  svtkNew<svtkRenderer> ren;
  axes->SetCamera(ren->GetActiveCamera());
  ren->AddActor(coneVolume);
  ren->AddActor(axes);
  ren->SetBackground(0.2, 0.3, 0.5);

  svtkNew<svtkRenderWindow> renWin;
  renWin->AddRenderer(ren);

  svtkNew<svtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  svtkSmartPointer<svtkCamera> camera = ren->GetActiveCamera();
  ren->ResetCamera();
  camera->Azimuth(30);

  renWin->SetSize(500, 500);
  renWin->Render();

  svtkNew<svtkGL2PSExporter> exp;
  exp->SetRenderWindow(renWin);
  exp->SetFileFormatToPS();
  exp->CompressOff();
  exp->SetSortToBSP();
  exp->DrawBackgroundOn();
  exp->Write3DPropsAsRasterImageOn();

  std::string fileprefix =
    svtkTestingInteractor::TempDirectory + std::string("/TestGL2PSExporterVolumeRaster");

  exp->SetFilePrefix(fileprefix.c_str());
  exp->Write();

  exp->SetFileFormatToPDF();
  exp->Write();

  renWin->SetMultiSamples(0);
  renWin->GetInteractor()->Initialize();
  renWin->GetInteractor()->Start();

  return EXIT_SUCCESS;
}
