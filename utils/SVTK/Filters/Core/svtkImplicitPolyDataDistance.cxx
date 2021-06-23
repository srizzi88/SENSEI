/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImplicitPolyDataDistance.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkImplicitPolyDataDistance.h"

#include "svtkCellData.h"
#include "svtkCellLocator.h"
#include "svtkGenericCell.h"
#include "svtkMath.h"
#include "svtkObjectFactory.h"
#include "svtkPolyData.h"
#include "svtkPolygon.h"
#include "svtkSmartPointer.h"
#include "svtkTriangleFilter.h"

svtkStandardNewMacro(svtkImplicitPolyDataDistance);

//-----------------------------------------------------------------------------
svtkImplicitPolyDataDistance::svtkImplicitPolyDataDistance()
{
  this->NoClosestPoint[0] = 0.0;
  this->NoClosestPoint[1] = 0.0;
  this->NoClosestPoint[2] = 0.0;

  this->NoGradient[0] = 0.0;
  this->NoGradient[1] = 0.0;
  this->NoGradient[2] = 1.0;

  this->NoValue = 0.0;

  this->Input = nullptr;
  this->Locator = nullptr;
  this->Tolerance = 1e-12;
}

//-----------------------------------------------------------------------------
void svtkImplicitPolyDataDistance::SetInput(svtkPolyData* input)
{
  if (this->Input != input)
  {
    // Use a svtkTriangleFilter on the polydata input.
    // This is done to filter out lines and vertices to leave only
    // polygons which are required by this algorithm for cell normals.
    svtkSmartPointer<svtkTriangleFilter> triangleFilter = svtkSmartPointer<svtkTriangleFilter>::New();
    triangleFilter->PassVertsOff();
    triangleFilter->PassLinesOff();

    triangleFilter->SetInputData(input);
    triangleFilter->Update();

    this->Input = triangleFilter->GetOutput();

    this->Input->BuildLinks();
    this->NoValue = this->Input->GetLength();

    this->CreateDefaultLocator();
    this->Locator->SetDataSet(this->Input);
    this->Locator->SetTolerance(this->Tolerance);
    this->Locator->SetNumberOfCellsPerBucket(10);
    this->Locator->CacheCellBoundsOn();
    this->Locator->AutomaticOn();
    this->Locator->BuildLocator();
  }
}

//-----------------------------------------------------------------------------
svtkMTimeType svtkImplicitPolyDataDistance::GetMTime()
{
  svtkMTimeType mTime = this->svtkImplicitFunction::GetMTime();
  svtkMTimeType InputMTime;

  if (this->Input != nullptr)
  {
    InputMTime = this->Input->GetMTime();
    mTime = (InputMTime > mTime ? InputMTime : mTime);
  }

  return mTime;
}

//-----------------------------------------------------------------------------
svtkImplicitPolyDataDistance::~svtkImplicitPolyDataDistance()
{
  if (this->Locator)
  {
    this->Locator->UnRegister(this);
    this->Locator = nullptr;
  }
}

//----------------------------------------------------------------------------
void svtkImplicitPolyDataDistance::CreateDefaultLocator()
{
  if (this->Locator == nullptr)
  {
    this->Locator = svtkCellLocator::New();
  }
}

//-----------------------------------------------------------------------------
double svtkImplicitPolyDataDistance::EvaluateFunction(double x[3])
{
  double g[3];
  double p[3];
  return this->SharedEvaluate(
    x, g, p); // get distance value returned, normal and closest point not used
}

//-----------------------------------------------------------------------------
double svtkImplicitPolyDataDistance::EvaluateFunctionAndGetClosestPoint(
  double x[3], double closestPoint[3])
{
  double g[3];
  return this->SharedEvaluate(x, g, closestPoint); // distance value returned and point on
                                                   // svtkPolyData stored in p (normal not used).
}

//-----------------------------------------------------------------------------
void svtkImplicitPolyDataDistance::EvaluateGradient(double x[3], double g[3])
{
  double p[3];
  this->SharedEvaluate(
    x, g, p); // get normal, returned distance value not used and closest point not used
}

