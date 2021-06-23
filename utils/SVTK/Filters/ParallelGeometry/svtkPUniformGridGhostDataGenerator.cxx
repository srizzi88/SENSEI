/*=========================================================================

 Program:   Visualization Toolkit
 Module:    svtkPUniformGridGhostDataGenerator.cxx

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/
#include "svtkPUniformGridGhostDataGenerator.h"
#include "svtkCommunicator.h"
#include "svtkInformation.h"
#include "svtkMultiBlockDataSet.h"
#include "svtkMultiProcessController.h"
#include "svtkObjectFactory.h"
#include "svtkPStructuredGridConnectivity.h"
#include "svtkStreamingDemandDrivenPipeline.h"
#include "svtkUniformGrid.h"

#include <cassert>

svtkStandardNewMacro(svtkPUniformGridGhostDataGenerator);

svtkPUniformGridGhostDataGenerator::svtkPUniformGridGhostDataGenerator()
{
  this->GridConnectivity = svtkPStructuredGridConnectivity::New();

  this->GlobalOrigin[0] = this->GlobalOrigin[1] = this->GlobalOrigin[2] = SVTK_DOUBLE_MAX;

  this->GlobalSpacing[0] = this->GlobalSpacing[1] = this->GlobalSpacing[2] = SVTK_DOUBLE_MIN;
}

//------------------------------------------------------------------------------
svtkPUniformGridGhostDataGenerator::~svtkPUniformGridGhostDataGenerator()
{
  this->GridConnectivity->Delete();
}

//------------------------------------------------------------------------------
void svtkPUniformGridGhostDataGenerator::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
void svtkPUniformGridGhostDataGenerator::RegisterGrids(svtkMultiBlockDataSet* in)
{
  assert("pre: input multi-block is nullptr" && (in != nullptr));
  assert("pre: Grid Connectivity is nullptr" && (this->GridConnectivity != nullptr));

  this->GridConnectivity->SetController(this->Controller);
  this->GridConnectivity->SetNumberOfGrids(in->GetNumberOfBlocks());
  this->GridConnectivity->SetNumberOfGhostLayers(0);
  this->GridConnectivity->SetWholeExtent(
    in->GetInformation()->Get(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()));
  this->GridConnectivity->Initialize();

  for (unsigned int i = 0; i < in->GetNumberOfBlocks(); ++i)
  {
    svtkUniformGrid* grid = svtkUniformGrid::SafeDownCast(in->GetBlock(i));
    if (grid != nullptr)
    {
      svtkInformation* info = in->GetMetaData(i);
      assert("pre: nullptr meta-data" && (info != nullptr));
      assert("pre: No piece meta-data" && info->Has(svtkDataObject::PIECE_EXTENT()));

      this->GridConnectivity->RegisterGrid(static_cast<int>(i),
        info->Get(svtkDataObject::PIECE_EXTENT()), grid->GetPointGhostArray(),
        grid->GetCellGhostArray(), grid->GetPointData(), grid->GetCellData(), nullptr);
    } // END if
  }   // END for all blocks
}

//------------------------------------------------------------------------------
void svtkPUniformGridGhostDataGenerator::GenerateGhostLayers(
  svtkMultiBlockDataSet* in, svtkMultiBlockDataSet* out)
{
  // Sanity check
  assert("pre: input multi-block is nullptr" && (in != nullptr));
  assert("pre: output multi-block is nullptr" && (out != nullptr));
  assert("pre: initialized" && (this->Initialized));
  assert("pre: Grid connectivity is nullptr" && (this->GridConnectivity != nullptr));
  assert("pre: controller should not be nullptr" && (this->Controller != nullptr));

  // STEP 0: Compute global grid parameters
  this->ComputeGlobalSpacing(in);
  this->ComputeOrigin(in);
  this->Barrier();

  // STEP 1: Register grids
  this->RegisterGrids(in);
  this->Barrier();

  // STEP 2: Compute neighbors
  this->GridConnectivity->ComputeNeighbors();

  // STEP 3: Generate ghost layers
  this->GridConnectivity->CreateGhostLayers(this->NumberOfGhostLayers);

  // STEP 4: Create the ghosted data-set
  this->CreateGhostedDataSet(in, out);
  this->Barrier();
}

//------------------------------------------------------------------------------
void svtkPUniformGridGhostDataGenerator::ComputeGlobalSpacing(svtkMultiBlockDataSet* in)
{
  assert("pre: input multi-block is nullptr" && (in != nullptr));
  assert("pre: Controller should not be nullptr" && (this->Controller != nullptr));

  for (unsigned int block = 0; block < in->GetNumberOfBlocks(); ++block)
  {
    svtkUniformGrid* grid = svtkUniformGrid::SafeDownCast(in->GetBlock(block));
    if (grid != nullptr)
    {
      grid->GetSpacing(this->GlobalSpacing);
    } // END if grid is not nullptr
  }   // End for all blocks
}

//------------------------------------------------------------------------------
void svtkPUniformGridGhostDataGenerator::CreateGhostedDataSet(
  svtkMultiBlockDataSet* in, svtkMultiBlockDataSet* out)
{
  assert("pre: input multi-block is nullptr" && (in != nullptr));
  assert("pre: output multi-block is nullptr" && (out != nullptr));

  out->SetNumberOfBlocks(in->GetNumberOfBlocks());
  int wholeExt[6];
  in->GetInformation()->Get(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), wholeExt);
  out->GetInformation()->Set(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), wholeExt, 6);

  int ghostedExtent[6];
  double origin[3];
  int dims[3];

  for (unsigned int i = 0; i < out->GetNumberOfBlocks(); ++i)
  {
    if (in->GetBlock(i) != nullptr)
    {
      // STEP 0: Get the computed ghosted grid extent
      this->GridConnectivity->GetGhostedGridExtent(i, ghostedExtent);

      // STEP 1: Get the ghosted grid dimensions from the ghosted extent
      svtkStructuredData::GetDimensionsFromExtent(ghostedExtent, dims);

      // STEP 2: Construct the ghosted grid instance
      svtkUniformGrid* ghostedGrid = svtkUniformGrid::New();
      assert("pre: Cannot create ghosted grid instance" && (ghostedGrid != nullptr));

      // STEP 3: Compute the ghosted grid origin
      origin[0] = this->GlobalOrigin[0] + ghostedExtent[0] * this->GlobalSpacing[0];
      origin[1] = this->GlobalOrigin[1] + ghostedExtent[2] * this->GlobalSpacing[1];
      origin[2] = this->GlobalOrigin[2] + ghostedExtent[4] * this->GlobalSpacing[2];

      // STEP 4: Set ghosted uniform grid attributes
      ghostedGrid->SetOrigin(origin);
      ghostedGrid->SetDimensions(dims);
      ghostedGrid->SetSpacing(this->GlobalSpacing);

      // STEP 5: Copy the node/cell data
      ghostedGrid->GetPointData()->DeepCopy(this->GridConnectivity->GetGhostedGridPointData(i));
      ghostedGrid->GetCellData()->DeepCopy(this->GridConnectivity->GetGhostedGridCellData(i));

      out->SetBlock(i, ghostedGrid);
      ghostedGrid->Delete();
    }
    else
    {
      out->SetBlock(i, nullptr);
    }
  } // END for all blocks
}

//------------------------------------------------------------------------------
void svtkPUniformGridGhostDataGenerator::ComputeOrigin(svtkMultiBlockDataSet* in)
{
  assert("pre: input multi-block is nullptr" && (in != nullptr));
  assert("pre: Controller should not be nullptr" && (this->Controller != nullptr));

  double localOrigin[3];
  localOrigin[0] = localOrigin[1] = localOrigin[2] = SVTK_DOUBLE_MAX;

  // STEP 1: Compute local origin
  double gridOrigin[3];
  for (unsigned int block = 0; block < in->GetNumberOfBlocks(); ++block)
  {
    svtkUniformGrid* grid = svtkUniformGrid::SafeDownCast(in->GetBlock(block));
    if (grid != nullptr)
    {
      grid->GetOrigin(gridOrigin);
      for (int i = 0; i < 3; ++i)
      {
        if (gridOrigin[i] < localOrigin[i])
        {
          localOrigin[i] = gridOrigin[i];
        }
      } // END for all dimensions
    }   // END if grid is not nullptr
  }     // END for all blocks

  // STEP 2: All reduce
  this->Controller->AllReduce(&localOrigin[0], &this->GlobalOrigin[0], 1, svtkCommunicator::MIN_OP);
  this->Controller->AllReduce(&localOrigin[1], &this->GlobalOrigin[1], 1, svtkCommunicator::MIN_OP);
  this->Controller->AllReduce(&localOrigin[2], &this->GlobalOrigin[2], 1, svtkCommunicator::MIN_OP);
}
