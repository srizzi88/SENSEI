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
// .NAME TestPStructuredGridConnectivity.cxx -- Parallel structured connectivity
//
// .SECTION Description
//  A test for parallel structured grid connectivity.

// C++ includes
#include <cassert>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

// MPI include
#include <svtk_mpi.h>

// SVTK includes
#include "svtkCellData.h"
#include "svtkDataObject.h"
#include "svtkDataSet.h"
#include "svtkInformation.h"
#include "svtkMPIController.h"
#include "svtkMathUtilities.h"
#include "svtkMultiBlockDataSet.h"
#include "svtkPStructuredGridConnectivity.h"
#include "svtkPointData.h"
#include "svtkStreamingDemandDrivenPipeline.h"
#include "svtkStructuredGridConnectivity.h"
#include "svtkStructuredNeighbor.h"
#include "svtkUniformGrid.h"
#include "svtkUniformGridPartitioner.h"
#include "svtkUnsignedCharArray.h"
#include "svtkXMLPMultiBlockDataWriter.h"

namespace
{

//------------------------------------------------------------------------------
//      G L O B A  L   D A T A
//------------------------------------------------------------------------------
svtkMultiProcessController* Controller;
int Rank;
int NumberOfProcessors;

//------------------------------------------------------------------------------
void WriteDistributedDataSet(const std::string& prefix, svtkMultiBlockDataSet* dataset)
{
#ifdef DEBUG_ON
  svtkXMLPMultiBlockDataWriter* writer = svtkXMLPMultiBlockDataWriter::New();
  std::ostringstream oss;
  oss << prefix << "." << writer->GetDefaultFileExtension();
  writer->SetFileName(oss.str().c_str());
  writer->SetInputData(dataset);
  if (Controller->GetLocalProcessId() == 0)
  {
    writer->SetWriteMetaFile(1);
  }
  writer->Update();
  writer->Delete();
#else
  (void)(prefix);
  (void)(dataset);
#endif
}

//------------------------------------------------------------------------------
void LogMessage(const std::string& msg)
{
  if (Controller->GetLocalProcessId() == 0)
  {
    std::cout << msg << std::endl;
    std::cout.flush();
  }
}

//------------------------------------------------------------------------------
int GetTotalNumberOfNodes(svtkMultiBlockDataSet* multiblock)
{
  assert("pre: Controller should not be nullptr" && (Controller != nullptr));
  assert("pre: multi-block grid is nullptr" && (multiblock != nullptr));

  // STEP 0: Count local number of nodes
  int numNodes = 0;
  for (unsigned int block = 0; block < multiblock->GetNumberOfBlocks(); ++block)
  {
    svtkUniformGrid* grid = svtkUniformGrid::SafeDownCast(multiblock->GetBlock(block));

    if (grid != nullptr)
    {
      svtkIdType pntIdx = 0;
      for (; pntIdx < grid->GetNumberOfPoints(); ++pntIdx)
      {
        if (grid->IsPointVisible(pntIdx))
        {
          ++numNodes;
        }
      } // END for all nodes
    }   // END if grid != nullptr

  } // END for all blocks

  // STEP 2: Synchronize processes
  Controller->Barrier();

  // STEP 3: Get a global sum
  int totalSum = 0;
  Controller->AllReduce(&numNodes, &totalSum, 1, svtkCommunicator::SUM_OP);

  return (totalSum);
}

//------------------------------------------------------------------------------
// Description:
// Generates a distributed multi-block dataset, each grid is added using
// round-robin assignment.
svtkMultiBlockDataSet* GetDataSet(const int numPartitions)
{
  int wholeExtent[6];
  wholeExtent[0] = 0;
  wholeExtent[1] = 99;
  wholeExtent[2] = 0;
  wholeExtent[3] = 99;
  wholeExtent[4] = 0;
  wholeExtent[5] = 99;

  int dims[3];
  dims[0] = wholeExtent[1] - wholeExtent[0] + 1;
  dims[1] = wholeExtent[3] - wholeExtent[2] + 1;
  dims[2] = wholeExtent[5] - wholeExtent[4] + 1;

  // Generate grid for the entire domain
  svtkUniformGrid* wholeGrid = svtkUniformGrid::New();
  wholeGrid->SetOrigin(0.0, 0.0, 0.0);
  wholeGrid->SetSpacing(0.5, 0.5, 0.5);
  wholeGrid->SetDimensions(dims);

  // partition the grid, the grid partitioner will generate the whole extent and
  // node extent information.
  svtkUniformGridPartitioner* gridPartitioner = svtkUniformGridPartitioner::New();
  gridPartitioner->SetInputData(wholeGrid);
  gridPartitioner->SetNumberOfPartitions(numPartitions);
  gridPartitioner->Update();
  svtkMultiBlockDataSet* partitionedGrid =
    svtkMultiBlockDataSet::SafeDownCast(gridPartitioner->GetOutput());
  assert("pre: partitionedGrid != nullptr" && (partitionedGrid != nullptr));

  // Each process has the same number of blocks, i.e., the same structure,
  // however some block entries are nullptr indicating that the data lives on
  // some other process
  svtkMultiBlockDataSet* mbds = svtkMultiBlockDataSet::New();
  mbds->SetNumberOfBlocks(numPartitions);
  mbds->GetInformation()->Set(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),
    partitionedGrid->GetInformation()->Get(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()), 6);
  //  mbds->SetWholeExtent( partitionedGrid->GetWholeExtent( ) );

