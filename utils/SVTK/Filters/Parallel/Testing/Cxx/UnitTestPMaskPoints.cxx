/*=========================================================================

  Program:   Visualization Toolkit
  Module:    UnitTestPMaskPoints.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkPMaskPoints.h"
#include "svtkSmartPointer.h"

#include "svtkPoints.h"
#include "svtkPolyData.h"

#include "svtkMPIController.h"

#include "svtkCommand.h"
#include "svtkMathUtilities.h"
#include "svtkTestErrorObserver.h"

// MPI include
#include <svtk_mpi.h>

#include <algorithm>
#include <cstdio>
#include <sstream>

static svtkSmartPointer<svtkPolyData> MakePolyData(unsigned int numPoints);

int UnitTestPMaskPoints(int argc, char* argv[])
{
  int status = 0;

  // Test empty input
  // std::cout << "Testing empty input...";
  std::ostringstream print0;
  svtkSmartPointer<svtkPMaskPoints> mask0 = svtkSmartPointer<svtkPMaskPoints>::New();
  // For coverage
  mask0->SetController(nullptr);
  mask0->SetController(nullptr);
  mask0->Print(print0);

  svtkMPIController* cntrl = svtkMPIController::New();
  cntrl->Initialize(&argc, &argv, 0);
  svtkMultiProcessController::SetGlobalController(cntrl);

  mask0->SetController(svtkMultiProcessController::GetGlobalController());

  mask0->SetInputData(MakePolyData(10000));
  mask0->GenerateVerticesOn();
  mask0->SetMaximumNumberOfPoints(99);
  mask0->ProportionalMaximumNumberOfPointsOn();
  mask0->SetOutputPointsPrecision(svtkAlgorithm::DEFAULT_PRECISION);
  mask0->Update();

  mask0->RandomModeOn();
  mask0->SetRandomModeType(0);
  mask0->Update();

  mask0->SetRandomModeType(1);
  mask0->Update();

  mask0->SetRandomModeType(2);
  mask0->SetOutputPointsPrecision(svtkAlgorithm::DOUBLE_PRECISION);
  mask0->Update();

  mask0->SetOutputPointsPrecision(svtkAlgorithm::DEFAULT_PRECISION);
  mask0->Update();

  mask0->SetRandomModeType(3);
  mask0->SetOutputPointsPrecision(svtkAlgorithm::SINGLE_PRECISION);
  mask0->SingleVertexPerCellOn();
  mask0->Update();

  mask0->Print(print0);

  cntrl->Finalize();
  cntrl->Delete();
  if (status)
  {
    return EXIT_FAILURE;
  }
  else
  {
    return EXIT_SUCCESS;
  }
}

svtkSmartPointer<svtkPolyData> MakePolyData(unsigned int numPoints)
{
  svtkSmartPointer<svtkPolyData> polyData = svtkSmartPointer<svtkPolyData>::New();
  svtkSmartPointer<svtkPoints> points = svtkSmartPointer<svtkPoints>::New();
  std::vector<double> line;
  for (unsigned int i = 0; i < numPoints; ++i)
  {
    line.push_back(static_cast<double>(i));
  }
  std::random_shuffle(line.begin(), line.end());
  for (unsigned int i = 0; i < numPoints; ++i)
  {
    points->InsertNextPoint(line[i], 0.0, 0.0);
  }
  polyData->SetPoints(points);
  return polyData;
}
