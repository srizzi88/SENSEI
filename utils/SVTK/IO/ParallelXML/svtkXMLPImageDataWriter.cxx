/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkXMLPImageDataWriter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkXMLPImageDataWriter.h"

#include "svtkErrorCode.h"
#include "svtkImageData.h"
#include "svtkInformation.h"
#include "svtkObjectFactory.h"
#include "svtkXMLImageDataWriter.h"

svtkStandardNewMacro(svtkXMLPImageDataWriter);

//----------------------------------------------------------------------------
svtkXMLPImageDataWriter::svtkXMLPImageDataWriter() = default;

//----------------------------------------------------------------------------
svtkXMLPImageDataWriter::~svtkXMLPImageDataWriter() = default;

//----------------------------------------------------------------------------
void svtkXMLPImageDataWriter::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
svtkImageData* svtkXMLPImageDataWriter::GetInput()
{
  return static_cast<svtkImageData*>(this->Superclass::GetInput());
}

//----------------------------------------------------------------------------
const char* svtkXMLPImageDataWriter::GetDataSetName()
{
  return "PImageData";
}

//----------------------------------------------------------------------------
const char* svtkXMLPImageDataWriter::GetDefaultFileExtension()
{
  return "pvti";
}

//----------------------------------------------------------------------------
void svtkXMLPImageDataWriter::WritePrimaryElementAttributes(ostream& os, svtkIndent indent)
{
  this->Superclass::WritePrimaryElementAttributes(os, indent);
  if (this->ErrorCode == svtkErrorCode::OutOfDiskSpaceError)
  {
    return;
  }

  svtkImageData* input = this->GetInput();
  this->WriteVectorAttribute("Origin", 3, input->GetOrigin());
  if (this->ErrorCode == svtkErrorCode::OutOfDiskSpaceError)
  {
    return;
  }

  this->WriteVectorAttribute("Spacing", 3, input->GetSpacing());
}

//----------------------------------------------------------------------------
svtkXMLStructuredDataWriter* svtkXMLPImageDataWriter::CreateStructuredPieceWriter()
{
  // Create the writer for the piece.
  svtkXMLImageDataWriter* pWriter = svtkXMLImageDataWriter::New();
  pWriter->SetInputConnection(this->GetInputConnection(0, 0));
  return pWriter;
}

//----------------------------------------------------------------------------
int svtkXMLPImageDataWriter::FillInputPortInformation(int svtkNotUsed(port), svtkInformation* info)
{
  info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkImageData");
  return 1;
}
