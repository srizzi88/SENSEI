/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkDistributedDataFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/

#include "svtkDistributedDataFilter.h"

#include "svtkAppendFilter.h"
#include "svtkBSPCuts.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkMultiBlockDataSet.h"
#include "svtkMultiProcessController.h"
#include "svtkObjectFactory.h"
#include "svtkPKdTree.h"
#include "svtkStreamingDemandDrivenPipeline.h"
#include "svtkUnstructuredGrid.h"

// Needed to let svtkPDistributedDataFilter be instantiated when available
svtkObjectFactoryNewMacro(svtkDistributedDataFilter);

//----------------------------------------------------------------------------
svtkDistributedDataFilter::svtkDistributedDataFilter()
{
  this->Kdtree = nullptr;

  this->Controller = nullptr;
  this->SetController(svtkMultiProcessController::GetGlobalController());

  this->Target = nullptr;
  this->Source = nullptr;

  this->NumConvexSubRegions = 0;
  this->ConvexSubRegionBounds = nullptr;

  this->MinimumGhostLevel = 0;
  this->GhostLevel = 0;

  this->RetainKdtree = 1;
  this->IncludeAllIntersectingCells = 0;
  this->ClipCells = 0;

  this->Timing = 0;

  this->UseMinimalMemory = 0;

  this->UserCuts = nullptr;
}

//----------------------------------------------------------------------------
svtkDistributedDataFilter::~svtkDistributedDataFilter()
{
  if (this->Kdtree)
  {
    this->Kdtree->Delete();
    this->Kdtree = nullptr;
  }

  this->SetController(nullptr);

  delete[] this->Target;
  this->Target = nullptr;

  delete[] this->Source;
  this->Source = nullptr;

  delete[] this->ConvexSubRegionBounds;
  this->ConvexSubRegionBounds = nullptr;

  if (this->UserCuts)
  {
    this->UserCuts->Delete();
    this->UserCuts = nullptr;
  }
}

//----------------------------------------------------------------------------
void svtkDistributedDataFilter::SetController(svtkMultiProcessController* c)
{
  if (this->Kdtree)
  {
    this->Kdtree->SetController(c);
  }

  if ((c == nullptr) || (c->GetNumberOfProcesses() == 0))
  {
    this->NumProcesses = 1;
    this->MyId = 0;
  }

  if (this->Controller == c)
  {
    return;
  }

  this->Modified();

  if (this->Controller != nullptr)
  {
    this->Controller->UnRegister(this);
    this->Controller = nullptr;
  }

  if (c == nullptr)
  {
    return;
  }

  this->Controller = c;

  c->Register(this);
  this->NumProcesses = c->GetNumberOfProcesses();
  this->MyId = c->GetLocalProcessId();
}

//-------------------------------------------------------------------------
svtkPKdTree* svtkDistributedDataFilter::GetKdtree()
{
  if (this->Kdtree == nullptr)
  {
    this->Kdtree = svtkPKdTree::New();
    this->Kdtree->AssignRegionsContiguous();
    this->Kdtree->SetTiming(this->GetTiming());
  }

  return this->Kdtree;
}

//----------------------------------------------------------------------------
void svtkDistributedDataFilter::SetBoundaryMode(int mode)
{
  int include_all, clip_cells;
  switch (mode)
  {
    case svtkDistributedDataFilter::ASSIGN_TO_ONE_REGION:
      include_all = 0;
      clip_cells = 0;
      break;
    case svtkDistributedDataFilter::ASSIGN_TO_ALL_INTERSECTING_REGIONS:
      include_all = 1;
      clip_cells = 0;
      break;
    case svtkDistributedDataFilter::SPLIT_BOUNDARY_CELLS:
    default:
      include_all = 1;
      clip_cells = 1;
      break;
  }

  if (this->IncludeAllIntersectingCells != include_all || this->ClipCells != clip_cells)
  {
    this->IncludeAllIntersectingCells = include_all;
    this->ClipCells = clip_cells;
    this->Modified();
  }
}

//----------------------------------------------------------------------------
int svtkDistributedDataFilter::GetBoundaryMode()
{
  if (!this->IncludeAllIntersectingCells && !this->ClipCells)
  {
    return svtkDistributedDataFilter::ASSIGN_TO_ONE_REGION;
  }
  if (this->IncludeAllIntersectingCells && !this->ClipCells)
  {
    return svtkDistributedDataFilter::ASSIGN_TO_ALL_INTERSECTING_REGIONS;
  }
  if (this->IncludeAllIntersectingCells && this->ClipCells)
  {
    return svtkDistributedDataFilter::SPLIT_BOUNDARY_CELLS;
  }

  return -1;
}

