/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkXMLPUnstructuredGridWriter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkXMLPUnstructuredGridWriter.h"

#include "svtkInformation.h"
#include "svtkObjectFactory.h"
#include "svtkUnstructuredGrid.h"
#include "svtkXMLUnstructuredGridWriter.h"

svtkStandardNewMacro(svtkXMLPUnstructuredGridWriter);

//----------------------------------------------------------------------------
svtkXMLPUnstructuredGridWriter::svtkXMLPUnstructuredGridWriter() = default;

//----------------------------------------------------------------------------
svtkXMLPUnstructuredGridWriter::~svtkXMLPUnstructuredGridWriter() = default;

//----------------------------------------------------------------------------
void svtkXMLPUnstructuredGridWriter::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
svtkUnstructuredGridBase* svtkXMLPUnstructuredGridWriter::GetInput()
{
  return static_cast<svtkUnstructuredGridBase*>(this->Superclass::GetInput());
}

//----------------------------------------------------------------------------
const char* svtkXMLPUnstructuredGridWriter::GetDataSetName()
{
  return "PUnstructuredGrid";
}

//----------------------------------------------------------------------------
const char* svtkXMLPUnstructuredGridWriter::GetDefaultFileExtension()
{
  return "pvtu";
}

//----------------------------------------------------------------------------
svtkXMLUnstructuredDataWriter* svtkXMLPUnstructuredGridWriter::CreateUnstructuredPieceWriter()
{
  // Create the writer for the piece.
  svtkXMLUnstructuredGridWriter* pWriter = svtkXMLUnstructuredGridWriter::New();
  pWriter->SetInputConnection(this->GetInputConnection(0, 0));
  return pWriter;
}

//----------------------------------------------------------------------------
int svtkXMLPUnstructuredGridWriter::FillInputPortInformation(int, svtkInformation* info)
{
  info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkUnstructuredGridBase");
  return 1;
}
