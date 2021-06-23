/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkUnstructuredGridReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkUnstructuredGridReader.h"

#include "svtkByteSwap.h"
#include "svtkCellArray.h"
#include "svtkFieldData.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkObjectFactory.h"
#include "svtkStreamingDemandDrivenPipeline.h"
#include "svtkUnstructuredGrid.h"
#include "svtkUpdateCellsV8toV9.h"

#include <vector>

svtkStandardNewMacro(svtkUnstructuredGridReader);

#ifdef read
#undef read
#endif

//----------------------------------------------------------------------------
svtkUnstructuredGridReader::svtkUnstructuredGridReader() = default;

//----------------------------------------------------------------------------
svtkUnstructuredGridReader::~svtkUnstructuredGridReader() = default;

//----------------------------------------------------------------------------
svtkUnstructuredGrid* svtkUnstructuredGridReader::GetOutput()
{
  return this->GetOutput(0);
}

//----------------------------------------------------------------------------
svtkUnstructuredGrid* svtkUnstructuredGridReader::GetOutput(int idx)
{
  return svtkUnstructuredGrid::SafeDownCast(this->GetOutputDataObject(idx));
}

//----------------------------------------------------------------------------
void svtkUnstructuredGridReader::SetOutput(svtkUnstructuredGrid* output)
{
  this->GetExecutive()->SetOutputData(0, output);
}

