/*=========================================================================

  Program:   ParaView
  Module:    svtkAppendArcLength.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkAppendArcLength.h"

#include "svtkCellArray.h"
#include "svtkDoubleArray.h"
#include "svtkFloatArray.h"
#include "svtkMath.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkPoints.h"
#include "svtkPolyData.h"

svtkStandardNewMacro(svtkAppendArcLength);
//----------------------------------------------------------------------------
svtkAppendArcLength::svtkAppendArcLength() = default;

//----------------------------------------------------------------------------
svtkAppendArcLength::~svtkAppendArcLength() = default;

//----------------------------------------------------------------------------
int svtkAppendArcLength::RequestData(
  svtkInformation*, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  svtkPolyData* input = svtkPolyData::GetData(inputVector[0], 0);
  svtkPolyData* output = svtkPolyData::GetData(outputVector, 0);
  if (input->GetNumberOfPoints() == 0)
  {
    return 1;
  }

  output->ShallowCopy(input);

  // Now add "arc_length" array.
  svtkDataArray* arc_length = nullptr;
  svtkPoints* points = output->GetPoints();
  svtkIdType numPoints = points->GetNumberOfPoints();
  if (points->GetDataType() == SVTK_DOUBLE)
  {
    arc_length = svtkDoubleArray::New();
  }
  else
  {
    arc_length = svtkFloatArray::New();
  }
  arc_length->SetName("arc_length");
  arc_length->SetNumberOfComponents(1);
  arc_length->SetNumberOfTuples(numPoints);
  arc_length->FillComponent(0, 0.0);

  svtkCellArray* lines = output->GetLines();
  svtkIdType numCellPoints;
  const svtkIdType* cellPoints;
  lines->InitTraversal();
  while (lines->GetNextCell(numCellPoints, cellPoints))
  {
    if (numCellPoints == 0)
    {
      continue;
    }
    double arc_distance = 0.0;
    double prevPoint[3];
    points->GetPoint(cellPoints[0], prevPoint);
    for (svtkIdType cc = 1; cc < numCellPoints; cc++)
    {
      double curPoint[3];
      points->GetPoint(cellPoints[cc], curPoint);
      double distance = sqrt(svtkMath::Distance2BetweenPoints(curPoint, prevPoint));
      arc_distance += distance;
      arc_length->SetTuple1(cellPoints[cc], arc_distance);
      memcpy(prevPoint, curPoint, 3 * sizeof(double));
    }
  }
  output->GetPointData()->AddArray(arc_length);
  arc_length->Delete();
  return 1;
}

//----------------------------------------------------------------------------
void svtkAppendArcLength::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
