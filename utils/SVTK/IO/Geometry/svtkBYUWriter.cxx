/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkBYUWriter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkBYUWriter.h"

#include "svtkCellArray.h"
#include "svtkErrorCode.h"
#include "svtkInformation.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkPolyData.h"
#include <svtksys/SystemTools.hxx>

#if !defined(_WIN32) || defined(__CYGWIN__)
#include <unistd.h> /* unlink */
#else
#include <io.h> /* unlink */
#endif

#include <string>

svtkStandardNewMacro(svtkBYUWriter);

// Create object so that it writes displacement, scalar, and texture files
// (if data is available).
svtkBYUWriter::svtkBYUWriter()
{
  this->GeometryFileName = nullptr;
  this->DisplacementFileName = nullptr;
  this->ScalarFileName = nullptr;
  this->TextureFileName = nullptr;

  this->WriteDisplacement = 1;
  this->WriteScalar = 1;
  this->WriteTexture = 1;
}

svtkBYUWriter::~svtkBYUWriter()
{
  delete[] this->GeometryFileName;
  delete[] this->DisplacementFileName;
  delete[] this->ScalarFileName;
  delete[] this->TextureFileName;
}

// Write out data in MOVIE.BYU format.
void svtkBYUWriter::WriteData()
{
  FILE* geomFp;
  svtkPolyData* input = this->GetInput();

  int numPts = input->GetNumberOfPoints();

  if (numPts < 1)
  {
    svtkErrorMacro(<< "No data to write!");
    return;
  }

  if (!this->GeometryFileName)
  {
    svtkErrorMacro(<< "Geometry file name was not specified");
    this->SetErrorCode(svtkErrorCode::NoFileNameError);
    return;
  }

  if ((geomFp = svtksys::SystemTools::Fopen(this->GeometryFileName, "w")) == nullptr)
  {
    svtkErrorMacro(<< "Couldn't open geometry file: " << this->GeometryFileName);
    this->SetErrorCode(svtkErrorCode::CannotOpenFileError);
    return;
  }
  else
  {
    this->WriteGeometryFile(geomFp, numPts);
    if (this->ErrorCode == svtkErrorCode::OutOfDiskSpaceError)
    {
      fclose(geomFp);
      svtkErrorMacro("Ran out of disk space; deleting file: " << this->GeometryFileName);
      unlink(this->GeometryFileName);
      return;
    }
  }

  this->WriteDisplacementFile(numPts);
  if (this->ErrorCode == svtkErrorCode::OutOfDiskSpaceError)
  {
    fclose(geomFp);
    unlink(this->GeometryFileName);
    unlink(this->DisplacementFileName);
    svtkErrorMacro("Ran out of disk space; deleting files: " << this->GeometryFileName << " "
                                                            << this->DisplacementFileName);
    return;
  }
  this->WriteScalarFile(numPts);
  if (this->ErrorCode == svtkErrorCode::OutOfDiskSpaceError)
  {
    std::string errorMessage;
    fclose(geomFp);
    unlink(this->GeometryFileName);
    errorMessage = "Ran out of disk space; deleting files: ";
    errorMessage += this->GeometryFileName;
    errorMessage += " ";
    if (this->DisplacementFileName)
    {
      unlink(this->DisplacementFileName);
      errorMessage += this->DisplacementFileName;
      errorMessage += " ";
    }
    unlink(this->ScalarFileName);
    errorMessage += this->ScalarFileName;
    svtkErrorMacro(<< errorMessage.c_str());
    return;
  }
  this->WriteTextureFile(numPts);
  if (this->ErrorCode == svtkErrorCode::OutOfDiskSpaceError)
  {
    fclose(geomFp);
    std::string errorMessage;
    unlink(this->GeometryFileName);
    errorMessage = "Ran out of disk space; deleting files: ";
    errorMessage += this->GeometryFileName;
    errorMessage += " ";
    if (this->DisplacementFileName)
    {
      unlink(this->DisplacementFileName);
      errorMessage += this->DisplacementFileName;
      errorMessage += " ";
    }
    if (this->ScalarFileName)
    {
      unlink(this->ScalarFileName);
      errorMessage += this->ScalarFileName;
      errorMessage += " ";
    }
    unlink(this->TextureFileName);
    errorMessage += this->TextureFileName;
    svtkErrorMacro(<< errorMessage.c_str());
    return;
  }

  // Close the file
  fclose(geomFp);
}

