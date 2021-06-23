/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestPStructuredGridConnectivity.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkDataSetTriangleFilter.h"
#include "svtkIdTypeArray.h"
#include "svtkImageData.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkMPIController.h"
#include "svtkNew.h"
#include "svtkObjectFactory.h"
#include "svtkPUnstructuredGridGhostCellsGenerator.h"
#include "svtkPointData.h"
#include "svtkRTAnalyticSource.h"
#include "svtkTimerLog.h"
#include "svtkUnsignedCharArray.h"
#include "svtkUnstructuredGrid.h"
#include <sstream>
#include <string>

namespace
{
// An RTAnalyticSource that generates GlobalNodeIds
class svtkRTAnalyticSource2 : public svtkRTAnalyticSource
{
public:
  static svtkRTAnalyticSource2* New();
  svtkTypeMacro(svtkRTAnalyticSource2, svtkRTAnalyticSource);

protected:
  svtkRTAnalyticSource2() {}

  virtual void ExecuteDataWithInformation(svtkDataObject* output, svtkInformation* outInfo) override
  {
    Superclass::ExecuteDataWithInformation(output, outInfo);

    // Split the update extent further based on piece request.
    svtkImageData* data = svtkImageData::GetData(outInfo);
    int* outExt = data->GetExtent();
    int* whlExt = this->GetWholeExtent();

    // find the region to loop over
    int maxX = (outExt[1] - outExt[0]) + 1;
    int maxY = (outExt[3] - outExt[2]) + 1;
    int maxZ = (outExt[5] - outExt[4]) + 1;

    int dX = (whlExt[1] - whlExt[0]) + 1;
    int dY = (whlExt[3] - whlExt[2]) + 1;

    svtkNew<svtkIdTypeArray> ids;
    ids->SetName("GlobalNodeIds");
    ids->SetNumberOfValues(maxX * maxY * maxZ);
    data->GetPointData()->SetGlobalIds(ids);

    svtkIdType cnt = 0;
    for (int idxZ = 0; idxZ < maxZ; idxZ++)
    {
      for (int idxY = 0; idxY < maxY; idxY++)
      {
        for (int idxX = 0; idxX < maxX; idxX++, cnt++)
        {
          ids->SetValue(
            cnt, (idxX + outExt[0]) + (idxY + outExt[2]) * dX + (idxZ + outExt[4]) * (dX * dY));
        }
      }
    }
  }

private:
  svtkRTAnalyticSource2(const svtkRTAnalyticSource2&) = delete;
  void operator=(const svtkRTAnalyticSource2&) = delete;
};

svtkStandardNewMacro(svtkRTAnalyticSource2);

bool CheckFieldData(svtkFieldData* fd)
{
  svtkUnsignedCharArray* fdArray = svtkUnsignedCharArray::SafeDownCast(fd->GetArray("FieldData"));
  if (!fdArray || fdArray->GetValue(0) != 2)
  {
    cerr << "Field data array value is not the same as the input" << endl;
    return false;
  }

  return true;
}

} // anonymous namespace

