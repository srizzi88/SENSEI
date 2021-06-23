/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkXMLPRectilinearGridWriter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkXMLPRectilinearGridWriter.h"

#include "svtkErrorCode.h"
#include "svtkInformation.h"
#include "svtkObjectFactory.h"
#include "svtkRectilinearGrid.h"
#include "svtkXMLRectilinearGridWriter.h"

svtkStandardNewMacro(svtkXMLPRectilinearGridWriter);

//----------------------------------------------------------------------------
svtkXMLPRectilinearGridWriter::svtkXMLPRectilinearGridWriter() = default;

//----------------------------------------------------------------------------
svtkXMLPRectilinearGridWriter::~svtkXMLPRectilinearGridWriter() = default;

//----------------------------------------------------------------------------
void svtkXMLPRectilinearGridWriter::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
svtkRectilinearGrid* svtkXMLPRectilinearGridWriter::GetInput()
{
  return static_cast<svtkRectilinearGrid*>(this->Superclass::GetInput());
}

//----------------------------------------------------------------------------
const char* svtkXMLPRectilinearGridWriter::GetDataSetName()
{
  return "PRectilinearGrid";
}

//----------------------------------------------------------------------------
const char* svtkXMLPRectilinearGridWriter::GetDefaultFileExtension()
{
  return "pvtr";
}

//----------------------------------------------------------------------------
svtkXMLStructuredDataWriter* svtkXMLPRectilinearGridWriter::CreateStructuredPieceWriter()
{
  // Create the writer for the piece.
  svtkXMLRectilinearGridWriter* pWriter = svtkXMLRectilinearGridWriter::New();
  pWriter->SetInputConnection(this->GetInputConnection(0, 0));
  return pWriter;
}

//----------------------------------------------------------------------------
void svtkXMLPRectilinearGridWriter::WritePData(svtkIndent indent)
{
  this->Superclass::WritePData(indent);
  if (this->ErrorCode == svtkErrorCode::OutOfDiskSpaceError)
  {
    return;
  }

  svtkRectilinearGrid* input = this->GetInput();
  this->WritePCoordinates(
    input->GetXCoordinates(), input->GetYCoordinates(), input->GetZCoordinates(), indent);
}

int svtkXMLPRectilinearGridWriter::FillInputPortInformation(
  int svtkNotUsed(port), svtkInformation* info)
{
  info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkRectilinearGrid");
  return 1;
}
