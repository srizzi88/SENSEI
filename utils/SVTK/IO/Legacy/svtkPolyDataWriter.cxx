/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPolyDataWriter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkPolyDataWriter.h"

#include "svtkInformation.h"
#include "svtkObjectFactory.h"
#include "svtkPolyData.h"

#if !defined(_WIN32) || defined(__CYGWIN__)
#include <unistd.h> /* unlink */
#else
#include <io.h> /* unlink */
#endif

svtkStandardNewMacro(svtkPolyDataWriter);

void svtkPolyDataWriter::WriteData()
{
  ostream* fp;
  svtkPolyData* input = this->GetInput();

  svtkDebugMacro(<< "Writing svtk polygonal data...");

  if (!(fp = this->OpenSVTKFile()) || !this->WriteHeader(fp))
  {
    if (fp)
    {
      if (this->FileName)
      {
        svtkErrorMacro("Ran out of disk space; deleting file: " << this->FileName);
        this->CloseSVTKFile(fp);
        unlink(this->FileName);
      }
      else
      {
        this->CloseSVTKFile(fp);
        svtkErrorMacro("Could not read memory header. ");
      }
    }
    return;
  }
  //
  // Write polygonal data specific stuff
  //
  *fp << "DATASET POLYDATA\n";

  //
  // Write data owned by the dataset
  int errorOccured = 0;
  if (!this->WriteDataSetData(fp, input))
  {
    errorOccured = 1;
  }
  if (!errorOccured && !this->WritePoints(fp, input->GetPoints()))
  {
    errorOccured = 1;
  }

  if (!errorOccured && input->GetVerts())
  {
    if (!this->WriteCells(fp, input->GetVerts(), "VERTICES"))
    {
      errorOccured = 1;
    }
  }
  if (!errorOccured && input->GetLines())
  {
    if (!this->WriteCells(fp, input->GetLines(), "LINES"))
    {
      errorOccured = 1;
    }
  }
  if (!errorOccured && input->GetPolys())
  {
    if (!this->WriteCells(fp, input->GetPolys(), "POLYGONS"))
    {
      errorOccured = 1;
    }
  }
  if (!errorOccured && input->GetStrips())
  {
    if (!this->WriteCells(fp, input->GetStrips(), "TRIANGLE_STRIPS"))
    {
      errorOccured = 1;
    }
  }

  if (!errorOccured && !this->WriteCellData(fp, input))
  {
    errorOccured = 1;
  }
  if (!errorOccured && !this->WritePointData(fp, input))
  {
    errorOccured = 1;
  }

  if (errorOccured)
  {
    if (this->FileName)
    {
      svtkErrorMacro("Ran out of disk space; deleting file: " << this->FileName);
      this->CloseSVTKFile(fp);
      unlink(this->FileName);
    }
    else
    {
      svtkErrorMacro("Error writing data set to memory");
      this->CloseSVTKFile(fp);
    }
    return;
  }
  this->CloseSVTKFile(fp);
}

int svtkPolyDataWriter::FillInputPortInformation(int, svtkInformation* info)
{
  info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkPolyData");
  return 1;
}

svtkPolyData* svtkPolyDataWriter::GetInput()
{
  return svtkPolyData::SafeDownCast(this->Superclass::GetInput());
}

svtkPolyData* svtkPolyDataWriter::GetInput(int port)
{
  return svtkPolyData::SafeDownCast(this->Superclass::GetInput(port));
}

void svtkPolyDataWriter::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
