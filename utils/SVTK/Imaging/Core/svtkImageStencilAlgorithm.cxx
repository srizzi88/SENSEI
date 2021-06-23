/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImageStencilAlgorithm.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkImageStencilAlgorithm.h"

#include "svtkImageStencilData.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkObjectFactory.h"
#include "svtkStreamingDemandDrivenPipeline.h"

svtkStandardNewMacro(svtkImageStencilAlgorithm);

//----------------------------------------------------------------------------
svtkImageStencilAlgorithm::svtkImageStencilAlgorithm()
{
  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(1);

  svtkImageStencilData* output = svtkImageStencilData::New();
  this->GetExecutive()->SetOutputData(0, output);

  // Releasing data for pipeline parallism.
  // Filters will know it is empty.
  output->ReleaseData();
  output->Delete();
}

//----------------------------------------------------------------------------
svtkImageStencilAlgorithm::~svtkImageStencilAlgorithm() = default;

//----------------------------------------------------------------------------
void svtkImageStencilAlgorithm::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void svtkImageStencilAlgorithm::SetOutput(svtkImageStencilData* output)
{
  this->GetExecutive()->SetOutputData(0, output);
}

//----------------------------------------------------------------------------
svtkImageStencilData* svtkImageStencilAlgorithm::GetOutput()
{
  if (this->GetNumberOfOutputPorts() < 1)
  {
    return nullptr;
  }

  return svtkImageStencilData::SafeDownCast(this->GetExecutive()->GetOutputData(0));
}

//----------------------------------------------------------------------------
svtkImageStencilData* svtkImageStencilAlgorithm::AllocateOutputData(svtkDataObject* out, int* uExt)
{
  svtkImageStencilData* res = svtkImageStencilData::SafeDownCast(out);
  if (!res)
  {
    svtkWarningMacro("Call to AllocateOutputData with non svtkImageStencilData"
                    " output");
    return nullptr;
  }
  res->SetExtent(uExt);
  res->AllocateExtents();

  return res;
}

//----------------------------------------------------------------------------
int svtkImageStencilAlgorithm::RequestData(
  svtkInformation*, svtkInformationVector**, svtkInformationVector* outputVector)
{
  svtkInformation* outInfo = outputVector->GetInformationObject(0);
  svtkDataObject* out = outInfo->Get(svtkDataObject::DATA_OBJECT());
  this->AllocateOutputData(out, outInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_EXTENT()));

  return 1;
}

//----------------------------------------------------------------------------
int svtkImageStencilAlgorithm::RequestInformation(
  svtkInformation*, svtkInformationVector**, svtkInformationVector*)
{
  return 1;
}

//----------------------------------------------------------------------------
int svtkImageStencilAlgorithm::RequestUpdateExtent(
  svtkInformation*, svtkInformationVector**, svtkInformationVector*)
{
  return 1;
}

//----------------------------------------------------------------------------
int svtkImageStencilAlgorithm::FillOutputPortInformation(int, svtkInformation* info)
{
  info->Set(svtkDataObject::DATA_TYPE_NAME(), "svtkImageStencilData");
  return 1;
}

//----------------------------------------------------------------------------
svtkTypeBool svtkImageStencilAlgorithm::ProcessRequest(
  svtkInformation* request, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // generate the data
  if (request->Has(svtkDemandDrivenPipeline::REQUEST_DATA()))
  {
    this->RequestData(request, inputVector, outputVector);
    return 1;
  }

  // execute information
  if (request->Has(svtkDemandDrivenPipeline::REQUEST_INFORMATION()))
  {
    this->RequestInformation(request, inputVector, outputVector);
    return 1;
  }

  if (request->Has(svtkStreamingDemandDrivenPipeline::REQUEST_UPDATE_EXTENT()))
  {
    this->RequestUpdateExtent(request, inputVector, outputVector);
    return 1;
  }

  return this->Superclass::ProcessRequest(request, inputVector, outputVector);
}
