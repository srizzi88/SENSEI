/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkHyperTreeGridThreshold.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkHyperTreeGridThreshold.h"

#include "svtkBitArray.h"
#include "svtkCellData.h"
#include "svtkHyperTree.h"
#include "svtkHyperTreeGrid.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkMath.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkUniformHyperTreeGrid.h"

#include "svtkHyperTreeGridNonOrientedCursor.h"

#include <cmath>

svtkStandardNewMacro(svtkHyperTreeGridThreshold);

//-----------------------------------------------------------------------------
svtkHyperTreeGridThreshold::svtkHyperTreeGridThreshold()
{
  // Use minimum double value by default for lower threshold bound
  this->LowerThreshold = std::numeric_limits<double>::min();

  // Use maximum double value by default for upper threshold bound
  this->UpperThreshold = std::numeric_limits<double>::max();

  // This filter always creates an output with a material mask
  // JBL Ce n'est que dans de tres rares cas que le mask produit par le
  // JBL threshold, que ce soit avec ou sans creation d'un nouveau maillage,
  // JBL ne contienne que des valeurs a false. Ce n'est que dans ces
  // JBL tres rares cas que la creation d'un mask n'aurait pas d'utilite.
  this->OutMask = svtkBitArray::New();

  // Output indices begin at 0
  this->CurrentId = 0;

  // Process active point scalars by default
  this->SetInputArrayToProcess(
    0, 0, 0, svtkDataObject::FIELD_ASSOCIATION_POINTS_THEN_CELLS, svtkDataSetAttributes::SCALARS);

  // Input scalars point to null by default
  this->InScalars = nullptr;

  // By default, just create a new mask
  this->JustCreateNewMask = true;

  // JB Pour sortir un maillage de meme type que celui en entree, si create
  this->AppropriateOutput = true;
}

//-----------------------------------------------------------------------------
svtkHyperTreeGridThreshold::~svtkHyperTreeGridThreshold()
{
  this->OutMask->Delete();
  this->OutMask = nullptr;
}

//----------------------------------------------------------------------------
void svtkHyperTreeGridThreshold::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "LowerThreshold: " << this->LowerThreshold << endl;
  os << indent << "UpperThreshold: " << this->UpperThreshold << endl;
  os << indent << "OutMask: " << this->OutMask << endl;
  os << indent << "CurrentId: " << this->CurrentId << endl;

  if (this->InScalars)
  {
    os << indent << "InScalars:\n";
    this->InScalars->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << indent << "InScalars: (none)\n";
  }
}

//----------------------------------------------------------------------------
int svtkHyperTreeGridThreshold::FillOutputPortInformation(int, svtkInformation* info)
{
  info->Set(svtkDataObject::DATA_TYPE_NAME(), "svtkHyperTreeGrid");
  return 1;
}

//----------------------------------------------------------------------------
void svtkHyperTreeGridThreshold::ThresholdBetween(double minimum, double maximum)
{
  this->LowerThreshold = minimum;
  this->UpperThreshold = maximum;
  this->Modified();
}

//-----------------------------------------------------------------------------
int svtkHyperTreeGridThreshold::ProcessTrees(svtkHyperTreeGrid* input, svtkDataObject* outputDO)
{
  // Downcast output data object to hyper tree grid
  svtkHyperTreeGrid* output = svtkHyperTreeGrid::SafeDownCast(outputDO);
  if (!output)
  {
    svtkErrorMacro("Incorrect type of output: " << outputDO->GetClassName());
    return 0;
  }

  // Retrieve scalar quantity of interest
  this->InScalars = this->GetInputArrayToProcess(0, input);
  if (!this->InScalars)
  {
    svtkWarningMacro(<< "No scalar data to threshold");
    return 1;
  }

  // JBL Pour les cas extremes ou le filtre est insere dans une chaine
  // JBL de traitement, on pourrait ajouter ici un controle optionnel
  // JBL afin de voir entre le datarange de inscalars et
  // JBL l'interval [LowerThreshold, UpperThreshold] il y a :
  // JBL - un total recouvrement, alors output est le input
  // JBL - pasde recouvrement, alors output est un maillage vide.

  // Retrieve material mask
  this->InMask = input->HasMask() ? input->GetMask() : nullptr;

  if (this->JustCreateNewMask)
  {
    output->ShallowCopy(input);

    this->OutMask->SetNumberOfTuples(output->GetNumberOfVertices());

    // Iterate over all input and output hyper trees
    svtkIdType outIndex;
    svtkHyperTreeGrid::svtkHyperTreeGridIterator it;
    output->InitializeTreeIterator(it);
    svtkNew<svtkHyperTreeGridNonOrientedCursor> outCursor;
    while (it.GetNextTree(outIndex))
    {
      // Initialize new grid cursor at root of current input tree
      output->InitializeNonOrientedCursor(outCursor, outIndex);
      // Limit depth recursively
      this->RecursivelyProcessTreeWithCreateNewMask(outCursor);
    } // it
  }
  else
  {
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

    // Iterate over all input and output hyper trees
    svtkIdType inIndex;
    svtkHyperTreeGrid::svtkHyperTreeGridIterator it;
    input->InitializeTreeIterator(it);
    svtkNew<svtkHyperTreeGridNonOrientedCursor> inCursor;
    svtkNew<svtkHyperTreeGridNonOrientedCursor> outCursor;
    while (it.GetNextTree(inIndex))
    {
      // Initialize new cursor at root of current input tree
      input->InitializeNonOrientedCursor(inCursor, inIndex);
      // Initialize new cursor at root of current output tree
      output->InitializeNonOrientedCursor(outCursor, inIndex, true);
      // Limit depth recursively
      this->RecursivelyProcessTree(inCursor, outCursor);
    } // it
  }

  // Squeeze and set output material mask if necessary
  this->OutMask->Squeeze();
  output->SetMask(this->OutMask);

  this->UpdateProgress(1.);
  return 1;
}

