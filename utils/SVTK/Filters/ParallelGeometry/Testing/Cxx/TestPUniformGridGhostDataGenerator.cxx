/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestPUniformGridGhostDataGenerator.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME TestPUniformGridGhostDataGenerator.cxx -- Tests ghost data generation
//
// .SECTION Description
// Parallel test that exercises the parallel uniform grid ghost data generator

// C++ includes
#include <cassert>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "svtkCell.h"
#include "svtkCellData.h"
#include "svtkDoubleArray.h"
#include "svtkInformation.h"
#include "svtkMPIController.h"
#include "svtkMathUtilities.h"
#include "svtkMultiBlockDataSet.h"
#include "svtkMultiProcessController.h"
#include "svtkPUniformGridGhostDataGenerator.h"
#include "svtkPointData.h"
#include "svtkStreamingDemandDrivenPipeline.h"
#include "svtkUniformGrid.h"
#include "svtkUniformGridPartitioner.h"
#include "svtkUnsignedCharArray.h"
#include "svtkXMLPMultiBlockDataWriter.h"

//#define DEBUG_ON

namespace
{

//------------------------------------------------------------------------------
//      G L O B A  L   D A T A
//------------------------------------------------------------------------------
svtkMultiProcessController* Controller;
int Rank;
int NumberOfProcessors;
int NumberOfPartitions;

//------------------------------------------------------------------------------
namespace Logger
{

void Println(const std::string& msg)
{
  if (Controller->GetLocalProcessId() == 0)
  {
    std::cout << msg << std::endl;
    std::cout.flush();
  }
  Controller->Barrier();
}
}

//------------------------------------------------------------------------------
void AddNodeCenteredXYZField(svtkMultiBlockDataSet* mbds)
{
  assert("pre: Multi-block is nullptr!" && (mbds != nullptr));

  for (unsigned int block = 0; block < mbds->GetNumberOfBlocks(); ++block)
  {
    svtkUniformGrid* grid = svtkUniformGrid::SafeDownCast(mbds->GetBlock(block));
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
void AddCellCenteredXYZField(svtkMultiBlockDataSet* mbds)
{
  assert("pre: Multi-block is nullptr!" && (mbds != nullptr));

  for (unsigned int block = 0; block < mbds->GetNumberOfBlocks(); ++block)
  {
    svtkUniformGrid* grid = svtkUniformGrid::SafeDownCast(mbds->GetBlock(block));
    if (grid == nullptr)
    {
      continue;
    }

    svtkDoubleArray* cellXYZArray = svtkDoubleArray::New();
    cellXYZArray->SetName("CELL-XYZ");
    cellXYZArray->SetNumberOfComponents(3);
    cellXYZArray->SetNumberOfTuples(grid->GetNumberOfCells());

    double centroid[3];
    double xyz[3];
    for (svtkIdType cellIdx = 0; cellIdx < grid->GetNumberOfCells(); ++cellIdx)
    {
      svtkCell* c = grid->GetCell(cellIdx);
      assert("pre: cell is not nullptr" && (c != nullptr));

      double xsum = 0.0;
      double ysum = 0.0;
      double zsum = 0.0;
      for (svtkIdType node = 0; node < c->GetNumberOfPoints(); ++node)
      {
        svtkIdType meshPntIdx = c->GetPointId(node);
        grid->GetPoint(meshPntIdx, xyz);
        xsum += xyz[0];
        ysum += xyz[1];
        zsum += xyz[2];
      } // END for all nodes

      centroid[0] = xsum / c->GetNumberOfPoints();
      centroid[1] = ysum / c->GetNumberOfPoints();
      centroid[2] = zsum / c->GetNumberOfPoints();

      cellXYZArray->SetComponent(cellIdx, 0, centroid[0]);
      cellXYZArray->SetComponent(cellIdx, 1, centroid[1]);
      cellXYZArray->SetComponent(cellIdx, 2, centroid[2]);
    } // END for all cells

    grid->GetCellData()->AddArray(cellXYZArray);
    cellXYZArray->Delete();
  } // END for all blocks
}

//------------------------------------------------------------------------------
bool CheckNodeFieldsForGrid(svtkUniformGrid* grid)
{
  assert("pre: grid should not be nullptr" && (grid != nullptr));
  assert("pre: grid should have a NODE-XYZ array" && grid->GetPointData()->HasArray("NODE-XYZ"));

  double xyz[3];
  svtkDoubleArray* array =
    svtkArrayDownCast<svtkDoubleArray>(grid->GetPointData()->GetArray("NODE-XYZ"));
  assert("pre: num tuples must match number of nodes" &&
    (array->GetNumberOfTuples() == grid->GetNumberOfPoints()));
  assert("pre: num components must be 3" && (array->GetNumberOfComponents() == 3));

  for (svtkIdType idx = 0; idx < grid->GetNumberOfPoints(); ++idx)
  {
    grid->GetPoint(idx, xyz);

    for (int i = 0; i < 3; ++i)
    {
      if (!svtkMathUtilities::FuzzyCompare(xyz[i], array->GetComponent(idx, i)))
      {
        return false;
      } // END if fuzzy-compare
    }   // END for all components
  }     // END for all nodes
  return true;
}

//------------------------------------------------------------------------------
bool CheckCellFieldsForGrid(svtkUniformGrid* grid)
{
  assert("pre: grid should not be nullptr" && (grid != nullptr));
  assert("pre: grid should have a NODE-XYZ array" && grid->GetCellData()->HasArray("CELL-XYZ"));

  double centroid[3];
  double xyz[3];
  svtkDoubleArray* array =
    svtkArrayDownCast<svtkDoubleArray>(grid->GetCellData()->GetArray("CELL-XYZ"));
  assert("pre: num tuples must match number of nodes" &&
    (array->GetNumberOfTuples() == grid->GetNumberOfCells()));
  assert("pre: num components must be 3" && (array->GetNumberOfComponents() == 3));

  for (svtkIdType cellIdx = 0; cellIdx < grid->GetNumberOfCells(); ++cellIdx)
  {
    svtkCell* c = grid->GetCell(cellIdx);
    assert("pre: cell is not nullptr" && (c != nullptr));

    double xsum = 0.0;
    double ysum = 0.0;
    double zsum = 0.0;
    for (svtkIdType node = 0; node < c->GetNumberOfPoints(); ++node)
    {
      svtkIdType meshPntIdx = c->GetPointId(node);
      grid->GetPoint(meshPntIdx, xyz);
      xsum += xyz[0];
      ysum += xyz[1];
      zsum += xyz[2];
    } // END for all nodes

    centroid[0] = xsum / c->GetNumberOfPoints();
    centroid[1] = ysum / c->GetNumberOfPoints();
    centroid[2] = zsum / c->GetNumberOfPoints();

    for (int i = 0; i < 3; ++i)
    {
      if (!svtkMathUtilities::FuzzyCompare(centroid[i], array->GetComponent(cellIdx, i)))
      {
        return false;
      } // END if fuzz-compare
    }   // END for all components
  }     // END for all cells
  return true;
}

//------------------------------------------------------------------------------
int CheckFields(svtkMultiBlockDataSet* mbds, bool hasNodeData, bool hasCellData)
{
  assert("pre: input multi-block is nullptr" && (mbds != nullptr));

  if (!hasNodeData && !hasCellData)
  {
    return 0;
  }

  for (unsigned int block = 0; block < mbds->GetNumberOfBlocks(); ++block)
  {
    svtkUniformGrid* grid = svtkUniformGrid::SafeDownCast(mbds->GetBlock(block));
    if (grid == nullptr)
    {
      continue;
    }

    if (hasNodeData)
    {
      if (!CheckNodeFieldsForGrid(grid))
      {
        return 1;
      }
    }

    if (hasCellData)
    {
      if (!CheckCellFieldsForGrid(grid))
      {
        return 1;
      }
    }

  } // END for all blocks

  return 0;
}

//------------------------------------------------------------------------------
bool ProcessOwnsBlock(const int block)
{
  if (Rank == static_cast<int>(block % NumberOfProcessors))
  {
    return true;
  }
  return false;
}

//------------------------------------------------------------------------------
svtkMultiBlockDataSet* GetDataSet(
  int wholeExtent[6], double origin[3], double spacing[3], const int numPartitions)
{
  int dims[3];
  svtkStructuredData::GetDimensionsFromExtent(wholeExtent, dims);

  // Generate grid for the entire domain
  svtkUniformGrid* wholeGrid = svtkUniformGrid::New();
  wholeGrid->SetOrigin(origin);
  wholeGrid->SetSpacing(spacing);
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

  // Populate blocks for this process
  unsigned int block = 0;
  for (; block < partitionedGrid->GetNumberOfBlocks(); ++block)
  {
    if (ProcessOwnsBlock(block))
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
  /* Silencing some compiler warnings */
  (void)(prefix);
  (void)(dataset);
#endif
}

//------------------------------------------------------------------------------
int Test2D(const bool hasNodeData, const bool hasCellData, const int factor, const int NG)
{
  std::ostringstream oss;
  oss.clear();
  oss << "=====================\n";
  oss << "Testing parallel 2-D ghost data generation...\n";
  oss << "Number of partitions: " << factor * NumberOfProcessors << std::endl;
  oss << "Number of ghost layers: " << NG << std::endl;
  oss << "Node-centered data: ";
  if (hasNodeData)
  {
    oss << "Yes\n";
  }
  else
  {
    oss << "No\n";
  }
  oss << "Cell-centered data: ";
  if (hasCellData)
  {
    oss << "Yes\n";
  }
  else
  {
    oss << "No\n";
  }
  Logger::Println(oss.str());

  int rc = 0;

  int WholeExtent[6] = { 0, 49, 0, 49, 0, 0 };
  double h[3] = { 0.5, 0.5, 0.5 };
  double p[3] = { 0.0, 0.0, 0.0 };

  NumberOfPartitions = factor * NumberOfProcessors;
  svtkMultiBlockDataSet* mbds = GetDataSet(WholeExtent, p, h, NumberOfPartitions);
  assert("pre: multi-block dataset is nullptr!" && (mbds != nullptr));
  if (hasNodeData)
  {
    AddNodeCenteredXYZField(mbds);
  }
  if (hasCellData)
  {
    AddCellCenteredXYZField(mbds);
  }
  WriteDistributedDataSet("P2DInitial", mbds);

  svtkPUniformGridGhostDataGenerator* ghostGenerator = svtkPUniformGridGhostDataGenerator::New();

  ghostGenerator->SetInputData(mbds);
  ghostGenerator->SetNumberOfGhostLayers(NG);
  ghostGenerator->SetController(Controller);
  ghostGenerator->Initialize();
  ghostGenerator->Update();

  svtkMultiBlockDataSet* ghostedDataSet = ghostGenerator->GetOutput();
  WriteDistributedDataSet("GHOSTED2D", ghostedDataSet);

  rc = CheckFields(ghostedDataSet, hasNodeData, hasCellData);
  mbds->Delete();
  ghostGenerator->Delete();
  return (rc);
}

//------------------------------------------------------------------------------
int Test3D(const bool hasNodeData, const bool hasCellData, const int factor, const int NG)
{
  std::ostringstream oss;
  oss.clear();
  oss << "=====================\n";
  oss << "Testing parallel 3-D ghost data generation...\n";
  oss << "Number of partitions: " << factor * NumberOfProcessors << std::endl;
  oss << "Number of ghost layers: " << NG << std::endl;
  oss << "Node-centered data: ";
  if (hasNodeData)
  {
    oss << "Yes\n";
  }
  else
  {
    oss << "No\n";
  }
  oss << "Cell-centered data: ";
  if (hasCellData)
  {
    oss << "Yes\n";
  }
  else
  {
    oss << "No\n";
  }
  Logger::Println(oss.str());

  int rc = 0;

  int WholeExtent[6] = { 0, 49, 0, 49, 0, 49 };
  double h[3] = { 0.5, 0.5, 0.5 };
  double p[3] = { 0.0, 0.0, 0.0 };

  NumberOfPartitions = factor * NumberOfProcessors;
  svtkMultiBlockDataSet* mbds = GetDataSet(WholeExtent, p, h, NumberOfPartitions);
  assert("pre: multi-block dataset is nullptr!" && (mbds != nullptr));
  if (hasNodeData)
  {
    AddNodeCenteredXYZField(mbds);
  }
  if (hasCellData)
  {
    AddCellCenteredXYZField(mbds);
  }
  WriteDistributedDataSet("P3DInitial", mbds);

  svtkPUniformGridGhostDataGenerator* ghostGenerator = svtkPUniformGridGhostDataGenerator::New();

  ghostGenerator->SetInputData(mbds);
  ghostGenerator->SetNumberOfGhostLayers(NG);
  ghostGenerator->SetController(Controller);
  ghostGenerator->Initialize();
  ghostGenerator->Update();

  svtkMultiBlockDataSet* ghostedDataSet = ghostGenerator->GetOutput();
  WriteDistributedDataSet("GHOSTED3D", ghostedDataSet);

  rc = CheckFields(ghostedDataSet, hasNodeData, hasCellData);
  mbds->Delete();
  ghostGenerator->Delete();
  return (rc);
}

}

//------------------------------------------------------------------------------
int TestPUniformGridGhostDataGenerator(int argc, char* argv[])
{
  int rc = 0;
  Controller = svtkMPIController::New();
  Controller->Initialize(&argc, &argv, 0);
  assert("pre: Controller should not be nullptr" && (Controller != nullptr));
  svtkMultiProcessController::SetGlobalController(Controller);

  Rank = Controller->GetLocalProcessId();
  NumberOfProcessors = Controller->GetNumberOfProcesses();
  assert("pre: NumberOfProcessors >= 1" && (NumberOfProcessors >= 1));
  assert("pre: Rank is out-of-bounds" && (Rank >= 0));

  // 2-D tests
  rc += Test2D(false, false, 1, 1);
  assert(rc == 0);
  rc += Test2D(true, false, 1, 1);
  assert(rc == 0);
  rc += Test2D(false, true, 1, 1);
  assert(rc == 0);
  rc += Test2D(true, true, 1, 1);
  assert(rc == 0);
  rc += Test2D(true, true, 1, 3);
  assert(rc == 0);

  // 3-D Tests
  rc += Test3D(true, false, 1, 1);
  assert(rc == 0);
  rc += Test3D(true, true, 1, 4);
  assert(rc == 0);
  rc += Test3D(true, true, 2, 4);
  assert(rc == 0);

  Controller->Finalize();
  Controller->Delete();
  return (rc);
}
