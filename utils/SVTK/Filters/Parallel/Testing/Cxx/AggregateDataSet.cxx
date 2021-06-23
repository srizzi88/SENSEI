/*=========================================================================

  Program:   Visualization Toolkit
  Module:    AggregateDataSet.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// Tests svtkAggregateDataSetFilter.

/*
** This test only builds if MPI is in use. It uses 4 MPI processes to
** test that the data is aggregated down to two processes. It also uses
** rendering to generate the pieces on each process but uses a simple
** point count to verify results. The SVTK XML writers could have been
** used but this module is not dependent on those writers yet so adding
** yet another dependency seemed bad.
*/
#include "svtkAggregateDataSetFilter.h"
#include "svtkContourFilter.h"
#include "svtkDataSet.h"
#include "svtkDataSetSurfaceFilter.h"
#include "svtkMPICommunicator.h"
#include "svtkMPIController.h"
#include "svtkNew.h"
#include "svtkPolyDataMapper.h"
#include "svtkRTAnalyticSource.h"
#include "svtkThresholdPoints.h"

#include <svtk_mpi.h>

int AggregateDataSet(int argc, char* argv[])
{
  // This is here to avoid false leak messages from svtkDebugLeaks when
  // using mpich. It appears that the root process which spawns all the
  // main processes waits in MPI_Init() and calls exit() when
  // the others are done, causing apparent memory leaks for any objects
  // created before MPI_Init().
  MPI_Init(&argc, &argv);

  // Note that this will create a svtkMPIController if MPI
  // is configured, svtkThreadedController otherwise.
  svtkMPIController* contr = svtkMPIController::New();
  contr->Initialize(&argc, &argv, 1);

  int retVal = EXIT_SUCCESS;

  svtkMultiProcessController::SetGlobalController(contr);

  int me = contr->GetLocalProcessId();

  if (!contr->IsA("svtkMPIController"))
  {
    if (me == 0)
    {
      cout << "AggregateDataSet test requires MPI" << endl;
    }
    contr->Delete();
    return EXIT_FAILURE;
  }

  int numProcs = contr->GetNumberOfProcesses();

  // Create and execute pipeline
  svtkRTAnalyticSource* wavelet = svtkRTAnalyticSource::New();
  svtkDataSetSurfaceFilter* toPolyData = svtkDataSetSurfaceFilter::New();
  svtkAggregateDataSetFilter* aggregate = svtkAggregateDataSetFilter::New();
  svtkPolyDataMapper* mapper = svtkPolyDataMapper::New();

  toPolyData->SetInputConnection(wavelet->GetOutputPort());
  aggregate->SetInputConnection(toPolyData->GetOutputPort());
  aggregate->SetNumberOfTargetProcesses(2);

  mapper->SetInputConnection(aggregate->GetOutputPort());
  mapper->SetScalarRange(0, numProcs);
  mapper->SetPiece(me);
  mapper->SetNumberOfPieces(numProcs);
  mapper->Update();

  if (me % 2 == 0 && svtkDataSet::SafeDownCast(aggregate->GetOutput())->GetNumberOfPoints() != 1408)
  {
    svtkGenericWarningMacro("Wrong number of polydata points on process "
      << me << ". Should be 1408 but is "
      << svtkDataSet::SafeDownCast(aggregate->GetOutput())->GetNumberOfPoints());
    retVal = EXIT_FAILURE;
  }
  else if (me % 2 != 0 &&
    svtkDataSet::SafeDownCast(aggregate->GetOutput())->GetNumberOfPoints() != 0)
  {
    svtkGenericWarningMacro("Wrong number of polydata points on process "
      << me << ". Should be 0 but is "
      << svtkDataSet::SafeDownCast(aggregate->GetOutput())->GetNumberOfPoints());
    retVal = EXIT_FAILURE;
  }

  svtkThresholdPoints* threshold = svtkThresholdPoints::New();
  threshold->ThresholdBetween(0, 500);
  threshold->SetInputConnection(wavelet->GetOutputPort());
  aggregate->SetInputConnection(threshold->GetOutputPort());

  svtkContourFilter* contour = svtkContourFilter::New();
  double scalar_range[2] = { 50, 400 };
  contour->GenerateValues(5, scalar_range);
  contour->SetInputConnection(aggregate->GetOutputPort());
  mapper->SetInputConnection(contour->GetOutputPort());
  mapper->Update();

  if (me % 2 == 0 && svtkDataSet::SafeDownCast(aggregate->GetOutput())->GetNumberOfPoints() != 5082)
  {
    svtkGenericWarningMacro("Wrong number of unstructured grid points on process "
      << me << ". Should be 5082 but is "
      << svtkDataSet::SafeDownCast(aggregate->GetOutput())->GetNumberOfPoints());
    retVal = EXIT_FAILURE;
  }
  else if (me % 2 != 0 &&
    svtkDataSet::SafeDownCast(aggregate->GetOutput())->GetNumberOfPoints() != 0)
  {
    svtkGenericWarningMacro("Wrong number of unstructured grid points on process "
      << me << ". Should be 0 but is "
      << svtkDataSet::SafeDownCast(aggregate->GetOutput())->GetNumberOfPoints());
    retVal = EXIT_FAILURE;
  }

  mapper->Delete();
  contour->Delete();
  threshold->Delete();
  aggregate->Delete();
  toPolyData->Delete();
  wavelet->Delete();

  contr->Finalize();
  contr->Delete();

  return retVal;
}