//-------------------------------------------------------------------------

int svtkDistributedDataFilter::RequestUpdateExtent(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  int piece, numPieces, ghostLevels;

  // We require preceding filters to refrain from creating ghost cells.

  piece = outInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
  numPieces = outInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());
  ghostLevels = 0;

  inInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(), piece);
  inInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(), numPieces);
  inInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(), ghostLevels);
  inInfo->Set(svtkStreamingDemandDrivenPipeline::EXACT_EXTENT(), 1);

  return 1;
}

//----------------------------------------------------------------------------
void svtkDistributedDataFilter::SetCuts(svtkBSPCuts* cuts)
{
  if (cuts == this->UserCuts)
  {
    return;
  }
  if (this->UserCuts)
  {
    this->UserCuts->Delete();
    this->UserCuts = nullptr;
  }
  if (cuts)
  {
    cuts->Register(this);
    this->UserCuts = cuts;
  }
  // Delete the Kdtree so that it is regenerated next time.
  if (this->Kdtree)
  {
    this->Kdtree->SetCuts(cuts);
  }
  this->Modified();
}

//----------------------------------------------------------------------------
void svtkDistributedDataFilter::SetUserRegionAssignments(const int* map, int numRegions)
{
  std::vector<int> copy(this->UserRegionAssignments);
  this->UserRegionAssignments.resize(numRegions);
  for (int cc = 0; cc < numRegions; cc++)
  {
    this->UserRegionAssignments[cc] = map[cc];
  }
  if (copy != this->UserRegionAssignments)
  {
    this->Modified();
  }
}

//----------------------------------------------------------------------------
int svtkDistributedDataFilter::RequestInformation(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  outInfo->Set(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),
    inInfo->Get(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()), 6);

  return 1;
}

//----------------------------------------------------------------------------
int svtkDistributedDataFilter::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output.
  svtkDataObject* input = svtkDataObject::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));
  svtkUnstructuredGrid* outputUG =
    svtkUnstructuredGrid::SafeDownCast(outInfo->Get(svtkUnstructuredGrid::DATA_OBJECT()));
  svtkCompositeDataSet* outputCD =
    svtkCompositeDataSet::SafeDownCast(outInfo->Get(svtkCompositeDataSet::DATA_OBJECT()));

  if (!input)
  {
    svtkErrorMacro("No input data!");
    return 0;
  }

  if (outputCD)
  {
    outputCD->ShallowCopy(input);
  }
  else
  {
    // svtkAppendFilter always produces a svtkUnstructuredGrid, so use it
    // to convert the svtkPolyData to an unstructured grid.
    svtkNew<svtkAppendFilter> converter;
    converter->SetInputData(input);
    converter->MergePointsOff();
    converter->Update();
    outputUG->ShallowCopy(converter->GetOutput());
  }

  return 1;
}

//-------------------------------------------------------------------------
int svtkDistributedDataFilter::RequestDataObject(
  svtkInformation*, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  if (!inInfo)
  {
    return 0;
  }

  svtkDataObject* input = svtkDataObject::GetData(inInfo);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);
  if (input)
  {
    svtkDataObject* output = svtkDataObject::GetData(outInfo);
    // If input is composite dataset, output is a svtkMultiBlockDataSet of
    // unstructrued grids.
    // If input is a dataset, output is an unstructured grid.
    if (!output || (input->IsA("svtkCompositeDataSet") && !output->IsA("svtkMultiBlockDataSet")) ||
      (input->IsA("svtkDataSet") && !output->IsA("svtkUnstructuredGrid")))
    {
      svtkDataObject* newOutput = nullptr;
      if (input->IsA("svtkCompositeDataSet"))
      {
        newOutput = svtkMultiBlockDataSet::New();
      }
      else // if (input->IsA("svtkDataSet"))
      {
        newOutput = svtkUnstructuredGrid::New();
      }
      outInfo->Set(svtkDataObject::DATA_OBJECT(), newOutput);
      newOutput->Delete();
    }
    return 1;
  }

  return 0;
}

//-------------------------------------------------------------------------
int svtkDistributedDataFilter::FillInputPortInformation(int, svtkInformation* info)
{
  info->Remove(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE());
  info->Append(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkCompositeDataSet");
  info->Append(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkDataSet");
  return 1;
}

//-------------------------------------------------------------------------
void svtkDistributedDataFilter::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
