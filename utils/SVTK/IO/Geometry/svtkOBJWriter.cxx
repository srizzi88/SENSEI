/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkOBJWriter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkOBJWriter.h"

#include "svtkDataSet.h"
#include "svtkErrorCode.h"
#include "svtkImageData.h"
#include "svtkInformation.h"
#include "svtkNumberToString.h"
#include "svtkPNGWriter.h"
#include "svtkPointData.h"
#include "svtkPolyData.h"
#include "svtkSmartPointer.h"
#include "svtkTriangleStrip.h"

#include "svtksys/FStream.hxx"
#include "svtksys/SystemTools.hxx"

namespace
{
//----------------------------------------------------------------------------
void WriteFaces(std::ostream& f, svtkCellArray* faces, bool withNormals, bool withTCoords)
{
  svtkIdType npts;
  const svtkIdType* indx;
  for (faces->InitTraversal(); faces->GetNextCell(npts, indx);)
  {
    f << "f";
    for (svtkIdType i = 0; i < npts; i++)
    {
      f << " " << indx[i] + 1;
      if (withTCoords)
      {
        f << "/" << indx[i] + 1;
        if (withNormals)
        {
          f << "/" << indx[i] + 1;
        }
      }
      else if (withNormals)
      {
        f << "//" << indx[i] + 1;
      }
    }
    f << "\n";
  }
}

//----------------------------------------------------------------------------
void WriteLines(std::ostream& f, svtkCellArray* lines)
{
  svtkIdType npts;
  const svtkIdType* indx;
  for (lines->InitTraversal(); lines->GetNextCell(npts, indx);)
  {
    f << "l";
    for (svtkIdType i = 0; i < npts; i++)
    {
      f << " " << indx[i] + 1;
    }
    f << "\n";
  }
}

//----------------------------------------------------------------------------
void WritePoints(std::ostream& f, svtkPoints* pts, svtkDataArray* normals, svtkDataArray* tcoords)
{
  svtkNumberToString convert;
  svtkIdType nbPts = pts->GetNumberOfPoints();

  // Positions
  for (svtkIdType i = 0; i < nbPts; i++)
  {
    double p[3];
    pts->GetPoint(i, p);
    f << "v " << convert(p[0]) << " " << convert(p[1]) << " " << convert(p[2]) << "\n";
  }

  // Normals
  if (normals)
  {
    for (svtkIdType i = 0; i < nbPts; i++)
    {
      double p[3];
      normals->GetTuple(i, p);
      f << "vn " << convert(p[0]) << " " << convert(p[1]) << " " << convert(p[2]) << "\n";
    }
  }

  // Textures
  if (tcoords)
  {
    for (svtkIdType i = 0; i < nbPts; i++)
    {
      double p[2];
      tcoords->GetTuple(i, p);
      f << "vt " << convert(p[0]) << " " << convert(p[1]) << "\n";
    }
  }
}

//----------------------------------------------------------------------------
bool WriteTexture(std::ostream& f, const std::string& baseName, svtkImageData* texture)
{
  std::string mtlName = baseName + ".mtl";
  svtksys::ofstream fmtl(mtlName.c_str(), svtksys::ofstream::out);
  if (fmtl.fail())
  {
    return false;
  }

  // write png file
  std::string pngName = baseName + ".png";
  svtkNew<svtkPNGWriter> pngWriter;
  pngWriter->SetInputData(texture);
  pngWriter->SetFileName(pngName.c_str());
  pngWriter->Write();

  // remove directories
  mtlName = svtksys::SystemTools::GetFilenameName(mtlName);
  pngName = svtksys::SystemTools::GetFilenameName(pngName);

  // set material
  fmtl << "newmtl svtktexture\n";
  fmtl << "map_Kd " << pngName << "\n";

  // declare material in obj file
  f << "mtllib " + mtlName + "\n";
  f << "usemtl svtktexture\n";

  return true;
}
}

//----------------------------------------------------------------------------
svtkStandardNewMacro(svtkOBJWriter);

//----------------------------------------------------------------------------
svtkOBJWriter::svtkOBJWriter()
{
  this->FileName = nullptr;
  this->SetNumberOfInputPorts(2);
}

//----------------------------------------------------------------------------
svtkOBJWriter::~svtkOBJWriter()
{
  this->SetFileName(nullptr);
}

