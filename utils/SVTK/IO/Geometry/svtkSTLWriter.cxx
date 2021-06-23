/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkSTLWriter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkSTLWriter.h"
#include "svtkSmartPointer.h"

#include "svtkByteSwap.h"
#include "svtkCellArray.h"
#include "svtkErrorCode.h"
#include "svtkInformation.h"
#include "svtkNew.h"
#include "svtkObjectFactory.h"
#include "svtkPolyData.h"
#include "svtkPolygon.h"
#include "svtkTriangle.h"
#include "svtkTriangleStrip.h"
#include "svtkUnsignedCharArray.h"
#include <svtksys/SystemTools.hxx>

#if !defined(_WIN32) || defined(__CYGWIN__)
#include <unistd.h> /* unlink */
#else
#include <io.h> /* unlink */
#endif

namespace
{
// For C format strings
constexpr int max_double_digits = std::numeric_limits<double>::max_digits10;
}

svtkStandardNewMacro(svtkSTLWriter);
svtkCxxSetObjectMacro(svtkSTLWriter, BinaryHeader, svtkUnsignedCharArray);

static char svtkSTLWriterDefaultHeader[] = "Visualization Toolkit generated SLA File";
static const int svtkSTLWriterBinaryHeaderSize = 80;

svtkSTLWriter::svtkSTLWriter()
{
  this->FileType = SVTK_ASCII;
  this->FileName = nullptr;
  this->Header = nullptr;
  this->SetHeader(svtkSTLWriterDefaultHeader);
  this->BinaryHeader = nullptr;
}

svtkSTLWriter::~svtkSTLWriter()
{
  this->SetFileName(nullptr);
  this->SetHeader(nullptr);
  this->SetBinaryHeader(nullptr);
}

void svtkSTLWriter::WriteData()
{
  svtkPoints* pts;
  svtkCellArray* polys;
  svtkCellArray* strips;
  svtkPolyData* input = this->GetInput();

  polys = input->GetPolys();
  strips = input->GetStrips();
  pts = input->GetPoints();
  if (pts == nullptr || polys == nullptr)
  {
    svtkErrorMacro(<< "No data to write!");
    this->SetErrorCode(svtkErrorCode::UnknownError);
    return;
  }

  if (this->FileName == nullptr)
  {
    svtkErrorMacro(<< "Please specify FileName to write");
    this->SetErrorCode(svtkErrorCode::NoFileNameError);
    return;
  }

  if (this->FileType == SVTK_BINARY)
  {
    this->WriteBinarySTL(pts, polys, strips);
    if (this->ErrorCode == svtkErrorCode::OutOfDiskSpaceError)
    {
      svtkErrorMacro("Ran out of disk space; deleting file: " << this->FileName);
      unlink(this->FileName);
    }
  }
  else
  {
    this->WriteAsciiSTL(pts, polys, strips);
    if (this->ErrorCode == svtkErrorCode::OutOfDiskSpaceError)
    {
      svtkErrorMacro("Ran out of disk space; deleting file: " << this->FileName);
      unlink(this->FileName);
    }
  }
}

