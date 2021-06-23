/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkInformationIntegerRequestKey.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkInformationIntegerRequestKey.h"

#include "svtkInformation.h"
#include "svtkStreamingDemandDrivenPipeline.h"

//----------------------------------------------------------------------------
svtkInformationIntegerRequestKey::svtkInformationIntegerRequestKey(
  const char* name, const char* location)
  : svtkInformationIntegerKey(name, location)
{
  this->DataKey = nullptr;
}

//----------------------------------------------------------------------------
svtkInformationIntegerRequestKey::~svtkInformationIntegerRequestKey() = default;

//----------------------------------------------------------------------------
void svtkInformationIntegerRequestKey::CopyDefaultInformation(
  svtkInformation* request, svtkInformation* fromInfo, svtkInformation* toInfo)
{
  if (request->Has(svtkStreamingDemandDrivenPipeline::REQUEST_UPDATE_EXTENT()))
  {
    this->ShallowCopy(fromInfo, toInfo);
  }
}

//----------------------------------------------------------------------------
bool svtkInformationIntegerRequestKey::NeedToExecute(
  svtkInformation* pipelineInfo, svtkInformation* dobjInfo)
{
  if (!dobjInfo->Has(this->DataKey) || dobjInfo->Get(this->DataKey) != pipelineInfo->Get(this))
  {
    return true;
  }
  return false;
}

//----------------------------------------------------------------------------
void svtkInformationIntegerRequestKey::StoreMetaData(
  svtkInformation*, svtkInformation* pipelineInfo, svtkInformation* dobjInfo)
{
  dobjInfo->Set(this->DataKey, pipelineInfo->Get(this));
}

//----------------------------------------------------------------------------
void svtkInformationIntegerRequestKey::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
