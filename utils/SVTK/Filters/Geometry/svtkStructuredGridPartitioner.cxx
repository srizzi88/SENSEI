/*=========================================================================

 Program:   Visualization Toolkit
 Module:    svtkStructuredGridPartitioner.cxx

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/
#include "svtkStructuredGridPartitioner.h"
#include "svtkExtentRCBPartitioner.h"
#include "svtkIndent.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkMultiBlockDataSet.h"
#include "svtkObjectFactory.h"
#include "svtkPoints.h"
#include "svtkStreamingDemandDrivenPipeline.h"
#include "svtkStructuredData.h"
#include "svtkStructuredExtent.h"
#include "svtkStructuredGrid.h"

#include <cassert>

svtkStandardNewMacro(svtkStructuredGridPartitioner);

//------------------------------------------------------------------------------
svtkStructuredGridPartitioner::svtkStructuredGridPartitioner()
{
  this->NumberOfPartitions = 2;
  this->NumberOfGhostLayers = 0;
  this->DuplicateNodes = 1;
  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(1);
}

//------------------------------------------------------------------------------
svtkStructuredGridPartitioner::~svtkStructuredGridPartitioner() = default;

//------------------------------------------------------------------------------
void svtkStructuredGridPartitioner::PrintSelf(std::ostream& oss, svtkIndent indent)
{
  this->Superclass::PrintSelf(oss, indent);
  oss << "NumberOfPartitions: " << this->NumberOfPartitions << std::endl;
  oss << "NumberOfGhostLayers: " << this->NumberOfGhostLayers << std::endl;
  oss << "DuplicateNodes: " << this->DuplicateNodes << std::endl;
}

//------------------------------------------------------------------------------
int svtkStructuredGridPartitioner::FillInputPortInformation(
  int svtkNotUsed(port), svtkInformation* info)
{
  info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkStructuredGrid");
  return 1;
}

//------------------------------------------------------------------------------
int svtkStructuredGridPartitioner::FillOutputPortInformation(
  int svtkNotUsed(port), svtkInformation* info)
{
  info->Set(svtkDataObject::DATA_TYPE_NAME(), "svtkMultiBlockDataSet");
  return 1;
}

//------------------------------------------------------------------------------
svtkPoints* svtkStructuredGridPartitioner::ExtractSubGridPoints(
  svtkStructuredGrid* wholeGrid, int subext[6])
{
  assert("pre: whole grid is nullptr" && (wholeGrid != nullptr));

  int numNodes = svtkStructuredData::GetNumberOfPoints(subext);
  svtkPoints* pnts = svtkPoints::New();
  pnts->SetDataTypeToDouble();
  pnts->SetNumberOfPoints(numNodes);

  int ijk[3];
  double p[3];
  int dataDescription = svtkStructuredData::GetDataDescriptionFromExtent(subext);
  for (int i = subext[0]; i <= subext[1]; ++i)
  {
    for (int j = subext[2]; j <= subext[3]; ++j)
    {
      for (int k = subext[4]; k <= subext[5]; ++k)
      {
        wholeGrid->GetPoint(i, j, k, p, false);

        ijk[0] = i;
        ijk[1] = j;
        ijk[2] = k;
        svtkIdType pntIdx = svtkStructuredData::ComputePointIdForExtent(subext, ijk, dataDescription);
        assert("pre: point index is out-of-bounds!" && (pntIdx >= 0) && (pntIdx < numNodes));
        pnts->SetPoint(pntIdx, p);
      } // END for all k
    }   // END for all j
  }     // END for all i
  return (pnts);
}

//------------------------------------------------------------------------------
int svtkStructuredGridPartitioner::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // STEP 0: Get input object
  svtkInformation* input = inputVector[0]->GetInformationObject(0);
  assert("pre: input information object is nullptr" && (input != nullptr));
  svtkStructuredGrid* grd =
    svtkStructuredGrid::SafeDownCast(input->Get(svtkDataObject::DATA_OBJECT()));

  // STEP 1: Get output object
  svtkInformation* output = outputVector->GetInformationObject(0);
  assert("pre: output information object is nullptr" && (output != nullptr));
  svtkMultiBlockDataSet* multiblock =
    svtkMultiBlockDataSet::SafeDownCast(output->Get(svtkDataObject::DATA_OBJECT()));
  assert("pre: multi-block grid is nullptr" && (multiblock != nullptr));

  // STEP 2: Get the global extent
  int extent[6];
  grd->GetExtent(extent);

  // STEP 3: Setup extent partitioner
  svtkExtentRCBPartitioner* extentPartitioner = svtkExtentRCBPartitioner::New();
  assert("pre: extent partitioner is nullptr" && (extentPartitioner != nullptr));
  extentPartitioner->SetGlobalExtent(extent);
  extentPartitioner->SetNumberOfPartitions(this->NumberOfPartitions);
  extentPartitioner->SetNumberOfGhostLayers(this->NumberOfGhostLayers);

  if (this->DuplicateNodes == 1)
  {
    extentPartitioner->DuplicateNodesOn();
  }
  else
  {
    extentPartitioner->DuplicateNodesOff();
  }

  // STEP 4: Partition
  extentPartitioner->Partition();

  // STEP 5: Extract partitions in a multi-block
  multiblock->SetNumberOfBlocks(extentPartitioner->GetNumExtents());

  // Set the whole extent of the grid
  multiblock->GetInformation()->Set(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), extent, 6);

  int subext[6];
  unsigned int blockIdx = 0;
  for (; blockIdx < multiblock->GetNumberOfBlocks(); ++blockIdx)
  {
    extentPartitioner->GetPartitionExtent(blockIdx, subext);

    svtkStructuredGrid* subgrid = svtkStructuredGrid::New();
    subgrid->SetExtent(subext);

    svtkPoints* points = this->ExtractSubGridPoints(grd, subext);
    assert("pre: subgrid points are nullptr" && (points != nullptr));
    subgrid->SetPoints(points);
    points->Delete();

    svtkInformation* metadata = multiblock->GetMetaData(blockIdx);
    assert("pre: metadata is nullptr" && (metadata != nullptr));
    metadata->Set(svtkDataObject::PIECE_EXTENT(), subext, 6);

    multiblock->SetBlock(blockIdx, subgrid);
    subgrid->Delete();
  } // END for all blocks

  extentPartitioner->Delete();
  return 1;
}
