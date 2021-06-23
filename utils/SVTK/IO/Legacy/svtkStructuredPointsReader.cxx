/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkStructuredPointsReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkStructuredPointsReader.h"

#include "svtkDataArray.h"
#include "svtkErrorCode.h"
#include "svtkFieldData.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkStreamingDemandDrivenPipeline.h"
#include "svtkStructuredPoints.h"

svtkStandardNewMacro(svtkStructuredPointsReader);

svtkStructuredPointsReader::svtkStructuredPointsReader() = default;
svtkStructuredPointsReader::~svtkStructuredPointsReader() = default;

//----------------------------------------------------------------------------
void svtkStructuredPointsReader::SetOutput(svtkStructuredPoints* output)
{
  this->GetExecutive()->SetOutputData(0, output);
}

//----------------------------------------------------------------------------
svtkStructuredPoints* svtkStructuredPointsReader::GetOutput()
{
  return this->GetOutput(0);
}

//----------------------------------------------------------------------------
svtkStructuredPoints* svtkStructuredPointsReader::GetOutput(int idx)
{
  return svtkStructuredPoints::SafeDownCast(this->GetOutputDataObject(idx));
}

//----------------------------------------------------------------------------
int svtkStructuredPointsReader::ReadMetaDataSimple(
  const std::string& fname, svtkInformation* metadata)
{
  this->SetErrorCode(svtkErrorCode::NoError);

  char line[256];
  int dimsRead = 0, arRead = 0, originRead = 0;

  if (!this->OpenSVTKFile(fname.c_str()) || !this->ReadHeader(fname.c_str()))
  {
    return 1;
  }

  // Read structured points specific stuff
  if (!this->ReadString(line))
  {
    svtkErrorMacro(<< "Data file ends prematurely!");
    this->CloseSVTKFile();
    this->SetErrorCode(svtkErrorCode::PrematureEndOfFileError);
    return 1;
  }

  if (!strncmp(this->LowerCase(line), "dataset", (unsigned long)7))
  {
    // Make sure we're reading right type of geometry
    if (!this->ReadString(line))
    {
      svtkErrorMacro(<< "Data file ends prematurely!");
      this->CloseSVTKFile();
      this->SetErrorCode(svtkErrorCode::PrematureEndOfFileError);
      return 1;
    }

    if (strncmp(this->LowerCase(line), "structured_points", 17))
    {
      svtkErrorMacro(<< "Cannot read dataset type: " << line);
      this->CloseSVTKFile();
      this->SetErrorCode(svtkErrorCode::UnrecognizedFileTypeError);
      return 1;
    }

    // Read keyword and number of points
    while (true)
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

      else if (!strncmp(line, "aspect_ratio", 12) || !strncmp(line, "spacing", 7))
      {
        double ar[3];
        if (!(this->Read(ar) && this->Read(ar + 1) && this->Read(ar + 2)))
        {
          svtkErrorMacro(<< "Error reading spacing!");
          this->CloseSVTKFile();
          this->SetErrorCode(svtkErrorCode::FileFormatError);
          return 1;
        }
        metadata->Set(svtkDataObject::SPACING(), ar, 3);
        arRead = 1;
      }

      else if (!strncmp(line, "origin", 6))
      {
        double origin[3];
        if (!(this->Read(origin) && this->Read(origin + 1) && this->Read(origin + 2)))
        {
          svtkErrorMacro(<< "Error reading origin!");
          this->CloseSVTKFile();
          this->SetErrorCode(svtkErrorCode::FileFormatError);
          return 1;
        }
        metadata->Set(svtkDataObject::ORIGIN(), origin, 3);
        originRead = 1;
      }

      else if (!strncmp(line, "point_data", 10))
      {
        svtkIdType npts;
        if (!this->Read(&npts))
        {
          svtkErrorMacro(<< "Cannot read point data!");
          this->CloseSVTKFile();
          this->SetErrorCode(svtkErrorCode::FileFormatError);
          return 1;
        }

        while (this->ReadString(line))
        {
          // read scalar data
          //
          if (!strncmp(this->LowerCase(line), "scalars", 7))
          {
            int scalarType = SVTK_DOUBLE;
            this->ReadString(line);
            this->ReadString(line);

            if (!strncmp(line, "bit", 3))
            {
              scalarType = SVTK_BIT;
            }
            else if (!strncmp(line, "char", 4))
            {
              scalarType = SVTK_CHAR;
            }
            else if (!strncmp(line, "unsigned_char", 13))
            {
              scalarType = SVTK_UNSIGNED_CHAR;
            }
            else if (!strncmp(line, "short", 5))
            {
              scalarType = SVTK_SHORT;
            }
            else if (!strncmp(line, "unsigned_short", 14))
            {
              scalarType = SVTK_UNSIGNED_SHORT;
            }
            else if (!strncmp(line, "int", 3))
            {
              scalarType = SVTK_INT;
            }
            else if (!strncmp(line, "unsigned_int", 12))
            {
              scalarType = SVTK_UNSIGNED_INT;
            }
            else if (!strncmp(line, "long", 4))
            {
              scalarType = SVTK_LONG;
            }
            else if (!strncmp(line, "unsigned_long", 13))
            {
              scalarType = SVTK_UNSIGNED_LONG;
            }
            else if (!strncmp(line, "float", 5))
            {
              scalarType = SVTK_FLOAT;
            }
            else if (!strncmp(line, "double", 6))
            {
              scalarType = SVTK_DOUBLE;
            }

            // the next string could be an integer number of components or a
            // lookup table
            this->ReadString(line);
            int numComp;
            if (strcmp(this->LowerCase(line), "lookup_table"))
            {
              numComp = atoi(line);
              if (numComp < 1 || !this->ReadString(line))
              {
                svtkErrorMacro(<< "Cannot read scalar header!"
                              << " for file: " << fname);
                return 1;
              }
            }
            else
            {
              numComp = 1;
            }

            svtkDataObject::SetPointDataActiveScalarInfo(metadata, scalarType, numComp);
            break;
          }
          else if (!strncmp(this->LowerCase(line), "color_scalars", 13))
          {
            this->ReadString(line);
            this->ReadString(line);
            int numComp = atoi(line);
            if (numComp < 1)
            {
              svtkErrorMacro("Cannot read color_scalar header!"
                << " for file: " << fname);
              return 1;
            }

            // Color scalar type is predefined by FileType.
            int scalarType;
            if (this->FileType == SVTK_BINARY)
            {
              scalarType = SVTK_UNSIGNED_CHAR;
            }
            else
            {
              scalarType = SVTK_FLOAT;
            }

            svtkDataObject::SetPointDataActiveScalarInfo(metadata, scalarType, numComp);
            break;
          }
        }
        break; // out of this loop
      }
    }

    if (!dimsRead || !arRead || !originRead)
    {
      svtkWarningMacro(<< "Not all meta data was read from the file.");
    }
  }

  this->CloseSVTKFile();

  return 1;
}

