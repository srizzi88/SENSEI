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
// .NAME TestImplicitConnectivity.cxx -- Parallel implicit connectivity test
//
// .SECTION Description
//  A test for parallel structured grid connectivity.

// C++ includes
#include <cassert>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

// SVTK includes
#include "svtkCellData.h"
#include "svtkDataArray.h"
#include "svtkDataObject.h"
#include "svtkDataSet.h"
#include "svtkDoubleArray.h"
#include "svtkImageToStructuredGrid.h"
#include "svtkInformation.h"
#include "svtkMPIController.h"
#include "svtkMPIUtilities.h"
#include "svtkMathUtilities.h"
#include "svtkMultiBlockDataSet.h"
#include "svtkPointData.h"
#include "svtkRectilinearGrid.h"
#include "svtkRectilinearGridPartitioner.h"
#include "svtkRectilinearGridWriter.h"
#include "svtkStreamingDemandDrivenPipeline.h"
#include "svtkStructuredGrid.h"
#include "svtkStructuredGridPartitioner.h"
#include "svtkStructuredImplicitConnectivity.h"
#include "svtkUniformGrid.h"
#include "svtkUnsignedCharArray.h"
#include "svtkXMLPMultiBlockDataWriter.h"

#define DEBUG_ON

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
void AddNodeCenteredXYZField(svtkMultiBlockDataSet* mbds)
{
  assert("pre: Multi-block is nullptr!" && (mbds != nullptr));

  for (unsigned int block = 0; block < mbds->GetNumberOfBlocks(); ++block)
  {
    svtkDataSet* grid = svtkDataSet::SafeDownCast(mbds->GetBlock(block));

    if (grid == nullptr)
    {
      continue;
    }

    svtkDoubleArray* nodeXYZArray = svtkDoubleArray::New();
    nodeXYZArray->SetName("NODE-XYZ");
    nodeXYZArray->SetNumberOfComponents(3);
    nodeXYZArray->SetNumberOfTuples(grid->GetNumberOfPoints());

    double xyz[3];
    for (svtkIdType pntIdx = 0; pntIdx < grid->GetNumberOfPoints(); ++pntIdx)
    {
      grid->GetPoint(pntIdx, xyz);
      nodeXYZArray->SetComponent(pntIdx, 0, xyz[0]);
      nodeXYZArray->SetComponent(pntIdx, 1, xyz[1]);
      nodeXYZArray->SetComponent(pntIdx, 2, xyz[2]);
    } // END for all points

    grid->GetPointData()->AddArray(nodeXYZArray);
    nodeXYZArray->Delete();
  } // END for all blocks
}

