/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkDataObjectReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkDataObjectReader.h"

#include "svtkDataObject.h"
#include "svtkExecutive.h"
#include "svtkFieldData.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkObjectFactory.h"

svtkStandardNewMacro(svtkDataObjectReader);

svtkDataObjectReader::svtkDataObjectReader() = default;
svtkDataObjectReader::~svtkDataObjectReader() = default;

//----------------------------------------------------------------------------
svtkDataObject* svtkDataObjectReader::GetOutput()
{
  return this->GetOutput(0);
}

//----------------------------------------------------------------------------
svtkDataObject* svtkDataObjectReader::GetOutput(int port)
{
  return svtkDataObject::SafeDownCast(this->GetOutputDataObject(port));
}

//----------------------------------------------------------------------------
void svtkDataObjectReader::SetOutput(svtkDataObject* output)
{
  this->GetExecutive()->SetOutputData(0, output);
}

int svtkDataObjectReader::ReadMeshSimple(const std::string& fname, svtkDataObject* output)
{
  char line[256];
  svtkFieldData* field = nullptr;

  svtkDebugMacro(<< "Reading svtk field data...");

  if (!(this->OpenSVTKFile(fname.c_str())) || !this->ReadHeader())
  {
    return 1;
  }

  // Read field data until end-of-file
  //
  while (this->ReadString(line) && !field)
  {
    if (!strncmp(this->LowerCase(line), "field", (unsigned long)5))
    {
      field = this->ReadFieldData(); // reads named field (or first found)
      if (field != nullptr)
      {
        output->SetFieldData(field);
        field->Delete();
      }
    }

    else if (!strncmp(this->LowerCase(line), "dataset", (unsigned long)7))
    {
      svtkErrorMacro(<< "Field reader cannot read datasets");
      this->CloseSVTKFile();
      return 1;
    }

    else
    {
      svtkErrorMacro(<< "Unrecognized keyword: " << line);
      this->CloseSVTKFile();
      return 1;
    }
  }
  // while field not read

  this->CloseSVTKFile();

  return 1;
}

int svtkDataObjectReader::FillOutputPortInformation(int, svtkInformation* info)
{
  info->Set(svtkDataObject::DATA_TYPE_NAME(), "svtkDataObject");
  return 1;
}

void svtkDataObjectReader::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
