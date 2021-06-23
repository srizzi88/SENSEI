/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkHyperTreeGridCellCenters.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkHyperTreeGridCellCenters.h"

#include "svtkAlgorithm.h"
#include "svtkBitArray.h"
#include "svtkCellArray.h"
#include "svtkHyperTree.h"
#include "svtkHyperTreeGrid.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkPoints.h"
#include "svtkStreamingDemandDrivenPipeline.h"

#include "svtkHyperTreeGridNonOrientedGeometryCursor.h"

svtkStandardNewMacro(svtkHyperTreeGridCellCenters);

//-----------------------------------------------------------------------------
svtkHyperTreeGridCellCenters::svtkHyperTreeGridCellCenters()
{
  this->Input = nullptr;
  this->Output = nullptr;

  this->InData = nullptr;
  this->OutData = nullptr;

  this->Points = nullptr;
}

//-----------------------------------------------------------------------------
svtkHyperTreeGridCellCenters::~svtkHyperTreeGridCellCenters() {}

//----------------------------------------------------------------------------
svtkTypeBool svtkHyperTreeGridCellCenters::ProcessRequest(
  svtkInformation* request, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // generate the data
  if (request->Has(svtkDemandDrivenPipeline::REQUEST_DATA()))
  {

    return this->RequestData(request, inputVector, outputVector);
  }

  if (request->Has(svtkStreamingDemandDrivenPipeline::REQUEST_UPDATE_EXTENT()))
  {
    return this->RequestUpdateExtent(request, inputVector, outputVector);
  }

  // execute information
  if (request->Has(svtkDemandDrivenPipeline::REQUEST_INFORMATION()))
  {
    return this->RequestInformation(request, inputVector, outputVector);
  }

  return this->Superclass::ProcessRequest(request, inputVector, outputVector);
}

//-----------------------------------------------------------------------------
int svtkHyperTreeGridCellCenters::FillInputPortInformation(
  int svtkNotUsed(port), svtkInformation* info)
{
  info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkHyperTreeGrid");
  return 1;
}

//----------------------------------------------------------------------------
void svtkHyperTreeGridCellCenters::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  if (this->Input)
  {
    os << indent << "Input:\n";
    this->Input->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << indent << "Input: ( none )\n";
  }

  if (this->Output)
  {
    os << indent << "Output:\n";
    this->Output->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << indent << "Output: ( none )\n";
  }

  if (this->Points)
  {
    os << indent << "Points:\n";
    this->Points->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << indent << "Points: ( none )\n";
  }
}

//----------------------------------------------------------------------------
int svtkHyperTreeGridCellCenters::RequestData(
  svtkInformation*, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // Get the information objects
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  // Retrieve input and output
  this->Input = svtkHyperTreeGrid::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));
  this->Output = svtkPolyData::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  // Initialize output cell data
  this->InData = this->Input->GetPointData();
  this->OutData = this->Output->GetPointData();
  this->OutData->CopyAllocate(this->InData);

  // General cell centers of hyper tree grid
  this->ProcessTrees();

  // Squeeze output data
  this->OutData->Squeeze();

  // Clean up
  this->Input = nullptr;
  this->Output = nullptr;
  this->InData = nullptr;
  this->OutData = nullptr;

  this->UpdateProgress(1.);

  return 1;
}

//----------------------------------------------------------------------------
void svtkHyperTreeGridCellCenters::ProcessTrees()
{
  // Create storage for corners of leaf cells
  this->Points = svtkPoints::New();

  // Retrieve material mask
  this->InMask = this->Input->HasMask() ? this->Input->GetMask() : nullptr;

  // Iterate over all hyper trees
  svtkIdType index;
  svtkHyperTreeGrid::svtkHyperTreeGridIterator it;
  this->Input->InitializeTreeIterator(it);
  svtkNew<svtkHyperTreeGridNonOrientedGeometryCursor> cursor;
  while (it.GetNextTree(index))
  {
    // Initialize new geometric cursor at root of current tree
    this->Input->InitializeNonOrientedGeometryCursor(cursor, index);
    // Generate leaf cell centers recursively
    this->RecursivelyProcessTree(cursor);
  } // it

  // Set output geometry and topology if required
  this->Output->SetPoints(this->Points);
  if (this->VertexCells)
  {
    svtkIdType np = this->Points->GetNumberOfPoints();
    svtkCellArray* vertices = svtkCellArray::New();
    vertices->AllocateEstimate(np, 1);
    for (svtkIdType i = 0; i < np; ++i)
    {
      vertices->InsertNextCell(1, &i);
    } // i
    this->Output->SetVerts(vertices);
    vertices->Delete();
  } // this->VertexCells

  // Clean up
  this->Points->Delete();
  this->Points = nullptr;
}

//----------------------------------------------------------------------------
void svtkHyperTreeGridCellCenters::RecursivelyProcessTree(
  svtkHyperTreeGridNonOrientedGeometryCursor* cursor)
{
  // Create cell center if cursor is at leaf
  if (cursor->IsLeaf())
  {
    // Cursor is at leaf, retrieve its global index
    svtkIdType id = cursor->GetGlobalNodeIndex();

    // If leaf is masked, skip it
    if (this->InMask && this->InMask->GetValue(id))
    {
      return;
    }

    // Retrieve cell center coordinates
    double pt[3];
    cursor->GetPoint(pt);

    // Insert next point
    svtkIdType outId = this->Points->InsertNextPoint(pt);

    // Copy cell center data from leaf data, when needed
    if (this->VertexCells)
    {
      this->OutData->CopyData(this->InData, id, outId);
    }
  }
  else
  {
    // Cursor is not at leaf, recurse to all children
    int numChildren = this->Input->GetNumberOfChildren();
    for (int child = 0; child < numChildren; ++child)
    {
      cursor->ToChild(child);
      // Recurse
      this->RecursivelyProcessTree(cursor);
      cursor->ToParent();
    } // child
  }   // else
}
