/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkHausdorffDistancePointSetFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// Copyright (c) 2011 LTSI INSERM U642
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:
//
//     * Redistributions of source code must retain the above copyright notice,
// this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above copyright notice,
// this list of conditions and the following disclaimer in the documentation and/or
// other materials provided with the distribution.
//     * Neither name of LTSI, INSERM nor the names
// of any contributors may be used to endorse or promote products derived from this
// software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR ANY
// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
// ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include "svtkHausdorffDistancePointSetFilter.h"

#include "svtkDoubleArray.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"

#include "svtkCellLocator.h"
#include "svtkGenericCell.h"
#include "svtkKdTreePointLocator.h"
#include "svtkPointSet.h"
#include "svtkSmartPointer.h"

svtkStandardNewMacro(svtkHausdorffDistancePointSetFilter);

svtkHausdorffDistancePointSetFilter::svtkHausdorffDistancePointSetFilter()
{
  this->RelativeDistance[0] = 0.0;
  this->RelativeDistance[1] = 0.0;
  this->HausdorffDistance = 0.0;

  this->SetNumberOfInputPorts(2);
  this->SetNumberOfInputConnections(0, 1);
  this->SetNumberOfInputConnections(1, 1);

  this->SetNumberOfOutputPorts(2);

  this->TargetDistanceMethod = POINT_TO_POINT;
}

svtkHausdorffDistancePointSetFilter::~svtkHausdorffDistancePointSetFilter() {}

