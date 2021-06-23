/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkXMLPPolyDataWriter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkXMLPPolyDataWriter.h"

#include "svtkInformation.h"
#include "svtkObjectFactory.h"
#include "svtkPolyData.h"
#include "svtkXMLPolyDataWriter.h"

svtkStandardNewMacro(svtkXMLPPolyDataWriter);

//----------------------------------------------------------------------------
svtkXMLPPolyDataWriter::svtkXMLPPolyDataWriter() = default;

//----------------------------------------------------------------------------
svtkXMLPPolyDataWriter::~svtkXMLPPolyDataWriter() = default;

//----------------------------------------------------------------------------
void svtkXMLPPolyDataWriter::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
svtkPolyData* svtkXMLPPolyDataWriter::GetInput()
{
  return static_cast<svtkPolyData*>(this->Superclass::GetInput());
}

//----------------------------------------------------------------------------
const char* svtkXMLPPolyDataWriter::GetDataSetName()
{
  return "PPolyData";
}

//----------------------------------------------------------------------------
const char* svtkXMLPPolyDataWriter::GetDefaultFileExtension()
{
  return "pvtp";
}

//----------------------------------------------------------------------------
svtkXMLUnstructuredDataWriter* svtkXMLPPolyDataWriter::CreateUnstructuredPieceWriter()
{
  // Create the writer for the piece.
  svtkXMLPolyDataWriter* pWriter = svtkXMLPolyDataWriter::New();
  pWriter->SetInputConnection(this->GetInputConnection(0, 0));
  return pWriter;
}

//----------------------------------------------------------------------------
int svtkXMLPPolyDataWriter::FillInputPortInformation(int, svtkInformation* info)
{
  info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkPolyData");
  return 1;
}
