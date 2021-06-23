/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkNonOverlappingAMRLevelIdScalars.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkNonOverlappingAMRLevelIdScalars.h"

#include "svtkCellData.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkNonOverlappingAMR.h"
#include "svtkObjectFactory.h"
#include "svtkUniformGrid.h"
#include "svtkUniformGridAMR.h"
#include "svtkUnsignedCharArray.h"

#include <cassert>

svtkStandardNewMacro(svtkNonOverlappingAMRLevelIdScalars);
//----------------------------------------------------------------------------
svtkNonOverlappingAMRLevelIdScalars::svtkNonOverlappingAMRLevelIdScalars() {}

//----------------------------------------------------------------------------
svtkNonOverlappingAMRLevelIdScalars::~svtkNonOverlappingAMRLevelIdScalars() {}

//----------------------------------------------------------------------------
void svtkNonOverlappingAMRLevelIdScalars::AddColorLevels(
  svtkUniformGridAMR* input, svtkUniformGridAMR* output)
{
  assert("pre: input should not be nullptr" && (input != nullptr));
  assert("pre: output should not be nullptr" && (output != nullptr));

  unsigned int numLevels = input->GetNumberOfLevels();
  output->SetNumberOfLevels(numLevels);

  for (unsigned int levelIdx = 0; levelIdx < numLevels; levelIdx++)
  {
    unsigned int numDS = input->GetNumberOfDataSets(levelIdx);
    output->SetNumberOfDataSets(levelIdx, numDS);

    // Copy level metadata.
    if (input->HasLevelMetaData(levelIdx))
    {
      output->GetLevelMetaData(levelIdx)->Copy(input->GetLevelMetaData(levelIdx));
    }

    for (unsigned int cc = 0; cc < numDS; cc++)
    {
      svtkUniformGrid* ds = input->GetDataSet(levelIdx, cc);
      if (ds != nullptr)
      {
        svtkUniformGrid* copy = this->ColorLevel(ds, levelIdx);
        output->SetDataSet(levelIdx, cc, copy);
        copy->Delete();
      }

      // Copy meta data for each dataset within a level.
      if (input->HasMetaData(levelIdx, cc))
      {
        output->GetMetaData(levelIdx, cc)->Copy(input->GetMetaData(levelIdx, cc));
      }
    }
  }
}

//----------------------------------------------------------------------------
// Map ids into attribute data
int svtkNonOverlappingAMRLevelIdScalars::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkUniformGridAMR* input =
    svtkUniformGridAMR::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));
  if (!input)
  {
    return 0;
  }

  svtkInformation* info = outputVector->GetInformationObject(0);
  svtkUniformGridAMR* output =
    svtkUniformGridAMR::SafeDownCast(info->Get(svtkDataObject::DATA_OBJECT()));
  if (!output)
  {
    return 0;
  }

  this->AddColorLevels(input, output);

  return 1;
}

//----------------------------------------------------------------------------
svtkUniformGrid* svtkNonOverlappingAMRLevelIdScalars::ColorLevel(svtkUniformGrid* input, int group)
{
  svtkUniformGrid* output = 0;
  output = input->NewInstance();
  output->ShallowCopy(input);
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
  return output;
}

//----------------------------------------------------------------------------
void svtkNonOverlappingAMRLevelIdScalars::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
