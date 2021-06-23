/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkTerrainContourLineInterpolator.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkTerrainContourLineInterpolator.h"

#include "svtkCellArray.h"
#include "svtkContourRepresentation.h"
#include "svtkImageData.h"
#include "svtkMath.h"
#include "svtkObjectFactory.h"
#include "svtkPoints.h"
#include "svtkProjectedTerrainPath.h"

svtkStandardNewMacro(svtkTerrainContourLineInterpolator);

//----------------------------------------------------------------------
svtkTerrainContourLineInterpolator::svtkTerrainContourLineInterpolator()
{
  this->ImageData = nullptr;
  this->Projector = svtkProjectedTerrainPath::New();
  this->Projector->SetHeightOffset(0.0);
  this->Projector->SetHeightTolerance(5);
  this->Projector->SetProjectionModeToHug();
}

//----------------------------------------------------------------------
svtkTerrainContourLineInterpolator::~svtkTerrainContourLineInterpolator()
{
  this->SetImageData(nullptr);
  this->Projector->Delete();
}

//----------------------------------------------------------------------
void svtkTerrainContourLineInterpolator::SetImageData(svtkImageData* image)
{
  if (this->ImageData != image)
  {
    svtkImageData* temp = this->ImageData;
    this->ImageData = image;
    if (this->ImageData != nullptr)
    {
      this->ImageData->Register(this);
      this->Projector->SetSourceData(this->ImageData);
    }
    if (temp != nullptr)
    {
      temp->UnRegister(this);
    }
    this->Modified();
  }
}

//----------------------------------------------------------------------
int svtkTerrainContourLineInterpolator::InterpolateLine(
  svtkRenderer*, svtkContourRepresentation* rep, int idx1, int idx2)
{
  if (!this->ImageData)
  {
    return 0; // No interpolation done if height-field data isn't specified.
  }

  double p1[3], p2[3];
  rep->GetNthNodeWorldPosition(idx1, p1);
  rep->GetNthNodeWorldPosition(idx2, p2);

  svtkPoints* pts = svtkPoints::New();
  pts->InsertNextPoint(p1);
  pts->InsertNextPoint(p2);
  svtkCellArray* lines = svtkCellArray::New();
  lines->InsertNextCell(2);
  lines->InsertCellPoint(0);
  lines->InsertCellPoint(1);

  svtkPolyData* terrainPath = svtkPolyData::New();
  terrainPath->SetPoints(pts);
  terrainPath->SetLines(lines);
  lines->Delete();
  pts->Delete();

  this->Projector->SetInputData(terrainPath);
  this->Projector->Update();
  terrainPath->Delete();

  svtkPolyData* interpolatedPd = this->Projector->GetOutput();
  svtkPoints* interpolatedPts = interpolatedPd->GetPoints();
  svtkCellArray* interpolatedCells = interpolatedPd->GetLines();

  const svtkIdType* ptIdx;
  svtkIdType npts = 0;

  // Add an ordered set of lines to the representation...
  // The Projected path is a recursive filter and will not generate an ordered
  // set of points. It generates a svtkPolyData with several lines. Each line
  // contains 2 points. We will, from this polydata figure out the ordered set
  // of points that form the projected path..

  const double tolerance = 1.0;
  bool traversalDone = false;
  while (!traversalDone)
  {
    for (interpolatedCells->InitTraversal(); interpolatedCells->GetNextCell(npts, ptIdx);)
    {

      double p[3];
      interpolatedPts->GetPoint(ptIdx[0], p);

      if ((p[0] - p1[0]) * (p[0] - p1[0]) + (p[1] - p1[1]) * (p[1] - p1[1]) < tolerance)
      {
        interpolatedPts->GetPoint(ptIdx[npts - 1], p1);
        if ((p2[0] - p1[0]) * (p2[0] - p1[0]) + (p2[1] - p1[1]) * (p2[1] - p1[1]) < tolerance)
        {
          --npts;
          traversalDone = true;
        }

        for (int i = 1; i < npts; i++)
        {
          rep->AddIntermediatePointWorldPosition(idx1, interpolatedPts->GetPoint(ptIdx[i]));
        }
        continue;
      }
    }
  }

  return 1;
}

//----------------------------------------------------------------------
int svtkTerrainContourLineInterpolator::UpdateNode(
  svtkRenderer*, svtkContourRepresentation*, double* svtkNotUsed(node), int svtkNotUsed(idx))
{
  return 0;
}

//----------------------------------------------------------------------
void svtkTerrainContourLineInterpolator::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "ImageData: " << this->ImageData << endl;
  if (this->ImageData)
  {
    this->ImageData->PrintSelf(os, indent.GetNextIndent());
  }

  os << indent << "Projector: " << this->Projector << endl;
  if (this->Projector)
  {
    this->Projector->PrintSelf(os, indent.GetNextIndent());
  }
}
