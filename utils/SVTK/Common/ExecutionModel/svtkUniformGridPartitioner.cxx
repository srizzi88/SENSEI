/*=========================================================================

 Program:   Visualization Toolkit
 Module:    svtkUniformGridPartitioner.cxx

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/

#include "svtkUniformGridPartitioner.h"
#include "svtkExtentRCBPartitioner.h"
#include "svtkIndent.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkMultiBlockDataSet.h"
#include "svtkObjectFactory.h"
#include "svtkStreamingDemandDrivenPipeline.h"
#include "svtkStructuredData.h"
#include "svtkStructuredExtent.h"
#include "svtkUniformGrid.h"

#include <cassert>

svtkStandardNewMacro(svtkUniformGridPartitioner);

//------------------------------------------------------------------------------
svtkUniformGridPartitioner::svtkUniformGridPartitioner()
{
  this->NumberOfPartitions = 2;
  this->NumberOfGhostLayers = 0;
  this->DuplicateNodes = 1;
}

//------------------------------------------------------------------------------
svtkUniformGridPartitioner::~svtkUniformGridPartitioner() = default;

//------------------------------------------------------------------------------
void svtkUniformGridPartitioner::PrintSelf(std::ostream& oss, svtkIndent indent)
{
  this->Superclass::PrintSelf(oss, indent);
  oss << "NumberOfPartitions: " << this->NumberOfPartitions << std::endl;
  oss << "NumberOfGhostLayers: " << this->NumberOfGhostLayers << std::endl;
  oss << "DuplicateNodes: " << this->DuplicateNodes << std::endl;
}

//------------------------------------------------------------------------------
int svtkUniformGridPartitioner::FillInputPortInformation(int svtkNotUsed(port), svtkInformation* info)
{
  info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkImageData");
  return 1;
}

//------------------------------------------------------------------------------
int svtkUniformGridPartitioner::FillOutputPortInformation(int svtkNotUsed(port), svtkInformation* info)
{
  info->Set(svtkDataObject::DATA_TYPE_NAME(), "svtkMultiBlockDataSet");
  return 1;
}

//------------------------------------------------------------------------------
int svtkUniformGridPartitioner::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // STEP 0: Get input object
  svtkInformation* input = inputVector[0]->GetInformationObject(0);
  assert("pre: input information object is nullptr" && (input != nullptr));

  svtkImageData* grd = svtkImageData::SafeDownCast(input->Get(svtkDataObject::DATA_OBJECT()));
  assert("pre: input grid is nullptr!" && (grd != nullptr));

  // STEP 1: Get output object
  svtkInformation* output = outputVector->GetInformationObject(0);
  assert("pre: output information object is nullptr" && (output != nullptr));
  svtkMultiBlockDataSet* multiblock =
    svtkMultiBlockDataSet::SafeDownCast(output->Get(svtkDataObject::DATA_OBJECT()));
  assert("pre: multiblock grid is nullptr!" && (multiblock != nullptr));

  // STEP 2: Get the global extent
  int extent[6];
  int dims[3];
  grd->GetDimensions(dims);
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

  // STEP 5: Extract partitions into a multi-block dataset.
  multiblock->SetNumberOfBlocks(extentPartitioner->GetNumExtents());

  // Set the whole extent of the grid
  multiblock->GetInformation()->Set(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), extent, 6);

  unsigned int blockIdx = 0;
  for (; blockIdx < multiblock->GetNumberOfBlocks(); ++blockIdx)
  {
    int ext[6];
    extentPartitioner->GetPartitionExtent(blockIdx, ext);

    double origin[3];
    int ijk[3];
    ijk[0] = ext[0];
    ijk[1] = ext[2];
    ijk[2] = ext[4];

    int subdims[3];
    svtkStructuredExtent::GetDimensions(ext, subdims);

    svtkIdType pntIdx = svtkStructuredData::ComputePointId(dims, ijk);

    grd->GetPoint(pntIdx, origin);

    svtkUniformGrid* subgrid = svtkUniformGrid::New();
    subgrid->SetOrigin(origin);
    subgrid->SetSpacing(grd->GetSpacing());
    subgrid->SetDimensions(subdims);

    // Set the global extent for each block
    svtkInformation* metadata = multiblock->GetMetaData(blockIdx);
    assert("pre: metadata is nullptr" && (metadata != nullptr));
    metadata->Set(svtkDataObject::PIECE_EXTENT(), ext, 6);

    multiblock->SetBlock(blockIdx, subgrid);
    subgrid->Delete();
  } // END for all blocks

  extentPartitioner->Delete();
  return 1;
}
