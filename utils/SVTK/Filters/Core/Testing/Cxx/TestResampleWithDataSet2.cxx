/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestResampleWithDataset2.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkResampleWithDataSet.h"

#include "svtkActor.h"
#include "svtkArrayCalculator.h"
#include "svtkCamera.h"
#include "svtkContourFilter.h"
#include "svtkDataSet.h"
#include "svtkExodusIIReader.h"
#include "svtkImageData.h"
#include "svtkPointData.h"
#include "svtkPolyDataMapper.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkTestUtilities.h"

enum
{
  TEST_PASSED_RETVAL = 0,
  TEST_FAILED_RETVAL = 1
};

int TestResampleWithDataSet2(int argc, char* argv[])
{
  svtkNew<svtkExodusIIReader> reader;
  char* fname = svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/can.ex2");
  reader->SetFileName(fname);
  delete[] fname;

  reader->UpdateInformation();
  reader->SetObjectArrayStatus(svtkExodusIIReader::NODAL, "VEL", 1);
  reader->Update();

  // based on can.ex2 bounds
  double origin[3] = { -7.8, -1.0, -15 };
  double spacing[3] = { 0.127, 0.072, 0.084 };
  int dims[3] = { 128, 128, 128 };

  svtkNew<svtkImageData> input;
  input->SetExtent(0, dims[0] - 1, 0, dims[1] - 1, 0, dims[2] - 1);
  input->SetOrigin(origin);
  input->SetSpacing(spacing);

  svtkNew<svtkResampleWithDataSet> resample;
  resample->SetInputData(input);
  resample->SetSourceConnection(reader->GetOutputPort());
  resample->UpdateTimeStep(0.00199999);

  svtkDataSet* result = static_cast<svtkDataSet*>(resample->GetOutput());

  // Render
  svtkNew<svtkContourFilter> toPoly;
  toPoly->SetInputData(result);
  toPoly->SetInputArrayToProcess(
    0, 0, 0, svtkDataObject::FIELD_ASSOCIATION_POINTS, "svtkValidPointMask");
  toPoly->SetValue(0, 0.5);

  svtkNew<svtkArrayCalculator> calculator;
  calculator->SetInputConnection(toPoly->GetOutputPort());
  calculator->AddVectorArrayName("VEL");
  calculator->SetFunction("mag(VEL)");
  calculator->SetResultArrayName("VEL_MAG");
  calculator->Update();

  double range[2];
  svtkDataSet::SafeDownCast(calculator->GetOutput())
    ->GetPointData()
    ->GetArray("VEL_MAG")
    ->GetRange(range);

  svtkNew<svtkPolyDataMapper> mapper;
  mapper->SetInputConnection(calculator->GetOutputPort());
  mapper->SetScalarRange(range);

  svtkNew<svtkActor> actor;
  actor->SetMapper(mapper);

  svtkNew<svtkRenderer> renderer;
  renderer->AddActor(actor);
  renderer->GetActiveCamera()->SetPosition(0.0, -1.0, 0.0);
  renderer->GetActiveCamera()->SetViewUp(0.0, 0.0, 1.0);
  renderer->ResetCamera();

  svtkNew<svtkRenderWindow> renWin;
  renWin->AddRenderer(renderer);

  svtkNew<svtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);
  iren->Initialize();

  renWin->Render();

  int retVal = svtkRegressionTestImage(renWin);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
