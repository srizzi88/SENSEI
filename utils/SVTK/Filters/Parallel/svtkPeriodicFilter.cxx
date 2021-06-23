/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPeriodicFiler.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

    This software is distributed WITHOUT ANY WARRANTY; without even
    the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
    PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkPeriodicFilter.h"

#include "svtkDataObjectTreeIterator.h"
#include "svtkDataSet.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkMultiBlockDataSet.h"
#include "svtkMultiProcessController.h"

//----------------------------------------------------------------------------
svtkPeriodicFilter::svtkPeriodicFilter()
{
  this->IterationMode = SVTK_ITERATION_MODE_MAX;
  this->NumberOfPeriods = 1;
  this->ReducePeriodNumbers = false;
}

//----------------------------------------------------------------------------
svtkPeriodicFilter::~svtkPeriodicFilter() = default;

//----------------------------------------------------------------------------
void svtkPeriodicFilter::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  if (this->IterationMode == SVTK_ITERATION_MODE_DIRECT_NB)
  {
    os << indent << "Iteration Mode: Direct Number" << endl;
    os << indent << "Number of Periods: " << this->NumberOfPeriods << endl;
  }
  else
  {
    os << indent << "Iteration Mode: Maximum" << endl;
  }
}

//----------------------------------------------------------------------------
void svtkPeriodicFilter::AddIndex(unsigned int index)
{
  this->Indices.insert(index);
  this->Modified();
}

//----------------------------------------------------------------------------
void svtkPeriodicFilter::RemoveIndex(unsigned int index)
{
  this->Indices.erase(index);
  this->Modified();
}

//----------------------------------------------------------------------------
void svtkPeriodicFilter::RemoveAllIndices()
{
  this->Indices.clear();
  this->Modified();
}

//----------------------------------------------------------------------------
int svtkPeriodicFilter::FillInputPortInformation(int svtkNotUsed(port), svtkInformation* info)
{
  // now add our info
  info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkDataObject");
  return 1;
}

//----------------------------------------------------------------------------
int svtkPeriodicFilter::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // Recover casted dataset
  svtkDataObject* inputObject = svtkDataObject::GetData(inputVector[0], 0);
  svtkDataObjectTree* input = svtkDataObjectTree::SafeDownCast(inputObject);
  svtkDataSet* dsInput = svtkDataSet::SafeDownCast(inputObject);
  svtkMultiBlockDataSet* mb = nullptr;

  svtkMultiBlockDataSet* output = svtkMultiBlockDataSet::GetData(outputVector, 0);

  if (dsInput)
  {
    mb = svtkMultiBlockDataSet::New();
    mb->SetNumberOfBlocks(1);
    mb->SetBlock(0, dsInput);
    this->AddIndex(1);
    input = mb;
  }
  else if (this->Indices.empty())
  {
    // Trivial case
    output->ShallowCopy(input);
    return 1;
  }

  this->PeriodNumbers.clear();

  output->CopyStructure(input);

  // Copy selected blocks over to the output.
  svtkDataObjectTreeIterator* iter = input->NewTreeIterator();

  // Generate leaf multipieces
  iter->VisitOnlyLeavesOn();
  iter->SkipEmptyNodesOff();
  iter->InitTraversal();
  while (!iter->IsDoneWithTraversal() && !this->Indices.empty())
  {
    const unsigned int index = iter->GetCurrentFlatIndex();
    if (this->Indices.find(index) != this->Indices.end())
    {
      this->CreatePeriodicDataSet(iter, output, input);
    }
    else
    {
      svtkDataObject* inputLeaf = input->GetDataSet(iter);
      if (inputLeaf)
      {
        svtkDataObject* newLeaf = inputLeaf->NewInstance();
        newLeaf->ShallowCopy(inputLeaf);
        output->SetDataSet(iter, newLeaf);
        newLeaf->Delete();
      }
    }
    iter->GoToNextItem();
  }

  // Reduce period number in case of parallelism, and update empty multipieces
  if (this->ReducePeriodNumbers)
  {
    int* reducedPeriodNumbers = new int[this->PeriodNumbers.size()];
    svtkMultiProcessController* controller = svtkMultiProcessController::GetGlobalController();
    if (controller)
    {
      controller->AllReduce(&this->PeriodNumbers.front(), reducedPeriodNumbers,
        static_cast<svtkIdType>(this->PeriodNumbers.size()), svtkCommunicator::MAX_OP);
      int i = 0;
      iter->InitTraversal();
      while (!iter->IsDoneWithTraversal() && !this->Indices.empty())
      {
        if (reducedPeriodNumbers[i] > this->PeriodNumbers[i])
        {
          const unsigned int index = iter->GetCurrentFlatIndex();
          if (this->Indices.find(index) != this->Indices.end())
          {
            this->SetPeriodNumber(iter, output, reducedPeriodNumbers[i]);
          }
        }
        iter->GoToNextItem();
        i++;
      }
    }
    delete[] reducedPeriodNumbers;
  }
  iter->Delete();

  if (mb != nullptr)
  {
    mb->Delete();
  }
  return 1;
}