int svtkStructuredPointsReader::ReadMeshSimple(const std::string& fname, svtkDataObject* doOutput)
{
  this->SetErrorCode(svtkErrorCode::NoError);
  svtkIdType numPts = 0, numCells = 0;
  char line[256];
  svtkIdType npts, ncells;
  int dimsRead = 0, arRead = 0, originRead = 0;
  svtkStructuredPoints* output = svtkStructuredPoints::SafeDownCast(doOutput);

  // ImageSource superclass does not do this.
  output->ReleaseData();

  svtkDebugMacro(<< "Reading svtk structured points file...");

  if (!this->OpenSVTKFile(fname.c_str()) || !this->ReadHeader(fname.c_str()))
  {
    return 1;
  }

  // Read structured points specific stuff
  //
  if (!this->ReadString(line))
  {
    svtkErrorMacro(<< "Data file ends prematurely!");
    this->CloseSVTKFile();
    this->SetErrorCode(svtkErrorCode::PrematureEndOfFileError);
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
      this->SetErrorCode(svtkErrorCode::PrematureEndOfFileError);
      return 1;
    }

    if (strncmp(this->LowerCase(line), "structured_points", 17))
    {
      svtkErrorMacro(<< "Cannot read dataset type: " << line);
      this->CloseSVTKFile();
      this->SetErrorCode(svtkErrorCode::UnrecognizedFileTypeError);
      return 1;
    }

    // Read keyword and number of points
    //
    numPts = output->GetNumberOfPoints(); // get default
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

      else if (!strncmp(line, "aspect_ratio", 12) || !strncmp(line, "spacing", 7))
      {
        double ar[3];
        if (!(this->Read(ar) && this->Read(ar + 1) && this->Read(ar + 2)))
        {
          svtkErrorMacro(<< "Error reading spacing!");
          this->CloseSVTKFile();
          this->SetErrorCode(svtkErrorCode::FileFormatError);
          return 1;
        }

        output->SetSpacing(ar);
        arRead = 1;
      }

      else if (!strncmp(line, "origin", 6))
      {
        double origin[3];
        if (!(this->Read(origin) && this->Read(origin + 1) && this->Read(origin + 2)))
        {
          svtkErrorMacro(<< "Error reading origin!");
          this->CloseSVTKFile();
          this->SetErrorCode(svtkErrorCode::FileFormatError);
          return 1;
        }

        output->SetOrigin(origin);
        originRead = 1;
      }

      else if (!strncmp(line, "cell_data", 9))
      {
        if (!this->Read(&ncells))
        {
          svtkErrorMacro(<< "Cannot read cell data!");
          this->CloseSVTKFile();
          this->SetErrorCode(svtkErrorCode::FileFormatError);
          return 1;
        }

        if (ncells != numCells)
        {
          svtkErrorMacro(<< "Number of cells don't match data values!");
          this->CloseSVTKFile();
          this->SetErrorCode(svtkErrorCode::FileFormatError);
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
          this->SetErrorCode(svtkErrorCode::FileFormatError);
          return 1;
        }

        if (npts != numPts)
        {
          svtkErrorMacro(<< "Number of points don't match data values!");
          this->CloseSVTKFile();
          this->SetErrorCode(svtkErrorCode::FileFormatError);
          return 1;
        }

        this->ReadPointData(output, npts);
        break; // out of this loop
      }

      else
      {
        svtkErrorMacro(<< "Unrecognized keyword: " << line);
        this->CloseSVTKFile();
        this->SetErrorCode(svtkErrorCode::FileFormatError);
        return 1;
      }
    }

    if (!dimsRead)
      svtkWarningMacro(<< "No dimensions read.");
    if (!arRead)
      svtkWarningMacro(<< "No spacing read.");
    if (!originRead)
      svtkWarningMacro(<< "No origin read.");
  }

  else if (!strncmp(line, "cell_data", 9))
  {
    svtkWarningMacro(<< "No geometry defined in data file!");
    if (!this->Read(&ncells))
    {
      svtkErrorMacro(<< "Cannot read cell data!");
      this->CloseSVTKFile();
      this->SetErrorCode(svtkErrorCode::FileFormatError);
      return 1;
    }
    this->ReadCellData(output, numCells);
  }

  else if (!strncmp(line, "point_data", 10))
  {
    svtkWarningMacro(<< "No geometry defined in data file!");
    if (!this->Read(&npts))
    {
      svtkErrorMacro(<< "Cannot read point data!");
      this->CloseSVTKFile();
      this->SetErrorCode(svtkErrorCode::FileFormatError);
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

int svtkStructuredPointsReader::FillOutputPortInformation(int, svtkInformation* info)
{
  info->Set(svtkDataObject::DATA_TYPE_NAME(), "svtkStructuredPoints");
  return 1;
}

void svtkStructuredPointsReader::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
