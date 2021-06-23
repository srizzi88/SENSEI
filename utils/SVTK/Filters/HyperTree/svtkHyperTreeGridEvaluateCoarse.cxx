/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkHyperTreeGridEvaluateCoarse.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkHyperTreeGridEvaluateCoarse.h"
#include "svtkBitArray.h"
#include "svtkHyperTree.h"
#include "svtkHyperTreeGrid.h"
#include "svtkInformation.h"
#include "svtkNew.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"

#include "svtkUniformHyperTreeGrid.h"

#include "svtkHyperTreeGridNonOrientedCursor.h"

#include <cmath>

svtkStandardNewMacro(svtkHyperTreeGridEvaluateCoarse);

//-----------------------------------------------------------------------------
svtkHyperTreeGridEvaluateCoarse::svtkHyperTreeGridEvaluateCoarse()
{
  this->Operator = svtkHyperTreeGridEvaluateCoarse::OPERATOR_DON_T_CHANGE;
  this->Mask = nullptr;

  this->Default = 0.;

  this->BranchFactor = 0;
  this->Dimension = 0;
  this->SplattingFactor = 1;

  // In order to output a mesh of the same type as that given as input
  this->AppropriateOutput = true;
}

//-----------------------------------------------------------------------------
svtkHyperTreeGridEvaluateCoarse::~svtkHyperTreeGridEvaluateCoarse() {}

//----------------------------------------------------------------------------
void svtkHyperTreeGridEvaluateCoarse::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
int svtkHyperTreeGridEvaluateCoarse::FillOutputPortInformation(int, svtkInformation* info)
{
  info->Set(svtkDataObject::DATA_TYPE_NAME(), "svtkHyperTreeGrid");
  return 1;
}

//----------------------------------------------------------------------------
int svtkHyperTreeGridEvaluateCoarse::ProcessTrees(svtkHyperTreeGrid* input, svtkDataObject* outputDO)
{
  // Downcast output data object to hypertree grid
  svtkHyperTreeGrid* output = svtkHyperTreeGrid::SafeDownCast(outputDO);
  if (!output)
  {
    svtkErrorMacro("Incorrect type of output: " << outputDO->GetClassName());
    return 0;
  }

  output->ShallowCopy(input);

  if (this->Operator == svtkHyperTreeGridEvaluateCoarse::OPERATOR_DON_T_CHANGE_FAST)
  {
    return 1;
  }

  this->Mask = output->HasMask() ? output->GetMask() : nullptr;

  this->BranchFactor = output->GetBranchFactor();
  this->Dimension = output->GetDimension();
  this->SplattingFactor = std::pow(this->BranchFactor, this->Dimension - 1);
  this->NumberOfChildren = output->GetNumberOfChildren();

  this->NbChilds = input->GetNumberOfChildren();
  this->InData = input->GetPointData();
  this->OutData = output->GetPointData();
  this->OutData->CopyAllocate(this->InData);
  // Iterate over all input and output hyper trees
  svtkIdType index;
  svtkHyperTreeGrid::svtkHyperTreeGridIterator in;
  output->InitializeTreeIterator(in);
  svtkNew<svtkHyperTreeGridNonOrientedCursor> outCursor;
  while (in.GetNextTree(index))
  {
    // Initialize new cursor at root of current output tree
    output->InitializeNonOrientedCursor(outCursor, index);
    // Recursively
    this->ProcessNode(outCursor);
    // Clean up
  }
  this->UpdateProgress(1.);
  return 1;
}

//----------------------------------------------------------------------------
void svtkHyperTreeGridEvaluateCoarse::ProcessNode(svtkHyperTreeGridNonOrientedCursor* outCursor)
{
  svtkIdType id = outCursor->GetGlobalNodeIndex();
  if (outCursor->IsLeaf())
  {
    this->OutData->CopyData(this->InData, id, id);
    return;
  }
  // If not operation
  if (this->Operator == svtkHyperTreeGridEvaluateCoarse::OPERATOR_DON_T_CHANGE)
  {
    this->OutData->CopyData(this->InData, id, id);
    // Coarse
    for (int ichild = 0; ichild < this->NbChilds; ++ichild)
    {
      outCursor->ToChild(ichild);
      // We go through the children's cells
      ProcessNode(outCursor);
      outCursor->ToParent();
    }
    return;
  }
  //
  int nbArray = this->InData->GetNumberOfArrays();
  //
  std::vector<std::vector<std::vector<double> > > values(nbArray);
  // Coarse
  for (int ichild = 0; ichild < this->NbChilds; ++ichild)
  {
    outCursor->ToChild(ichild);
    // Iterate children
    ProcessNode(outCursor);
    // Memorize children values
    svtkIdType idChild = outCursor->GetGlobalNodeIndex();
    for (int i = 0; i < nbArray; ++i)
    {
      svtkDataArray* arr = this->OutData->GetArray(i);
      int nbC = arr->GetNumberOfComponents();
      values[i].resize(nbC);
      if (!this->Mask || !this->Mask->GetTuple1(idChild))
      {
        double* tmp = arr->GetTuple(idChild);
        for (int iC = 0; iC < nbC; ++iC)
        {
          values[i][iC].push_back(tmp[iC]);
        }
      }
    }
    outCursor->ToParent();
  }
  // Reduction operation
  for (int i = 0; i < nbArray; ++i)
  {
    svtkDataArray* arr = this->OutData->GetArray(i);
    int nbC = arr->GetNumberOfComponents();
    for (int iC = 0; iC < nbC; ++iC)
    {
      arr->SetComponent(id, iC, EvalCoarse(values[i][iC]));
    }
    values[i].clear();
  }
}

