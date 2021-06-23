/*=========================================================================

 Program:   Visualization Toolkit
 Module:    svtkRectilinearGridPartitioner.cxx

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/
#include "svtkRectilinearGridPartitioner.h"

// SVTK includes
#include "svtkDoubleArray.h"
#include "svtkExtentRCBPartitioner.h"
#include "svtkIndent.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkMultiBlockDataSet.h"
#include "svtkObjectFactory.h"
#include "svtkRectilinearGrid.h"
#include "svtkStreamingDemandDrivenPipeline.h"
#include "svtkStructuredData.h"
#include "svtkStructuredExtent.h"

#include <cassert>

svtkStandardNewMacro(svtkRectilinearGridPartitioner);

//------------------------------------------------------------------------------
svtkRectilinearGridPartitioner::svtkRectilinearGridPartitioner()
{
  this->NumberOfPartitions = 2;
  this->NumberOfGhostLayers = 0;
  this->DuplicateNodes = 1;
  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(1);
}

//------------------------------------------------------------------------------
svtkRectilinearGridPartitioner::~svtkRectilinearGridPartitioner() = default;

//------------------------------------------------------------------------------
void svtkRectilinearGridPartitioner::PrintSelf(std::ostream& oss, svtkIndent indent)
{
  this->Superclass::PrintSelf(oss, indent);
  oss << "NumberOfPartitions: " << this->NumberOfPartitions << std::endl;
  oss << "NumberOfGhostLayers: " << this->NumberOfGhostLayers << std::endl;
}

//------------------------------------------------------------------------------
int svtkRectilinearGridPartitioner::FillInputPortInformation(
  int svtkNotUsed(port), svtkInformation* info)
{
  info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkRectilinearGrid");
  return 1;
}

//------------------------------------------------------------------------------
int svtkRectilinearGridPartitioner::FillOutputPortInformation(
  int svtkNotUsed(port), svtkInformation* info)
{
  info->Set(svtkDataObject::DATA_TYPE_NAME(), "svtkMultiBlockDataSet");
  return 1;
}

//------------------------------------------------------------------------------
void svtkRectilinearGridPartitioner::ExtractGridCoordinates(svtkRectilinearGrid* grd, int subext[6],
  svtkDoubleArray* xcoords, svtkDoubleArray* ycoords, svtkDoubleArray* zcoords)
{
  assert("pre: nullptr rectilinear grid" && (grd != nullptr));
  assert("pre: nullptr xcoords" && (xcoords != nullptr));
  assert("pre: nullptr ycoords" && (ycoords != nullptr));
  assert("pre: nullptr zcoords" && (zcoords != nullptr));

  int dataDescription = svtkStructuredData::GetDataDescriptionFromExtent(subext);

  int ndims[3];
  svtkStructuredData::GetDimensionsFromExtent(subext, ndims, dataDescription);

  svtkDoubleArray* coords[3];
  coords[0] = xcoords;
  coords[1] = ycoords;
  coords[2] = zcoords;

  svtkDataArray* src_coords[3];
  src_coords[0] = grd->GetXCoordinates();
  src_coords[1] = grd->GetYCoordinates();
  src_coords[2] = grd->GetZCoordinates();

  for (int dim = 0; dim < 3; ++dim)
  {
    coords[dim]->SetNumberOfComponents(1);
    coords[dim]->SetNumberOfTuples(ndims[dim]);

    for (int idx = subext[dim * 2]; idx <= subext[dim * 2 + 1]; ++idx)
    {
      svtkIdType lidx = idx - subext[dim * 2];
      coords[dim]->SetTuple1(lidx, src_coords[dim]->GetTuple1(idx));
    } // END for all ids

  } // END for all dimensions
}

//------------------------------------------------------------------------------
int svtkRectilinearGridPartitioner::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // STEP 0: Get input object
  svtkInformation* input = inputVector[0]->GetInformationObject(0);
  assert("pre: input information object is nullptr" && (input != nullptr));
  svtkRectilinearGrid* grd =
    svtkRectilinearGrid::SafeDownCast(input->Get(svtkDataObject::DATA_OBJECT()));

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

  // STEP 5: Extract partition in a multi-block
  multiblock->SetNumberOfBlocks(extentPartitioner->GetNumExtents());

  // Set the whole extent of the grid
  multiblock->GetInformation()->Set(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), extent, 6);

  int subext[6];
  unsigned int blockIdx = 0;
  for (; blockIdx < multiblock->GetNumberOfBlocks(); ++blockIdx)
  {
    extentPartitioner->GetPartitionExtent(blockIdx, subext);
    svtkRectilinearGrid* subgrid = svtkRectilinearGrid::New();
    subgrid->SetExtent(subext);

    svtkDoubleArray* xcoords = svtkDoubleArray::New();
    svtkDoubleArray* ycoords = svtkDoubleArray::New();
    svtkDoubleArray* zcoords = svtkDoubleArray::New();

    this->ExtractGridCoordinates(grd, subext, xcoords, ycoords, zcoords);

    subgrid->SetXCoordinates(xcoords);
    subgrid->SetYCoordinates(ycoords);
    subgrid->SetZCoordinates(zcoords);
    xcoords->Delete();
    ycoords->Delete();
    zcoords->Delete();

    svtkInformation* metadata = multiblock->GetMetaData(blockIdx);
    assert("pre: metadata is nullptr" && (metadata != nullptr));
    metadata->Set(svtkDataObject::PIECE_EXTENT(), subext, 6);

    multiblock->SetBlock(blockIdx, subgrid);
    subgrid->Delete();
  } // END for all blocks

  extentPartitioner->Delete();
  return 1;
}
