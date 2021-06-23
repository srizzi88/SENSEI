/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkStructuredPointsWriter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkStructuredPointsWriter.h"

#include "svtkInformation.h"
#include "svtkObjectFactory.h"
#include "svtkStructuredPoints.h"

#if !defined(_WIN32) || defined(__CYGWIN__)
#include <unistd.h> /* unlink */
#else
#include <io.h> /* unlink */
#endif

svtkStandardNewMacro(svtkStructuredPointsWriter);

void svtkStructuredPointsWriter::WriteData()
{
  ostream* fp;
  svtkImageData* input = svtkImageData::SafeDownCast(this->GetInput());
  int dim[3];
  int* ext;
  double spacing[3], origin[3];

  svtkDebugMacro(<< "Writing svtk structured points...");

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
  // Write structured points specific stuff
  //
  *fp << "DATASET STRUCTURED_POINTS\n";

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

  input->GetSpacing(spacing);
  *fp << "SPACING " << spacing[0] << " " << spacing[1] << " " << spacing[2] << "\n";

  input->GetOrigin(origin);
  if (this->WriteExtent)
  {
    *fp << "ORIGIN " << origin[0] << " " << origin[1] << " " << origin[2] << "\n";
  }
  else
  {
    // Do the electric slide. Move origin to min corner of extent.
    // The alternative is to change the format to include an extent instead of dimensions.
    ext = input->GetExtent();
    origin[0] += ext[0] * spacing[0];
    origin[1] += ext[2] * spacing[1];
    origin[2] += ext[4] * spacing[2];
    *fp << "ORIGIN " << origin[0] << " " << origin[1] << " " << origin[2] << "\n";
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

int svtkStructuredPointsWriter::FillInputPortInformation(int, svtkInformation* info)
{
  info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkImageData");
  return 1;
}

svtkImageData* svtkStructuredPointsWriter::GetInput()
{
  return svtkImageData::SafeDownCast(this->Superclass::GetInput());
}

svtkImageData* svtkStructuredPointsWriter::GetInput(int port)
{
  return svtkImageData::SafeDownCast(this->Superclass::GetInput(port));
}

void svtkStructuredPointsWriter::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