//-----------------------------------------------------------------------------
double svtkImplicitPolyDataDistance::SharedEvaluate(double x[3], double g[3], double closestPoint[3])
{
  // Set defaults
  double ret = this->NoValue;

  for (int i = 0; i < 3; i++)
  {
    g[i] = this->NoGradient[i];
  }

  for (int i = 0; i < 3; i++)
  {
    closestPoint[i] = this->NoClosestPoint[i];
  }

  // See if data set with polygons has been specified
  if (this->Input == nullptr || Input->GetNumberOfCells() == 0)
  {
    svtkErrorMacro(<< "No polygons to evaluate function!");
    return ret;
  }

  double p[3];
  svtkIdType cellId;
  int subId;
  double vlen2;

  svtkDataArray* cnorms = nullptr;
  if (this->Input->GetCellData() && this->Input->GetCellData()->GetNormals())
  {
    cnorms = this->Input->GetCellData()->GetNormals();
  }

  // Get point id of closest point in data set.
  svtkSmartPointer<svtkGenericCell> cell = svtkSmartPointer<svtkGenericCell>::New();
  this->Locator->FindClosestPoint(x, p, cell, cellId, subId, vlen2);

  if (cellId != -1) // point located
  {
    // dist = | point - x |
    ret = sqrt(vlen2);
    // grad = (point - x) / dist
    for (int i = 0; i < 3; i++)
    {
      g[i] = (p[i] - x[i]) / (ret == 0. ? 1. : ret);
    }

    double dist2, weights[3], pcoords[3], awnorm[3] = { 0, 0, 0 };
    cell->EvaluatePosition(p, closestPoint, subId, pcoords, dist2, weights);

    svtkIdList* idList = svtkIdList::New();
    int count = 0;
    for (int i = 0; i < 3; i++)
    {
      count += (fabs(weights[i]) < this->Tolerance ? 1 : 0);
    }
    // Face case - weights contains no 0s
    if (count == 0)
    {
      // Compute face normal.
      if (cnorms)
      {
        cnorms->GetTuple(cellId, awnorm);
      }
      else
      {
        svtkPolygon::ComputeNormal(cell->Points, awnorm);
      }
    }
    // Edge case - weights contain one 0
    else if (count == 1)
    {
      // ... edge ... get two adjacent faces, compute average normal
      int a = -1, b = -1;
      for (int edge = 0; edge < 3; edge++)
      {
        if (fabs(weights[edge]) < this->Tolerance)
        {
          a = cell->PointIds->GetId((edge + 1) % 3);
          b = cell->PointIds->GetId((edge + 2) % 3);
          break;
        }
      }

      if (a == -1)
      {
        svtkErrorMacro(<< "Could not find edge when closest point is "
                      << "expected to be on an edge.");
        return this->NoValue;
      }

      // The first argument is the cell ID. We pass a bogus cell ID so that
      // all face IDs attached to the edge are returned in the idList.
      this->Input->GetCellEdgeNeighbors(SVTK_ID_MAX, a, b, idList);
      for (int i = 0; i < idList->GetNumberOfIds(); i++)
      {
        double norm[3];
        if (cnorms)
        {
          cnorms->GetTuple(idList->GetId(i), norm);
        }
        else
        {
          svtkPolygon::ComputeNormal(this->Input->GetCell(idList->GetId(i))->GetPoints(), norm);
        }
        awnorm[0] += norm[0];
        awnorm[1] += norm[1];
        awnorm[2] += norm[2];
      }
      svtkMath::Normalize(awnorm);
    }

    // Vertex case - weights contain two 0s
    else if (count == 2)
    {
      // ... vertex ... this is the expensive case, get all adjacent
      // faces and compute sum(a_i * n_i) Angle-Weighted Pseudo
      // Normals, J. Andreas Baerentzen and Henrik Aanaes
      int a = -1;
      for (int i = 0; i < 3; i++)
      {
        if (fabs(weights[i]) > this->Tolerance)
        {
          a = cell->PointIds->GetId(i);
        }
      }

      if (a == -1)
      {
        svtkErrorMacro(<< "Could not find point when closest point is "
                      << "expected to be a point.");
        return this->NoValue;
      }

      this->Input->GetPointCells(a, idList);
      for (int i = 0; i < idList->GetNumberOfIds(); i++)
      {
        double norm[3];
        if (cnorms)
        {
          cnorms->GetTuple(idList->GetId(i), norm);
        }
        else
        {
          svtkPolygon::ComputeNormal(this->Input->GetCell(idList->GetId(i))->GetPoints(), norm);
        }

        // Compute angle at point a
        int b = this->Input->GetCell(idList->GetId(i))->GetPointId(0);
        int c = this->Input->GetCell(idList->GetId(i))->GetPointId(1);
        if (a == b)
        {
          b = this->Input->GetCell(idList->GetId(i))->GetPointId(2);
        }
        else if (a == c)
        {
          c = this->Input->GetCell(idList->GetId(i))->GetPointId(2);
        }
        double pa[3], pb[3], pc[3];
        this->Input->GetPoint(a, pa);
        this->Input->GetPoint(b, pb);
        this->Input->GetPoint(c, pc);
        for (int j = 0; j < 3; j++)
        {
          pb[j] -= pa[j];
          pc[j] -= pa[j];
        }
        svtkMath::Normalize(pb);
        svtkMath::Normalize(pc);
        double alpha = acos(svtkMath::Dot(pb, pc));
        awnorm[0] += alpha * norm[0];
        awnorm[1] += alpha * norm[1];
        awnorm[2] += alpha * norm[2];
      }
      svtkMath::Normalize(awnorm);
    }
    idList->Delete();

    // sign(dist) = dot(grad, cell normal)
    if (ret == 0)
    {
      for (int i = 0; i < 3; i++)
      {
        g[i] = awnorm[i];
      }
    }
    ret *= (svtkMath::Dot(g, awnorm) < 0.) ? 1. : -1.;

    if (ret > 0.)
    {
      for (int i = 0; i < 3; i++)
      {
        g[i] = -g[i];
      }
    }
  }

  return ret;
}

//-----------------------------------------------------------------------------
void svtkImplicitPolyDataDistance::PrintSelf(ostream& os, svtkIndent indent)
{
  svtkImplicitFunction::PrintSelf(os, indent);

  os << indent << "NoValue: " << this->NoValue << "\n";
  os << indent << "NoGradient: (" << this->NoGradient[0] << ", " << this->NoGradient[1] << ", "
     << this->NoGradient[2] << ")\n";
  os << indent << "Tolerance: " << this->Tolerance << "\n";

  if (this->Input)
  {
    os << indent << "Input : " << this->Input << "\n";
  }
  else
  {
    os << indent << "Input : (none)\n";
  }
}
