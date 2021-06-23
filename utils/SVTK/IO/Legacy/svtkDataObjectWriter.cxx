/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkDataObjectWriter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkDataObjectWriter.h"

#include "svtkDataObject.h"
#include "svtkInformation.h"
#include "svtkObjectFactory.h"

svtkStandardNewMacro(svtkDataObjectWriter);

svtkDataObjectWriter::svtkDataObjectWriter()
{
  this->Writer = svtkDataWriter::New();
}

svtkDataObjectWriter::~svtkDataObjectWriter()
{
  this->Writer->Delete();
}

// Write FieldData data to file
void svtkDataObjectWriter::WriteData()
{
  ostream* fp;
  svtkFieldData* f = this->GetInput()->GetFieldData();

  svtkDebugMacro(<< "Writing svtk FieldData data...");

  this->Writer->SetInputData(this->GetInput());

  if (!(fp = this->Writer->OpenSVTKFile()) || !this->Writer->WriteHeader(fp))
  {
    return;
  }
  //
  // Write FieldData data specific stuff
  //
  this->Writer->WriteFieldData(fp, f);

  this->Writer->CloseSVTKFile(fp);

  this->Writer->SetInputData(nullptr);
}

void svtkDataObjectWriter::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent
     << "File Name: " << (this->Writer->GetFileName() ? this->Writer->GetFileName() : "(none)")
     << "\n";

  if (this->Writer->GetFileType() == SVTK_BINARY)
  {
    os << indent << "File Type: BINARY\n";
  }
  else
  {
    os << indent << "File Type: ASCII\n";
  }

  if (this->Writer->GetHeader())
  {
    os << indent << "Header: " << this->Writer->GetHeader() << "\n";
  }
  else
  {
    os << indent << "Header: (None)\n";
  }

  if (this->Writer->GetFieldDataName())
  {
    os << indent << "Field Data Name: " << this->Writer->GetFieldDataName() << "\n";
  }
  else
  {
    os << indent << "Field Data Name: (None)\n";
  }
}

int svtkDataObjectWriter::FillInputPortInformation(int, svtkInformation* info)
{
  info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkDataObject");
  return 1;
}
