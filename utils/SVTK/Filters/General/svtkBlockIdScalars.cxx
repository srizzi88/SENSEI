/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkBlockIdScalars.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkBlockIdScalars.h"

#include "svtkCellData.h"
#include "svtkDataObjectTreeIterator.h"
#include "svtkDataSet.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkMultiBlockDataSet.h"
#include "svtkObjectFactory.h"
#include "svtkUnsignedCharArray.h"

svtkStandardNewMacro(svtkBlockIdScalars);
//----------------------------------------------------------------------------
svtkBlockIdScalars::svtkBlockIdScalars() = default;

//----------------------------------------------------------------------------
svtkBlockIdScalars::~svtkBlockIdScalars() = default;

//----------------------------------------------------------------------------
// Map ids into attribute data
int svtkBlockIdScalars::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkMultiBlockDataSet* input =
    svtkMultiBlockDataSet::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));
  if (!input)
  {
    return 0;
  }

  svtkInformation* info = outputVector->GetInformationObject(0);
  svtkMultiBlockDataSet* output =
    svtkMultiBlockDataSet::SafeDownCast(info->Get(svtkDataObject::DATA_OBJECT()));
  if (!output)
  {
    return 0;
  }

  unsigned int numBlocks = input->GetNumberOfBlocks();
  output->SetNumberOfBlocks(numBlocks);

  svtkDataObjectTreeIterator* iter = input->NewTreeIterator();
  iter->TraverseSubTreeOff();
  iter->VisitOnlyLeavesOff();

  int blockIdx = 0;
  for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem(), blockIdx++)
  {
    svtkDataObject* dObj = iter->GetCurrentDataObject();
    if (dObj)
    {
      svtkDataObject* block = this->ColorBlock(dObj, blockIdx);
      if (block)
      {
        output->SetDataSet(iter, block);
        block->Delete();
      }
    }
  }

  iter->Delete();
  return 1;
}

//----------------------------------------------------------------------------
svtkDataObject* svtkBlockIdScalars::ColorBlock(svtkDataObject* input, int group)
{
  svtkDataObject* output = nullptr;
  if (input->IsA("svtkCompositeDataSet"))
  {
    svtkCompositeDataSet* mbInput = svtkCompositeDataSet::SafeDownCast(input);

    output = input->NewInstance();
    svtkCompositeDataSet* mbOutput = svtkCompositeDataSet::SafeDownCast(output);
    mbOutput->CopyStructure(mbInput);

    svtkCompositeDataIterator* inIter = mbInput->NewIterator();
    for (inIter->InitTraversal(); !inIter->IsDoneWithTraversal(); inIter->GoToNextItem())
    {
      svtkDataObject* src = inIter->GetCurrentDataObject();
      svtkDataObject* dest = nullptr;
      if (src)
      {
        dest = this->ColorBlock(src, group);
      }
      mbOutput->SetDataSet(inIter, dest);
    }
  }
  else
  {
    svtkDataSet* ds = svtkDataSet::SafeDownCast(input);
    if (ds)
    {
      output = ds->NewInstance();
      output->ShallowCopy(ds);
      svtkDataSet* dsOutput = svtkDataSet::SafeDownCast(output);
      svtkIdType numCells = dsOutput->GetNumberOfCells();
      svtkUnsignedCharArray* cArray = svtkUnsignedCharArray::New();
      cArray->SetNumberOfTuples(numCells);
      for (svtkIdType cellIdx = 0; cellIdx < numCells; cellIdx++)
      {
        cArray->SetValue(cellIdx, group);
      }
      cArray->SetName("BlockIdScalars");
      dsOutput->GetCellData()->AddArray(cArray);
      cArray->Delete();
    }
  }
  return output;
}

//----------------------------------------------------------------------------
void svtkBlockIdScalars::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
