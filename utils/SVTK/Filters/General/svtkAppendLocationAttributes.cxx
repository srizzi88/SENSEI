/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkAppendLocationAttributes.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkAppendLocationAttributes.h"

#include "svtkCell.h"
#include "svtkCellArray.h"
#include "svtkCellCenters.h"
#include "svtkCellData.h"
#include "svtkDataSet.h"
#include "svtkDoubleArray.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkPoints.h"
#include "svtkPolyData.h"

svtkStandardNewMacro(svtkAppendLocationAttributes);

//----------------------------------------------------------------------------
// Generate points
int svtkAppendLocationAttributes::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the input and output
  svtkDataSet* input = svtkDataSet::GetData(inputVector[0]);
  svtkDataSet* output = svtkDataSet::GetData(outputVector);

  output->ShallowCopy(input);

  // Create cell centers array
  svtkNew<svtkDoubleArray> cellCenterArray;
  if (this->AppendCellCenters)
  {
    svtkIdType numCells = input->GetNumberOfCells();
    cellCenterArray->SetName("CellCenters");
    cellCenterArray->SetNumberOfComponents(3);
    cellCenterArray->SetNumberOfTuples(numCells);

    svtkCellCenters::ComputeCellCenters(input, cellCenterArray);

    svtkCellData* outCD = output->GetCellData();
    outCD->AddArray(cellCenterArray);

    this->UpdateProgress(0.66);
  }

  if (this->AppendPointLocations)
  {
    svtkPointData* outPD = output->GetPointData();
    svtkPointSet* outPointSet = svtkPointSet::SafeDownCast(output);
    if (outPointSet && outPointSet->GetPoints())
    {
      // Access point data array and shallow copy it to a point data array
      svtkDataArray* pointArray = outPointSet->GetPoints()->GetData();
      svtkSmartPointer<svtkDataArray> arrayCopy;
      arrayCopy.TakeReference(pointArray->NewInstance());
      arrayCopy->ShallowCopy(pointArray);
      arrayCopy->SetName("PointLocations");
      outPD->AddArray(arrayCopy);
    }
    else
    {
      // Use slower API to get point positions
      svtkNew<svtkDoubleArray> pointArray;
      pointArray->SetName("PointLocations");
      pointArray->SetNumberOfComponents(3);
      svtkIdType numPoints = input->GetNumberOfPoints();
      pointArray->SetNumberOfTuples(numPoints);
      for (svtkIdType id = 0; id < numPoints; ++id)
      {
        double x[3];
        input->GetPoint(id, x);
        pointArray->SetTypedTuple(id, x);
      }
      outPD->AddArray(pointArray);
    }
  }

  this->UpdateProgress(1.0);
  return 1;
}

//----------------------------------------------------------------------------
int svtkAppendLocationAttributes::FillInputPortInformation(int, svtkInformation* info)
{
  info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkDataSet");
  return 1;
}

//----------------------------------------------------------------------------
void svtkAppendLocationAttributes::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "AppendPointLocations: " << (this->AppendPointLocations ? "On\n" : "Off\n");
  os << indent << "AppendCellCenters: " << (this->AppendCellCenters ? "On" : "Off") << endl;
}