int svtkHausdorffDistancePointSetFilter::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // Get the info objects
  svtkInformation* inInfoA = inputVector[0]->GetInformationObject(0);
  svtkInformation* inInfoB = inputVector[1]->GetInformationObject(0);
  svtkInformation* outInfoA = outputVector->GetInformationObject(0);
  svtkInformation* outInfoB = outputVector->GetInformationObject(1);

  if (inInfoA == nullptr || inInfoB == nullptr)
  {
    return 0;
  }

  // Get the input
  svtkPointSet* inputA = svtkPointSet::SafeDownCast(inInfoA->Get(svtkDataObject::DATA_OBJECT()));
  svtkPointSet* inputB = svtkPointSet::SafeDownCast(inInfoB->Get(svtkDataObject::DATA_OBJECT()));
  svtkPointSet* outputA = svtkPointSet::SafeDownCast(outInfoA->Get(svtkDataObject::DATA_OBJECT()));
  svtkPointSet* outputB = svtkPointSet::SafeDownCast(outInfoB->Get(svtkDataObject::DATA_OBJECT()));

  if (inputA->GetNumberOfPoints() == 0 || inputB->GetNumberOfPoints() == 0)
  {
    return 0;
  }

  // Re-initialize the distances
  this->RelativeDistance[0] = 0.0;
  this->RelativeDistance[1] = 0.0;
  this->HausdorffDistance = 0.0;

  svtkSmartPointer<svtkKdTreePointLocator> pointLocatorA =
    svtkSmartPointer<svtkKdTreePointLocator>::New();
  svtkSmartPointer<svtkKdTreePointLocator> pointLocatorB =
    svtkSmartPointer<svtkKdTreePointLocator>::New();

  svtkSmartPointer<svtkCellLocator> cellLocatorA = svtkSmartPointer<svtkCellLocator>::New();
  svtkSmartPointer<svtkCellLocator> cellLocatorB = svtkSmartPointer<svtkCellLocator>::New();

  if (this->TargetDistanceMethod == POINT_TO_POINT)
  {
    pointLocatorA->SetDataSet(inputA);
    pointLocatorA->BuildLocator();
    pointLocatorB->SetDataSet(inputB);
    pointLocatorB->BuildLocator();
  }
  else
  {
    cellLocatorA->SetDataSet(inputA);
    cellLocatorA->BuildLocator();
    cellLocatorB->SetDataSet(inputB);
    cellLocatorB->BuildLocator();
  }

  double dist;
  double currentPoint[3];
  double closestPoint[3];
  svtkIdType cellId;
  svtkSmartPointer<svtkGenericCell> cell = svtkSmartPointer<svtkGenericCell>::New();
  int subId;

  svtkSmartPointer<svtkDoubleArray> distanceAToB = svtkSmartPointer<svtkDoubleArray>::New();
  distanceAToB->SetNumberOfComponents(1);
  distanceAToB->SetNumberOfTuples(inputA->GetNumberOfPoints());
  distanceAToB->SetName("Distance");

  svtkSmartPointer<svtkDoubleArray> distanceBToA = svtkSmartPointer<svtkDoubleArray>::New();
  distanceBToA->SetNumberOfComponents(1);
  distanceBToA->SetNumberOfTuples(inputB->GetNumberOfPoints());
  distanceBToA->SetName("Distance");

  // Find the nearest neighbors to each point and add edges between them,
  // if they do not already exist and they are not self loops
  for (int i = 0; i < inputA->GetNumberOfPoints(); i++)
  {
    inputA->GetPoint(i, currentPoint);
    if (this->TargetDistanceMethod == POINT_TO_POINT)
    {
      svtkIdType closestPointId = pointLocatorB->FindClosestPoint(currentPoint);
      inputB->GetPoint(closestPointId, closestPoint);
    }
    else
    {
      cellLocatorB->FindClosestPoint(currentPoint, closestPoint, cell, cellId, subId, dist);
    }

    dist = std::sqrt(std::pow(currentPoint[0] - closestPoint[0], 2) +
      std::pow(currentPoint[1] - closestPoint[1], 2) +
      std::pow(currentPoint[2] - closestPoint[2], 2));
    distanceAToB->SetValue(i, dist);

    if (dist > this->RelativeDistance[0])
    {
      this->RelativeDistance[0] = dist;
    }
  }

  for (int i = 0; i < inputB->GetNumberOfPoints(); i++)
  {
    inputB->GetPoint(i, currentPoint);
    if (this->TargetDistanceMethod == POINT_TO_POINT)
    {
      svtkIdType closestPointId = pointLocatorA->FindClosestPoint(currentPoint);
      inputA->GetPoint(closestPointId, closestPoint);
    }
    else
    {
      cellLocatorA->FindClosestPoint(currentPoint, closestPoint, cell, cellId, subId, dist);
    }

    dist = std::sqrt(std::pow(currentPoint[0] - closestPoint[0], 2) +
      std::pow(currentPoint[1] - closestPoint[1], 2) +
      std::pow(currentPoint[2] - closestPoint[2], 2));
    distanceBToA->SetValue(i, dist);

    if (dist > this->RelativeDistance[1])
    {
      this->RelativeDistance[1] = dist;
    }
  }

  if (this->RelativeDistance[0] >= RelativeDistance[1])
  {
    this->HausdorffDistance = this->RelativeDistance[0];
  }
  else
  {
    this->HausdorffDistance = this->RelativeDistance[1];
  }

  svtkSmartPointer<svtkDoubleArray> relativeDistanceAtoB = svtkSmartPointer<svtkDoubleArray>::New();
  relativeDistanceAtoB->SetNumberOfComponents(1);
  relativeDistanceAtoB->SetName("RelativeDistanceAtoB");
  relativeDistanceAtoB->InsertNextValue(RelativeDistance[0]);

  svtkSmartPointer<svtkDoubleArray> relativeDistanceBtoA = svtkSmartPointer<svtkDoubleArray>::New();
  relativeDistanceBtoA->SetNumberOfComponents(1);
  relativeDistanceBtoA->SetName("RelativeDistanceBtoA");
  relativeDistanceBtoA->InsertNextValue(RelativeDistance[1]);

  svtkSmartPointer<svtkDoubleArray> hausdorffDistanceFieldDataA =
    svtkSmartPointer<svtkDoubleArray>::New();
  hausdorffDistanceFieldDataA->SetNumberOfComponents(1);
  hausdorffDistanceFieldDataA->SetName("HausdorffDistance");
  hausdorffDistanceFieldDataA->InsertNextValue(HausdorffDistance);

  svtkSmartPointer<svtkDoubleArray> hausdorffDistanceFieldDataB =
    svtkSmartPointer<svtkDoubleArray>::New();
  hausdorffDistanceFieldDataB->SetNumberOfComponents(1);
  hausdorffDistanceFieldDataB->SetName("HausdorffDistance");
  hausdorffDistanceFieldDataB->InsertNextValue(HausdorffDistance);

  outputA->DeepCopy(inputA);
  outputA->GetPointData()->AddArray(distanceAToB);
  outputA->GetFieldData()->AddArray(relativeDistanceAtoB);
  outputA->GetFieldData()->AddArray(hausdorffDistanceFieldDataA);

  outputB->DeepCopy(inputB);
  outputB->GetPointData()->AddArray(distanceBToA);
  outputB->GetFieldData()->AddArray(relativeDistanceBtoA);
  outputB->GetFieldData()->AddArray(hausdorffDistanceFieldDataB);

  return 1;
}

int svtkHausdorffDistancePointSetFilter::FillInputPortInformation(int port, svtkInformation* info)
{
  // The input should be two svtkPointsSets
  if (port == 0)
  {
    info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkPointSet");
    return 1;
  }
  if (port == 1)
  {
    info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkPointSet");
    return 1;
  }
  return 0;
}

void svtkHausdorffDistancePointSetFilter::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "HausdorffDistance: " << this->GetHausdorffDistance() << "\n";
  os << indent << "RelativeDistance: " << this->GetRelativeDistance()[0] << ", "
     << this->GetRelativeDistance()[1] << "\n";
  os << indent << "TargetDistanceMethod: " << this->GetTargetDistanceMethodAsString() << "\n";
}
