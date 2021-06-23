/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkInformationDataObjectMetaDataKey.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkInformationDataObjectMetaDataKey.h"

#include "svtkInformation.h"
#include "svtkStreamingDemandDrivenPipeline.h"

//----------------------------------------------------------------------------
svtkInformationDataObjectMetaDataKey::svtkInformationDataObjectMetaDataKey(
  const char* name, const char* location)
  : svtkInformationDataObjectKey(name, location)
{
}

//----------------------------------------------------------------------------
svtkInformationDataObjectMetaDataKey::~svtkInformationDataObjectMetaDataKey() = default;

//----------------------------------------------------------------------------
void svtkInformationDataObjectMetaDataKey::CopyDefaultInformation(
  svtkInformation* request, svtkInformation* fromInfo, svtkInformation* toInfo)
{
  if (request->Has(svtkStreamingDemandDrivenPipeline::REQUEST_INFORMATION()))
  {
    this->ShallowCopy(fromInfo, toInfo);
  }
}

//----------------------------------------------------------------------------
void svtkInformationDataObjectMetaDataKey::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