//------------------------------------------------------------------------------
// Program main
int TestPUnstructuredGridGhostCellsGenerator(int argc, char* argv[])
{
  int ret = EXIT_SUCCESS;
  // Initialize the MPI controller
  svtkNew<svtkMPIController> controller;
  controller->Initialize(&argc, &argv, 0);
  svtkMultiProcessController::SetGlobalController(controller);
  int myRank = controller->GetLocalProcessId();
  int nbRanks = controller->GetNumberOfProcesses();

  // Create the pipeline to produce the initial grid
  svtkNew<svtkRTAnalyticSource2> wavelet;
  const int gridSize = 50;
  wavelet->SetWholeExtent(0, gridSize, 0, gridSize, 0, gridSize);
  svtkNew<svtkDataSetTriangleFilter> tetrahedralize;
  tetrahedralize->SetInputConnection(wavelet->GetOutputPort());
  tetrahedralize->UpdatePiece(myRank, nbRanks, 0);

  svtkNew<svtkUnstructuredGrid> initialGrid;
  initialGrid->ShallowCopy(tetrahedralize->GetOutput());

  // Add field data
  svtkNew<svtkUnsignedCharArray> fdArray;
  fdArray->SetNumberOfTuples(1);
  fdArray->SetName("FieldData");
  fdArray->SetValue(0, 2);
  svtkNew<svtkFieldData> fd;
  fd->AddArray(fdArray);
  initialGrid->SetFieldData(fd);

  // Prepare the ghost cells generator
  svtkNew<svtkPUnstructuredGridGhostCellsGenerator> ghostGenerator;
  ghostGenerator->SetInputData(initialGrid);
  ghostGenerator->SetController(controller);
  ghostGenerator->UseGlobalPointIdsOn();

  // Check BuildIfRequired option
  ghostGenerator->BuildIfRequiredOff();
  ghostGenerator->UpdatePiece(myRank, nbRanks, 0);

  if (ghostGenerator->GetOutput()->GetCellGhostArray() == nullptr)
  {
    cerr << "Ghost were not generated but were explicitly requested on process "
         << controller->GetLocalProcessId() << endl;
    ret = EXIT_FAILURE;
  }

  ghostGenerator->BuildIfRequiredOn();
  ghostGenerator->UpdatePiece(myRank, nbRanks, 0);

  if (ghostGenerator->GetOutput()->GetCellGhostArray())
  {
    cerr << "Ghost were generated but were not requested on process "
         << controller->GetLocalProcessId() << endl;
    ret = EXIT_FAILURE;
  }

  // Check that field data is copied
  ghostGenerator->Update();
  if (!CheckFieldData(ghostGenerator->GetOutput()->GetFieldData()))
  {
    cerr << "Field data was not copied correctly" << std::endl;
    ret = EXIT_FAILURE;
  }

  // Check if algorithm works with empty input on all nodes except first one
  svtkNew<svtkUnstructuredGrid> emptyGrid;
  ghostGenerator->SetInputData(myRank == 0 ? initialGrid : emptyGrid);
  for (int step = 0; step < 2; ++step)
  {
    ghostGenerator->SetUseGlobalPointIds(step == 0 ? 1 : 0);
    ghostGenerator->UpdatePiece(myRank, nbRanks, 1);
  }
  ghostGenerator->SetInputData(initialGrid);
  ghostGenerator->Modified();

  // Check ghost cells generated with and without the global point ids
  // for several ghost layer levels
  int maxGhostLevel = 2;
  svtkSmartPointer<svtkUnstructuredGrid> outGrids[2];
  for (int ghostLevel = 1; ghostLevel <= maxGhostLevel; ++ghostLevel)
  {
    for (int step = 0; step < 2; ++step)
    {
      ghostGenerator->SetUseGlobalPointIds(step == 0 ? 1 : 0);
      ghostGenerator->Modified();
      svtkNew<svtkTimerLog> timer;
      timer->StartTimer();
      ghostGenerator->UpdatePiece(myRank, nbRanks, ghostLevel);
      timer->StopTimer();

      // Save the grid for further analysis
      outGrids[step] = ghostGenerator->GetOutput();

      if (!CheckFieldData(outGrids[step]->GetFieldData()))
      {
        cerr << "Field data was not copied" << std::endl;
        ret = EXIT_FAILURE;
      }

      double elapsed = timer->GetElapsedTime();

      // get some performance statistics
      double minGhostUpdateTime = 0.0;
      double maxGhostUpdateTime = 0.0;
      double avgGhostUpdateTime = 0.0;
      controller->Reduce(&elapsed, &minGhostUpdateTime, 1, svtkCommunicator::MIN_OP, 0);
      controller->Reduce(&elapsed, &maxGhostUpdateTime, 1, svtkCommunicator::MAX_OP, 0);
      controller->Reduce(&elapsed, &avgGhostUpdateTime, 1, svtkCommunicator::SUM_OP, 0);
      avgGhostUpdateTime /= static_cast<double>(nbRanks);
      if (controller->GetLocalProcessId() == 0)
      {
        cerr << "-- Ghost Level: " << ghostLevel
             << " UseGlobalPointIds: " << ghostGenerator->GetUseGlobalPointIds()
             << " Elapsed Time: min=" << minGhostUpdateTime << ", avg=" << avgGhostUpdateTime
             << ", max=" << maxGhostUpdateTime << endl;
      }
    }

    svtkIdType initialNbOfCells = initialGrid->GetNumberOfCells();

    // quantitative correct values for runs with 4 MPI processes
    // components are for [ghostlevel][procid][bounds]
    svtkIdType correctCellCounts[2] = { 675800 / 4, 728800 / 4 };
    double correctBounds[2][4][6] = {
      {
        { 0.000000, 50.000000, 0.000000, 26.000000, 0.000000, 26.000000 },
        { 0.000000, 50.000000, 24.000000, 50.000000, 0.000000, 26.000000 },
        { 0.000000, 50.000000, 0.000000, 26.000000, 24.000000, 50.000000 },
        { 0.000000, 50.000000, 24.000000, 50.000000, 24.000000, 50.000000 },
      },
      { { 0.000000, 50.000000, 0.000000, 27.000000, 0.000000, 27.000000 },
        { 0.000000, 50.000000, 23.000000, 50.000000, 0.000000, 27.000000 },
        { 0.000000, 50.000000, 0.000000, 27.000000, 23.000000, 50.000000 },
        { 0.000000, 50.000000, 23.000000, 50.000000, 23.000000, 50.000000 } }
    };
    for (int step = 0; step < 2; ++step)
    {
      if (nbRanks == 4)
      {
        if (outGrids[step]->GetNumberOfCells() != correctCellCounts[ghostLevel - 1])
        {
          cerr << "Wrong number of cells on process " << myRank << " for " << ghostLevel
               << " ghost levels!\n";
          ret = EXIT_FAILURE;
        }
        double bounds[6];
        outGrids[step]->GetBounds(bounds);
        for (int i = 0; i < 6; i++)
        {
          if (std::abs(bounds[i] - correctBounds[ghostLevel - 1][myRank][i]) > .001)
          {
            cerr << "Wrong bounds for " << ghostLevel << " ghost levels!\n";
            ret = EXIT_FAILURE;
          }
        }
      }

      svtkUnsignedCharArray* ghosts =
        svtkArrayDownCast<svtkUnsignedCharArray>(outGrids[step]->GetCellGhostArray());
      if (initialNbOfCells >= outGrids[step]->GetNumberOfCells())
      {
        cerr << "Obtained grids for ghost level " << ghostLevel
             << " has less or as many cells as the input grid!\n";
        ret = EXIT_FAILURE;
      }
      if (!ghosts)
      {
        cerr << "Ghost cells array not found at ghost level " << ghostLevel << ", step " << step
             << "!\n";
        ret = EXIT_FAILURE;
        continue;
      }

      for (svtkIdType i = 0; i < ghosts->GetNumberOfTuples(); ++i)
      {
        unsigned char val = ghosts->GetValue(i);
        if (i < initialNbOfCells && val != 0)
        {
          cerr << "Ghost Level " << ghostLevel << " Cell " << i
               << " is not supposed to be a ghost cell but it is!\n";
          ret = EXIT_FAILURE;
          break;
        }
        if (i >= initialNbOfCells && val != 1)
        {
          cerr << "Ghost Level " << ghostLevel << " Cell " << i
               << " is supposed to be a ghost cell but it's not!\n";
          ret = EXIT_FAILURE;
          break;
        }
      }
    }
  }

  controller->Finalize();
  return ret;
}
