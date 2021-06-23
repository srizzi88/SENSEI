/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPassThrough.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkPassThrough.h"

#include "svtkDataObject.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkObjectFactory.h"
#include "svtkPolyData.h"

svtkStandardNewMacro(svtkPassThrough);

//----------------------------------------------------------------------------
svtkPassThrough::svtkPassThrough()
  : DeepCopyInput(0)
  , AllowNullInput(false)
{
}

//----------------------------------------------------------------------------
svtkPassThrough::~svtkPassThrough() = default;

//----------------------------------------------------------------------------
int svtkPassThrough::RequestDataObject(
  svtkInformation* request, svtkInformationVector** inVec, svtkInformationVector* outVec)
{
  if (this->AllowNullInput && this->GetNumberOfInputPorts() != 0 &&
    inVec[0]->GetInformationObject(0) == nullptr)
  {
    for (int i = 0; i < this->GetNumberOfOutputPorts(); ++i)
    {
      svtkPolyData* obj = svtkPolyData::New();
      outVec->GetInformationObject(i)->Set(svtkDataObject::DATA_OBJECT(), obj);
      obj->FastDelete();
    }
    return 1;
  }
  else
  {
    return this->Superclass::RequestDataObject(request, inVec, outVec);
  }
}

//----------------------------------------------------------------------------
void svtkPassThrough::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "DeepCopyInput: " << (this->DeepCopyInput ? "on" : "off") << endl
     << indent << "AllowNullInput: " << (this->AllowNullInput ? "on" : "off") << endl;
}

//----------------------------------------------------------------------------
int svtkPassThrough::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  if (!inInfo)
  {
    return this->AllowNullInput ? 1 : 0;
  }

  svtkDataObject* input = inInfo->Get(svtkDataObject::DATA_OBJECT());
  svtkDataObject* output = outInfo->Get(svtkDataObject::DATA_OBJECT());
  if (this->DeepCopyInput)
  {
    output->DeepCopy(input);
  }
  else
  {
    output->ShallowCopy(input);
  }

  return 1;
}

//----------------------------------------------------------------------------
int svtkPassThrough::FillInputPortInformation(int port, svtkInformation* info)
{
  if (port == 0)
  {
    info->Set(svtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
    return 1;
  }
  return 0;
}
