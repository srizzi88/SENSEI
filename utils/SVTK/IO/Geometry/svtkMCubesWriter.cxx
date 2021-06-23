/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkMCubesWriter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkMCubesWriter.h"

#include "svtkByteSwap.h"
#include "svtkCellArray.h"
#include "svtkInformation.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkPolyData.h"
#include <svtksys/SystemTools.hxx>

svtkStandardNewMacro(svtkMCubesWriter);

// Create object.
svtkMCubesWriter::svtkMCubesWriter()
{
  this->FileName = nullptr;
  this->LimitsFileName = nullptr;
}

svtkMCubesWriter::~svtkMCubesWriter()
{
  delete[] this->FileName;
  delete[] this->LimitsFileName;
}

// Write out data in MOVIE.BYU format.
void svtkMCubesWriter::WriteData()
{
  svtkPoints* pts;
  svtkDataArray* normals;
  svtkCellArray* polys;
  svtkPolyData* input = this->GetInput();

  polys = input->GetPolys();
  pts = input->GetPoints();
  if (pts == nullptr || polys == nullptr)
  {
    svtkErrorMacro(<< "No data to write!");
    return;
  }

  normals = input->GetPointData()->GetNormals();
  if (normals == nullptr)
  {
    svtkErrorMacro(<< "No normals to write!: use svtkPolyDataNormals to generate them");
    return;
  }

  if (this->FileName == nullptr)
  {
    svtkErrorMacro(<< "Please specify FileName to write");
    return;
  }

  svtkDebugMacro("Writing MCubes tri file");
  FILE* fp;
  if ((fp = svtksys::SystemTools::Fopen(this->FileName, "w")) == nullptr)
  {
    svtkErrorMacro(<< "Couldn't open file: " << this->FileName);
    return;
  }
  this->WriteMCubes(fp, pts, normals, polys);
  fclose(fp);

  if (this->LimitsFileName)
  {
    svtkDebugMacro("Writing MCubes limits file");
    if ((fp = svtksys::SystemTools::Fopen(this->LimitsFileName, "w")) == nullptr)
    {
      svtkErrorMacro(<< "Couldn't open file: " << this->LimitsFileName);
      return;
    }
    this->WriteLimits(fp, input->GetBounds());
    fclose(fp);
  }
}

void svtkMCubesWriter::WriteMCubes(
  FILE* fp, svtkPoints* pts, svtkDataArray* normals, svtkCellArray* polys)
{
  typedef struct
  {
    float x[3], n[3];
  } pointType;
  pointType point;
  int i;
  svtkIdType npts;
  const svtkIdType* indx = nullptr;

  //  Write out triangle polygons.  In not a triangle polygon, create
  //  triangles.
  //
  double p[3], n[3];
  bool status = true;
  for (polys->InitTraversal(); polys->GetNextCell(npts, indx) && status;)
  {
    for (i = 0; i < 3 && status; i++)
    {
      pts->GetPoint(indx[i], p);
      normals->GetTuple(indx[i], n);
      point.x[0] = static_cast<float>(p[0]);
      point.x[1] = static_cast<float>(p[1]);
      point.x[2] = static_cast<float>(p[2]);
      point.n[0] = static_cast<float>(n[0]);
      point.n[1] = static_cast<float>(n[1]);
      point.n[2] = static_cast<float>(n[2]);
      status = svtkByteSwap::SwapWrite4BERange(reinterpret_cast<float*>(&point), 6, fp);
      if (!status)
      {
        svtkErrorMacro(<< "SwapWrite failed.");
      }
    }
  }
}
void svtkMCubesWriter::WriteLimits(FILE* fp, double* bounds)
{
  float fbounds[6];
  fbounds[0] = static_cast<float>(bounds[0]);
  fbounds[1] = static_cast<float>(bounds[1]);
  fbounds[2] = static_cast<float>(bounds[2]);
  fbounds[3] = static_cast<float>(bounds[3]);
  fbounds[4] = static_cast<float>(bounds[4]);
  fbounds[5] = static_cast<float>(bounds[5]);
  bool status = svtkByteSwap::SwapWrite4BERange(fbounds, 6, fp);
  if (!status)
  {
    svtkErrorMacro(<< "SwapWrite failed.");
  }
  else
  {
    status = svtkByteSwap::SwapWrite4BERange(fbounds, 6, fp);
    if (!status)
    {
      svtkErrorMacro(<< "SwapWrite failed.");
    }
  }
}

void svtkMCubesWriter::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Limits File Name: " << (this->LimitsFileName ? this->LimitsFileName : "(none)")
     << "\n";
}

svtkPolyData* svtkMCubesWriter::GetInput()
{
  return svtkPolyData::SafeDownCast(this->Superclass::GetInput());
}

svtkPolyData* svtkMCubesWriter::GetInput(int port)
{
  return svtkPolyData::SafeDownCast(this->Superclass::GetInput(port));
}

int svtkMCubesWriter::FillInputPortInformation(int, svtkInformation* info)
{
  info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkPolyData");
  return 1;
}
