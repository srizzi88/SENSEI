/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkDistancePolyDataFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkDistancePolyDataFilter.h"

#include "svtkCellData.h"
#include "svtkDoubleArray.h"
#include "svtkImplicitPolyDataDistance.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkStreamingDemandDrivenPipeline.h"
#include "svtkTriangle.h"

// The 3D cell with the maximum number of points is SVTK_LAGRANGE_HEXAHEDRON.
// We support up to 6th order hexahedra.
#define SVTK_MAXIMUM_NUMBER_OF_POINTS 216

svtkStandardNewMacro(svtkDistancePolyDataFilter);

//-----------------------------------------------------------------------------
svtkDistancePolyDataFilter::svtkDistancePolyDataFilter()
  : svtkPolyDataAlgorithm()
{
  this->SignedDistance = 1;
  this->NegateDistance = 0;
  this->ComputeSecondDistance = 1;
  this->ComputeCellCenterDistance = 1;

  this->SetNumberOfInputPorts(2);
  this->SetNumberOfOutputPorts(2);
}

//-----------------------------------------------------------------------------
svtkDistancePolyDataFilter::~svtkDistancePolyDataFilter() = default;

//-----------------------------------------------------------------------------
int svtkDistancePolyDataFilter::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  svtkPolyData* input0 = svtkPolyData::GetData(inputVector[0], 0);
  svtkPolyData* input1 = svtkPolyData::GetData(inputVector[1], 0);
  svtkPolyData* output0 = svtkPolyData::GetData(outputVector, 0);
  svtkPolyData* output1 = svtkPolyData::GetData(outputVector, 1);

  output0->CopyStructure(input0);
  output0->GetPointData()->PassData(input0->GetPointData());
  output0->GetCellData()->PassData(input0->GetCellData());
  output0->BuildCells();
  this->GetPolyDataDistance(output0, input1);

  if (this->ComputeSecondDistance)
  {
    output1->CopyStructure(input1);
    output1->GetPointData()->PassData(input1->GetPointData());
    output1->GetCellData()->PassData(input1->GetCellData());
    output1->BuildCells();
    this->GetPolyDataDistance(output1, input0);
  }
  return 1;
}

//-----------------------------------------------------------------------------
void svtkDistancePolyDataFilter::GetPolyDataDistance(svtkPolyData* mesh, svtkPolyData* src)
{
  svtkDebugMacro(<< "Start svtkDistancePolyDataFilter::GetPolyDataDistance");

  if (mesh->GetNumberOfCells() == 0 || mesh->GetNumberOfPoints() == 0)
  {
    svtkErrorMacro(<< "No points/cells to operate on");
    return;
  }

  if (src->GetNumberOfPolys() == 0 || src->GetNumberOfPoints() == 0)
  {
    svtkErrorMacro(<< "No points/cells to difference from");
    return;
  }

  svtkImplicitPolyDataDistance* imp = svtkImplicitPolyDataDistance::New();
  imp->SetInput(src);

  // Calculate distance from points.
  int numPts = mesh->GetNumberOfPoints();

  svtkDoubleArray* pointArray = svtkDoubleArray::New();
  pointArray->SetName("Distance");
  pointArray->SetNumberOfComponents(1);
  pointArray->SetNumberOfTuples(numPts);

  for (svtkIdType ptId = 0; ptId < numPts; ptId++)
  {
    double pt[3];
    mesh->GetPoint(ptId, pt);
    double val = imp->EvaluateFunction(pt);
    double dist = SignedDistance ? (NegateDistance ? -val : val) : fabs(val);
    pointArray->SetValue(ptId, dist);
  }

  mesh->GetPointData()->AddArray(pointArray);
  pointArray->Delete();
  mesh->GetPointData()->SetActiveScalars("Distance");

  // Calculate distance from cell centers.
  if (this->ComputeCellCenterDistance)
  {
    int numCells = mesh->GetNumberOfCells();

    svtkDoubleArray* cellArray = svtkDoubleArray::New();
    cellArray->SetName("Distance");
    cellArray->SetNumberOfComponents(1);
    cellArray->SetNumberOfTuples(numCells);

    for (svtkIdType cellId = 0; cellId < numCells; cellId++)
    {
      svtkCell* cell = mesh->GetCell(cellId);
      int subId;
      double pcoords[3], x[3], weights[SVTK_MAXIMUM_NUMBER_OF_POINTS];

      cell->GetParametricCenter(pcoords);
      cell->EvaluateLocation(subId, pcoords, x, weights);

      double val = imp->EvaluateFunction(x);
      double dist = SignedDistance ? (NegateDistance ? -val : val) : fabs(val);
      cellArray->SetValue(cellId, dist);
    }

    mesh->GetCellData()->AddArray(cellArray);
    cellArray->Delete();
    mesh->GetCellData()->SetActiveScalars("Distance");
  }

  imp->Delete();

  svtkDebugMacro(<< "End svtkDistancePolyDataFilter::GetPolyDataDistance");
}

//-----------------------------------------------------------------------------
svtkPolyData* svtkDistancePolyDataFilter::GetSecondDistanceOutput()
{
  if (!this->ComputeSecondDistance)
  {
    return nullptr;
  }
  return svtkPolyData::SafeDownCast(this->GetOutputDataObject(1));
}

//-----------------------------------------------------------------------------
void svtkDistancePolyDataFilter::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "SignedDistance: " << this->SignedDistance << "\n";
  os << indent << "NegateDistance: " << this->NegateDistance << "\n";
  os << indent << "ComputeSecondDistance: " << this->ComputeSecondDistance << "\n";
  os << indent << "ComputeCellCenterDistance: " << this->ComputeCellCenterDistance << "\n";
}
