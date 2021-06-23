/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPMergeArrays.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkPMergeArrays.h"
#include "svtkDataObject.h"
#include "svtkMultiProcessController.h"
#include "svtkObjectFactory.h"

svtkStandardNewMacro(svtkPMergeArrays);

//----------------------------------------------------------------------------
svtkPMergeArrays::svtkPMergeArrays() {}

//----------------------------------------------------------------------------
void svtkPMergeArrays::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
int svtkPMergeArrays::MergeDataObjectFields(svtkDataObject* input, int idx, svtkDataObject* output)
{
  int checks[svtkDataObject::NUMBER_OF_ATTRIBUTE_TYPES];
  for (int attr = 0; attr < svtkDataObject::NUMBER_OF_ATTRIBUTE_TYPES; attr++)
  {
    checks[attr] = output->GetNumberOfElements(attr) == input->GetNumberOfElements(attr) ? 0 : 1;
  }
  int globalChecks[svtkDataObject::NUMBER_OF_ATTRIBUTE_TYPES];
  auto controller = svtkMultiProcessController::GetGlobalController();
  if (controller == nullptr)
  {
    for (int i = 0; i < svtkDataObject::NUMBER_OF_ATTRIBUTE_TYPES; ++i)
    {
      globalChecks[i] = checks[i];
    }
  }
  else
  {
    controller->AllReduce(
      checks, globalChecks, svtkDataObject::NUMBER_OF_ATTRIBUTE_TYPES, svtkCommunicator::MAX_OP);
  }

  for (int attr = 0; attr < svtkDataObject::NUMBER_OF_ATTRIBUTE_TYPES; attr++)
  {
    if (globalChecks[attr] == 0)
    {
      // only merge arrays when the number of elements in the input and output are the same
      this->MergeArrays(
        idx, input->GetAttributesAsFieldData(attr), output->GetAttributesAsFieldData(attr));
    }
  }

  return 1;
}
