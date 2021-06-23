/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkStructuredGridReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkStructuredGridReader.h"

#include "svtkDataSetAttributes.h"
#include "svtkErrorCode.h"
#include "svtkFieldData.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkStreamingDemandDrivenPipeline.h"
#include "svtkStructuredGrid.h"
#include "svtkUnsignedCharArray.h"

svtkStandardNewMacro(svtkStructuredGridReader);

svtkStructuredGridReader::svtkStructuredGridReader() = default;
svtkStructuredGridReader::~svtkStructuredGridReader() = default;

//----------------------------------------------------------------------------
svtkStructuredGrid* svtkStructuredGridReader::GetOutput()
{
  return this->GetOutput(0);
}

//----------------------------------------------------------------------------
svtkStructuredGrid* svtkStructuredGridReader::GetOutput(int idx)
{
  return svtkStructuredGrid::SafeDownCast(this->GetOutputDataObject(idx));
}

//-----------------------------------------------------------------------------
void svtkStructuredGridReader::SetOutput(svtkStructuredGrid* output)
{
  this->GetExecutive()->SetOutputData(0, output);
}

//-----------------------------------------------------------------------------
int svtkStructuredGridReader::ReadMetaDataSimple(const std::string& fname, svtkInformation* metadata)
{
  char line[256];
  bool dimsRead = 0;

  if (!this->OpenSVTKFile(fname.c_str()) || !this->ReadHeader(fname.c_str()))
  {
    return 1;
  }

  // Read structured grid specific stuff
  //
  if (!this->ReadString(line))
  {
    svtkErrorMacro(<< "Data file ends prematurely!");
    this->CloseSVTKFile();
    return 1;
  }

  if (!strncmp(this->LowerCase(line), "dataset", (unsigned long)7))
  {
    // Make sure we're reading right type of geometry
    //
    if (!this->ReadString(line))
    {
      svtkErrorMacro(<< "Data file ends prematurely!");
      this->CloseSVTKFile();
      return 1;
    }

    if (strncmp(this->LowerCase(line), "structured_grid", 15))
    {
      svtkErrorMacro(<< "Cannot read dataset type: " << line);
      this->CloseSVTKFile();
      return 1;
    }

    // Read keyword and dimensions
    //
    while (true)
    {
      if (!this->ReadString(line))
      {
        break;
      }

      // Have to read field data because it may be binary.
      if (!strncmp(this->LowerCase(line), "field", 5))
      {
        svtkFieldData* fd = this->ReadFieldData();
        fd->Delete();
      }

      if (!strncmp(this->LowerCase(line), "dimensions", 10) && !dimsRead)
      {
        int dim[3];
        if (!(this->Read(dim) && this->Read(dim + 1) && this->Read(dim + 2)))
        {
          svtkErrorMacro(<< "Error reading dimensions!");
          this->CloseSVTKFile();
          this->SetErrorCode(svtkErrorCode::FileFormatError);
          return 1;
        }
        metadata->Set(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), 0, dim[0] - 1, 0,
          dim[1] - 1, 0, dim[2] - 1);
        dimsRead = 1;
      }

      else if (!strncmp(line, "extent", 6) && !dimsRead)
      {
        int extent[6];
        if (!(this->Read(extent) && this->Read(extent + 1) && this->Read(extent + 2) &&
              this->Read(extent + 3) && this->Read(extent + 4) && this->Read(extent + 5)))
        {
          svtkErrorMacro(<< "Error reading extent!");
          this->CloseSVTKFile();
          this->SetErrorCode(svtkErrorCode::FileFormatError);
          return 1;
        }

        metadata->Set(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), extent[0], extent[1],
          extent[2], extent[3], extent[4], extent[5]);

        dimsRead = 1;
      }
    }
  }

  if (!dimsRead)
  {
    svtkWarningMacro(<< "Could not read dimensions or extents from the file.");
  }
  this->CloseSVTKFile();

  return 1;
}