//----------------------------------------------------------------------------
int svtkUnstructuredGridReader::ReadMeshSimple(const std::string& fname, svtkDataObject* doOutput)
{
  svtkIdType i, numPts = 0, numCells = 0;
  char line[256];
  svtkIdType npts, size = 0, ncells = 0;
  int piece, numPieces, skip1, read2, skip3, tmp;
  svtkCellArray* cells = nullptr;
  int* types = nullptr;
  svtkUnstructuredGrid* output = svtkUnstructuredGrid::SafeDownCast(doOutput);

  svtkDebugMacro(<< "Reading svtk unstructured grid...");

  if (!this->OpenSVTKFile(fname.c_str()) || !this->ReadHeader(fname.c_str()))
  {
    return 1;
  }

  // Read unstructured grid specific stuff
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

    if (strncmp(this->LowerCase(line), "unstructured_grid", 17))
    {
      svtkErrorMacro(<< "Cannot read dataset type: " << line);
      this->CloseSVTKFile();
      return 1;
    }

    // Might find points, cells, and cell types
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

        if (!this->ReadPointCoordinates(output, numPts))
        {
          this->CloseSVTKFile();
          return 1;
        }
      }

      else if (!strncmp(line, "cells", 5))
      {
        if (this->FileMajorVersion >= 5)
        {
          // Just read all of the cells. The legacy path goes through the
          // streaming API, but hardcodes piece / numpieces to 0/1...
          svtkSmartPointer<svtkCellArray> tmpCells;
          if (!this->ReadCells(tmpCells))
          {
            this->CloseSVTKFile();
            return 1;
          }

          cells = tmpCells;
          cells->Register(nullptr);
        }
        else // maj vers >= 5
        {    // we still want to support the pre-5.x cell format:
          piece = 0;
          numPieces = 1;
          if (!(this->Read(&ncells) && this->Read(&size)))
          {
            svtkErrorMacro(<< "Cannot read cells!");
            this->CloseSVTKFile();
            return 1;
          }

          // the number of ints to read before we get to the piece.
          skip1 = piece * ncells / numPieces;
          // the number of ints to read as part of piece.
          read2 = ((piece + 1) * ncells / numPieces) - skip1;
          // the number of ints after the piece
          skip3 = ncells - skip1 - read2;

          const std::size_t connSize = static_cast<std::size_t>(size);
          std::vector<int> tempArray(connSize);
          std::vector<svtkIdType> idArray(static_cast<std::size_t>(size));

          if (!this->ReadCellsLegacy(size, tempArray.data(), skip1, read2, skip3))
          {
            this->CloseSVTKFile();
            return 1;
          }

          for (std::size_t connIdx = 0; connIdx < connSize; connIdx++)
          {
            idArray[connIdx] = static_cast<svtkIdType>(tempArray[connIdx]);
          }

          cells = svtkCellArray::New();
          cells->ImportLegacyFormat(idArray.data(), size);
        }

        // Update the dataset
        if (cells && types)
        {
          output->SetCells(types, cells);
        }
      }

      else if (!strncmp(line, "cell_types", 10))
      {
        piece = 0;
        numPieces = 1;
        if (!this->Read(&ncells))
        {
          svtkErrorMacro(<< "Cannot read cell types!");
          this->CloseSVTKFile();
          return 1;
        }
        // the number of ints to read before we get to the piece.
        skip1 = piece * ncells / numPieces;
        // the number of ints to read as part of piece.
        read2 = ((piece + 1) * ncells / numPieces) - skip1;
        // the number of ints after the piece
        skip3 = ncells - skip1 - read2;

        // cerr << skip1 << " --- " << read2 << " --- " << skip3 << endl;
        // allocate array for piece cell types
        types = new int[read2];
        if (this->GetFileType() == SVTK_BINARY)
        {
          // suck up newline
          this->GetIStream()->getline(line, 256);
          // skip
          if (skip1 != 0)
          {
            this->GetIStream()->seekg((long)sizeof(int) * skip1, ios::cur);
          }
          this->GetIStream()->read((char*)types, sizeof(int) * read2);
          // skip
          if (skip3 != 0)
          {
            this->GetIStream()->seekg((long)sizeof(int) * skip3, ios::cur);
          }

          if (this->GetIStream()->eof())
          {
            svtkErrorMacro(<< "Error reading binary cell types!");
            this->CloseSVTKFile();
            return 1;
          }
          svtkByteSwap::Swap4BERange(types, read2);
        }
        else // ascii
        {
          // skip types before piece
          for (i = 0; i < skip1; i++)
          {
            if (!this->Read(&tmp))
            {
              svtkErrorMacro(<< "Error reading cell types!");
              this->CloseSVTKFile();
              return 1;
            }
          }
          // read types for piece
          for (i = 0; i < read2; i++)
          {
            if (!this->Read(types + i))
            {
              svtkErrorMacro(<< "Error reading cell types!");
              this->CloseSVTKFile();
              return 1;
            }
          }
          // skip types after piece
          for (i = 0; i < skip3; i++)
          {
            if (!this->Read(&tmp))
            {
              svtkErrorMacro(<< "Error reading cell types!");
              this->CloseSVTKFile();
              return 1;
            }
          }
        }
        if (cells && types)
        {
          output->SetCells(types, cells);
        }
      }

      else if (!strncmp(line, "cell_data", 9))
      {
        if (!this->Read(&numCells))
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
    if (!output->GetPoints())
      svtkWarningMacro(<< "No points read!");
    // if ( ! (cells && types) )  svtkWarningMacro(<<"No topology read!");
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

  // Permute node numbering on higher order hexahedra for legacy files (see
  // https://gitlab.kitware.com/svtk/svtk/-/merge_requests/6678 )
  if (this->FileMajorVersion < 5 || (this->FileMajorVersion == 5 && this->FileMinorVersion < 1))
  {
    svtkUpdateCellsV8toV9(output);
  }

  // Clean-up and get out
  //
  delete[] types;
  if (cells)
  {
    cells->Delete();
  }

  svtkDebugMacro(<< "Read " << output->GetNumberOfPoints() << " points,"
                << output->GetNumberOfCells() << " cells.\n");

  this->CloseSVTKFile();
  return 1;
}

//----------------------------------------------------------------------------
int svtkUnstructuredGridReader::FillOutputPortInformation(int, svtkInformation* info)
{
  info->Set(svtkDataObject::DATA_TYPE_NAME(), "svtkUnstructuredGrid");
  return 1;
}

//----------------------------------------------------------------------------
void svtkUnstructuredGridReader::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