//------------------------------------------------------------------------------
// Description:
// Generates a distributed multi-block dataset, each grid is added using
// round-robin assignment.
svtkMultiBlockDataSet* GetDataSet(
  const int numPartitions, double origin[3], double h[3], int wholeExtent[6])
{

  int dims[3];
  int desc = svtkStructuredData::GetDataDescriptionFromExtent(wholeExtent);
  svtkStructuredData::GetDimensionsFromExtent(wholeExtent, dims, desc);

  // Generate grid for the entire domain
  svtkUniformGrid* wholeGrid = svtkUniformGrid::New();
  wholeGrid->SetOrigin(origin[0], origin[1], origin[2]);
  wholeGrid->SetSpacing(h[0], h[1], h[2]);
  wholeGrid->SetDimensions(dims);

  // Convert to structured grid
  svtkImageToStructuredGrid* img2sgrid = svtkImageToStructuredGrid::New();
  img2sgrid->SetInputData(wholeGrid);
  img2sgrid->Update();
  svtkStructuredGrid* wholeStructuredGrid = svtkStructuredGrid::New();
  wholeStructuredGrid->DeepCopy(img2sgrid->GetOutput());
  img2sgrid->Delete();
  wholeGrid->Delete();

  // partition the grid, the grid partitioner will generate the whole extent and
  // node extent information.
  svtkStructuredGridPartitioner* gridPartitioner = svtkStructuredGridPartitioner::New();
  gridPartitioner->SetInputData(wholeStructuredGrid);
  gridPartitioner->SetNumberOfPartitions(numPartitions);
  gridPartitioner->SetNumberOfGhostLayers(0);
  gridPartitioner->DuplicateNodesOff(); /* to create a gap */
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

  // Populate blocks for this process
  unsigned int block = 0;
  for (; block < partitionedGrid->GetNumberOfBlocks(); ++block)
  {
    if (Rank == static_cast<int>(block % NumberOfProcessors))
    {
      // Copy the structured grid
      svtkStructuredGrid* grid = svtkStructuredGrid::New();
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

  wholeStructuredGrid->Delete();
  gridPartitioner->Delete();

  assert("pre: mbds is nullptr" && (mbds != nullptr));

  AddNodeCenteredXYZField(mbds);
  Controller->Barrier();

  return (mbds);
}

//------------------------------------------------------------------------------
double exponential_distribution(const int i, const double beta)
{
  double xi = ((exp(i * beta) - 1) / (exp(beta) - 1));
  return (xi);
}

//------------------------------------------------------------------------------
void GenerateRectGrid(svtkRectilinearGrid* grid, int ext[6], double origin[3])
{
  grid->Initialize();
  grid->SetExtent(ext);

  svtkDataArray* coords[3];

  int dims[3];
  int dataDesc = svtkStructuredData::GetDataDescriptionFromExtent(ext);
  svtkStructuredData::GetDimensionsFromExtent(ext, dims, dataDesc);

  // compute & populate coordinate vectors
  double beta = 0.01; /* controls the intensity of the stretching */
  for (int i = 0; i < 3; ++i)
  {
    coords[i] = svtkDataArray::CreateDataArray(SVTK_DOUBLE);
    if (dims[i] == 0)
    {
      continue;
    }
    coords[i]->SetNumberOfTuples(dims[i]);

    double prev = origin[i];
    for (int j = 0; j < dims[i]; ++j)
    {
      double val = prev + ((j == 0) ? 0.0 : exponential_distribution(j, beta));
      coords[i]->SetTuple(j, &val);
      prev = val;
    } // END for all points along this dimension
  }   // END for all dimensions

  grid->SetXCoordinates(coords[0]);
  grid->SetYCoordinates(coords[1]);
  grid->SetZCoordinates(coords[2]);
  coords[0]->Delete();
  coords[1]->Delete();
  coords[2]->Delete();
}

//------------------------------------------------------------------------------
// Description:
// Generates a distributed multi-block dataset, each grid is added using
// round-robin assignment.
svtkMultiBlockDataSet* GetRectGridDataSet(
  const int numPartitions, double origin[3], int wholeExtent[6])
{
  int dims[3];
  int desc = svtkStructuredData::GetDataDescriptionFromExtent(wholeExtent);
  svtkStructuredData::GetDimensionsFromExtent(wholeExtent, dims, desc);

  svtkRectilinearGrid* wholeGrid = svtkRectilinearGrid::New();
  GenerateRectGrid(wholeGrid, wholeExtent, origin);
  //#ifdef DEBUG_ON
  //  if( Controller->GetLocalProcessId() == 0 )
  //    {
  //    svtkRectilinearGridWriter* writer = svtkRectilinearGridWriter::New();
  //    writer->SetFileName("RectilinearGrid.svtk");
  //    writer->SetInputData( wholeGrid );
  //    writer->Write();
  //    writer->Delete();
  //    }
  //#endif
  svtkRectilinearGridPartitioner* gridPartitioner = svtkRectilinearGridPartitioner::New();
  gridPartitioner->SetInputData(wholeGrid);
  gridPartitioner->SetNumberOfPartitions(numPartitions);
  gridPartitioner->SetNumberOfGhostLayers(0);
  gridPartitioner->DuplicateNodesOff(); /* to create a gap */
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

  // Populate blocks for this process
  unsigned int block = 0;
  for (; block < partitionedGrid->GetNumberOfBlocks(); ++block)
  {
    if (Rank == static_cast<int>(block % NumberOfProcessors))
    {
      // Copy the structured grid
      svtkRectilinearGrid* grid = svtkRectilinearGrid::New();
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

  AddNodeCenteredXYZField(mbds);
  Controller->Barrier();

  return mbds;
}

//------------------------------------------------------------------------------
void RegisterRectGrid(svtkMultiBlockDataSet* mbds, svtkStructuredImplicitConnectivity* connectivity)
{
  assert("pre: Multi-block is nullptr!" && (mbds != nullptr));
  assert("pre: connectivity is nullptr!" && (connectivity != nullptr));

  for (unsigned int block = 0; block < mbds->GetNumberOfBlocks(); ++block)
  {
    svtkRectilinearGrid* grid = svtkRectilinearGrid::SafeDownCast(mbds->GetBlock(block));

    if (grid != nullptr)
    {
      svtkInformation* info = mbds->GetMetaData(block);
      assert("pre: metadata should not be nullptr" && (info != nullptr));
      assert("pre: must have piece extent!" && info->Has(svtkDataObject::PIECE_EXTENT()));

      connectivity->RegisterRectilinearGrid(block, info->Get(svtkDataObject::PIECE_EXTENT()),
        grid->GetXCoordinates(), grid->GetYCoordinates(), grid->GetZCoordinates(),
        grid->GetPointData());
    } // END if block belongs to this process
  }   // END for all blocks
}

//------------------------------------------------------------------------------
void RegisterGrid(svtkMultiBlockDataSet* mbds, svtkStructuredImplicitConnectivity* connectivity)
{
  assert("pre: Multi-block is nullptr!" && (mbds != nullptr));
  assert("pre: connectivity is nullptr!" && (connectivity != nullptr));

  for (unsigned int block = 0; block < mbds->GetNumberOfBlocks(); ++block)
  {
    svtkStructuredGrid* grid = svtkStructuredGrid::SafeDownCast(mbds->GetBlock(block));
    if (grid != nullptr)
    {
      svtkInformation* info = mbds->GetMetaData(block);
      assert("pre: metadata should not be nullptr" && (info != nullptr));
      assert("pre: must have piece extent!" && info->Has(svtkDataObject::PIECE_EXTENT()));
      connectivity->RegisterGrid(
        block, info->Get(svtkDataObject::PIECE_EXTENT()), grid->GetPoints(), grid->GetPointData());
    } // END if block belongs to this process
  }   // END for all blocks
}

//------------------------------------------------------------------------------
int CheckGrid(svtkDataSet* grid)
{
  assert("pre: grid should not be nullptr!" && (grid != nullptr));
  int rc = 0;

  svtkPointData* PD = grid->GetPointData();
  assert("pre: PD should not be nullptr!" && (PD != nullptr));

  if (!PD->HasArray("NODE-XYZ"))
  {
    std::cerr << "ERROR: NODE-XYZ array does not exist!\n";
    return 1;
  }

  svtkDoubleArray* array = svtkArrayDownCast<svtkDoubleArray>(PD->GetArray("NODE-XYZ"));
  if (array == nullptr)
  {
    std::cerr << "ERROR: null svtkDataArray!\n";
    return 1;
  }

  if (PD->GetNumberOfTuples() != grid->GetNumberOfPoints())
  {
    std::cerr << "ERROR: PointData numTuples != num grid points!\n";
    return 1;
  }

  double pnt[3];
  double* dataPtr = static_cast<double*>(array->GetVoidPointer(0));

  svtkIdType N = grid->GetNumberOfPoints();
  for (svtkIdType idx = 0; idx < N; ++idx)
  {
    grid->GetPoint(idx, pnt);

    if (!svtkMathUtilities::NearlyEqual(pnt[0], dataPtr[idx * 3], 1.e-9) ||
      !svtkMathUtilities::NearlyEqual(pnt[1], dataPtr[idx * 3 + 1], 1.e-9) ||
      !svtkMathUtilities::NearlyEqual(pnt[2], dataPtr[idx * 3 + 2], 1.e-9))
    {
      ++rc;
    } // END if rc

  } // END for all points

  return (rc);
}

//------------------------------------------------------------------------------
int TestOutput(svtkMultiBlockDataSet* mbds, int wholeExtent[6])
{
  int rc = 0;

  // Check if the output grid has gaps
  svtkStructuredImplicitConnectivity* gridConnectivity = svtkStructuredImplicitConnectivity::New();
  gridConnectivity->SetWholeExtent(wholeExtent);

  svtkDataSet* grid = nullptr;
  for (unsigned int block = 0; block < mbds->GetNumberOfBlocks(); ++block)
  {
    grid = svtkDataSet::SafeDownCast(mbds->GetBlock(block));
    if (grid != nullptr)
    {
      if (grid->IsA("svtkStructuredGrid"))
      {
        svtkStructuredGrid* sGrid = svtkStructuredGrid::SafeDownCast(grid);
        gridConnectivity->RegisterGrid(
          block, sGrid->GetExtent(), sGrid->GetPoints(), sGrid->GetPointData());
      }
      else
      {
        assert("pre: expected rectilinear grid!" && grid->IsA("svtkRectilinearGrid"));
        svtkRectilinearGrid* rGrid = svtkRectilinearGrid::SafeDownCast(grid);
        gridConnectivity->RegisterRectilinearGrid(block, rGrid->GetExtent(),
          rGrid->GetXCoordinates(), rGrid->GetYCoordinates(), rGrid->GetZCoordinates(),
          rGrid->GetPointData());
      }

      rc += CheckGrid(grid);
    } // END if grid != nullptr

  } // END for all blocks

  int rcLocal = rc;
  Controller->AllReduce(&rcLocal, &rc, 1, svtkCommunicator::SUM_OP);
  if (rc > 0)
  {
    svtkMPIUtilities::Printf(
      svtkMPIController::SafeDownCast(Controller), "ERROR: Check grid failed!");
  }

  gridConnectivity->EstablishConnectivity();

  if (gridConnectivity->HasImplicitConnectivity())
  {
    svtkMPIUtilities::Printf(
      svtkMPIController::SafeDownCast(Controller), "ERROR: output grid still has a gap!\n");
    ++rc;
  }
  svtkMPIUtilities::Printf(svtkMPIController::SafeDownCast(Controller), "Grid has no gaps!\n");
  gridConnectivity->Delete();

  return (rc);
}

//------------------------------------------------------------------------------
// Tests StructuredGridConnectivity on a distributed data-set
int TestImplicitGridConnectivity2DYZ()
{
  assert("pre: MPI Controller is nullptr!" && (Controller != nullptr));

  svtkMPIUtilities::Printf(svtkMPIController::SafeDownCast(Controller),
    "=======================\nTesting 2-D Dataset on the YZ-plane\n");

  int rc = 0;

  int wholeExtent[6];
  wholeExtent[0] = 0;
  wholeExtent[1] = 0;
  wholeExtent[2] = 0;
  wholeExtent[3] = 49;
  wholeExtent[4] = 0;
  wholeExtent[5] = 49;
  double h[3] = { 0.5, 0.5, 0.5 };
  double origin[3] = { 0.0, 0.0, 0.0 };

  // STEP 0: We generate the same number of partitions as processes
  int numPartitions = NumberOfProcessors;

  // STEP 1: Acquire the distributed structured grid for this process.
  // Each process has the same number of blocks, but not all entries are
  // populated. A nullptr entry indicates that the block belongs to a different
  // process.
  svtkMultiBlockDataSet* mbds = GetDataSet(numPartitions, origin, h, wholeExtent);
  Controller->Barrier();
  assert("pre: mbds != nullptr" && (mbds != nullptr));
  assert(
    "pre: numBlocks mismatch" && (static_cast<int>(mbds->GetNumberOfBlocks()) == numPartitions));
  WriteDistributedDataSet("INPUT2DYZ", mbds);

  // STEP 2: Setup the grid connectivity
  svtkStructuredImplicitConnectivity* gridConnectivity = svtkStructuredImplicitConnectivity::New();
  gridConnectivity->SetWholeExtent(
    mbds->GetInformation()->Get(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()));

  // STEP 3: Register the grids
  RegisterGrid(mbds, gridConnectivity);
  Controller->Barrier();

  // STEP 4: Compute neighbors
  gridConnectivity->EstablishConnectivity();
  Controller->Barrier();

  // print the neighboring information from each process
  std::ostringstream oss;
  gridConnectivity->Print(oss);
  svtkMPIUtilities::SynchronizedPrintf(
    svtkMPIController::SafeDownCast(Controller), "%s\n", oss.str().c_str());

  if (!gridConnectivity->HasImplicitConnectivity())
  {
    ++rc;
  }

  // STEP 5: Exchange the data
  gridConnectivity->ExchangeData();

  // STEP 6: Get output data
  int gridID = Controller->GetLocalProcessId();
  svtkStructuredGrid* outGrid = svtkStructuredGrid::New();
  gridConnectivity->GetOutputStructuredGrid(gridID, outGrid);

  svtkMultiBlockDataSet* outputMBDS = svtkMultiBlockDataSet::New();
  outputMBDS->SetNumberOfBlocks(numPartitions);
  outputMBDS->SetBlock(gridID, outGrid);
  outGrid->Delete();

  WriteDistributedDataSet("OUTPUT2DYZ", outputMBDS);

  // STEP 7: Verify Test output data
  TestOutput(outputMBDS, wholeExtent);

  // STEP 8: Deallocate
  mbds->Delete();
  outputMBDS->Delete();
  gridConnectivity->Delete();

  return (rc);
}

//------------------------------------------------------------------------------
// Tests StructuredGridConnectivity on a distributed data-set
int TestImplicitGridConnectivity2DXZ()
{
  assert("pre: MPI Controller is nullptr!" && (Controller != nullptr));

  svtkMPIUtilities::Printf(svtkMPIController::SafeDownCast(Controller),
    "=======================\nTesting 2-D Dataset on the XZ-plane\n");

  int rc = 0;

  int wholeExtent[6];
  wholeExtent[0] = 0;
  wholeExtent[1] = 49;
  wholeExtent[2] = 0;
  wholeExtent[3] = 0;
  wholeExtent[4] = 0;
  wholeExtent[5] = 49;
  double h[3] = { 0.5, 0.5, 0.5 };
  double origin[3] = { 0.0, 0.0, 0.0 };

  // STEP 0: We generate the same number of partitions as processes
  int numPartitions = NumberOfProcessors;

  // STEP 1: Acquire the distributed structured grid for this process.
  // Each process has the same number of blocks, but not all entries are
  // populated. A nullptr entry indicates that the block belongs to a different
  // process.
  svtkMultiBlockDataSet* mbds = GetDataSet(numPartitions, origin, h, wholeExtent);
  Controller->Barrier();
  assert("pre: mbds != nullptr" && (mbds != nullptr));
  assert(
    "pre: numBlocks mismatch" && (static_cast<int>(mbds->GetNumberOfBlocks()) == numPartitions));
  WriteDistributedDataSet("INPUT2DXZ", mbds);

  // STEP 2: Setup the grid connectivity
  svtkStructuredImplicitConnectivity* gridConnectivity = svtkStructuredImplicitConnectivity::New();
  gridConnectivity->SetWholeExtent(
    mbds->GetInformation()->Get(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()));

  // STEP 3: Register the grids
  RegisterGrid(mbds, gridConnectivity);
  Controller->Barrier();

  // STEP 4: Compute neighbors
  gridConnectivity->EstablishConnectivity();
  Controller->Barrier();

  // print the neighboring information from each process
  std::ostringstream oss;
  gridConnectivity->Print(oss);
  svtkMPIUtilities::SynchronizedPrintf(
    svtkMPIController::SafeDownCast(Controller), "%s\n", oss.str().c_str());

  if (!gridConnectivity->HasImplicitConnectivity())
  {
    ++rc;
  }

  // STEP 5: Exchange the data
  gridConnectivity->ExchangeData();

  // STEP 6: Get output data
  int gridID = Controller->GetLocalProcessId();
  svtkStructuredGrid* outGrid = svtkStructuredGrid::New();
  gridConnectivity->GetOutputStructuredGrid(gridID, outGrid);

  svtkMultiBlockDataSet* outputMBDS = svtkMultiBlockDataSet::New();
  outputMBDS->SetNumberOfBlocks(numPartitions);
  outputMBDS->SetBlock(gridID, outGrid);
  outGrid->Delete();

  WriteDistributedDataSet("OUTPUT2DXZ", outputMBDS);

  // STEP 7: Verify Test output data
  TestOutput(outputMBDS, wholeExtent);

  // STEP 8: Deallocate
  mbds->Delete();
  outputMBDS->Delete();
  gridConnectivity->Delete();

  return (rc);
}

//------------------------------------------------------------------------------
// Tests StructuredGridConnectivity on a distributed data-set
int TestImplicitGridConnectivity2DXY()
{
  assert("pre: MPI Controller is nullptr!" && (Controller != nullptr));

  svtkMPIUtilities::Printf(svtkMPIController::SafeDownCast(Controller),
    "=======================\nTesting 2-D Dataset on the XY-plane\n");

  int rc = 0;

  int wholeExtent[6];
  wholeExtent[0] = 0;
  wholeExtent[1] = 49;
  wholeExtent[2] = 0;
  wholeExtent[3] = 49;
  wholeExtent[4] = 0;
  wholeExtent[5] = 0;
  double h[3] = { 0.5, 0.5, 0.5 };
  double origin[3] = { 0.0, 0.0, 0.0 };

  // STEP 0: We generate the same number of partitions as processes
  int numPartitions = NumberOfProcessors;

  // STEP 1: Acquire the distributed structured grid for this process.
  // Each process has the same number of blocks, but not all entries are
  // populated. A nullptr entry indicates that the block belongs to a different
  // process.
  svtkMultiBlockDataSet* mbds = GetDataSet(numPartitions, origin, h, wholeExtent);
  Controller->Barrier();
  assert("pre: mbds != nullptr" && (mbds != nullptr));
  assert(
    "pre: numBlocks mismatch" && (static_cast<int>(mbds->GetNumberOfBlocks()) == numPartitions));
  WriteDistributedDataSet("INPUT2DXY", mbds);

  // STEP 2: Setup the grid connectivity
  svtkStructuredImplicitConnectivity* gridConnectivity = svtkStructuredImplicitConnectivity::New();
  gridConnectivity->SetWholeExtent(
    mbds->GetInformation()->Get(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()));

  // STEP 3: Register the grids
  RegisterGrid(mbds, gridConnectivity);
  Controller->Barrier();

  // STEP 4: Compute neighbors
  gridConnectivity->EstablishConnectivity();
  Controller->Barrier();

  // print the neighboring information from each process
  std::ostringstream oss;
  gridConnectivity->Print(oss);
  svtkMPIUtilities::SynchronizedPrintf(
    svtkMPIController::SafeDownCast(Controller), "%s\n", oss.str().c_str());

  if (!gridConnectivity->HasImplicitConnectivity())
  {
    ++rc;
  }

  // STEP 5: Exchange the data
  gridConnectivity->ExchangeData();

  // STEP 6: Get output data
  int gridID = Controller->GetLocalProcessId();
  svtkStructuredGrid* outGrid = svtkStructuredGrid::New();
  gridConnectivity->GetOutputStructuredGrid(gridID, outGrid);

  svtkMultiBlockDataSet* outputMBDS = svtkMultiBlockDataSet::New();
  outputMBDS->SetNumberOfBlocks(numPartitions);
  outputMBDS->SetBlock(gridID, outGrid);
  outGrid->Delete();

  WriteDistributedDataSet("OUTPUT2DXY", outputMBDS);

  // STEP 7: Verify Test output data
  TestOutput(outputMBDS, wholeExtent);

  // STEP 8: Deallocate
  mbds->Delete();
  outputMBDS->Delete();
  gridConnectivity->Delete();

  return (rc);
}

//------------------------------------------------------------------------------
// Tests StructuredGridConnectivity on a distributed data-set
int TestImplicitGridConnectivity3D()
{
  assert("pre: MPI Controller is nullptr!" && (Controller != nullptr));

  svtkMPIUtilities::Printf(
    svtkMPIController::SafeDownCast(Controller), "=======================\nTesting 3-D Dataset\n");

  int rc = 0;

  int wholeExtent[6];
  wholeExtent[0] = 0;
  wholeExtent[1] = 99;
  wholeExtent[2] = 0;
  wholeExtent[3] = 99;
  wholeExtent[4] = 0;
  wholeExtent[5] = 99;
  double h[3] = { 0.5, 0.5, 0.5 };
  double origin[3] = { 0.0, 0.0, 0.0 };

  // STEP 0: We generate the same number of partitions as processes
  int numPartitions = NumberOfProcessors;

  // STEP 1: Acquire the distributed structured grid for this process.
  // Each process has the same number of blocks, but not all entries are
  // populated. A nullptr entry indicates that the block belongs to a different
  // process.
  svtkMultiBlockDataSet* mbds = GetDataSet(numPartitions, origin, h, wholeExtent);

  Controller->Barrier();
  assert("pre: mbds != nullptr" && (mbds != nullptr));
  assert(
    "pre: numBlocks mismatch" && (static_cast<int>(mbds->GetNumberOfBlocks()) == numPartitions));
  WriteDistributedDataSet("INPUT3D", mbds);

  // STEP 2: Setup the grid connectivity
  svtkStructuredImplicitConnectivity* gridConnectivity = svtkStructuredImplicitConnectivity::New();
  gridConnectivity->SetWholeExtent(
    mbds->GetInformation()->Get(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()));

  // STEP 3: Register the grids
  RegisterGrid(mbds, gridConnectivity);
  Controller->Barrier();

  // STEP 4: Compute neighbors
  gridConnectivity->EstablishConnectivity();
  Controller->Barrier();

  // print the neighboring information from each process
  std::ostringstream oss;
  gridConnectivity->Print(oss);
  svtkMPIUtilities::SynchronizedPrintf(
    svtkMPIController::SafeDownCast(Controller), "%s\n", oss.str().c_str());

  if (!gridConnectivity->HasImplicitConnectivity())
  {
    ++rc;
  }

  // STEP 5: Exchange the data
  gridConnectivity->ExchangeData();

  // STEP 6: Get output data
  int gridID = Controller->GetLocalProcessId();
  svtkStructuredGrid* outGrid = svtkStructuredGrid::New();
  gridConnectivity->GetOutputStructuredGrid(gridID, outGrid);

  svtkMultiBlockDataSet* outputMBDS = svtkMultiBlockDataSet::New();
  outputMBDS->SetNumberOfBlocks(numPartitions);
  outputMBDS->SetBlock(gridID, outGrid);
  outGrid->Delete();

  WriteDistributedDataSet("OUTPUT3D", outputMBDS);

  // STEP 7: Verify Test output data
  TestOutput(outputMBDS, wholeExtent);

  // STEP 8: Deallocate
  mbds->Delete();
  outputMBDS->Delete();
  gridConnectivity->Delete();

  return (rc);
}

//------------------------------------------------------------------------------
int TestRectGridImplicitConnectivity3D()
{
  assert("pre: MPI Controller is nullptr!" && (Controller != nullptr));

  svtkMPIUtilities::Printf(svtkMPIController::SafeDownCast(Controller),
    "=======================\nTesting 3-D Rectilinear Grid Dataset\n");

  int rc = 0;

  int wholeExtent[6];
  wholeExtent[0] = 0;
  wholeExtent[1] = 99;
  wholeExtent[2] = 0;
  wholeExtent[3] = 99;
  wholeExtent[4] = 0;
  wholeExtent[5] = 99;
  double origin[3] = { 0.0, 0.0, 0.0 };

  // STEP 0: We generate the same number of partitions as processes
  int numPartitions = NumberOfProcessors;

  // STEP 1: Acquire the distributed rectilinear grid for this process.
  // Each process has the same number of blocks, but not all entries are
  // populated. A nullptr entry indicates that the block belongs to a different
  // process.
  svtkMultiBlockDataSet* mbds = GetRectGridDataSet(numPartitions, origin, wholeExtent);

  Controller->Barrier();
  assert("pre: mbds != nullptr" && (mbds != nullptr));
  assert(
    "pre: numBlocks mismatch" && (static_cast<int>(mbds->GetNumberOfBlocks()) == numPartitions));
  WriteDistributedDataSet("INPUT-3D-RECTGRID", mbds);

  // STEP 2: Setup the grid connectivity
  svtkStructuredImplicitConnectivity* gridConnectivity = svtkStructuredImplicitConnectivity::New();
  gridConnectivity->SetWholeExtent(
    mbds->GetInformation()->Get(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()));

  // STEP 3: Register the grids
  RegisterRectGrid(mbds, gridConnectivity);
  Controller->Barrier();

  // STEP 4: Compute neighbors
  gridConnectivity->EstablishConnectivity();
  Controller->Barrier();

  // print the neighboring information from each process
  std::ostringstream oss;
  gridConnectivity->Print(oss);
  svtkMPIUtilities::SynchronizedPrintf(
    svtkMPIController::SafeDownCast(Controller), "%s\n", oss.str().c_str());

  if (!gridConnectivity->HasImplicitConnectivity())
  {
    ++rc;
  }

  // STEP 5: Exchange the data
  gridConnectivity->ExchangeData();

  // STEP 6: Get output data
  int gridID = Controller->GetLocalProcessId();
  svtkRectilinearGrid* outGrid = svtkRectilinearGrid::New();
  gridConnectivity->GetOutputRectilinearGrid(gridID, outGrid);

  svtkMultiBlockDataSet* outputMBDS = svtkMultiBlockDataSet::New();
  outputMBDS->SetNumberOfBlocks(numPartitions);
  outputMBDS->SetBlock(gridID, outGrid);
  outGrid->Delete();

  WriteDistributedDataSet("OUTPUT-3D-RECTGRID", outputMBDS);

  // STEP 7: Verify Test output data
  TestOutput(outputMBDS, wholeExtent);

  // STEP 8: Deallocate
  mbds->Delete();
  outputMBDS->Delete();
  gridConnectivity->Delete();

  return (rc);
}

//------------------------------------------------------------------------------
// Program main
int TestImplicitConnectivity(int argc, char* argv[])
{
  int rc = 0;

  // STEP 0: Initialize
  Controller = svtkMPIController::New();
  Controller->Initialize(&argc, &argv, 0);
  assert("pre: Controller should not be nullptr" && (Controller != nullptr));
  svtkMultiProcessController::SetGlobalController(Controller);

  Rank = Controller->GetLocalProcessId();
  NumberOfProcessors = Controller->GetNumberOfProcesses();
  svtkMPIUtilities::Printf(
    svtkMPIController::SafeDownCast(Controller), "Rank=%d NumRanks=%d\n", Rank, NumberOfProcessors);
  assert("pre: NumberOfProcessors >= 1" && (NumberOfProcessors >= 1));
  assert("pre: Rank is out-of-bounds" && (Rank >= 0));

  // STEP 1: Run 2-D Test on XY-plane
  rc += TestImplicitGridConnectivity2DXY();
  Controller->Barrier();

  // STEP 2: Run 2-D Test on XZ-plane
  rc += TestImplicitGridConnectivity2DXZ();
  Controller->Barrier();

  // STEP 3: Run 2-D Test on YZ-plane
  rc += TestImplicitGridConnectivity2DYZ();
  Controller->Barrier();

  // STEP 2: Run 3-D Test
  rc += TestImplicitGridConnectivity3D();
  Controller->Barrier();

  // STEP 3: Test 3-D Recilinear Grid
  rc += TestRectGridImplicitConnectivity3D();
  Controller->Barrier();

  // STEP 3: Deallocate controller and exit
  svtkMPIUtilities::Printf(svtkMPIController::SafeDownCast(Controller), "Finalizing...\n");
  Controller->Finalize();
  Controller->Delete();

  if (rc != 0)
  {
    std::cout << "Test Failed!\n";
    rc = 0;
  }
  return (rc);
}
