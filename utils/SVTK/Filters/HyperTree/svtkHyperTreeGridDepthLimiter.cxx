/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkHyperTreeGridDepthLimiter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkHyperTreeGridDepthLimiter.h"

#include "svtkBitArray.h"
#include "svtkDoubleArray.h"
#include "svtkHyperTree.h"
#include "svtkHyperTreeGrid.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkUniformHyperTreeGrid.h"
#include "svtkUnstructuredGrid.h"

#include "svtkHyperTreeGridNonOrientedCursor.h"

svtkStandardNewMacro(svtkHyperTreeGridDepthLimiter);

//-----------------------------------------------------------------------------
svtkHyperTreeGridDepthLimiter::svtkHyperTreeGridDepthLimiter()
{
  // Require root-level depth by default
  this->Depth = 0;

  // Default mask is emplty
  this->OutMask = nullptr;

  // Output indices begin at 0
  this->CurrentId = 0;

  // By default, just create a new mask
  this->JustCreateNewMask = true;

  // JB Pour sortir un maillage de meme type que celui en entree, si create
  this->AppropriateOutput = true;
}

//-----------------------------------------------------------------------------
svtkHyperTreeGridDepthLimiter::~svtkHyperTreeGridDepthLimiter()
{
  if (this->OutMask)
  {
    this->OutMask->Delete();
    this->OutMask = nullptr;
  }
}

//----------------------------------------------------------------------------
void svtkHyperTreeGridDepthLimiter::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Depth: " << this->Depth << endl;
  os << indent << "OutMask: " << this->OutMask << endl;
  os << indent << "CurrentId: " << this->CurrentId << endl;
}

//----------------------------------------------------------------------------
int svtkHyperTreeGridDepthLimiter::FillOutputPortInformation(int, svtkInformation* info)
{
  info->Set(svtkDataObject::DATA_TYPE_NAME(), "svtkHyperTreeGrid");
  return 1;
}

//-----------------------------------------------------------------------------
int svtkHyperTreeGridDepthLimiter::ProcessTrees(svtkHyperTreeGrid* input, svtkDataObject* outputDO)
{
  // Downcast output data object to hyper tree grid
  svtkHyperTreeGrid* output = svtkHyperTreeGrid::SafeDownCast(outputDO);
  if (!output)
  {
    svtkErrorMacro("Incorrect type of output: " << outputDO->GetClassName());
    return 0;
  }

  if (this->JustCreateNewMask)
  {
    output->ShallowCopy(input);
    output->SetDepthLimiter(this->Depth);
    return 1;
  }

  // Retrieve material mask
  this->InMask = input->HasMask() ? input->GetMask() : nullptr;

  // Set grid parameters
  output->SetDimensions(input->GetDimensions());
  output->SetTransposedRootIndexing(input->GetTransposedRootIndexing());
  output->SetBranchFactor(input->GetBranchFactor());
  output->CopyCoordinates(input);
  output->SetHasInterface(input->GetHasInterface());
  output->SetInterfaceNormalsName(input->GetInterfaceNormalsName());
  output->SetInterfaceInterceptsName(input->GetInterfaceInterceptsName());

  // Initialize output point data
  this->InData = input->GetPointData();
  this->OutData = output->GetPointData();
  this->OutData->CopyAllocate(this->InData);

  // Output indices begin at 0
  this->CurrentId = 0;

  // Create material mask bit array if one is present on input
  if (!this->OutMask && input->HasMask())
  {
    this->OutMask = svtkBitArray::New();
  }

  // Output indices begin at 0
  this->CurrentId = 0;

  // Iterate over all input and output hyper trees
  svtkIdType inIndex;
  svtkHyperTreeGrid::svtkHyperTreeGridIterator it;
  input->InitializeTreeIterator(it);
  svtkNew<svtkHyperTreeGridNonOrientedCursor> inCursor;
  svtkNew<svtkHyperTreeGridNonOrientedCursor> outCursor;
  while (it.GetNextTree(inIndex))
  {
    // Initialize new grid cursor at root of current input tree
    input->InitializeNonOrientedCursor(inCursor, inIndex);

    // Initialize new cursor at root of current output tree
    output->InitializeNonOrientedCursor(outCursor, inIndex, true);

    // Limit depth recursively
    this->RecursivelyProcessTree(inCursor, outCursor);
  } // it

  // Squeeze and set output material mask if necessary
  if (this->OutMask)
  {
    this->OutMask->Squeeze();
    output->SetMask(this->OutMask);
  }

  return 1;
}

//-----------------------------------------------------------------------------
void svtkHyperTreeGridDepthLimiter::RecursivelyProcessTree(
  svtkHyperTreeGridNonOrientedCursor* inCursor, svtkHyperTreeGridNonOrientedCursor* outCursor)
{
  // Retrieve global index of input cursor
  svtkIdType inId = inCursor->GetGlobalNodeIndex();

  // Increase index count on output: postfix is intended
  svtkIdType outId = this->CurrentId++;

  // Retrieve output tree and set global index of output cursor
  svtkHyperTree* outTree = outCursor->GetTree();
  outTree->SetGlobalIndexFromLocal(outCursor->GetVertexId(), outId);

  // Update material mask if relevant
  if (this->InMask)
  {
    // Check whether non-leaf at maximum depth is reached
    if (inCursor->GetLevel() == this->Depth && !inCursor->IsLeaf())
    {
      // If yes, then it becomes an output leaf that must be visible
      this->OutMask->InsertValue(outId, false);
    }
    else
    {
      // Otherwise, use input mask value
      this->OutMask->InsertValue(outId, this->InMask->GetValue(inId));
    }
  } // if ( this->InMask )

  // Copy output cell data from that of input cell
  this->OutData->CopyData(this->InData, inId, outId);

  // Descend further into input trees only if cursor is not at leaf and depth not reached
  if (!inCursor->IsLeaf() && inCursor->GetLevel() < this->Depth)
  {
    // Cursor is not at leaf, subdivide output tree one level further
    outCursor->SubdivideLeaf();

    // If input cursor is neither at leaf nor at maximum depth, recurse to all children
    int numChildren = inCursor->GetNumberOfChildren();
    for (int child = 0; child < numChildren; ++child)
    {
      inCursor->ToChild(child);
      // Descend into child in output grid as well
      outCursor->ToChild(child);
      // Recurse
      this->RecursivelyProcessTree(inCursor, outCursor);
      // Return to parent in output grid
      inCursor->ToParent();
      outCursor->ToParent();
    } // child
  }   // if ( ! inCursor->IsLeaf() && inCursor->GetCurrentDepth() < this->Depth )
}
