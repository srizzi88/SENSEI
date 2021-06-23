/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestDistancePolyDataFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

    This software is distributed WITHOUT ANY WARRANTY; without even
    the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
    PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#include "svtkActor.h"
#include "svtkDataSetMapper.h"
#include "svtkEvenlySpacedStreamlines2D.h"
#include "svtkProperty.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkSmartPointer.h"
#include "svtkStreamTracer.h"
#include "svtkTestUtilities.h"
#include "svtkXMLMultiBlockDataReader.h"

int TestEvenlySpacedStreamlines2D(int argc, char* argv[])
{
  char* fname = svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/clt.vtm");
  auto reader = svtkSmartPointer<svtkXMLMultiBlockDataReader>::New();
  reader->SetFileName(fname);
  delete[] fname;
  reader->Update();

  auto stream = svtkSmartPointer<svtkEvenlySpacedStreamlines2D>::New();
  stream->SetInputConnection(reader->GetOutputPort());
  stream->SetInputArrayToProcess(0, 0, 0, svtkDataObject::FIELD_ASSOCIATION_POINTS, "result");
  stream->SetInitialIntegrationStep(0.2);
  stream->SetClosedLoopMaximumDistance(0.2);
  stream->SetMaximumNumberOfSteps(2000);
  stream->SetSeparatingDistance(2);
  stream->SetSeparatingDistanceRatio(0.3);
  stream->SetStartPosition(0, 0, 200);

  auto streamMapper = svtkSmartPointer<svtkDataSetMapper>::New();
  streamMapper->SetInputConnection(stream->GetOutputPort());
  streamMapper->ScalarVisibilityOff();

  auto streamActor = svtkSmartPointer<svtkActor>::New();
  streamActor->SetMapper(streamMapper);
  streamActor->GetProperty()->SetColor(0, 0, 0);
  streamActor->GetProperty()->SetLineWidth(1.);
  streamActor->SetPosition(0, 0, 1);

  auto renderer = svtkSmartPointer<svtkRenderer>::New();
  renderer->AddActor(streamActor);
  renderer->ResetCamera();
  renderer->SetBackground(1., 1., 1.);

  auto renWin = svtkSmartPointer<svtkRenderWindow>::New();
  renWin->AddRenderer(renderer);
  renWin->SetMultiSamples(0);
  renWin->SetSize(300, 300);

  auto iren = svtkSmartPointer<svtkRenderWindowInteractor>::New();
  iren->SetRenderWindow(renWin);

  int retVal = svtkRegressionTestImage(renWin);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }
  return !retVal;
}
