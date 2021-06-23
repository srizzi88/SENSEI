/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkXMLImageDataReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkXMLImageDataReader.h"

#include "svtkDataArray.h"
#include "svtkImageData.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkStreamingDemandDrivenPipeline.h"
#include "svtkXMLDataElement.h"

svtkStandardNewMacro(svtkXMLImageDataReader);

//----------------------------------------------------------------------------
svtkXMLImageDataReader::svtkXMLImageDataReader() = default;

//----------------------------------------------------------------------------
svtkXMLImageDataReader::~svtkXMLImageDataReader() = default;

//----------------------------------------------------------------------------
void svtkXMLImageDataReader::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
svtkImageData* svtkXMLImageDataReader::GetOutput()
{
  return this->GetOutput(0);
}

//----------------------------------------------------------------------------
svtkImageData* svtkXMLImageDataReader::GetOutput(int idx)
{
  return svtkImageData::SafeDownCast(this->GetOutputDataObject(idx));
}

//----------------------------------------------------------------------------
const char* svtkXMLImageDataReader::GetDataSetName()
{
  return "ImageData";
}

//----------------------------------------------------------------------------
void svtkXMLImageDataReader::SetOutputExtent(int* extent)
{
  svtkImageData::SafeDownCast(this->GetCurrentOutput())->SetExtent(extent);
}

//----------------------------------------------------------------------------
int svtkXMLImageDataReader::ReadPrimaryElement(svtkXMLDataElement* ePrimary)
{
  if (!this->Superclass::ReadPrimaryElement(ePrimary))
  {
    return 0;
  }

  // Get the image's origin.
  if (ePrimary->GetVectorAttribute("Origin", 3, this->Origin) != 3)
  {
    this->Origin[0] = 0;
    this->Origin[1] = 0;
    this->Origin[2] = 0;
  }

  // Get the image's spacing.
  if (ePrimary->GetVectorAttribute("Spacing", 3, this->Spacing) != 3)
  {
    this->Spacing[0] = 1;
    this->Spacing[1] = 1;
    this->Spacing[2] = 1;
  }

  // Get the image's direction.
  if (ePrimary->GetVectorAttribute("Direction", 9, this->Direction) != 9)
  {
    this->Direction[0] = 1;
    this->Direction[1] = 0;
    this->Direction[2] = 0;
    this->Direction[3] = 0;
    this->Direction[4] = 1;
    this->Direction[5] = 0;
    this->Direction[6] = 0;
    this->Direction[7] = 0;
    this->Direction[8] = 1;
  }

  return 1;
}

//----------------------------------------------------------------------------
// Note that any changes (add or removing information) made to this method
// should be replicated in CopyOutputInformation
void svtkXMLImageDataReader::SetupOutputInformation(svtkInformation* outInfo)
{
  this->Superclass::SetupOutputInformation(outInfo);

  outInfo->Set(svtkDataObject::ORIGIN(), this->Origin, 3);
  outInfo->Set(svtkDataObject::SPACING(), this->Spacing, 3);
  outInfo->Set(svtkDataObject::DIRECTION(), this->Direction, 9);
}

//----------------------------------------------------------------------------
void svtkXMLImageDataReader::CopyOutputInformation(svtkInformation* outInfo, int port)
{
  this->Superclass::CopyOutputInformation(outInfo, port);
  svtkInformation* localInfo = this->GetExecutive()->GetOutputInformation(port);
  if (localInfo->Has(svtkDataObject::ORIGIN()))
  {
    outInfo->CopyEntry(localInfo, svtkDataObject::ORIGIN());
  }
  if (localInfo->Has(svtkDataObject::SPACING()))
  {
    outInfo->CopyEntry(localInfo, svtkDataObject::SPACING());
  }
  if (localInfo->Has(svtkDataObject::DIRECTION()))
  {
    outInfo->CopyEntry(localInfo, svtkDataObject::DIRECTION());
  }
}

//----------------------------------------------------------------------------
int svtkXMLImageDataReader::FillOutputPortInformation(int, svtkInformation* info)
{
  info->Set(svtkDataObject::DATA_TYPE_NAME(), "svtkImageData");
  return 1;
}
