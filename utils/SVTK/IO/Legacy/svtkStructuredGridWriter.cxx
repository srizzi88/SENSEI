/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkStructuredGridWriter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkStructuredGridWriter.h"

#include "svtkInformation.h"
#include "svtkObjectFactory.h"
#include "svtkStructuredGrid.h"
#include "svtkUnsignedCharArray.h"

#if !defined(_WIN32) || defined(__CYGWIN__)
#include <unistd.h> /* unlink */
#else
#include <io.h> /* unlink */
#endif

svtkStandardNewMacro(svtkStructuredGridWriter);

void svtkStructuredGridWriter::WriteData()
{
  ostream* fp;
  svtkStructuredGrid* input = svtkStructuredGrid::SafeDownCast(this->GetInput());
  int dim[3];

  svtkDebugMacro(<< "Writing svtk structured grid...");

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

  // Write structured grid specific stuff
  //
  *fp << "DATASET STRUCTURED_GRID\n";

  // Write data owned by the dataset
  if (!this->WriteDataSetData(fp, input))
  {
    svtkErrorMacro("Ran out of disk space; deleting file: " << this->FileName);
    this->CloseSVTKFile(fp);
    unlink(this->FileName);
    return;
  }

  if (this->WriteExtent)
  {
    int extent[6];
    input->GetExtent(extent);
    *fp << "EXTENT " << extent[0] << " " << extent[1] << " " << extent[2] << " " << extent[3] << " "
        << extent[4] << " " << extent[5] << "\n";
  }
  else
  {
    input->GetDimensions(dim);
    *fp << "DIMENSIONS " << dim[0] << " " << dim[1] << " " << dim[2] << "\n";
  }

  if (!this->WritePoints(fp, input->GetPoints()))
  {
    svtkErrorMacro("Ran out of disk space; deleting file: " << this->FileName);
    this->CloseSVTKFile(fp);
    unlink(this->FileName);
    return;
  }

  if (!this->WriteCellData(fp, input))
  {
    svtkErrorMacro("Ran out of disk space; deleting file: " << this->FileName);
    this->CloseSVTKFile(fp);
    unlink(this->FileName);
    return;
  }
  if (!this->WritePointData(fp, input))
  {
    svtkErrorMacro("Ran out of disk space; deleting file: " << this->FileName);
    this->CloseSVTKFile(fp);
    unlink(this->FileName);
    return;
  }

  this->CloseSVTKFile(fp);
}

int svtkStructuredGridWriter::FillInputPortInformation(int, svtkInformation* info)
{
  info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkStructuredGrid");
  return 1;
}

svtkStructuredGrid* svtkStructuredGridWriter::GetInput()
{
  return svtkStructuredGrid::SafeDownCast(this->Superclass::GetInput());
}

svtkStructuredGrid* svtkStructuredGridWriter::GetInput(int port)
{
  return svtkStructuredGrid::SafeDownCast(this->Superclass::GetInput(port));
}

void svtkStructuredGridWriter::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
