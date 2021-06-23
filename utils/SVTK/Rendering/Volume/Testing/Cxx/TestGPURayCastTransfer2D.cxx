/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestGPURayCastTransfer2D.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * Test 2D transfer function support in GPUVolumeRayCastMapper.  The transfer
 * function is created manually using known value/gradient histogram information
 * of the test data (tooth.hdr). A filter to create these histograms will be
 * added in the future.
 */

#include "svtkCamera.h"
#include "svtkColorTransferFunction.h"
#include "svtkFloatArray.h"
#include "svtkGPUVolumeRayCastMapper.h"
#include "svtkImageData.h"
#include "svtkInteractorStyleTrackballCamera.h"
#include "svtkNew.h"
#include "svtkNrrdReader.h"
#include "svtkPiecewiseFunction.h"
#include "svtkPointData.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkTestUtilities.h"
#include "svtkVolume.h"
#include "svtkVolumeProperty.h"

typedef svtkSmartPointer<svtkImageData> Transfer2DPtr;
Transfer2DPtr Create2DTransfer()
{
  int bins[2] = { 256, 256 };
  Transfer2DPtr image = Transfer2DPtr::New();
  image->SetDimensions(bins[0], bins[1], 1);
  image->AllocateScalars(SVTK_FLOAT, 4);
  svtkFloatArray* arr = svtkFloatArray::SafeDownCast(image->GetPointData()->GetScalars());

  // Initialize to zero
  void* dataPtr = arr->GetVoidPointer(0);
  memset(dataPtr, 0, bins[0] * bins[1] * 4 * sizeof(float));

  // Setting RGBA [1.0, 0,0, 0.05] for a square in the histogram (known)
  // containing some of the interesting edges (e.g. tooth root).
  for (int j = 0; j < bins[1]; j++)
    for (int i = 0; i < bins[0]; i++)
    {
      if (i > 130 && i < 190 && j < 50)
      {
        double const jFactor = 256.0 / 50;

        svtkIdType const index = bins[0] * j + i;
        double const red = static_cast<double>(i) / bins[0];
        double const green = jFactor * static_cast<double>(j) / bins[1];
        double const blue = jFactor * static_cast<double>(j) / bins[1];
        double const alpha = 0.25 * jFactor * static_cast<double>(j) / bins[0];

        double color[4] = { red, green, blue, alpha };
        arr->SetTuple(index, color);
      }
    }

  return image;
}

////////////////////////////////////////////////////////////////////////////////
int TestGPURayCastTransfer2D(int argc, char* argv[])
{
  cout << "CTEST_FULL_OUTPUT (Avoid ctest truncation of output)" << endl;

  // Load data
  char* fname = svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/tooth.nhdr");
  svtkNew<svtkNrrdReader> reader;
  reader->SetFileName(fname);
  reader->Update();
  delete[] fname;

  svtkNew<svtkVolumeProperty> volumeProperty;
  volumeProperty->ShadeOn();
  volumeProperty->SetInterpolationType(SVTK_LINEAR_INTERPOLATION);

  svtkDataArray* arr = reader->GetOutput()->GetPointData()->GetScalars();
  double range[2];
  arr->GetRange(range);

  // Prepare 1D Transfer Functions
  svtkNew<svtkColorTransferFunction> ctf;
  ctf->AddRGBPoint(0, 0.0, 0.0, 0.0);
  ctf->AddRGBPoint(510, 0.4, 0.4, 1.0);
  ctf->AddRGBPoint(640, 1.0, 1.0, 1.0);
  ctf->AddRGBPoint(range[1], 0.9, 0.1, 0.1);

  svtkNew<svtkPiecewiseFunction> pf;
  pf->AddPoint(0, 0.00);
  pf->AddPoint(510, 0.00);
  pf->AddPoint(640, 0.5);
  pf->AddPoint(range[1], 0.4);

  svtkNew<svtkPiecewiseFunction> gf;
  gf->AddPoint(0, 0.0);
  gf->AddPoint(range[1] / 4.0, 1.0);

  volumeProperty->SetScalarOpacity(pf);
  volumeProperty->SetGradientOpacity(gf);
  volumeProperty->SetColor(ctf);

  // Prepare 2D Transfer Functions
  Transfer2DPtr tf2d = Create2DTransfer();

  volumeProperty->SetTransferFunction2D(tf2d);

  // Setup rendering context
  svtkNew<svtkRenderWindow> renWin;
  renWin->SetSize(512, 512);
  renWin->SetMultiSamples(0);

  svtkNew<svtkRenderer> ren;
  renWin->AddRenderer(ren);
  ren->SetBackground(0.0, 0.0, 0.0);

  svtkNew<svtkGPUVolumeRayCastMapper> mapper;
  mapper->SetInputConnection(reader->GetOutputPort());
  mapper->SetUseJittering(1);

  svtkNew<svtkVolume> volume;
  volume->SetMapper(mapper);
  volume->SetProperty(volumeProperty);
  ren->AddVolume(volume);

  ren->ResetCamera();
  ren->GetActiveCamera()->Elevation(-90.0);
  ren->GetActiveCamera()->Zoom(1.4);

  // Interactor
  svtkNew<svtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  svtkNew<svtkInteractorStyleTrackballCamera> style;
  iren->SetInteractorStyle(style);

  renWin->Render();

  // Simulate modification of 2D transfer function to test for shader issues
  tf2d->Modified();
  renWin->Render();

  int retVal = svtkTesting::Test(argc, argv, renWin, 90);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !((retVal == svtkTesting::PASSED) || (retVal == svtkTesting::DO_INTERACTOR));
}
