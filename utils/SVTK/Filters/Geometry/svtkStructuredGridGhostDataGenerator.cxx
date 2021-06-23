/*=========================================================================

 Program:   Visualization Toolkit
 Module:    svtkStructuredGridGhostDataGenerator.cxx

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/
#include "svtkStructuredGridGhostDataGenerator.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkMultiBlockDataSet.h"
#include "svtkObjectFactory.h"
#include "svtkStructuredGrid.h"
#include "svtkStructuredGridConnectivity.h"

#include "svtkStreamingDemandDrivenPipeline.h"

svtkStandardNewMacro(svtkStructuredGridGhostDataGenerator);

//------------------------------------------------------------------------------
svtkStructuredGridGhostDataGenerator::svtkStructuredGridGhostDataGenerator()
{
  this->GridConnectivity = svtkStructuredGridConnectivity::New();
}

//------------------------------------------------------------------------------
svtkStructuredGridGhostDataGenerator::~svtkStructuredGridGhostDataGenerator()
{
  this->GridConnectivity->Delete();
}

//------------------------------------------------------------------------------
void svtkStructuredGridGhostDataGenerator::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
void svtkStructuredGridGhostDataGenerator::RegisterGrids(svtkMultiBlockDataSet* in)
{
  assert("pre: Input multi-block is nullptr" && (in != nullptr));
  assert("pre: Grid connectivity should not be nullptr" && (this->GridConnectivity != nullptr));

  this->GridConnectivity->SetNumberOfGrids(in->GetNumberOfBlocks());
  this->GridConnectivity->SetNumberOfGhostLayers(0);
  this->GridConnectivity->SetWholeExtent(
    in->GetInformation()->Get(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()));

  for (unsigned int i = 0; i < in->GetNumberOfBlocks(); ++i)
  {
    svtkStructuredGrid* grid = svtkStructuredGrid::SafeDownCast(in->GetBlock(i));
    assert("pre: grid block is nullptr" && (grid != nullptr));

    svtkInformation* info = in->GetMetaData(i);
    assert("pre: nullptr meta-data" && (info != nullptr));
    assert("pre: No piece meta-data" && info->Has(svtkDataObject::PIECE_EXTENT()));

    this->GridConnectivity->RegisterGrid(static_cast<int>(i),
      info->Get(svtkDataObject::PIECE_EXTENT()), grid->GetPointGhostArray(),
      grid->GetCellGhostArray(), grid->GetPointData(), grid->GetCellData(), grid->GetPoints());
  } // END for all blocks
}

//------------------------------------------------------------------------------
void svtkStructuredGridGhostDataGenerator::CreateGhostedDataSet(
  svtkMultiBlockDataSet* in, svtkMultiBlockDataSet* out)
{
  assert("pre: Input multi-block is nullptr" && (in != nullptr));
  assert("pre: Output multi-block is nullptr" && (out != nullptr));
  assert("pre: Grid connectivity should not be nullptr" && (this->GridConnectivity != nullptr));

  out->SetNumberOfBlocks(in->GetNumberOfBlocks());
  int wholeExt[6];
  in->GetInformation()->Get(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), wholeExt);
  svtkInformation* outInfo = out->GetInformation();
  outInfo->Set(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), wholeExt, 6);

  int ghostedExtent[6];
  for (unsigned int i = 0; i < out->GetNumberOfBlocks(); ++i)
  {
    // STEP 0: Get the computed ghosted grid extent
    this->GridConnectivity->GetGhostedGridExtent(i, ghostedExtent);

    // STEP 1: Construct the ghosted structured grid instance
    svtkStructuredGrid* ghostedGrid = svtkStructuredGrid::New();
    assert("pre: Cannot create ghosted grid instance" && (ghostedGrid != nullptr));
    ghostedGrid->SetExtent(ghostedExtent);

    svtkPoints* ghostedGridPoints = svtkPoints::New();
    ghostedGridPoints->DeepCopy(this->GridConnectivity->GetGhostedPoints(i));
    ghostedGrid->SetPoints(ghostedGridPoints);
    ghostedGridPoints->Delete();

    // STEP 2: Copy the node/cell data
    ghostedGrid->GetPointData()->DeepCopy(this->GridConnectivity->GetGhostedGridPointData(i));
    ghostedGrid->GetCellData()->DeepCopy(this->GridConnectivity->GetGhostedGridCellData(i));

    out->SetBlock(i, ghostedGrid);
    ghostedGrid->Delete();
  } // END for all blocks
}

//------------------------------------------------------------------------------
void svtkStructuredGridGhostDataGenerator::GenerateGhostLayers(
  svtkMultiBlockDataSet* in, svtkMultiBlockDataSet* out)
{
  assert("pre: Input multi-block is nullptr" && (in != nullptr));
  assert("pre: Output multi-block is nullptr" && (out != nullptr));
  assert("pre: Grid connectivity should not be nullptr" && (this->GridConnectivity != nullptr));

  // STEP 0: Register the input grids
  this->RegisterGrids(in);

  // STEP 1: Computes the neighbors
  this->GridConnectivity->ComputeNeighbors();

  // STEP 2: Generate the ghost layers
  this->GridConnectivity->CreateGhostLayers(this->NumberOfGhostLayers);

  // STEP 3: Get the output dataset
  this->CreateGhostedDataSet(in, out);
}