//----------------------------------------------------------------------------
void svtkOBJWriter::WriteData()
{
  svtkPolyData* input = this->GetInputGeometry();
  svtkImageData* texture = this->GetInputTexture();

  if (input == nullptr)
  {
    svtkErrorMacro("No geometry to write!");
    this->SetErrorCode(svtkErrorCode::UnknownError);
    return;
  }

  svtkPoints* pts = input->GetPoints();
  svtkCellArray* polys = input->GetPolys();
  svtkCellArray* strips = input->GetStrips();
  svtkCellArray* lines = input->GetLines();
  svtkDataArray* normals = input->GetPointData()->GetNormals();
  svtkDataArray* tcoords = input->GetPointData()->GetTCoords();

  if (pts == nullptr)
  {
    svtkErrorMacro("No data to write!");
    this->SetErrorCode(svtkErrorCode::UnknownError);
    return;
  }

  if (this->FileName == nullptr)
  {
    svtkErrorMacro("Please specify FileName to write");
    this->SetErrorCode(svtkErrorCode::NoFileNameError);
    return;
  }

  svtkIdType npts = 0;

  svtksys::ofstream f(this->FileName, svtksys::ofstream::out);
  if (f.fail())
  {
    svtkErrorMacro("Unable to open file: " << this->FileName);
    this->SetErrorCode(svtkErrorCode::CannotOpenFileError);
    return;
  }

  // Write header
  f << "# Generated by Visualization Toolkit\n";

  // Write material if a texture is specified
  if (texture)
  {
    std::vector<std::string> comp;
    svtksys::SystemTools::SplitPath(svtksys::SystemTools::GetFilenamePath(this->FileName), comp);
    comp.push_back(svtksys::SystemTools::GetFilenameWithoutLastExtension(this->FileName));
    if (!::WriteTexture(f, svtksys::SystemTools::JoinPath(comp), texture))
    {
      svtkErrorMacro("Unable to create material file");
    }
  }

  // Write points
  ::WritePoints(f, pts, normals, tcoords);

  // Decompose any triangle strips into triangles
  svtkNew<svtkCellArray> polyStrips;
  if (strips->GetNumberOfCells() > 0)
  {
    const svtkIdType* ptIds = nullptr;
    for (strips->InitTraversal(); strips->GetNextCell(npts, ptIds);)
    {
      svtkTriangleStrip::DecomposeStrip(npts, ptIds, polyStrips);
    }
  }

  // Write triangle strips
  ::WriteFaces(f, polyStrips, normals != nullptr, tcoords != nullptr);

  // Write polygons.
  if (polys)
  {
    ::WriteFaces(f, polys, normals != nullptr, tcoords != nullptr);
  }

  // Write lines.
  if (lines)
  {
    ::WriteLines(f, lines);
  }

  f.close();
}

//----------------------------------------------------------------------------
void svtkOBJWriter::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "FileName: " << (this->GetFileName() ? this->GetFileName() : "(none)") << endl;
  os << indent << "Input: " << this->GetInputGeometry() << endl;

  svtkImageData* texture = this->GetInputTexture();
  if (texture)
  {
    os << indent << "Texture:" << endl;
    texture->PrintSelf(os, indent.GetNextIndent());
  }
}

//----------------------------------------------------------------------------
svtkPolyData* svtkOBJWriter::GetInputGeometry()
{
  return svtkPolyData::SafeDownCast(this->GetInput(0));
}

//----------------------------------------------------------------------------
svtkImageData* svtkOBJWriter::GetInputTexture()
{
  return svtkImageData::SafeDownCast(this->GetInput(1));
}

//----------------------------------------------------------------------------
svtkDataSet* svtkOBJWriter::GetInput(int port)
{
  return svtkDataSet::SafeDownCast(this->Superclass::GetInput(port));
}

//----------------------------------------------------------------------------
int svtkOBJWriter::FillInputPortInformation(int port, svtkInformation* info)
{
  if (port == 0)
  {
    info->Set(svtkDataObject::DATA_TYPE_NAME(), "svtkPolyData");
    return 1;
  }
  if (port == 1)
  {
    info->Set(svtkDataObject::DATA_TYPE_NAME(), "svtkImageData");
    info->Set(svtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
    return 1;
  }
  return 0;
}