//-----------------------------------------------------------------------------
bool svtkHyperTreeGridThreshold::RecursivelyProcessTree(
  svtkHyperTreeGridNonOrientedCursor* inCursor, svtkHyperTreeGridNonOrientedCursor* outCursor)
{
  // Retrieve global index of input cursor
  svtkIdType inId = inCursor->GetGlobalNodeIndex();

  // Increase index count on output: postfix is intended
  svtkIdType outId = this->CurrentId++;

  // Copy out cell data from that of input cell
  this->OutData->CopyData(this->InData, inId, outId);

  // Retrieve output tree and set global index of output cursor
  svtkHyperTree* outTree = outCursor->GetTree();
  outTree->SetGlobalIndexFromLocal(outCursor->GetVertexId(), outId);

  // Flag to recursively decide whether a tree node should discarded
  bool discard = true;

  if (this->InMask && this->InMask->GetValue(inId))
  {
    // Mask output cell if necessary
    this->OutMask->InsertTuple1(outId, discard);

    // Return whether current node is within range
    return discard;
  }

  // Descend further into input trees only if cursor is not at leaf
  if (!inCursor->IsLeaf())
  {
    // Cursor is not at leaf, subdivide output tree one level further
    outCursor->SubdivideLeaf();

    // If input cursor is neither at leaf nor at maximum depth, recurse to all children
    int numChildren = inCursor->GetNumberOfChildren();
    for (int ichild = 0; ichild < numChildren; ++ichild)
    {
      // Descend into child in intput grid as well
      inCursor->ToChild(ichild);
      // Descend into child in output grid as well
      outCursor->ToChild(ichild);
      // Recurse and keep track of whether some children are kept
      discard &= this->RecursivelyProcessTree(inCursor, outCursor);
      // Return to parent in input grid
      outCursor->ToParent();
      // Return to parent in output grid
      inCursor->ToParent();
    } // child
  }   // if (! inCursor->IsLeaf() && inCursor->GetCurrentDepth() < this->Depth)
  else
  {
    // Input cursor is at leaf, check whether it is within range
    double value = this->InScalars->GetTuple1(inId);
    if (!(this->InMask && this->InMask->GetValue(inId)) && value >= this->LowerThreshold &&
      value <= this->UpperThreshold)
    {
      // Cell is not masked and is within range, keep it
      discard = false;
    }
  } // else

  // Mask output cell if necessary
  this->OutMask->InsertTuple1(outId, discard);

  // Return whether current node is within range
  return discard;
}

//-----------------------------------------------------------------------------
bool svtkHyperTreeGridThreshold::RecursivelyProcessTreeWithCreateNewMask(
  svtkHyperTreeGridNonOrientedCursor* outCursor)
{
  // Retrieve global index of input cursor
  svtkIdType outId = outCursor->GetGlobalNodeIndex();

  // Flag to recursively decide whether a tree node should discarded
  bool discard = true;

  if (this->InMask && this->InMask->GetValue(outId))
  {
    // Mask output cell if necessary
    this->OutMask->InsertTuple1(outId, discard);

    // Return whether current node is within range
    return discard;
  }

  // Descend further into input trees only if cursor is not at leaf
  if (!outCursor->IsLeaf())
  {
    // If input cursor is neither at leaf nor at maximum depth, recurse to all children
    int numChildren = outCursor->GetNumberOfChildren();
    for (int ichild = 0; ichild < numChildren; ++ichild)
    {
      // Descend into child in output grid as well
      outCursor->ToChild(ichild);
      // Recurse and keep track of whether some children are kept
      discard &= this->RecursivelyProcessTreeWithCreateNewMask(outCursor);
      // Return to parent in output grid
      outCursor->ToParent();
    } // child
  }   // if (! inCursor->IsLeaf() && inCursor->GetCurrentDepth() < this->Depth)
  else
  {
    // Input cursor is at leaf, check whether it is within range
    double value = this->InScalars->GetTuple1(outId);
    discard = value < this->LowerThreshold || value > this->UpperThreshold;
  } // else

  // Mask output cell if necessary
  this->OutMask->InsertTuple1(outId, discard);

  // Return whether current node is within range
  return discard;
}