void svtkSTLWriter::WriteAsciiSTL(svtkPoints* pts, svtkCellArray* polys, svtkCellArray* strips)
{
  FILE* fp;
  double n[3], v1[3], v2[3], v3[3];
  svtkIdType npts = 0;
  const svtkIdType* indx = nullptr;

  if ((fp = svtksys::SystemTools::Fopen(this->FileName, "w")) == nullptr)
  {
    svtkErrorMacro(<< "Couldn't open file: " << this->FileName
                  << " Reason: " << svtksys::SystemTools::GetLastSystemError());
    this->SetErrorCode(svtkErrorCode::CannotOpenFileError);
    return;
  }
  //
  //  Write header
  //
  svtkDebugMacro("Writing ASCII sla file");
  fprintf(fp, "solid ");
  if (this->GetHeader())
  {
    fprintf(fp, "%s", this->GetHeader());
  }
  fprintf(fp, "\n");

  //
  // Decompose any triangle strips into triangles
  //
  svtkSmartPointer<svtkCellArray> polyStrips = svtkSmartPointer<svtkCellArray>::New();
  if (strips->GetNumberOfCells() > 0)
  {
    const svtkIdType* ptIds = nullptr;
    for (strips->InitTraversal(); strips->GetNextCell(npts, ptIds);)
    {
      svtkTriangleStrip::DecomposeStrip(npts, ptIds, polyStrips);
    }
  }

  //  Write out triangle strips
  //
  for (polyStrips->InitTraversal(); polyStrips->GetNextCell(npts, indx);)
  {
    pts->GetPoint(indx[0], v1);
    pts->GetPoint(indx[1], v2);
    pts->GetPoint(indx[2], v3);

    svtkTriangle::ComputeNormal(pts, npts, indx, n);

    fprintf(fp, " facet normal %.*g %.*g %.*g\n  outer loop\n", max_double_digits, n[0],
      max_double_digits, n[1], max_double_digits, n[2]);
    fprintf(fp, "   vertex %.*g %.*g %.*g\n", max_double_digits, v1[0], max_double_digits, v1[1],
      max_double_digits, v1[2]);
    fprintf(fp, "   vertex %.*g %.*g %.*g\n", max_double_digits, v2[0], max_double_digits, v2[1],
      max_double_digits, v2[2]);
    fprintf(fp, "   vertex %.*g %.*g %.*g\n", max_double_digits, v3[0], max_double_digits, v3[1],
      max_double_digits, v3[2]);
    fprintf(fp, "  endloop\n endfacet\n");
  }

  // Write out triangle polygons. If not a triangle polygon, triangulate it
  // and write out the results.
  //
  for (polys->InitTraversal(); polys->GetNextCell(npts, indx);)
  {
    if (npts == 3)
    {
      pts->GetPoint(indx[0], v1);
      pts->GetPoint(indx[1], v2);
      pts->GetPoint(indx[2], v3);

      svtkTriangle::ComputeNormal(pts, npts, indx, n);

      fprintf(fp, " facet normal %.*g %.*g %.*g\n  outer loop\n", max_double_digits, n[0],
        max_double_digits, n[1], max_double_digits, n[2]);
      fprintf(fp, "   vertex %.*g %.*g %.*g\n", max_double_digits, v1[0], max_double_digits, v1[1],
        max_double_digits, v1[2]);
      fprintf(fp, "   vertex %.*g %.*g %.*g\n", max_double_digits, v2[0], max_double_digits, v2[1],
        max_double_digits, v2[2]);
      fprintf(fp, "   vertex %.*g %.*g %.*g\n", max_double_digits, v3[0], max_double_digits, v3[1],
        max_double_digits, v3[2]);
      fprintf(fp, "  endloop\n endfacet\n");
    }
    else if (npts > 3)
    {
      // Initialize the polygon.
      svtkNew<svtkPolygon> poly;
      poly->PointIds->SetNumberOfIds(npts);
      poly->Points->SetNumberOfPoints(npts);
      for (svtkIdType i = 0; i < npts; ++i)
      {
        poly->PointIds->SetId(i, indx[i]);
        poly->Points->SetPoint(i, pts->GetPoint(indx[i]));
      }

      // Do the triangulation
      svtkNew<svtkIdList> ptIds;
      ptIds->Allocate(SVTK_CELL_SIZE);
      poly->Triangulate(ptIds);

      svtkIdType numPts = ptIds->GetNumberOfIds();
      svtkIdType numSimplices = numPts / 3;
      for (svtkIdType i = 0; i < numSimplices; ++i)
      {
        svtkTriangle::ComputeNormal(pts, 3, ptIds->GetPointer(3 * i), n);

        fprintf(fp, " facet normal %.6g %.6g %.6g\n  outer loop\n", n[0], n[1], n[2]);

        for (svtkIdType j = 0; j < 3; ++j)
        {
          svtkIdType ptId = ptIds->GetId(3 * i + j);
          poly->GetPoints()->GetPoint(ptId, v1);
          fprintf(fp, "   vertex %.6g %.6g %.6g\n", v1[0], v1[1], v1[2]);
        }
        fprintf(fp, "  endloop\n endfacet\n");
      }
    }
  }

  fprintf(fp, "endsolid\n");
  if (fflush(fp))
  {
    fclose(fp);
    this->SetErrorCode(svtkErrorCode::OutOfDiskSpaceError);
    return;
  }
  fclose(fp);
}

