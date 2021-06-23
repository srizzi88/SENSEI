/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestDistributedPointCloudFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkBoundingBox.h"
#include "svtkDistributedPointCloudFilter.h"
#include "svtkDoubleArray.h"
#include "svtkIdFilter.h"
#include "svtkMPICommunicator.h"
#include "svtkMPIController.h"
#include "svtkMinimalStandardRandomSequence.h"
#include "svtkNew.h"
#include "svtkPointData.h"
#include "svtkPoints.h"
#include "svtkPolyData.h"
#include "svtkProcessIdScalars.h"
#include "svtkStringArray.h"

//#define DEBUG_ON

#ifdef DEBUG_ON
#include "svtkXMLPPolyDataWriter.h"
#endif

#include <sstream>

int TestDistributedPointCloudFilter(int argc, char* argv[])
{
  svtkNew<svtkMPIController> controller;
  controller->Initialize(&argc, &argv, 0);
  assert("pre: Controller should not be nullptr" && (controller != nullptr));
  svtkMultiProcessController::SetGlobalController(controller);

  const int rank = controller->GetLocalProcessId();
  const int numberOfProcessors = controller->GetNumberOfProcesses();
  assert("pre: NumberOfProcessors >= 1" && (numberOfProcessors >= 1));
  assert("pre: Rank is out-of-bounds" && (rank >= 0));

  const int finalNumberOfPointsPerRank = 40;
  const int totalNumberOfPoints = numberOfProcessors * finalNumberOfPointsPerRank;
  const int initialNumberOfPoints = totalNumberOfPoints / (numberOfProcessors > 1 ? 2 : 1);
  svtkNew<svtkPolyData> inputPoly;
  // Create random set of points on the two first ranks only
  if (rank == 0 || rank == 1)
  {
    svtkNew<svtkMinimalStandardRandomSequence> random;
    random->Initialize(rank);

    svtkNew<svtkPoints> points;
    points->SetNumberOfPoints(initialNumberOfPoints);
    inputPoly->SetPoints(points);

    svtkNew<svtkDoubleArray> data;
    data->SetNumberOfValues(initialNumberOfPoints);
    data->SetName("ReverseOrder");
    inputPoly->GetPointData()->AddArray(data);

    svtkNew<svtkStringArray> sdata;
    sdata->SetNumberOfValues(initialNumberOfPoints);
    sdata->SetName("RankString");
    inputPoly->GetPointData()->AddArray(sdata);

    std::stringstream ss;
    ss << "Rank_" << rank;

    for (svtkIdType i = 0; i < initialNumberOfPoints; i++)
    {
      double coords[3];
      coords[0] = random->GetValue();
      random->Next();
      coords[1] = random->GetValue();
      random->Next();
      coords[2] = random->GetValue();
      random->Next();
      points->SetPoint(i, coords);
      data->SetValue(i, totalNumberOfPoints - i - 1);
      sdata->SetValue(i, ss.str());
    }
  }

  // attach initial ids and process ids
  svtkNew<svtkIdFilter> idFilter;
  idFilter->SetInputData(inputPoly);
  idFilter->SetPointIdsArrayName("OriginalId");
  idFilter->SetCellIdsArrayName("OriginalId");
  svtkNew<svtkProcessIdScalars> procIdScalars;
  procIdScalars->SetInputConnection(idFilter->GetOutputPort());
  procIdScalars->Update();
  procIdScalars->GetOutput()->GetPointData()->GetArray("ProcessId")->SetName("OriginalProcessId");

  // distribute the points over the processors
  svtkNew<svtkDistributedPointCloudFilter> filter;
  filter->SetInputConnection(procIdScalars->GetOutputPort());

  // attach new process ids
  svtkNew<svtkProcessIdScalars> outProcIdScalars;
  outProcIdScalars->SetInputConnection(filter->GetOutputPort());
  outProcIdScalars->Update();
  svtkPolyData* outputPoly = svtkPolyData::SafeDownCast(outProcIdScalars->GetOutput());

  bool error = false;
  int nbOfLocallyReceivedPoints = outputPoly->GetNumberOfPoints();
  if (nbOfLocallyReceivedPoints != finalNumberOfPointsPerRank)
  {
    cerr << "No point on the node " << rank << "\n";
    // do not exit here so MPI can end correctly
    error = true;
  }

  if (outputPoly->GetPointData()->GetNumberOfArrays() != 5)
  {
    cerr << "Incorrect number of point data arrays on rank " << rank << "\n";
    error = true;
  }

  double bounds[6];
  outputPoly->GetBounds(bounds);
  svtkBoundingBox bbox(bounds);
  if (!bbox.IsValid() || bbox.GetLength(0) == 0. || bbox.GetLength(1) == 0. ||
    bbox.GetLength(2) == 0.)
  {
    cerr << "Incorrect bounding box of output points on rank " << rank << "\n";
    error = true;
  }

  svtkMPICommunicator* com = svtkMPICommunicator::SafeDownCast(controller->GetCommunicator());
  std::vector<int> nbOfReceivedPoints(numberOfProcessors);
  com->AllGather(&nbOfLocallyReceivedPoints, nbOfReceivedPoints.data(), 1);

  int totalNumberOfReceivedPoints = 0;
  for (svtkIdType i = 0; i < numberOfProcessors; i++)
  {
    totalNumberOfReceivedPoints += nbOfReceivedPoints[i];
  }

  if (totalNumberOfReceivedPoints != totalNumberOfPoints)
  {
    cerr << "Wrong total of points: " << totalNumberOfReceivedPoints << " instead of "
         << totalNumberOfPoints << "\n";
    cerr << "Rank " << rank << ":";
    for (int i = 0; i < nbOfLocallyReceivedPoints; i++)
    {
      cout << " " << outputPoly->GetPoints()->GetPoint(i)[0];
    }
    cerr << endl;
    error = true;
  }

#ifdef DEBUG_ON
  svtkNew<svtkXMLPPolyDataWriter> writer;
  std::stringstream ss;
  ss << "TestDistributedPointCloudFilter-" << numberOfProcessors << "ranks.pvtp";
  writer->SetFileName(ss.str().c_str());
  writer->SetInputData(inputPoly);
  writer->SetNumberOfPieces(numberOfProcessors);
  writer->SetStartPiece(rank);
  writer->SetEndPiece(rank);
  writer->SetWriteSummaryFile(1);
  writer->Update();
#endif

  controller->Finalize();

  return error ? EXIT_FAILURE : EXIT_SUCCESS;
}