  // Populate blocks for this process
  unsigned int block = 0;
  for (; block < partitionedGrid->GetNumberOfBlocks(); ++block)
  {
    if (Rank == static_cast<int>(block % NumberOfProcessors))
    {
      // Copy the uniform grid
      svtkUniformGrid* grid = svtkUniformGrid::New();
      grid->DeepCopy(partitionedGrid->GetBlock(block));

      mbds->SetBlock(block, grid);
      grid->Delete();

      // Copy the global extent for the blockinformation
      svtkInformation* info = partitionedGrid->GetMetaData(block);
      assert("pre: null metadata!" && (info != nullptr));
      assert("pre: must have a piece extent!" && (info->Has(svtkDataObject::PIECE_EXTENT())));

      svtkInformation* metadata = mbds->GetMetaData(block);
      assert("pre: null metadata!" && (metadata != nullptr));
      metadata->Set(svtkDataObject::PIECE_EXTENT(), info->Get(svtkDataObject::PIECE_EXTENT()), 6);
    } // END if we own the block
    else
    {
      mbds->SetBlock(block, nullptr);
    } // END else we don't own the block
  }   // END for all blocks

  wholeGrid->Delete();
  gridPartitioner->Delete();

  assert("pre: mbds is nullptr" && (mbds != nullptr));
  return (mbds);
}

//------------------------------------------------------------------------------
void RegisterGrids(svtkMultiBlockDataSet* mbds, svtkPStructuredGridConnectivity* connectivity)
{
  assert("pre: Multi-block is nullptr!" && (mbds != nullptr));
  assert("pre: connectivity is nullptr!" && (connectivity != nullptr));

  for (unsigned int block = 0; block < mbds->GetNumberOfBlocks(); ++block)
  {
    svtkUniformGrid* grid = svtkUniformGrid::SafeDownCast(mbds->GetBlock(block));
    if (grid != nullptr)
    {
      svtkInformation* info = mbds->GetMetaData(block);
      assert("pre: metadata should not be nullptr" && (info != nullptr));
      assert("pre: must have piece extent!" && info->Has(svtkDataObject::PIECE_EXTENT()));
      connectivity->RegisterGrid(block, info->Get(svtkDataObject::PIECE_EXTENT()),
        grid->GetPointGhostArray(), grid->GetCellGhostArray(), grid->GetPointData(),
        grid->GetCellData(), nullptr);
    } // END if block belongs to this process
  }   // END for all blocks
}

//------------------------------------------------------------------------------
// Tests StructuredGridConnectivity on a distributed data-set
int TestPStructuredGridConnectivity(const int factor)
{
  assert("pre: MPI Controller is nullptr!" && (Controller != nullptr));

  int expected = 100 * 100 * 100;

  // STEP 0: Calculation number of partitions as factor of the number of
  // processes.
  assert("pre: factor >= 1" && (factor >= 1));
  int numPartitions = factor * NumberOfProcessors;

  // STEP 1: Acquire the distributed structured grid for this process.
  // Each process has the same number of blocks, but not all entries are
  // poplulated. A nullptr entry indicates that the block belongs to a different
  // process.
  svtkMultiBlockDataSet* mbds = GetDataSet(numPartitions);
  Controller->Barrier();
  assert("pre: mbds != nullptr" && (mbds != nullptr));
  assert(
    "pre: numBlocks mismatch" && (static_cast<int>(mbds->GetNumberOfBlocks()) == numPartitions));

  // STEP 2: Setup the grid connectivity
  svtkPStructuredGridConnectivity* gridConnectivity = svtkPStructuredGridConnectivity::New();
  gridConnectivity->SetController(Controller);
  gridConnectivity->SetNumberOfGrids(mbds->GetNumberOfBlocks());
  gridConnectivity->SetWholeExtent(
    mbds->GetInformation()->Get(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()));
  gridConnectivity->Initialize();

  // STEP 3: Register the grids
  RegisterGrids(mbds, gridConnectivity);
  Controller->Barrier();

  // STEP 4: Compute neighbors
  gridConnectivity->ComputeNeighbors();
  Controller->Barrier();

  // STEP 6: Total global count of the nodes
  int count = GetTotalNumberOfNodes(mbds);
  Controller->Barrier();

  // STEP 7: Deallocate
  mbds->Delete();
  gridConnectivity->Delete();

  // STEP 8: return success or failure
  if (count != expected)
    return 1;
  return 0;
}

