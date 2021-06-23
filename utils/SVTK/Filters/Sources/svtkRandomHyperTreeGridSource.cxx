/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkRandomHyperTreeGridSource.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkRandomHyperTreeGridSource.h"

#include "svtkDoubleArray.h"
#include "svtkExtentTranslator.h"
#include "svtkHyperTree.h"
#include "svtkHyperTreeGrid.h"
#include "svtkHyperTreeGridNonOrientedCursor.h"
#include "svtkIdTypeArray.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkMinimalStandardRandomSequence.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkStreamingDemandDrivenPipeline.h"

svtkStandardNewMacro(svtkRandomHyperTreeGridSource);

//------------------------------------------------------------------------------
void svtkRandomHyperTreeGridSource::PrintSelf(std::ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
svtkRandomHyperTreeGridSource::svtkRandomHyperTreeGridSource()
  : Seed(0)
  , MaxDepth(5)
  , SplitFraction(0.5)
  , Levels(nullptr)
{
  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(1);

  this->Dimensions[0] = 5 + 1;
  this->Dimensions[1] = 5 + 1;
  this->Dimensions[2] = 2 + 1;

  for (size_t i = 0; i < 3; ++i)
  {
    this->OutputBounds[2 * i] = -10.;
    this->OutputBounds[2 * i + 1] = 10.;
  }
}

//------------------------------------------------------------------------------
svtkRandomHyperTreeGridSource::~svtkRandomHyperTreeGridSource() {}

//------------------------------------------------------------------------------
int svtkRandomHyperTreeGridSource::RequestInformation(
  svtkInformation* req, svtkInformationVector** inInfo, svtkInformationVector* outInfo)
{
  using SDDP = svtkStreamingDemandDrivenPipeline;

  if (!this->Superclass::RequestInformation(req, inInfo, outInfo))
  {
    return 0;
  }

  int wholeExtent[6] = {
    0,
    static_cast<int>(this->Dimensions[0] - 1),
    0,
    static_cast<int>(this->Dimensions[1] - 1),
    0,
    static_cast<int>(this->Dimensions[2] - 1),
  };

  svtkInformation* info = outInfo->GetInformationObject(0);
  info->Set(SDDP::WHOLE_EXTENT(), wholeExtent, 6);
  info->Set(svtkAlgorithm::CAN_PRODUCE_SUB_EXTENT(), 1);

  return 1;
}

//------------------------------------------------------------------------------
int svtkRandomHyperTreeGridSource::RequestData(
  svtkInformation*, svtkInformationVector**, svtkInformationVector* outInfos)
{
  using SDDP = svtkStreamingDemandDrivenPipeline;

  svtkInformation* outInfo = outInfos->GetInformationObject(0);

  int* updateExtent = outInfo->Get(SDDP::UPDATE_EXTENT());

  // Create dataset:
  auto fillArray = [](
                     svtkDoubleArray* array, svtkIdType numPoints, double minBound, double maxBound) {
    array->SetNumberOfComponents(1);
    array->SetNumberOfTuples(numPoints);
    double step = (maxBound - minBound) / static_cast<double>(numPoints - 1);
    for (int i = 0; i < numPoints; ++i)
    {
      array->SetTypedComponent(i, 0, minBound + step * i);
    }
  };

  svtkHyperTreeGrid* htg = svtkHyperTreeGrid::GetData(outInfo);
  htg->Initialize();
  htg->SetDimensions(this->Dimensions);
  htg->SetBranchFactor(2);

  {
    svtkNew<svtkDoubleArray> coords;
    fillArray(coords, this->Dimensions[0], this->OutputBounds[0], this->OutputBounds[1]);
    htg->SetXCoordinates(coords);
  }

  {
    svtkNew<svtkDoubleArray> coords;
    fillArray(coords, this->Dimensions[1], this->OutputBounds[2], this->OutputBounds[3]);
    htg->SetYCoordinates(coords);
  }

  {
    svtkNew<svtkDoubleArray> coords;
    fillArray(coords, this->Dimensions[2], this->OutputBounds[4], this->OutputBounds[5]);
    htg->SetZCoordinates(coords);
  }

  svtkNew<svtkDoubleArray> levels;
  levels->SetName("Depth");
  htg->GetPointData()->AddArray(levels);
  this->Levels = levels;

  svtkIdType treeOffset = 0;
  for (int i = updateExtent[0]; i < updateExtent[1]; ++i)
  {
    for (int j = updateExtent[2]; j < updateExtent[3]; ++j)
    {
      for (int k = updateExtent[4]; k < updateExtent[5]; ++k)
      {
        svtkIdType treeId;
        htg->GetIndexFromLevelZeroCoordinates(treeId, static_cast<unsigned int>(i),
          static_cast<unsigned int>(j), static_cast<unsigned int>(k));

        // Initialize RNG per tree to make it easier to parallelize
        this->RNG->Initialize(this->Seed + treeId);

        // Build this tree:
        svtkHyperTreeGridNonOrientedCursor* cursor = htg->NewNonOrientedCursor(treeId, true);
        cursor->GetTree()->SetGlobalIndexStart(treeOffset);
        this->SubdivideLeaves(cursor, treeId);
        treeOffset += cursor->GetTree()->GetNumberOfVertices();
        cursor->Delete();
      }
    }
  }

  // Cleanup
  this->Levels = nullptr;

  return 1;
}

//------------------------------------------------------------------------------
int svtkRandomHyperTreeGridSource::FillOutputPortInformation(int, svtkInformation* info)
{
  info->Set(svtkDataObject::DATA_TYPE_NAME(), "svtkHyperTreeGrid");
  return 1;
}

//------------------------------------------------------------------------------
void svtkRandomHyperTreeGridSource::SubdivideLeaves(
  svtkHyperTreeGridNonOrientedCursor* cursor, svtkIdType treeId)
{
  svtkIdType vertexId = cursor->GetVertexId();
  svtkHyperTree* tree = cursor->GetTree();
  svtkIdType idx = tree->GetGlobalIndexFromLocal(vertexId);
  svtkIdType level = cursor->GetLevel();

  this->Levels->InsertValue(idx, level);

  if (cursor->IsLeaf())
  {
    if (this->ShouldRefine(level))
    {
      cursor->SubdivideLeaf();
      this->SubdivideLeaves(cursor, treeId);
    }
  }
  else
  {
    int numChildren = cursor->GetNumberOfChildren();
    for (int childIdx = 0; childIdx < numChildren; ++childIdx)
    {
      cursor->ToChild(childIdx);
      this->SubdivideLeaves(cursor, treeId);
      cursor->ToParent();
    }
  }
}

//------------------------------------------------------------------------------
bool svtkRandomHyperTreeGridSource::ShouldRefine(svtkIdType level)
{
  this->RNG->Next();
  return level < this->MaxDepth && this->RNG->GetValue() < this->SplitFraction;
}