void svtkBYUWriter::WriteGeometryFile(FILE* geomFile, int numPts)
{
  int numPolys, numEdges;
  int i;
  double* x;
  svtkIdType npts = 0;
  const svtkIdType* pts = nullptr;
  svtkPoints* inPts;
  svtkCellArray* inPolys;
  svtkPolyData* input = this->GetInput();
  //
  // Check input
  //
  inPolys = input->GetPolys();
  if ((inPts = input->GetPoints()) == nullptr || inPolys == nullptr)
  {
    svtkErrorMacro(<< "No data to write!");
    return;
  }
  //
  // Write header (not using fixed format! - potential problem in some files.)
  //
  numPolys = input->GetPolys()->GetNumberOfCells();
  for (numEdges = 0, inPolys->InitTraversal(); inPolys->GetNextCell(npts, pts);)
  {
    numEdges += npts;
  }

  if (fprintf(geomFile, "%d %d %d %d\n", 1, numPts, numPolys, numEdges) < 0)
  {
    this->SetErrorCode(svtkErrorCode::OutOfDiskSpaceError);
    return;
  }
  if (fprintf(geomFile, "%d %d\n", 1, numPolys) < 0)
  {
    this->SetErrorCode(svtkErrorCode::OutOfDiskSpaceError);
    return;
  }

  //
  // Write data
  //
  // write point coordinates
  for (i = 0; i < numPts; i++)
  {
    x = inPts->GetPoint(i);
    if (fprintf(geomFile, "%e %e %e ", x[0], x[1], x[2]) < 0)
    {
      this->SetErrorCode(svtkErrorCode::OutOfDiskSpaceError);
      return;
    }
    if ((i % 2))
    {
      if (fprintf(geomFile, "\n") < 0)
      {
        this->SetErrorCode(svtkErrorCode::OutOfDiskSpaceError);
        return;
      }
    }
  }
  if ((numPts % 2))
  {
    if (fprintf(geomFile, "\n") < 0)
    {
      this->SetErrorCode(svtkErrorCode::OutOfDiskSpaceError);
      return;
    }
  }

  // write poly data. Remember 1-offset.
  for (inPolys->InitTraversal(); inPolys->GetNextCell(npts, pts);)
  {
    // write this polygon
    // treating svtkIdType as int
    for (i = 0; i < (npts - 1); i++)
    {
      if (fprintf(geomFile, "%d ", static_cast<int>(pts[i] + 1)) < 0)
      {
        this->SetErrorCode(svtkErrorCode::OutOfDiskSpaceError);
        return;
      }
    }
    if (fprintf(geomFile, "%d\n", static_cast<int>(-(pts[npts - 1] + 1))) < 0)
    {
      this->SetErrorCode(svtkErrorCode::OutOfDiskSpaceError);
      return;
    }
  }

  svtkDebugMacro(<< "Wrote " << numPts << " points, " << numPolys << " polygons");
}

void svtkBYUWriter::WriteDisplacementFile(int numPts)
{
  FILE* dispFp;
  int i;
  double* v;
  svtkDataArray* inVectors;
  svtkPolyData* input = this->GetInput();

  if (this->WriteDisplacement && this->DisplacementFileName &&
    (inVectors = input->GetPointData()->GetVectors()) != nullptr)
  {
    if (!(dispFp = svtksys::SystemTools::Fopen(this->DisplacementFileName, "w")))
    {
      svtkErrorMacro(<< "Couldn't open displacement file");
      this->SetErrorCode(svtkErrorCode::CannotOpenFileError);
      return;
    }
  }
  else
  {
    return;
  }
  //
  // Write data
  //
  for (i = 0; i < numPts; i++)
  {
    v = inVectors->GetTuple(i);
    if (fprintf(dispFp, "%e %e %e", v[0], v[1], v[2]) < 0)
    {
      this->SetErrorCode(svtkErrorCode::OutOfDiskSpaceError);
      fclose(dispFp);
      return;
    }
    if ((i % 2))
    {
      if (fprintf(dispFp, "\n") < 0)
      {
        this->SetErrorCode(svtkErrorCode::OutOfDiskSpaceError);
        fclose(dispFp);
        return;
      }
    }
  }

  svtkDebugMacro(<< "Wrote " << numPts << " displacements");
  fclose(dispFp);
}

