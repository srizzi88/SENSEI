/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkRectilinearGridToTetrahedra.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/
#include "svtkRectilinearGridToPointSet.h"

#include "svtkCellData.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkRectilinearGrid.h"
#include "svtkStructuredGrid.h"

#include "svtkNew.h"

svtkStandardNewMacro(svtkRectilinearGridToPointSet);

//-------------------------------------------------------------------------
svtkRectilinearGridToPointSet::svtkRectilinearGridToPointSet() = default;

svtkRectilinearGridToPointSet::~svtkRectilinearGridToPointSet() = default;

void svtkRectilinearGridToPointSet::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//-------------------------------------------------------------------------
int svtkRectilinearGridToPointSet::FillInputPortInformation(int port, svtkInformation* info)
{
  if (!this->Superclass::FillInputPortInformation(port, info))
  {
    return 0;
  }
  info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkRectilinearGrid");
  return 1;
}

//-------------------------------------------------------------------------
int svtkRectilinearGridToPointSet::CopyStructure(
  svtkStructuredGrid* outData, svtkRectilinearGrid* inData)
{
  svtkDataArray* xcoord = inData->GetXCoordinates();
  svtkDataArray* ycoord = inData->GetYCoordinates();
  svtkDataArray* zcoord = inData->GetZCoordinates();

  int extent[6];
  inData->GetExtent(extent);

  outData->SetExtent(extent);

  svtkNew<svtkPoints> points;
  points->SetDataTypeToDouble();
  points->SetNumberOfPoints(inData->GetNumberOfPoints());

  svtkIdType pointId = 0;
  int ijk[3];
  for (ijk[2] = extent[4]; ijk[2] <= extent[5]; ijk[2]++)
  {
    for (ijk[1] = extent[2]; ijk[1] <= extent[3]; ijk[1]++)
    {
      for (ijk[0] = extent[0]; ijk[0] <= extent[1]; ijk[0]++)
      {
        double coord[3];
        coord[0] = xcoord->GetComponent(ijk[0] - extent[0], 0);
        coord[1] = ycoord->GetComponent(ijk[1] - extent[2], 0);
        coord[2] = zcoord->GetComponent(ijk[2] - extent[4], 0);

        points->SetPoint(pointId, coord);
        pointId++;
      }
    }
  }

  if (pointId != points->GetNumberOfPoints())
  {
    svtkErrorMacro(<< "Somehow miscounted points");
    return 0;
  }

  outData->SetPoints(points);

  return 1;
}

//-------------------------------------------------------------------------
int svtkRectilinearGridToPointSet::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  svtkRectilinearGrid* inData = svtkRectilinearGrid::GetData(inputVector[0]);
  svtkStructuredGrid* outData = svtkStructuredGrid::GetData(outputVector);

  if (inData == nullptr)
  {
    svtkErrorMacro(<< "Input data is nullptr.");
    return 0;
  }
  if (outData == nullptr)
  {
    svtkErrorMacro(<< "Output data is nullptr.");
    return 0;
  }

  int result = svtkRectilinearGridToPointSet::CopyStructure(outData, inData);
  if (!result)
  {
    return 0;
  }

  outData->GetPointData()->PassData(inData->GetPointData());
  outData->GetCellData()->PassData(inData->GetCellData());

  return 1;
}
