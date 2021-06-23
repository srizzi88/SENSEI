/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkXMLPStructuredGridWriter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkXMLPStructuredGridWriter.h"

#include "svtkErrorCode.h"
#include "svtkInformation.h"
#include "svtkObjectFactory.h"
#include "svtkStructuredGrid.h"
#include "svtkXMLStructuredGridWriter.h"

svtkStandardNewMacro(svtkXMLPStructuredGridWriter);

//----------------------------------------------------------------------------
svtkXMLPStructuredGridWriter::svtkXMLPStructuredGridWriter() = default;

//----------------------------------------------------------------------------
svtkXMLPStructuredGridWriter::~svtkXMLPStructuredGridWriter() = default;

//----------------------------------------------------------------------------
void svtkXMLPStructuredGridWriter::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
svtkStructuredGrid* svtkXMLPStructuredGridWriter::GetInput()
{
  return static_cast<svtkStructuredGrid*>(this->Superclass::GetInput());
}

//----------------------------------------------------------------------------
const char* svtkXMLPStructuredGridWriter::GetDataSetName()
{
  return "PStructuredGrid";
}

//----------------------------------------------------------------------------
const char* svtkXMLPStructuredGridWriter::GetDefaultFileExtension()
{
  return "pvts";
}

//----------------------------------------------------------------------------
svtkXMLStructuredDataWriter* svtkXMLPStructuredGridWriter::CreateStructuredPieceWriter()
{
  // Create the writer for the piece.
  svtkXMLStructuredGridWriter* pWriter = svtkXMLStructuredGridWriter::New();
  pWriter->SetInputConnection(this->GetInputConnection(0, 0));
  return pWriter;
}

//----------------------------------------------------------------------------
void svtkXMLPStructuredGridWriter::WritePData(svtkIndent indent)
{
  this->Superclass::WritePData(indent);
  if (this->ErrorCode == svtkErrorCode::OutOfDiskSpaceError)
  {
    return;
  }
  svtkStructuredGrid* input = this->GetInput();
  this->WritePPoints(input->GetPoints(), indent);
}

//----------------------------------------------------------------------------
int svtkXMLPStructuredGridWriter::FillInputPortInformation(int, svtkInformation* info)
{
  info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkStructuredGrid");
  return 1;
}