void svtkBYUWriter::WriteScalarFile(int numPts)
{
  FILE* scalarFp;
  int i;
  float s;
  svtkDataArray* inScalars;
  svtkPolyData* input = this->GetInput();

  if (this->WriteScalar && this->ScalarFileName &&
    (inScalars = input->GetPointData()->GetScalars()) != nullptr)
  {
    if (!(scalarFp = svtksys::SystemTools::Fopen(this->ScalarFileName, "w")))
    {
      svtkErrorMacro(<< "Couldn't open scalar file");
      this->SetErrorCode(svtkErrorCode::CannotOpenFileError);
      return;
    }
  }
  else
  {
    return;
  }
  //
  // Write data
  //
  for (i = 0; i < numPts; i++)
  {
    s = inScalars->GetComponent(i, 0);
    if (fprintf(scalarFp, "%e ", s) < 0)
    {
      this->SetErrorCode(svtkErrorCode::OutOfDiskSpaceError);
      fclose(scalarFp);
      return;
    }
    if (i != 0 && !(i % 6))
    {
      if (fprintf(scalarFp, "\n") < 0)
      {
        this->SetErrorCode(svtkErrorCode::OutOfDiskSpaceError);
        fclose(scalarFp);
        return;
      }
    }
  }

  fclose(scalarFp);
  svtkDebugMacro(<< "Wrote " << numPts << " scalars");
}

void svtkBYUWriter::WriteTextureFile(int numPts)
{
  FILE* textureFp;
  int i;
  double* t;
  svtkDataArray* inTCoords;
  svtkPolyData* input = this->GetInput();

  if (this->WriteTexture && this->TextureFileName &&
    (inTCoords = input->GetPointData()->GetTCoords()) != nullptr)
  {
    if (!(textureFp = svtksys::SystemTools::Fopen(this->TextureFileName, "w")))
    {
      svtkErrorMacro(<< "Couldn't open texture file");
      this->SetErrorCode(svtkErrorCode::CannotOpenFileError);
      return;
    }
  }
  else
  {
    return;
  }
  //
  // Write data
  //
  for (i = 0; i < numPts; i++)
  {
    if (i != 0 && !(i % 3))
    {
      if (fprintf(textureFp, "\n") < 0)
      {
        this->SetErrorCode(svtkErrorCode::OutOfDiskSpaceError);
        fclose(textureFp);
        return;
      }
    }
    t = inTCoords->GetTuple(i);
    if (fprintf(textureFp, "%e %e", t[0], t[1]) < 0)
    {
      this->SetErrorCode(svtkErrorCode::OutOfDiskSpaceError);
      fclose(textureFp);
      return;
    }
  }

  fclose(textureFp);
  svtkDebugMacro(<< "Wrote " << numPts << " texture coordinates");
}

void svtkBYUWriter::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent
     << "Geometry File Name: " << (this->GeometryFileName ? this->GeometryFileName : "(none)")
     << "\n";

  os << indent << "Write Displacement: " << (this->WriteDisplacement ? "On\n" : "Off\n");
  os << indent << "Displacement File Name: "
     << (this->DisplacementFileName ? this->DisplacementFileName : "(none)") << "\n";

  os << indent << "Write Scalar: " << (this->WriteScalar ? "On\n" : "Off\n");
  os << indent << "Scalar File Name: " << (this->ScalarFileName ? this->ScalarFileName : "(none)")
     << "\n";

  os << indent << "Write Texture: " << (this->WriteTexture ? "On\n" : "Off\n");
  os << indent
     << "Texture File Name: " << (this->TextureFileName ? this->TextureFileName : "(none)") << "\n";
}

svtkPolyData* svtkBYUWriter::GetInput()
{
  return svtkPolyData::SafeDownCast(this->Superclass::GetInput());
}

svtkPolyData* svtkBYUWriter::GetInput(int port)
{
  return svtkPolyData::SafeDownCast(this->Superclass::GetInput(port));
}

int svtkBYUWriter::FillInputPortInformation(int, svtkInformation* info)
{
  info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkPolyData");
  return 1;
}