//------------------------------------------------------------------------------
// Assuming a 100x100x100 domain and a field given by F=X+Y+Z at each
// node, the method computes the average.
double CalculateExpectedAverage()
{
  int wholeExtent[6];
  wholeExtent[0] = 0;
  wholeExtent[1] = 99;
  wholeExtent[2] = 0;
  wholeExtent[3] = 99;
  wholeExtent[4] = 0;
  wholeExtent[5] = 99;

  int dims[3];
  dims[0] = wholeExtent[1] - wholeExtent[0] + 1;
  dims[1] = wholeExtent[3] - wholeExtent[2] + 1;
  dims[2] = wholeExtent[5] - wholeExtent[4] + 1;

  // Generate grid for the entire domain
  svtkUniformGrid* wholeGrid = svtkUniformGrid::New();
  wholeGrid->SetOrigin(0.0, 0.0, 0.0);
  wholeGrid->SetSpacing(0.5, 0.5, 0.5);
  wholeGrid->SetDimensions(dims);

  double pnt[3];
  double sum = 0.0;
  for (svtkIdType pntIdx = 0; pntIdx < wholeGrid->GetNumberOfPoints(); ++pntIdx)
  {
    wholeGrid->GetPoint(pntIdx, pnt);
    sum += pnt[0];
    sum += pnt[1];
    sum += pnt[2];
  } // END for all points

  double N = static_cast<double>(wholeGrid->GetNumberOfPoints());
  wholeGrid->Delete();
  return (sum / N);
}

//------------------------------------------------------------------------------
double GetXYZSumForGrid(svtkUniformGrid* grid)
{
  assert("pre: input grid is nullptr!" && (grid != nullptr));

  double pnt[3];
  double sum = 0.0;
  for (svtkIdType pntIdx = 0; pntIdx < grid->GetNumberOfPoints(); ++pntIdx)
  {
    if (grid->IsPointVisible(pntIdx))
    {
      grid->GetPoint(pntIdx, pnt);
      sum += pnt[0];
      sum += pnt[1];
      sum += pnt[2];
    }
  } // END for all points
  return (sum);
}

//------------------------------------------------------------------------------
// Tests computing the average serially vs. in paraller using a factor*N
// partitions where N is the total number of processes. An artificialy field
// F=X+Y+Z is imposed on each node.
int TestAverage(const int factor)
{
  // STEP 0: Calculate expected value
  double expected = CalculateExpectedAverage();

  // STEP 1: Calculation number of partitions as factor of the number of
  // processes.
  assert("pre: factor >= 1" && (factor >= 1));
  int numPartitions = factor * NumberOfProcessors;

  // STEP 2: Acquire the distributed structured grid for this process.
  // Each process has the same number of blocks, but not all entries are
  // poplulated. A nullptr entry indicates that the block belongs to a different
  // process.
  svtkMultiBlockDataSet* mbds = GetDataSet(numPartitions);
  assert("pre: mbds != nullptr" && (mbds != nullptr));
  assert(
    "pre: numBlocks mismatch" && (static_cast<int>(mbds->GetNumberOfBlocks()) == numPartitions));

  // STEP 2: Setup the grid connectivity
  svtkPStructuredGridConnectivity* gridConnectivity = svtkPStructuredGridConnectivity::New();
  gridConnectivity->SetController(Controller);
  gridConnectivity->SetNumberOfGrids(mbds->GetNumberOfBlocks());
  gridConnectivity->SetWholeExtent(
    mbds->GetInformation()->Get(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()));
  gridConnectivity->Initialize();

  // STEP 3: Register the grids
  RegisterGrids(mbds, gridConnectivity);
  Controller->Barrier();

  // STEP 4: Compute neighbors
  gridConnectivity->ComputeNeighbors();
  Controller->Barrier();

  // STEP 6: Total global count of the nodes
  int count = GetTotalNumberOfNodes(mbds);
  Controller->Barrier();

  // STEP 7: Compute partial local sum
  double partialSum = 0.0;
  unsigned int blockIdx = 0;
  for (; blockIdx < mbds->GetNumberOfBlocks(); ++blockIdx)
  {
    svtkUniformGrid* blockPtr = svtkUniformGrid::SafeDownCast(mbds->GetBlock(blockIdx));
    if (blockPtr != nullptr)
    {
      partialSum += GetXYZSumForGrid(blockPtr);
    } // END if
  }   // END for all blocks

  // STEP 8: All reduce to the global sum
  double globalSum = 0.0;
  Controller->AllReduce(&partialSum, &globalSum, 1, svtkCommunicator::SUM_OP);

  // STEP 9: Compute average
  double average = globalSum / static_cast<double>(count);

  // STEP 7: Deallocate
  mbds->Delete();
  gridConnectivity->Delete();

  // STEP 8: return success or failure
  if (svtkMathUtilities::FuzzyCompare(average, expected))
  {
    if (Rank == 0)
    {
      std::cout << "Computed: " << average << " Expected: " << expected << "\n";
      std::cout.flush();
    }
    return 0;
  }

  if (Rank == 0)
  {
    std::cout << "Global sum: " << globalSum << std::endl;
    std::cout << "Number of Nodes: " << count << std::endl;
    std::cout << "Computed: " << average << " Expected: " << expected << "\n";
    std::cout.flush();
  }

  return 1;
}

