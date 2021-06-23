/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkRectilinearGridReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkRectilinearGridReader.h"

#include "svtkDataArray.h"
#include "svtkErrorCode.h"
#include "svtkFieldData.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkObjectFactory.h"
#include "svtkRectilinearGrid.h"
#include "svtkStreamingDemandDrivenPipeline.h"

svtkStandardNewMacro(svtkRectilinearGridReader);

svtkRectilinearGridReader::svtkRectilinearGridReader() = default;
svtkRectilinearGridReader::~svtkRectilinearGridReader() = default;

//----------------------------------------------------------------------------
svtkRectilinearGrid* svtkRectilinearGridReader::GetOutput()
{
  return this->GetOutput(0);
}

//----------------------------------------------------------------------------
svtkRectilinearGrid* svtkRectilinearGridReader::GetOutput(int idx)
{
  return svtkRectilinearGrid::SafeDownCast(this->GetOutputDataObject(idx));
}

//----------------------------------------------------------------------------
void svtkRectilinearGridReader::SetOutput(svtkRectilinearGrid* output)
{
  this->GetExecutive()->SetOutputData(0, output);
}

//----------------------------------------------------------------------------
int svtkRectilinearGridReader::ReadMetaDataSimple(const std::string& fname, svtkInformation* outInfo)
{
  char line[256];
  bool dimsRead = 0;

  svtkDebugMacro(<< "Reading svtk rectilinear grid file info...");

  if (!this->OpenSVTKFile(fname.c_str()) || !this->ReadHeader(fname.c_str()))
  {
    return 1;
  }

  // Read rectilinear grid specific stuff
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

    if (strncmp(this->LowerCase(line), "rectilinear_grid", 16))
    {
      svtkErrorMacro(<< "Cannot read dataset type: " << line);
      this->CloseSVTKFile();
      return 1;
    }

    // Read keyword and number of points
    //
    while (1)
    {
      if (!this->ReadString(line))
      {
        break;
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
        outInfo->Set(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), 0, dim[0] - 1, 0, dim[1] - 1,
          0, dim[2] - 1);
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

        outInfo->Set(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), extent[0], extent[1],
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

//----------------------------------------------------------------------------
int svtkRectilinearGridReader::ReadMeshSimple(const std::string& fname, svtkDataObject* doOutput)
{
  svtkIdType numPts = 0, npts, ncoords, numCells = 0, ncells;
  char line[256];
  int dimsRead = 0;
  svtkRectilinearGrid* output = svtkRectilinearGrid::SafeDownCast(doOutput);

  svtkDebugMacro(<< "Reading svtk rectilinear grid file...");
  if (this->Debug)
  {
    this->DebugOn();
  }
  else
  {
    this->DebugOff();
  }

  if (!this->OpenSVTKFile(fname.c_str()) || !this->ReadHeader(fname.c_str()))
  {
    return 1;
  }

  // Read rectilinear grid specific stuff
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

    if (strncmp(this->LowerCase(line), "rectilinear_grid", 16))
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

      else if (!strncmp(line, "x_coordinate", 12))
      {
        if (!this->Read(&ncoords))
        {
          svtkErrorMacro(<< "Error reading x coordinates!");
          this->CloseSVTKFile();
          return 1;
        }

        this->ReadCoordinates(output, 0, ncoords);
      }

      else if (!strncmp(line, "y_coordinate", 12))
      {
        if (!this->Read(&ncoords))
        {
          svtkErrorMacro(<< "Error reading y coordinates!");
          this->CloseSVTKFile();
          return 1;
        }

        this->ReadCoordinates(output, 1, ncoords);
      }

      else if (!strncmp(line, "z_coordinate", 12))
      {
        if (!this->Read(&ncoords))
        {
          svtkErrorMacro(<< "Error reading z coordinates!");
          this->CloseSVTKFile();
          return 1;
        }

        this->ReadCoordinates(output, 2, ncoords);
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
        if (!this->Read(&npts))
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
    if (!output->GetXCoordinates() || output->GetXCoordinates()->GetNumberOfTuples() < 1)
    {
      svtkWarningMacro(<< "No x coordinatess read.");
    }
    if (!output->GetYCoordinates() || output->GetYCoordinates()->GetNumberOfTuples() < 1)
    {
      svtkWarningMacro(<< "No y coordinates read.");
    }
    if (!output->GetZCoordinates() || output->GetZCoordinates()->GetNumberOfTuples() < 1)
    {
      svtkWarningMacro(<< "No z coordinates read.");
    }
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
int svtkRectilinearGridReader::FillOutputPortInformation(int, svtkInformation* info)
{
  info->Set(svtkDataObject::DATA_TYPE_NAME(), "svtkRectilinearGrid");
  return 1;
}

//----------------------------------------------------------------------------
void svtkRectilinearGridReader::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