//-----------------------------------------------------------------------------
int svtkStructuredGridReader::ReadMeshSimple(const std::string& fname, svtkDataObject* doOutput)
{
  svtkIdType numPts = 0, npts = 0, numCells = 0, ncells;
  char line[256];
  int dimsRead = 0;
  svtkStructuredGrid* output = svtkStructuredGrid::SafeDownCast(doOutput);

  svtkDebugMacro(<< "Reading svtk structured grid file...");

  if (!this->OpenSVTKFile(fname.c_str()) || !this->ReadHeader(fname.c_str()))
  {
    return 1;
  }

  // Read structured grid specific stuff
  //
  if (!this->ReadString(line))
  {
    svtkErrorMacro(<< "Data file ends prematurely!");
    this->CloseSVTKFile();
    return 1;
  }

  if (!strncmp(this->LowerCase(line), "dataset", (unsigned long)7))
  {
    // Make sure we're reading right type of geometry
    //
    if (!this->ReadString(line))
    {
      svtkErrorMacro(<< "Data file ends prematurely!");
      this->CloseSVTKFile();
      return 1;
    }

    if (strncmp(this->LowerCase(line), "structured_grid", 15))
    {
      svtkErrorMacro(<< "Cannot read dataset type: " << line);
      this->CloseSVTKFile();
      return 1;
    }

    // Read keyword and number of points
    //
    while (true)
    {
      if (!this->ReadString(line))
      {
        break;
      }

      if (!strncmp(this->LowerCase(line), "field", 5))
      {
        svtkFieldData* fd = this->ReadFieldData();
        output->SetFieldData(fd);
        fd->Delete(); // ?
      }

      else if (!strncmp(line, "extent", 6) && !dimsRead)
      {
        int extent[6];
        if (!(this->Read(extent) && this->Read(extent + 1) && this->Read(extent + 2) &&
              this->Read(extent + 3) && this->Read(extent + 4) && this->Read(extent + 5)))
        {
          svtkErrorMacro(<< "Error reading extent!");
          this->CloseSVTKFile();
          this->SetErrorCode(svtkErrorCode::FileFormatError);
          return 1;
        }

        output->SetExtent(extent);
        numPts = output->GetNumberOfPoints();
        numCells = output->GetNumberOfCells();
        dimsRead = 1;
      }
      else if (!strncmp(line, "dimensions", 10))
      {
        int dim[3];
        if (!(this->Read(dim) && this->Read(dim + 1) && this->Read(dim + 2)))
        {
          svtkErrorMacro(<< "Error reading dimensions!");
          this->CloseSVTKFile();
          this->SetErrorCode(svtkErrorCode::FileFormatError);
          return 1;
        }

        numPts = dim[0] * dim[1] * dim[2];
        output->SetDimensions(dim);
        numCells = output->GetNumberOfCells();
        dimsRead = 1;
      }

      else if (this->FileMajorVersion < 4 && !strncmp(line, "blanking", 8))
      {
        if (!this->Read(&npts))
        {
          svtkErrorMacro(<< "Error reading blanking!");
          this->CloseSVTKFile();
          return 1;
        }

        if (!this->ReadString(line))
        {
          svtkErrorMacro(<< "Cannot read blank type!");
          this->CloseSVTKFile();
          return 1;
        }

        svtkUnsignedCharArray* data =
          svtkArrayDownCast<svtkUnsignedCharArray>(this->ReadArray(line, numPts, 1));

        if (data != nullptr)
        {
          svtkUnsignedCharArray* ghosts = svtkUnsignedCharArray::New();
          ghosts->SetNumberOfValues(numPts);
          ghosts->SetName(svtkDataSetAttributes::GhostArrayName());
          for (svtkIdType ptId = 0; ptId < numPts; ++ptId)
          {
            unsigned char value = 0;
            if (data->GetValue(ptId) == 0)
            {
              value |= svtkDataSetAttributes::HIDDENPOINT;
            }
            ghosts->SetValue(ptId, value);
          }
          output->GetPointData()->AddArray(ghosts);
          ghosts->Delete();
          data->Delete();
        }
      }

      else if (!strncmp(line, "points", 6))
      {
        if (!this->Read(&npts))
        {
          svtkErrorMacro(<< "Error reading points!");
          this->CloseSVTKFile();
          return 1;
        }

        this->ReadPointCoordinates(output, npts);
      }

      else if (!strncmp(line, "cell_data", 9))
      {
        if (!this->Read(&ncells))
        {
          svtkErrorMacro(<< "Cannot read cell data!");
          this->CloseSVTKFile();
          return 1;
        }

        if (ncells != numCells)
        {
          svtkErrorMacro(<< "Number of cells don't match!");
          this->CloseSVTKFile();
          return 1;
        }

        this->ReadCellData(output, ncells);
        break; // out of this loop
      }

      else if (!strncmp(line, "point_data", 10))
      {
        if (!this->Read(&numPts))
        {
          svtkErrorMacro(<< "Cannot read point data!");
          this->CloseSVTKFile();
          return 1;
        }

        if (npts != numPts)
        {
          svtkErrorMacro(<< "Number of points don't match!");
          this->CloseSVTKFile();
          return 1;
        }

        this->ReadPointData(output, npts);
        break; // out of this loop
      }

      else
      {
        svtkErrorMacro(<< "Unrecognized keyword: " << line);
        this->CloseSVTKFile();
        return 1;
      }
    }

    if (!dimsRead)
      svtkWarningMacro(<< "No dimensions read.");
    if (!output->GetPoints())
      svtkWarningMacro(<< "No points read.");
  }

  else if (!strncmp(line, "cell_data", 9))
  {
    svtkWarningMacro(<< "No geometry defined in data file!");
    if (!this->Read(&ncells))
    {
      svtkErrorMacro(<< "Cannot read cell data!");
      this->CloseSVTKFile();
      return 1;
    }
    this->ReadCellData(output, ncells);
  }

  else if (!strncmp(line, "point_data", 10))
  {
    svtkWarningMacro(<< "No geometry defined in data file!");
    if (!this->Read(&npts))
    {
      svtkErrorMacro(<< "Cannot read point data!");
      this->CloseSVTKFile();
      return 1;
    }
    this->ReadPointData(output, npts);
  }

  else
  {
    svtkErrorMacro(<< "Unrecognized keyword: " << line);
  }
  this->CloseSVTKFile();

  return 1;
}

//----------------------------------------------------------------------------
int svtkStructuredGridReader::FillOutputPortInformation(int, svtkInformation* info)
{
  info->Set(svtkDataObject::DATA_TYPE_NAME(), "svtkStructuredGrid");
  return 1;
}

void svtkStructuredGridReader::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
