/*=========================================================================

  Program:   Visualization Toolkit
  Module:    DIYAggregateDataSet.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

  =========================================================================*/

// Tests svtkDIYAggregateDataSetFilter.

/*
** This test only builds if MPI is in use. It uses 4 MPI processes to
** test that the data is aggregated down to two processes. It uses a simple
** point count to verify results.
*/
#include "svtkDIYAggregateDataSetFilter.h"
#include "svtkDataSet.h"
#include "svtkIdentityTransform.h"
#include "svtkMPICommunicator.h"
#include "svtkMPIController.h"
#include "svtkNew.h"
#include "svtkPointSet.h"
#include "svtkRTAnalyticSource.h"
#include "svtkTransformFilter.h"

#include <svtk_mpi.h>

int DIYAggregateDataSet(int argc, char* argv[])
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
      cout << "DIYAggregateDataSet test requires MPI" << endl;
    }
    contr->Delete();
    return EXIT_FAILURE;
  }

  int numProcs = contr->GetNumberOfProcesses();

  // Create and execute pipeline
  svtkRTAnalyticSource* wavelet = svtkRTAnalyticSource::New();
  wavelet->UpdatePiece(me, numProcs, 0);
  svtkDIYAggregateDataSetFilter* aggregate = svtkDIYAggregateDataSetFilter::New();

  aggregate->SetInputConnection(wavelet->GetOutputPort());
  aggregate->SetNumberOfTargetProcesses(2);

  aggregate->UpdatePiece(me, numProcs, 0);

  if (me % 2 == 1 && svtkDataSet::SafeDownCast(aggregate->GetOutput())->GetNumberOfPoints() != 4851)
  {
    svtkGenericWarningMacro("Wrong number of imagedata points on process "
      << me << ". Should be 4851 but is "
      << svtkDataSet::SafeDownCast(aggregate->GetOutput())->GetNumberOfPoints());
    retVal = EXIT_FAILURE;
  }
  else if (me % 2 != 1 &&
    svtkDataSet::SafeDownCast(aggregate->GetOutput())->GetNumberOfPoints() != 0)
  {
    svtkGenericWarningMacro("Wrong number of imagedata points on process "
      << me << ". Should be 0 but is "
      << svtkDataSet::SafeDownCast(aggregate->GetOutput())->GetNumberOfPoints());
    retVal = EXIT_FAILURE;
  }

  aggregate->Delete();
  wavelet->Delete();

  // Now do the same thing for a structured grid (the transform filter converts the wavelet
  // from an image data to a structured grid). Also, do it for a 2D grid to make sure it
  // works for that as well.
  svtkRTAnalyticSource* wavelet2 = svtkRTAnalyticSource::New();
  wavelet2->SetWholeExtent(-10, 10, -10, 10, 0, 0);
  svtkTransformFilter* transform = svtkTransformFilter::New();
  transform->SetInputConnection(wavelet2->GetOutputPort());
  svtkNew<svtkIdentityTransform> identityTransform;
  transform->SetTransform(identityTransform);
  transform->UpdatePiece(me, numProcs, 0);

  svtkDIYAggregateDataSetFilter* aggregate2 = svtkDIYAggregateDataSetFilter::New();
  aggregate2->SetInputConnection(transform->GetOutputPort());
  aggregate2->SetNumberOfTargetProcesses(2);

  aggregate2->UpdatePiece(me, numProcs, 0);

  if (me % 2 == 1 && svtkDataSet::SafeDownCast(aggregate2->GetOutput())->GetNumberOfPoints() != 231)
  {
    svtkGenericWarningMacro("Wrong number of structured grid points on process "
      << me << ". Should be 4851 but is "
      << svtkDataSet::SafeDownCast(aggregate2->GetOutput())->GetNumberOfPoints());
    retVal = EXIT_FAILURE;
  }
  else if (me % 2 != 1 &&
    svtkDataSet::SafeDownCast(aggregate2->GetOutput())->GetNumberOfPoints() != 0)
  {
    svtkGenericWarningMacro("Wrong number of structured grid points on process "
      << me << ". Should be 0 but is "
      << svtkDataSet::SafeDownCast(aggregate2->GetOutput())->GetNumberOfPoints());
    retVal = EXIT_FAILURE;
  }

  aggregate2->Delete();
  transform->Delete();
  wavelet2->Delete();

  contr->Finalize();
  contr->Delete();

  return retVal;
}
