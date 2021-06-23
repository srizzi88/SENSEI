/*=========================================================================

 Program:   Visualization Toolkit
 Module:    svtkPStructuredGridGhostDataGenerator.cxx

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/
#include "svtkPStructuredGridGhostDataGenerator.h"
#include "svtkCommunicator.h"
#include "svtkInformation.h"
#include "svtkMultiBlockDataSet.h"
#include "svtkMultiProcessController.h"
#include "svtkObjectFactory.h"
#include "svtkPStructuredGridConnectivity.h"
#include "svtkStreamingDemandDrivenPipeline.h"
#include "svtkStructuredGrid.h"

#include <cassert>

svtkStandardNewMacro(svtkPStructuredGridGhostDataGenerator);

//------------------------------------------------------------------------------
svtkPStructuredGridGhostDataGenerator::svtkPStructuredGridGhostDataGenerator()
{
  this->GridConnectivity = svtkPStructuredGridConnectivity::New();
}

//------------------------------------------------------------------------------
svtkPStructuredGridGhostDataGenerator::~svtkPStructuredGridGhostDataGenerator()
{
  this->GridConnectivity->Delete();
}

//------------------------------------------------------------------------------
void svtkPStructuredGridGhostDataGenerator::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
void svtkPStructuredGridGhostDataGenerator::RegisterGrids(svtkMultiBlockDataSet* in)
{
  assert("pre: input multi-block is nullptr" && (in != nullptr));
  assert("pre: grid connectivity is nullptr" && (this->GridConnectivity != nullptr));

  this->GridConnectivity->SetController(this->Controller);
  this->GridConnectivity->SetNumberOfGrids(in->GetNumberOfBlocks());
  this->GridConnectivity->SetNumberOfGhostLayers(0);
  this->GridConnectivity->SetWholeExtent(
    in->GetInformation()->Get(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()));
  this->GridConnectivity->Initialize();

  for (unsigned int i = 0; i < in->GetNumberOfBlocks(); ++i)
  {
    svtkStructuredGrid* grid = svtkStructuredGrid::SafeDownCast(in->GetBlock(i));
    if (grid != nullptr)
    {
      svtkInformation* info = in->GetMetaData(i);
      assert("pre: nullptr meta-data" && (info != nullptr));
      assert("pre: No piece meta-data" && info->Has(svtkDataObject::PIECE_EXTENT()));

      this->GridConnectivity->RegisterGrid(static_cast<int>(i),
        info->Get(svtkDataObject::PIECE_EXTENT()), grid->GetPointGhostArray(),
        grid->GetCellGhostArray(), grid->GetPointData(), grid->GetCellData(), grid->GetPoints());
    } // END if the grid is not nullptr
  }   // END for all blocks
}

//------------------------------------------------------------------------------
void svtkPStructuredGridGhostDataGenerator::CreateGhostedDataSet(
  svtkMultiBlockDataSet* in, svtkMultiBlockDataSet* out)
{
  assert("pre: input multi-block is nullptr" && (in != nullptr));
  assert("pre: output multi-block is nullptr" && (out != nullptr));

  out->SetNumberOfBlocks(in->GetNumberOfBlocks());
  int wholeExt[6];
  in->GetInformation()->Get(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), wholeExt);
  out->GetInformation()->Set(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), wholeExt, 6);

  int ghostedExtent[6];
  for (unsigned int i = 0; i < out->GetNumberOfBlocks(); ++i)
  {
    if (in->GetBlock(i) != nullptr)
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
    }
    else
    {
      out->SetBlock(i, nullptr);
    }
  } // END for all blocks
}

//------------------------------------------------------------------------------
void svtkPStructuredGridGhostDataGenerator::GenerateGhostLayers(
  svtkMultiBlockDataSet* in, svtkMultiBlockDataSet* out)
{
  assert("pre: input multi-block is nullptr" && (in != nullptr));
  assert("pre: output multi-block is nullptr" && (out != nullptr));
  assert("pre: grid connectivity is nullptr" && (this->GridConnectivity != nullptr));
  assert("pre: controller should not be nullptr" && (this->Controller != nullptr));

  // STEP 0: Register grids
  this->RegisterGrids(in);
  this->Barrier();

  // STEP 1: Compute neighboring topology
  this->GridConnectivity->ComputeNeighbors();

  // STEP 2: Create ghost layers
  this->GridConnectivity->CreateGhostLayers(this->NumberOfGhostLayers);

  // STEP 3: Create the ghosted data-set
  this->CreateGhostedDataSet(in, out);
  this->Barrier();
}
