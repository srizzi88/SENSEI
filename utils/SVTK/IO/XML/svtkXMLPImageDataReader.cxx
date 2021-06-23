/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkXMLPImageDataReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkXMLPImageDataReader.h"

#include "svtkDataArray.h"
#include "svtkImageData.h"
#include "svtkInformation.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkStreamingDemandDrivenPipeline.h"
#include "svtkXMLDataElement.h"
#include "svtkXMLImageDataReader.h"

svtkStandardNewMacro(svtkXMLPImageDataReader);

//----------------------------------------------------------------------------
svtkXMLPImageDataReader::svtkXMLPImageDataReader() = default;

//----------------------------------------------------------------------------
svtkXMLPImageDataReader::~svtkXMLPImageDataReader() = default;

//----------------------------------------------------------------------------
void svtkXMLPImageDataReader::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
svtkImageData* svtkXMLPImageDataReader::GetOutput()
{
  return this->GetOutput(0);
}

//----------------------------------------------------------------------------
svtkImageData* svtkXMLPImageDataReader::GetOutput(int idx)
{
  return svtkImageData::SafeDownCast(this->GetOutputDataObject(idx));
}

//----------------------------------------------------------------------------
svtkImageData* svtkXMLPImageDataReader::GetPieceInput(int index)
{
  svtkXMLImageDataReader* reader = static_cast<svtkXMLImageDataReader*>(this->PieceReaders[index]);
  return reader->GetOutput();
}

//----------------------------------------------------------------------------
const char* svtkXMLPImageDataReader::GetDataSetName()
{
  return "PImageData";
}

//----------------------------------------------------------------------------
void svtkXMLPImageDataReader::SetupEmptyOutput()
{
  this->GetCurrentOutput()->Initialize();
}

//----------------------------------------------------------------------------
void svtkXMLPImageDataReader::SetOutputExtent(int* extent)
{
  svtkImageData::SafeDownCast(this->GetCurrentOutput())->SetExtent(extent);
}

//----------------------------------------------------------------------------
void svtkXMLPImageDataReader::GetPieceInputExtent(int index, int* extent)
{
  this->GetPieceInput(index)->GetExtent(extent);
}

//----------------------------------------------------------------------------
int svtkXMLPImageDataReader::ReadPrimaryElement(svtkXMLDataElement* ePrimary)
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

  return 1;
}

//----------------------------------------------------------------------------
// Note that any changes (add or removing information) made to this method
// should be replicated in CopyOutputInformation
void svtkXMLPImageDataReader::SetupOutputInformation(svtkInformation* outInfo)
{
  this->Superclass::SetupOutputInformation(outInfo);

  outInfo->Set(svtkDataObject::ORIGIN(), this->Origin, 3);
  outInfo->Set(svtkDataObject::SPACING(), this->Spacing, 3);
}

//----------------------------------------------------------------------------
void svtkXMLPImageDataReader::CopyOutputInformation(svtkInformation* outInfo, int port)
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
}

//----------------------------------------------------------------------------
svtkXMLDataReader* svtkXMLPImageDataReader::CreatePieceReader()
{
  return svtkXMLImageDataReader::New();
}

//----------------------------------------------------------------------------
int svtkXMLPImageDataReader::FillOutputPortInformation(int, svtkInformation* info)
{
  info->Set(svtkDataObject::DATA_TYPE_NAME(), "svtkImageData");
  return 1;
}
