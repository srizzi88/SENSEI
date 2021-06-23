/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkRectilinearGridWriter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkRectilinearGridWriter.h"

#include "svtkInformation.h"
#include "svtkObjectFactory.h"
#include "svtkRectilinearGrid.h"

#if !defined(_WIN32) || defined(__CYGWIN__)
#include <unistd.h> /* unlink */
#else
#include <io.h> /* unlink */
#endif

svtkStandardNewMacro(svtkRectilinearGridWriter);

void svtkRectilinearGridWriter::WriteData()
{
  ostream* fp;
  svtkRectilinearGrid* input = svtkRectilinearGrid::SafeDownCast(this->GetInput());
  int dim[3];

  svtkDebugMacro(<< "Writing svtk rectilinear grid...");

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
  // Write rectilinear grid specific stuff
  //
  *fp << "DATASET RECTILINEAR_GRID\n";

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

  if (!this->WriteCoordinates(fp, input->GetXCoordinates(), 0))
  {
    svtkErrorMacro("Ran out of disk space; deleting file: " << this->FileName);
    this->CloseSVTKFile(fp);
    unlink(this->FileName);
    return;
  }
  if (!this->WriteCoordinates(fp, input->GetYCoordinates(), 1))
  {
    svtkErrorMacro("Ran out of disk space; deleting file: " << this->FileName);
    this->CloseSVTKFile(fp);
    unlink(this->FileName);
    return;
  }
  if (!this->WriteCoordinates(fp, input->GetZCoordinates(), 2))
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

int svtkRectilinearGridWriter::FillInputPortInformation(int, svtkInformation* info)
{
  info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkRectilinearGrid");
  return 1;
}

svtkRectilinearGrid* svtkRectilinearGridWriter::GetInput()
{
  return svtkRectilinearGrid::SafeDownCast(this->Superclass::GetInput());
}

svtkRectilinearGrid* svtkRectilinearGridWriter::GetInput(int port)
{
  return svtkRectilinearGrid::SafeDownCast(this->Superclass::GetInput(port));
}

void svtkRectilinearGridWriter::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
