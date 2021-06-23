/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkTableWriter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkTableWriter.h"

#include "svtkByteSwap.h"
#include "svtkCellArray.h"
#include "svtkInformation.h"
#include "svtkObjectFactory.h"
#include "svtkTable.h"

#if !defined(_WIN32) || defined(__CYGWIN__)
#include <unistd.h> /* unlink */
#else
#include <io.h> /* unlink */
#endif

svtkStandardNewMacro(svtkTableWriter);

void svtkTableWriter::WriteData()
{
  ostream* fp = nullptr;
  svtkDebugMacro(<< "Writing svtk table data...");

  if (!(fp = this->OpenSVTKFile()) || !this->WriteHeader(fp))
  {
    if (fp)
    {
      svtkErrorMacro("Ran out of disk space; deleting file: " << this->FileName);
      this->CloseSVTKFile(fp);
      unlink(this->FileName);
    }
    return;
  }
  //
  // Write table specific stuff
  //
  *fp << "DATASET TABLE\n";

  this->WriteFieldData(fp, this->GetInput()->GetFieldData());
  this->WriteRowData(fp, this->GetInput());

  this->CloseSVTKFile(fp);
}

int svtkTableWriter::FillInputPortInformation(int, svtkInformation* info)
{
  info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkTable");
  return 1;
}

svtkTable* svtkTableWriter::GetInput()
{
  return svtkTable::SafeDownCast(this->Superclass::GetInput());
}

svtkTable* svtkTableWriter::GetInput(int port)
{
  return svtkTable::SafeDownCast(this->Superclass::GetInput(port));
}

void svtkTableWriter::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
