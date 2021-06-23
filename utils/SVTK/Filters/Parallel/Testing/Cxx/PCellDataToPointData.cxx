/*=========================================================================

  Program:   Visualization Toolkit
  Module:    PCellDataToPointData.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// Tests PCellDataToPointData.

/*
** This test only builds if MPI is in use. It uses 2 MPI processes and checks
** that the svtkPCellDataToPointDataFilter works properly.
*/
#include "svtkDataSet.h"
#include "svtkDataSetSurfaceFilter.h"
#include "svtkMPICommunicator.h"
#include "svtkMPIController.h"
#include "svtkNew.h"
#include "svtkPCellDataToPointData.h"
#include "svtkPointDataToCellData.h"
#include "svtkPolyDataMapper.h"
#include "svtkRTAnalyticSource.h"

#include <svtk_mpi.h>

int PCellDataToPointData(int argc, char* argv[])
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
      cout << "PCellDataToPointData test requires MPI" << endl;
    }
    contr->Delete();
    return EXIT_FAILURE;
  }

  int numProcs = contr->GetNumberOfProcesses();

  // Create and execute pipeline
  svtkRTAnalyticSource* wavelet = svtkRTAnalyticSource::New();
  svtkPointDataToCellData* pd2cd = svtkPointDataToCellData::New();
  svtkPCellDataToPointData* cd2pd = svtkPCellDataToPointData::New();
  svtkDataSetSurfaceFilter* toPolyData = svtkDataSetSurfaceFilter::New();
  svtkPolyDataMapper* mapper = svtkPolyDataMapper::New();

  pd2cd->SetInputConnection(wavelet->GetOutputPort());
  cd2pd->SetInputConnection(pd2cd->GetOutputPort());
  cd2pd->SetPieceInvariant(1); // should be the default anyway
  toPolyData->SetInputConnection(cd2pd->GetOutputPort());

  mapper->SetInputConnection(toPolyData->GetOutputPort());
  mapper->SetScalarRange(0, numProcs);
  mapper->SetPiece(me);
  mapper->SetNumberOfPieces(numProcs);
  mapper->Update();

  svtkIdType correct = 5292;
  if (svtkDataSet::SafeDownCast(cd2pd->GetOutput())->GetNumberOfPoints() != correct)
  {
    svtkGenericWarningMacro("Wrong number of unstructured grid points on process "
      << me << ". Should be " << correct << " but is "
      << svtkDataSet::SafeDownCast(cd2pd->GetOutput())->GetNumberOfPoints());
    retVal = EXIT_FAILURE;
  }

  mapper->Delete();
  toPolyData->Delete();
  cd2pd->Delete();
  pd2cd->Delete();
  wavelet->Delete();

  contr->Finalize();
  contr->Delete();

  return retVal;
}