//----------------------------------------------------------------------------
double svtkHyperTreeGridEvaluateCoarse::EvalCoarse(const std::vector<double>& array)
{
  switch (this->Operator)
  {
    case svtkHyperTreeGridEvaluateCoarse::OPERATOR_ELDER_CHILD:
    {
      return this->ElderChild(array);
    }
    case svtkHyperTreeGridEvaluateCoarse::OPERATOR_MIN:
    {
      return this->Min(array);
    }
    case svtkHyperTreeGridEvaluateCoarse::OPERATOR_MAX:
    {
      return this->Max(array);
    }
    case svtkHyperTreeGridEvaluateCoarse::OPERATOR_SUM:
    {
      return this->Sum(array);
    }
    case svtkHyperTreeGridEvaluateCoarse::OPERATOR_AVERAGE:
    {
      return this->Average(array);
    }
    case svtkHyperTreeGridEvaluateCoarse::OPERATOR_UNMASKED_AVERAGE:
    {
      return this->UnmaskedAverage(array);
    }
    case svtkHyperTreeGridEvaluateCoarse::OPERATOR_SPLATTING_AVERAGE:
    {
      return this->SplattingAverage(array);
    }
    default:
    {
      break;
    }
  }
  return NAN;
}

//----------------------------------------------------------------------------
double svtkHyperTreeGridEvaluateCoarse::Min(const std::vector<double>& array)
{
  if (array.size() == 0)
  {
    return NAN;
  }
  double val = array[0];
  for (std::vector<double>::const_iterator it = array.begin() + 1; it != array.end(); ++it)
  {
    if (*it < val)
    {
      val = *it;
    }
  }
  return val;
}

//----------------------------------------------------------------------------
double svtkHyperTreeGridEvaluateCoarse::Max(const std::vector<double>& array)
{
  if (array.size() == 0)
  {
    return NAN;
  }
  double val = array[0];
  for (std::vector<double>::const_iterator it = array.begin() + 1; it != array.end(); ++it)
  {
    if (*it > val)
    {
      val = *it;
    }
  }
  return val;
}

//----------------------------------------------------------------------------
double svtkHyperTreeGridEvaluateCoarse::Sum(const std::vector<double>& array)
{
  double val = array[0];
  for (std::vector<double>::const_iterator it = array.begin() + 1; it != array.end(); ++it)
  {
    val += *it;
  }
  return val;
}

//----------------------------------------------------------------------------
double svtkHyperTreeGridEvaluateCoarse::Average(const std::vector<double>& array)
{
  if (array.size() == 0)
  {
    return this->Default;
  }
  double sum = Sum(array);
  if (this->Default != 0.)
  {
    sum += this->Default * (this->NumberOfChildren - array.size());
  }
  return sum / this->NumberOfChildren;
}

//----------------------------------------------------------------------------
double svtkHyperTreeGridEvaluateCoarse::UnmaskedAverage(const std::vector<double>& array)
{
  if (array.size() == 0)
  {
    return NAN;
  }
  return Sum(array) / array.size();
}

//----------------------------------------------------------------------------
double svtkHyperTreeGridEvaluateCoarse::ElderChild(const std::vector<double>& array)
{
  if (array.size() == 0)
  {
    return NAN;
  }
  return array[0];
}

//----------------------------------------------------------------------------
double svtkHyperTreeGridEvaluateCoarse::SplattingAverage(const std::vector<double>& array)
{
  if (array.size() == 0)
  {
    return this->Default;
  }
  double sum = Sum(array);
  if (this->Default != 0.)
  {
    sum += this->Default * (this->NumberOfChildren - array.size());
  }
  return sum / this->SplattingFactor;
}
