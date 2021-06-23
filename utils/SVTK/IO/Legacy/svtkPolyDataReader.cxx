/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPolyDataReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkPolyDataReader.h"

#include "svtkCellArray.h"
#include "svtkFieldData.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkObjectFactory.h"
#include "svtkPolyData.h"
#include "svtkStreamingDemandDrivenPipeline.h"

#include <vector>

svtkStandardNewMacro(svtkPolyDataReader);

//----------------------------------------------------------------------------
svtkPolyDataReader::svtkPolyDataReader() = default;

//----------------------------------------------------------------------------
svtkPolyDataReader::~svtkPolyDataReader() = default;

//----------------------------------------------------------------------------
svtkPolyData* svtkPolyDataReader::GetOutput()
{
  return this->GetOutput(0);
}

//----------------------------------------------------------------------------
svtkPolyData* svtkPolyDataReader::GetOutput(int idx)
{
  return svtkPolyData::SafeDownCast(this->GetOutputDataObject(idx));
}

//----------------------------------------------------------------------------
void svtkPolyDataReader::SetOutput(svtkPolyData* output)
{
  this->GetExecutive()->SetOutputData(0, output);
}

//----------------------------------------------------------------------------
int svtkPolyDataReader::ReadMeshSimple(const std::string& fname, svtkDataObject* doOutput)
{
  svtkIdType numPts = 0;
  char line[256];
  svtkIdType npts, size = 0, ncells;
  svtkPolyData* output = svtkPolyData::SafeDownCast(doOutput);

  // Helper function to handle legacy cell data fallback:
  auto readCellArray = [&](svtkSmartPointer<svtkCellArray>& cellArray) -> bool {
    if (this->FileMajorVersion >= 5)
    { // Cells are written as offsets + connectivity arrays:
      return this->ReadCells(cellArray) != 0;
    }
    else
    { // Import cells from legacy format:
      if (!(this->Read(&ncells) && this->Read(&size)))
      {
        return false;
      }

      std::size_t connSize = static_cast<std::size_t>(size);
      std::vector<int> tempArray(connSize);
      std::vector<svtkIdType> idArray(connSize);

      if (!this->ReadCellsLegacy(size, tempArray.data()))
      {
        this->CloseSVTKFile();
        return 1;
      }

      // Convert to id type
      for (std::size_t connIdx = 0; connIdx < connSize; connIdx++)
      {
        idArray[connIdx] = static_cast<svtkIdType>(tempArray[connIdx]);
      }

      cellArray = svtkSmartPointer<svtkCellArray>::New();
      cellArray->ImportLegacyFormat(idArray.data(), size);
      return true;
    } // end legacy cell read
  };

  svtkDebugMacro(<< "Reading svtk polygonal data...");

  if (!(this->OpenSVTKFile(fname.c_str())) || !this->ReadHeader(fname.c_str()))
  {
    return 1;
  }
  //
  // Read polygonal data specific stuff
  //
  if (!this->ReadString(line))
  {
    svtkErrorMacro(<< "Data file ends prematurely!");
    this->CloseSVTKFile();
    return 1;
  }

  if (!strncmp(this->LowerCase(line), "dataset", (unsigned long)7))
  {
    //
    // Make sure we're reading right type of geometry
    //
    if (!this->ReadString(line))
    {
      svtkErrorMacro(<< "Data file ends prematurely!");
      this->CloseSVTKFile();
      return 1;
    }

    if (strncmp(this->LowerCase(line), "polydata", 8))
    {
      svtkErrorMacro(<< "Cannot read dataset type: " << line);
      this->CloseSVTKFile();
      return 1;
    }
    //
    // Might find points, vertices, lines, polygons, or triangle strips
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
      else if (!strncmp(line, "points", 6))
      {
        if (!this->Read(&numPts))
        {
          svtkErrorMacro(<< "Cannot read number of points!");
          this->CloseSVTKFile();
          return 1;
        }

        this->ReadPointCoordinates(output, numPts);
      }
      else if (!strncmp(line, "vertices", 8))
      {
        svtkSmartPointer<svtkCellArray> cells;
        if (!readCellArray(cells))
        {
          svtkErrorMacro("Error reading vertices.");
          this->CloseSVTKFile();
          return 1;
        }
        output->SetVerts(cells);
        svtkDebugMacro("Read " << cells->GetNumberOfCells() << " vertices");
      }

      else if (!strncmp(line, "lines", 5))
      {
        svtkSmartPointer<svtkCellArray> cells;
        if (!readCellArray(cells))
        {
          svtkErrorMacro("Error reading lines.");
          this->CloseSVTKFile();
          return 1;
        }
        output->SetLines(cells);
        svtkDebugMacro("Read " << cells->GetNumberOfCells() << " lines");
      }

      else if (!strncmp(line, "polygons", 8))
      {
        svtkSmartPointer<svtkCellArray> cells;
        if (!readCellArray(cells))
        {
          svtkErrorMacro("Error reading polygons.");
          this->CloseSVTKFile();
          return 1;
        }
        output->SetPolys(cells);
        svtkDebugMacro("Read " << cells->GetNumberOfCells() << " polygons");
      }

      else if (!strncmp(line, "triangle_strips", 15))
      {
        svtkSmartPointer<svtkCellArray> cells;
        if (!readCellArray(cells))
        {
          svtkErrorMacro("Error reading triangle_strips.");
          this->CloseSVTKFile();
          return 1;
        }
        output->SetStrips(cells);
        svtkDebugMacro("Read " << cells->GetNumberOfCells() << " triangle strips");
      }

      else if (!strncmp(line, "cell_data", 9))
      {
        if (!this->Read(&ncells))
        {
          svtkErrorMacro(<< "Cannot read cell data!");
          this->CloseSVTKFile();
          return 1;
        }

        if (ncells != output->GetNumberOfCells())
        {
          svtkErrorMacro(<< "Number of cells don't match number data values!");
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
          svtkErrorMacro(<< "Number of points don't match number data values!");
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

    if (!output->GetPoints())
      svtkWarningMacro(<< "No points read!");
    if (!(output->GetVerts() || output->GetLines() || output->GetPolys() || output->GetStrips()))
      svtkWarningMacro(<< "No topology read!");
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
    if (!this->Read(&numPts))
    {
      svtkErrorMacro(<< "Cannot read point data!");
      this->CloseSVTKFile();
      return 1;
    }

    this->ReadPointData(output, numPts);
  }

  else
  {
    svtkErrorMacro(<< "Unrecognized keyword: " << line);
  }
  this->CloseSVTKFile();

  return 1;
}

//----------------------------------------------------------------------------
int svtkPolyDataReader::FillOutputPortInformation(int, svtkInformation* info)
{
  info->Set(svtkDataObject::DATA_TYPE_NAME(), "svtkPolyData");
  return 1;
}

//----------------------------------------------------------------------------
void svtkPolyDataReader::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
