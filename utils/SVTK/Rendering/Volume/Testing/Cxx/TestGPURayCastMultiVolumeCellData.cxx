/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestGPURayCastMultiVolumeCellData.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * Sets two inputs in svtkGPUVolumeRayCastMapper and uses a svtkMultiVolume
 * instance to render the two inputs simultaneously (one point-data and one
 * cell-data). Each svtkVolume contains independent transfer functions (one
 * a set of 1D Tfs and the other a 2D Tf).
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
#include "svtkNrrdReader.h"
#include "svtkPNGReader.h"
#include "svtkPiecewiseFunction.h"
#include "svtkPointData.h"
#include "svtkPointDataToCellData.h"
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

svtkSmartPointer<svtkImageData> ConvertImageToFloat(svtkDataObject* image)
{
  auto imageIn = svtkImageData::SafeDownCast(image);

  svtkSmartPointer<svtkImageData> imageOut = svtkSmartPointer<svtkImageData>::New();
  imageOut->SetDimensions(imageIn->GetDimensions());
  imageOut->AllocateScalars(SVTK_FLOAT, 4);

  auto arrayIn = imageIn->GetPointData()->GetScalars();
  auto arrayOut = imageOut->GetPointData()->GetScalars();
  const auto numTuples = arrayOut->GetNumberOfTuples();

  for (svtkIdType i = 0; i < numTuples; i++)
  {
    double* value = arrayIn->GetTuple(i);

    double valuef[4];
    valuef[0] = value[0] / 255.;
    valuef[1] = value[1] / 255.;
    valuef[2] = value[2] / 255.;
    valuef[3] = value[3] / 255.;
    arrayOut->SetTuple(i, valuef);
  }

  // return std::move(imageOut);
  return imageOut;
}

}

////////////////////////////////////////////////////////////////////////////////
int TestGPURayCastMultiVolumeCellData(int argc, char* argv[])
{
  // Load data
  svtkNew<svtkVolume16Reader> headReader;
  headReader->SetDataDimensions(64, 64);
  headReader->SetImageRange(1, 93);
  headReader->SetDataByteOrderToLittleEndian();
  char* fname = svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/headsq/quarter");
  headReader->SetFilePrefix(fname);
  headReader->SetDataSpacing(3.2, 3.2, 1.5);
  delete[] fname;

  fname = svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/tooth.nhdr");
  svtkNew<svtkNrrdReader> toothReader;
  toothReader->SetFileName(fname);
  delete[] fname;

  svtkNew<svtkPNGReader> reader2dtf;
  fname = svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/tooth_2dtransf.png");
  reader2dtf->SetFileName(fname);
  reader2dtf->Update();
  delete[] fname;

  svtkNew<svtkAxesActor> axis;
  axis->SetTotalLength(100., 100., 100.);
  axis->SetNormalizedTipLength(0.1, 0.1, 0.1);
  axis->SetNormalizedShaftLength(1., 1., 1.);
  axis->AxisLabelsOff();
  axis->SetConeRadius(0.5);

  // Volume 0 (upsampled headmr)
  // ---------------------------
  // Transform the head dataset to cells
  svtkNew<svtkImageResize> headmrSource;
  headmrSource->SetInputConnection(headReader->GetOutputPort());
  headmrSource->SetResizeMethodToOutputDimensions();
  headmrSource->SetOutputDimensions(128, 128, 128);

  svtkNew<svtkPointDataToCellData> pointsToCells;
  pointsToCells->SetInputConnection(headmrSource->GetOutputPort());
  pointsToCells->Update();

  svtkNew<svtkColorTransferFunction> ctf;
  ctf->AddRGBPoint(0, 0.0, 0.0, 0.0);
  ctf->AddRGBPoint(500, 0.1, 0.6, 0.3);
  ctf->AddRGBPoint(1000, 0.1, 0.6, 0.3);
  ctf->AddRGBPoint(1150, 1.0, 1.0, 0.9);

  svtkNew<svtkPiecewiseFunction> pf;
  pf->AddPoint(0, 0.00);
  pf->AddPoint(500, 0.15);
  pf->AddPoint(1000, 0.15);
  pf->AddPoint(1150, 0.85);

  svtkNew<svtkPiecewiseFunction> gf;
  gf->AddPoint(0, 0.0);
  gf->AddPoint(90, 0.07);
  gf->AddPoint(100, 0.7);

  svtkNew<svtkVolume> vol;
  vol->GetProperty()->SetScalarOpacity(pf);
  vol->GetProperty()->SetColor(ctf);
  vol->GetProperty()->SetGradientOpacity(gf);
  vol->GetProperty()->SetInterpolationType(SVTK_LINEAR_INTERPOLATION);
  // vol->GetProperty()->ShadeOn();

  // Volume 1 (tooth)
  // -----------------------------
  svtkNew<svtkVolume> vol1;
  auto tf2d = ConvertImageToFloat(reader2dtf->GetOutputDataObject(0));
  vol1->GetProperty()->SetTransferFunction2D(tf2d);
  vol1->GetProperty()->SetInterpolationType(SVTK_LINEAR_INTERPOLATION);

  vol1->RotateX(180.);
  vol1->RotateZ(90.);
  vol1->SetScale(1.8, 1.8, 1.8);
  vol1->SetPosition(175., 190., 210.);

  // Multi volume instance
  // ---------------------
  // Create an overlapping volume prop (add specific properties to each
  // entity).
  svtkNew<svtkMultiVolume> overlappingVol;
  svtkNew<svtkGPUVolumeRayCastMapper> mapper;
  overlappingVol->SetMapper(mapper);

  mapper->SetInputConnection(0, pointsToCells->GetOutputPort());
  overlappingVol->SetVolume(vol, 0);

  mapper->SetInputConnection(3, toothReader->GetOutputPort());
  overlappingVol->SetVolume(vol1, 3);

  mapper->SetUseJittering(1);

  // Rendering context
  svtkNew<svtkRenderWindow> renWin;
  renWin->SetSize(800, 400);
  renWin->SetMultiSamples(0);

  // Outside renderer (left)
  svtkNew<svtkRenderer> ren;
  renWin->AddRenderer(ren);
  ren->SetBackground(1.0, 1.0, 1.0);
  ren->SetViewport(0.0, 0.0, 0.5, 1.0);

  ren->AddActor(axis);
  ren->AddVolume(overlappingVol);

  svtkNew<svtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  svtkNew<svtkInteractorStyleTrackballCamera> style;
  iren->SetInteractorStyle(style);

  auto cam = ren->GetActiveCamera();
  cam->SetFocalPoint(85.7721, 88.4044, 33.8576);
  cam->SetPosition(-173.392, 611.09, -102.892);
  cam->SetViewUp(0.130638, -0.194997, -0.972065);

  // Inside renderer (right)
  svtkNew<svtkRenderer> ren2;
  renWin->AddRenderer(ren2);
  ren2->SetBackground(1.0, 1.0, 1.0);
  ren2->SetViewport(0.5, 0.0, 1.0, 1.0);
  ren2->AddVolume(overlappingVol);

  cam = ren2->GetActiveCamera();
  cam->SetFocalPoint(97.8834, 78.0104, 31.3285);
  cam->SetPosition(99.8672, 68.0964, 91.3188);
  cam->SetViewUp(-0.00395866, 0.986589, 0.163175);

  renWin->Render();

  int retVal = svtkTesting::Test(argc, argv, renWin, 90);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !((retVal == svtkTesting::PASSED) || (retVal == svtkTesting::DO_INTERACTOR));
}
