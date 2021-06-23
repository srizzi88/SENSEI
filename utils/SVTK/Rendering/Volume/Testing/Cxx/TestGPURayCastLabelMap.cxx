/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestGPURayCastLabelMap.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * Labeled data volume rendering
 *
 * This test loads the tooth dataset and creates a similar size label map for it
 * where:
 * - label 1: Voxels in a spherical region in the center of the tooth
 * - label 2: Voxels in the bottom half of the tooth with scalar intensities in
 *   the range of (0, 375). This label is used to mark the root canals.
 *
 */

#include "svtkCamera.h"
#include "svtkColorTransferFunction.h"
#include "svtkGPUVolumeRayCastMapper.h"
#include "svtkImageData.h"
#include "svtkNew.h"
#include "svtkNrrdReader.h"
#include "svtkPiecewiseFunction.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkSphere.h"
#include "svtkTestUtilities.h"
#include "svtkTesting.h"
#include "svtkTransform.h"
#include "svtkVolume.h"
#include "svtkVolumeProperty.h"

namespace TestGPURayCastLabelMapNS
{

void CreateMaskForImage(svtkImageData* input, svtkImageData* mask)
{
  if (!input)
  {
    return;
  }
  int dims[3];
  input->GetDimensions(dims);
  double origin[3], spacing[3];
  input->GetOrigin(origin);
  input->GetSpacing(spacing);

  mask->SetDimensions(dims);
  mask->SetOrigin(origin);
  mask->SetSpacing(spacing);
  mask->AllocateScalars(SVTK_UNSIGNED_CHAR, 1);

  svtkNew<svtkSphere> sphere;
  sphere->SetCenter(48.23, 48.5, 98.7);
  sphere->SetRadius(35);

  svtkNew<svtkTransform> t;
  t->Scale(spacing);
  t->Translate(origin);
  sphere->SetTransform(t);

  for (int k = 0; k < dims[2]; ++k)
  {
    for (int j = 0; j < dims[1]; ++j)
    {
      for (int i = 0; i < dims[0]; ++i)
      {
        unsigned char* ptr = static_cast<unsigned char*>(mask->GetScalarPointer(i, j, k));
        if (sphere->FunctionValue(i, j, k) < 0)
        {
          // point inside sphere
          // assign mask label 1
          *ptr = 1;
        }
        else if ((*(static_cast<unsigned short*>(input->GetScalarPointer(i, j, k))) < 375) &&
          k < dims[2] / 2)
        {
          *ptr = 2;
        }
        else
        {
          *ptr = 0;
        }
      }
    }
  }
}

} // end of namespace TestGPURayCastLabelMapNS

int TestGPURayCastLabelMap(int argc, char* argv[])
{
  cout << "CTEST_FULL_OUTPUT (Avoid ctest truncation of output)" << endl;

  char* fname = svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/tooth.nhdr");
  svtkNew<svtkNrrdReader> reader;
  reader->SetFileName(fname);
  reader->Update();
  svtkImageData* im = reader->GetOutput();

  svtkNew<svtkRenderWindow> renWin;
  renWin->SetMultiSamples(0);
  renWin->SetSize(301, 300); // Intentional NPOT size

  svtkNew<svtkRenderer> ren;
  renWin->AddRenderer(ren.GetPointer());

  // This will be label 0 of label map
  svtkNew<svtkColorTransferFunction> ctf;
  ctf->SetColorSpaceToDiverging();
  ctf->AddRGBPoint(500, 0.5, 0.1, 0.5);
  ctf->AddRGBPoint(900, 0.9, 0.4, 0.3);
  ctf->AddRGBPoint(1300, 1.0, 0.9, 0.5);
  svtkNew<svtkPiecewiseFunction> pf;
  pf->AddPoint(0, 0.0);
  pf->AddPoint(500, 0.0);
  pf->AddPoint(900, 0.05);
  pf->AddPoint(1300, 0.0);

  // Transfer functions to render label 1
  svtkNew<svtkColorTransferFunction> ctf_1;
  ctf_1->SetColorSpaceToDiverging();
  ctf_1->AddRGBSegment(0, 0, 0, 0, 500, 0.97, 1.0, 0.98);
  ctf_1->AddRGBSegment(500, 0.97, 1.0, 0.98, 1300, 0.97, 1.0, 0.98);
  svtkNew<svtkPiecewiseFunction> pf_1;
  pf_1->AddPoint(0, 0);
  pf_1->AddPoint(500, 0);
  pf_1->AddPoint(915, 1.0);
  pf_1->AddPoint(1300, 0.0);
  svtkNew<svtkPiecewiseFunction> gf_1;
  gf_1->AddPoint(0, 0);
  gf_1->AddPoint(50, 1.0);

  // Transfer function for label 2
  svtkNew<svtkColorTransferFunction> ctf_2;
  ctf_2->SetColorSpaceToDiverging();
  ctf_2->AddRGBPoint(0, 0.01, 0.62, 1.00);
  ctf_2->AddRGBPoint(500, 0.01, 0.62, 1.00);
  ctf_2->AddRGBPoint(1300, 1, 1, 1);
  svtkNew<svtkPiecewiseFunction> pf_2;
  pf_2->AddPoint(0, 0.0);
  pf_2->AddPoint(0.0, 1.0, 0.0, 0.0);
  pf_2->AddPoint(375, 0.0);
  pf_2->AddPoint(1300, 0.0);
  svtkNew<svtkPiecewiseFunction> gf_2;
  gf_2->AddPoint(0, 1.0);
  gf_2->AddPoint(50, 1.0);

  svtkNew<svtkVolumeProperty> volumeProperty;
  svtkNew<svtkVolume> volume;
  svtkNew<svtkGPUVolumeRayCastMapper> mapper;
  mapper->SetInputConnection(reader->GetOutputPort());

  // Create and set the mask input
  svtkNew<svtkImageData> mask;
  TestGPURayCastLabelMapNS::CreateMaskForImage(im, mask);
  mapper->SetMaskInput(mask);
  mapper->SetMaskTypeToLabelMap();
  mapper->SetUseJittering(1);

  volumeProperty->SetColor(ctf);
  volumeProperty->SetScalarOpacity(pf);
  volumeProperty->SetInterpolationTypeToLinear();
  volumeProperty->SetShade(0);

  volumeProperty->SetLabelColor(1, ctf_1);
  volumeProperty->SetLabelScalarOpacity(1, pf);
  volumeProperty->SetLabelGradientOpacity(1, gf_1);
  volumeProperty->SetLabelColor(2, ctf_2);
  volumeProperty->SetLabelScalarOpacity(2, pf_2);
  // volumeProperty->SetLabelGradientOpacity(2, gf_2);

  volume->SetProperty(volumeProperty);
  volume->SetMapper(mapper);
  ren->AddVolume(volume);
  svtkCamera* cam = ren->GetActiveCamera();
  cam->SetPosition(0, 0, 0);
  cam->SetFocalPoint(0, 1, 0);
  cam->SetViewUp(0, 0, 1);
  ren->ResetCamera();
  cam->Zoom(1.3);

  svtkNew<svtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin.GetPointer());

  renWin->Render();

  delete[] fname;
  int retVal = svtkTesting::Test(argc, argv, renWin, 90);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !((retVal == svtkTesting::PASSED) || (retVal == svtkTesting::DO_INTERACTOR));
}