void svtkSTLWriter::WriteBinarySTL(svtkPoints* pts, svtkCellArray* polys, svtkCellArray* strips)
{
  FILE* fp;
  double dn[3], v1[3], v2[3], v3[3];
  svtkIdType npts = 0;
  const svtkIdType* indx = nullptr;
  unsigned short ibuff2 = 0;

  if ((fp = svtksys::SystemTools::Fopen(this->FileName, "wb")) == nullptr)
  {
    svtkErrorMacro(<< "Couldn't open file: " << this->FileName
                  << " Reason: " << svtksys::SystemTools::GetLastSystemError());
    this->SetErrorCode(svtkErrorCode::CannotOpenFileError);
    return;
  }

  //  Write header
  //
  svtkDebugMacro("Writing Binary STL file");

  char binaryFileHeader[svtkSTLWriterBinaryHeaderSize + 1] = { 0 };

  // Check for STL ASCII format key word 'solid'. According to STL file format
  // only ASCII files can have 'solid' as start key word, so we ignore it and
  // use default SVTK header instead.

  if (this->BinaryHeader)
  {
    // Use binary header
    if (this->BinaryHeader->GetNumberOfValues() >= 5 &&
      memcmp(this->BinaryHeader->GetVoidPointer(0), "solid", 5) == 0)
    {
      svtkErrorMacro(
        "Invalid header for Binary STL file. Cannot start with \"solid\". Changing header to\n"
        << svtkSTLWriterDefaultHeader);
      strncpy(binaryFileHeader, svtkSTLWriterDefaultHeader, svtkSTLWriterBinaryHeaderSize);
    }
    else
    {
      svtkIdType numberOfValues =
        (this->BinaryHeader->GetNumberOfValues() <= svtkSTLWriterBinaryHeaderSize
            ? this->BinaryHeader->GetNumberOfValues()
            : svtkSTLWriterBinaryHeaderSize);
      memcpy(binaryFileHeader, this->BinaryHeader->GetVoidPointer(0), numberOfValues);
      if (numberOfValues < svtkSTLWriterBinaryHeaderSize)
      {
        memset(binaryFileHeader + numberOfValues, 0, svtkSTLWriterBinaryHeaderSize - numberOfValues);
      }
    }
  }
  else
  {
    // Use text header
    if (svtksys::SystemTools::StringStartsWith(this->Header, "solid"))
    {
      svtkErrorMacro(
        "Invalid header for Binary STL file. Cannot start with \"solid\". Changing header to\n"
        << svtkSTLWriterDefaultHeader);
      strncpy(binaryFileHeader, svtkSTLWriterDefaultHeader, svtkSTLWriterBinaryHeaderSize);
    }
    else
    {
      strncpy(binaryFileHeader, this->Header, svtkSTLWriterBinaryHeaderSize);
    }
  }

  fwrite(binaryFileHeader, 1, svtkSTLWriterBinaryHeaderSize, fp);

  //
  // Decompose any triangle strips into triangles
  //
  svtkSmartPointer<svtkCellArray> polyStrips = svtkSmartPointer<svtkCellArray>::New();
  if (strips->GetNumberOfCells() > 0)
  {
    const svtkIdType* ptIds = nullptr;
    for (strips->InitTraversal(); strips->GetNextCell(npts, ptIds);)
    {
      svtkTriangleStrip::DecomposeStrip(npts, ptIds, polyStrips);
    }
  }

  // Write a dummy length as a placeholder. Fill it in later after the number
  // of triangles is known.
  unsigned long numTris = 0;
  fwrite(&numTris, 1, 4, fp);

  //  Write out triangle strips
  //
  numTris += polyStrips->GetNumberOfCells();
  for (polyStrips->InitTraversal(); polyStrips->GetNextCell(npts, indx);)
  {
    pts->GetPoint(indx[0], v1);
    pts->GetPoint(indx[1], v2);
    pts->GetPoint(indx[2], v3);

    svtkTriangle::ComputeNormal(pts, npts, indx, dn);
    float n[3];
    n[0] = (float)dn[0];
    n[1] = (float)dn[1];
    n[2] = (float)dn[2];
    svtkByteSwap::Swap4LE(n);
    svtkByteSwap::Swap4LE(n + 1);
    svtkByteSwap::Swap4LE(n + 2);
    fwrite(n, 4, 3, fp);

    n[0] = (float)v1[0];
    n[1] = (float)v1[1];
    n[2] = (float)v1[2];
    svtkByteSwap::Swap4LE(n);
    svtkByteSwap::Swap4LE(n + 1);
    svtkByteSwap::Swap4LE(n + 2);
    fwrite(n, 4, 3, fp);

    n[0] = (float)v2[0];
    n[1] = (float)v2[1];
    n[2] = (float)v2[2];
    svtkByteSwap::Swap4LE(n);
    svtkByteSwap::Swap4LE(n + 1);
    svtkByteSwap::Swap4LE(n + 2);
    fwrite(n, 4, 3, fp);

    n[0] = (float)v3[0];
    n[1] = (float)v3[1];
    n[2] = (float)v3[2];
    svtkByteSwap::Swap4LE(n);
    svtkByteSwap::Swap4LE(n + 1);
    svtkByteSwap::Swap4LE(n + 2);
    fwrite(n, 4, 3, fp);

    fwrite(&ibuff2, 2, 1, fp);
  }

  // Write out triangle polygons. If not a triangle polygon, triangulate it
  // and write out the results.
  //
  for (polys->InitTraversal(); polys->GetNextCell(npts, indx);)
  {
    if (npts == 3)
    {
      pts->GetPoint(indx[0], v1);
      pts->GetPoint(indx[1], v2);
      pts->GetPoint(indx[2], v3);

      svtkTriangle::ComputeNormal(pts, npts, indx, dn);
      float n[3];
      n[0] = (float)dn[0];
      n[1] = (float)dn[1];
      n[2] = (float)dn[2];
      svtkByteSwap::Swap4LE(n);
      svtkByteSwap::Swap4LE(n + 1);
      svtkByteSwap::Swap4LE(n + 2);
      fwrite(n, 4, 3, fp);

      n[0] = (float)v1[0];
      n[1] = (float)v1[1];
      n[2] = (float)v1[2];
      svtkByteSwap::Swap4LE(n);
      svtkByteSwap::Swap4LE(n + 1);
      svtkByteSwap::Swap4LE(n + 2);
      fwrite(n, 4, 3, fp);

      n[0] = (float)v2[0];
      n[1] = (float)v2[1];
      n[2] = (float)v2[2];
      svtkByteSwap::Swap4LE(n);
      svtkByteSwap::Swap4LE(n + 1);
      svtkByteSwap::Swap4LE(n + 2);
      fwrite(n, 4, 3, fp);

      n[0] = (float)v3[0];
      n[1] = (float)v3[1];
      n[2] = (float)v3[2];
      svtkByteSwap::Swap4LE(n);
      svtkByteSwap::Swap4LE(n + 1);
      svtkByteSwap::Swap4LE(n + 2);
      fwrite(n, 4, 3, fp);
      fwrite(&ibuff2, 2, 1, fp);
      numTris++;
    }
    else if (npts > 3)
    {
      // Initialize the polygon.
      svtkNew<svtkPolygon> poly;
      poly->PointIds->SetNumberOfIds(npts);
      poly->Points->SetNumberOfPoints(npts);
      for (svtkIdType i = 0; i < npts; ++i)
      {
        poly->PointIds->SetId(i, indx[i]);
        poly->Points->SetPoint(i, pts->GetPoint(indx[i]));
      }

      // Do the triangulation
      svtkNew<svtkIdList> ptIds;
      ptIds->Allocate(SVTK_CELL_SIZE);
      poly->Triangulate(ptIds);

      svtkIdType numPts = ptIds->GetNumberOfIds();
      svtkIdType numSimplices = numPts / 3;
      for (svtkIdType i = 0; i < numSimplices; ++i)
      {
        svtkTriangle::ComputeNormal(poly->GetPoints(), 3, ptIds->GetPointer(3 * i), dn);

        float n[3];
        n[0] = (float)dn[0];
        n[1] = (float)dn[1];
        n[2] = (float)dn[2];
        svtkByteSwap::Swap4LE(n);
        svtkByteSwap::Swap4LE(n + 1);
        svtkByteSwap::Swap4LE(n + 2);
        fwrite(n, 4, 3, fp);

        for (svtkIdType j = 0; j < 3; ++j)
        {
          svtkIdType ptId = ptIds->GetId(3 * i + j);
          poly->GetPoints()->GetPoint(ptId, v1);

          n[0] = (float)v1[0];
          n[1] = (float)v1[1];
          n[2] = (float)v1[2];
          svtkByteSwap::Swap4LE(n);
          svtkByteSwap::Swap4LE(n + 1);
          svtkByteSwap::Swap4LE(n + 2);
          fwrite(n, 4, 3, fp);
        }
        fwrite(&ibuff2, 2, 1, fp);
        numTris++;
      }
    }
  }

  svtkByteSwap::Swap4LE(&numTris);
  fseek(fp, 80, SEEK_SET);
  fwrite(&numTris, 1, 4, fp);

  if (fflush(fp))
  {
    fclose(fp);
    this->SetErrorCode(svtkErrorCode::OutOfDiskSpaceError);
    return;
  }
  fclose(fp);
}

//----------------------------------------------------------------------------
void svtkSTLWriter::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent
     << "FileName: " << ((this->GetFileName() == nullptr) ? "(none)" : this->GetFileName())
     << std::endl;
  os << indent << "FileType: " << ((this->GetFileType() == SVTK_ASCII) ? "SVTK_ASCII" : "SVTK_BINARY")
     << std::endl;
  os << indent << "Header: " << this->GetHeader() << std::endl;
  os << indent << "Input: " << this->GetInput() << std::endl;
}

//----------------------------------------------------------------------------
svtkPolyData* svtkSTLWriter::GetInput()
{
  return svtkPolyData::SafeDownCast(this->GetInput(0));
}

//----------------------------------------------------------------------------
svtkPolyData* svtkSTLWriter::GetInput(int port)
{
  return svtkPolyData::SafeDownCast(this->Superclass::GetInput(port));
}

//----------------------------------------------------------------------------
int svtkSTLWriter::FillInputPortInformation(int, svtkInformation* info)
{
  info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkPolyData");
  return 1;
}
