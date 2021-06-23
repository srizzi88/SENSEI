/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkXMLImageDataWriter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkXMLImageDataWriter.h"
#include "svtkImageData.h"
#include "svtkInformation.h"
#include "svtkMatrix3x3.h"
#include "svtkObjectFactory.h"

svtkStandardNewMacro(svtkXMLImageDataWriter);

//----------------------------------------------------------------------------
svtkXMLImageDataWriter::svtkXMLImageDataWriter() = default;

//----------------------------------------------------------------------------
svtkXMLImageDataWriter::~svtkXMLImageDataWriter() = default;

//----------------------------------------------------------------------------
void svtkXMLImageDataWriter::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
svtkImageData* svtkXMLImageDataWriter::GetInput()
{
  return static_cast<svtkImageData*>(this->Superclass::GetInput());
}

//----------------------------------------------------------------------------
void svtkXMLImageDataWriter::GetInputExtent(int* extent)
{
  this->GetInput()->GetExtent(extent);
}

//----------------------------------------------------------------------------
const char* svtkXMLImageDataWriter::GetDataSetName()
{
  return "ImageData";
}

//----------------------------------------------------------------------------
const char* svtkXMLImageDataWriter::GetDefaultFileExtension()
{
  return "vti";
}

//----------------------------------------------------------------------------
void svtkXMLImageDataWriter::WritePrimaryElementAttributes(ostream& os, svtkIndent indent)
{
  this->Superclass::WritePrimaryElementAttributes(os, indent);
  svtkImageData* input = this->GetInput();
  this->WriteVectorAttribute("Origin", 3, input->GetOrigin());
  this->WriteVectorAttribute("Spacing", 3, input->GetSpacing());
  this->WriteVectorAttribute("Direction", 9, input->GetDirectionMatrix()->GetData());
}

//----------------------------------------------------------------------------
int svtkXMLImageDataWriter::FillInputPortInformation(int svtkNotUsed(port), svtkInformation* info)
{
  info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkImageData");
  return 1;
}