//------------------------------------------------------------------------------
int TestGhostLayerCreation(int factor, int ng)
{
  // STEP 1: Calculation number of partitions as factor of the number of
  // processes.
  assert("pre: factor >= 1" && (factor >= 1));
  int numPartitions = factor * NumberOfProcessors;

  // STEP 2: Acquire the distributed structured grid for this process.
  // Each process has the same number of blocks, but not all entries are
  // poplulated. A nullptr entry indicates that the block belongs to a different
  // process.
  svtkMultiBlockDataSet* mbds = GetDataSet(numPartitions);
  WriteDistributedDataSet("PINITIAL", mbds);
  assert("pre: mbds != nullptr" && (mbds != nullptr));
  assert(
    "pre: numBlocks mismatch" && (static_cast<int>(mbds->GetNumberOfBlocks()) == numPartitions));

  // STEP 2: Setup the grid connectivity
  svtkPStructuredGridConnectivity* gridConnectivity = svtkPStructuredGridConnectivity::New();
  gridConnectivity->SetController(Controller);
  gridConnectivity->SetNumberOfGrids(mbds->GetNumberOfBlocks());
  gridConnectivity->SetWholeExtent(
    mbds->GetInformation()->Get(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()));
  gridConnectivity->Initialize();

  // STEP 3: Register the grids
  RegisterGrids(mbds, gridConnectivity);
  Controller->Barrier();

  // STEP 4: Compute neighbors
  gridConnectivity->ComputeNeighbors();
  Controller->Barrier();

  // STEP 5: Create ghost layers
  gridConnectivity->CreateGhostLayers(ng);
  Controller->Barrier();

  mbds->Delete();
  gridConnectivity->Delete();
  return 0;
}

}

//------------------------------------------------------------------------------
// Program main
int TestPStructuredGridConnectivity(int argc, char* argv[])
{
  int rc = 0;

  // STEP 0: Initialize
  Controller = svtkMPIController::New();
  Controller->Initialize(&argc, &argv, 0);
  assert("pre: Controller should not be nullptr" && (Controller != nullptr));
  svtkMultiProcessController::SetGlobalController(Controller);
  LogMessage("Finished MPI Initialization!");

  LogMessage("Getting Rank ID and NumberOfProcessors...");
  Rank = Controller->GetLocalProcessId();
  NumberOfProcessors = Controller->GetNumberOfProcesses();
  assert("pre: NumberOfProcessors >= 1" && (NumberOfProcessors >= 1));
  assert("pre: Rank is out-of-bounds" && (Rank >= 0));

  // STEP 1: Run test where the number of partitions is equal to the number of
  // processes
  Controller->Barrier();
  LogMessage("Testing with same number of partitions as processes...");
  rc += TestPStructuredGridConnectivity(1);
  Controller->Barrier();

  // STEP 2: Run test where the number of partitions is double the number of
  // processes
  Controller->Barrier();
  LogMessage("Testing with double the number of partitions as processes...");
  rc += TestPStructuredGridConnectivity(2);
  Controller->Barrier();

  // STEP 4: Compute average
  LogMessage("Calculating average with same number of partitions as processes");
  rc += TestAverage(1);
  Controller->Barrier();

  LogMessage("Calculating average with double the number of partitions");
  rc += TestAverage(2);
  Controller->Barrier();

  LogMessage("Creating ghost-layers");
  rc += TestGhostLayerCreation(1, 1);

  // STEP 3: Deallocate controller and exit
  LogMessage("Finalizing...");
  Controller->Finalize();
  Controller->Delete();

  if (rc != 0)
  {
    std::cout << "Test Failed!\n";
    rc = 0;
  }
  return (rc);
}
