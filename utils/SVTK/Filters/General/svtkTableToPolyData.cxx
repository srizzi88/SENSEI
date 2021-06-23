/*=========================================================================

  Program:   ParaView
  Module:    svtkTableToPolyData.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkTableToPolyData.h"

#include "svtkCellArray.h"
#include "svtkDoubleArray.h"
#include "svtkInformation.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkPoints.h"
#include "svtkPolyData.h"
#include "svtkTable.h"

svtkStandardNewMacro(svtkTableToPolyData);
//----------------------------------------------------------------------------
svtkTableToPolyData::svtkTableToPolyData()
{
  this->XColumn = nullptr;
  this->YColumn = nullptr;
  this->ZColumn = nullptr;
  this->XColumnIndex = -1;
  this->YColumnIndex = -1;
  this->ZColumnIndex = -1;
  this->XComponent = 0;
  this->YComponent = 0;
  this->ZComponent = 0;
  this->Create2DPoints = 0;
  this->PreserveCoordinateColumnsAsDataArrays = false;
}

//----------------------------------------------------------------------------
svtkTableToPolyData::~svtkTableToPolyData()
{
  this->SetXColumn(nullptr);
  this->SetYColumn(nullptr);
  this->SetZColumn(nullptr);
}

//----------------------------------------------------------------------------
int svtkTableToPolyData::FillInputPortInformation(int svtkNotUsed(port), svtkInformation* info)
{
  info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkTable");
  return 1;
}

//----------------------------------------------------------------------------
int svtkTableToPolyData::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  svtkTable* input = svtkTable::GetData(inputVector[0], 0);
  svtkPolyData* output = svtkPolyData::GetData(outputVector, 0);

  if (input->GetNumberOfRows() == 0)
  {
    // empty input.
    return 1;
  }

  svtkDataArray* xarray = nullptr;
  svtkDataArray* yarray = nullptr;
  svtkDataArray* zarray = nullptr;

  if (this->XColumn && this->YColumn)
  {
    xarray = svtkArrayDownCast<svtkDataArray>(input->GetColumnByName(this->XColumn));
    yarray = svtkArrayDownCast<svtkDataArray>(input->GetColumnByName(this->YColumn));
    zarray = svtkArrayDownCast<svtkDataArray>(input->GetColumnByName(this->ZColumn));
  }
  else if (this->XColumnIndex >= 0)
  {
    xarray = svtkArrayDownCast<svtkDataArray>(input->GetColumn(this->XColumnIndex));
    yarray = svtkArrayDownCast<svtkDataArray>(input->GetColumn(this->YColumnIndex));
    zarray = svtkArrayDownCast<svtkDataArray>(input->GetColumn(this->ZColumnIndex));
  }

  // zarray is optional
  if (this->Create2DPoints)
  {
    if (!xarray || !yarray)
    {
      svtkErrorMacro("Failed to locate the columns to use for the point"
                    " coordinates");
      return 0;
    }
  }
  else
  {
    if (!xarray || !yarray || !zarray)
    {
      svtkErrorMacro("Failed to locate the columns to use for the point"
                    " coordinates");
      return 0;
    }
  }

  svtkPoints* newPoints = svtkPoints::New();

  if (xarray == yarray && yarray == zarray && this->XComponent == 0 && this->YComponent == 1 &&
    this->ZComponent == 2 && xarray->GetNumberOfComponents() == 3)
  {
    newPoints->SetData(xarray);
  }
  else
  {
    // Ideally we determine the smallest data type that can contain the values
    // in all the 3 arrays. For now I am just going with doubles.
    svtkDoubleArray* newData = svtkDoubleArray::New();
    newData->SetNumberOfComponents(3);
    newData->SetNumberOfTuples(input->GetNumberOfRows());
    svtkIdType numtuples = newData->GetNumberOfTuples();
    if (this->Create2DPoints)
    {
      for (svtkIdType cc = 0; cc < numtuples; cc++)
      {
        newData->SetComponent(cc, 0, xarray->GetComponent(cc, this->XComponent));
        newData->SetComponent(cc, 1, yarray->GetComponent(cc, this->YComponent));
        newData->SetComponent(cc, 2, 0.0);
      }
    }
    else
    {
      for (svtkIdType cc = 0; cc < numtuples; cc++)
      {
        newData->SetComponent(cc, 0, xarray->GetComponent(cc, this->XComponent));
        newData->SetComponent(cc, 1, yarray->GetComponent(cc, this->YComponent));
        newData->SetComponent(cc, 2, zarray->GetComponent(cc, this->ZComponent));
      }
    }
    newPoints->SetData(newData);
    newData->Delete();
  }

  output->SetPoints(newPoints);
  newPoints->Delete();

  // Now create a poly-vertex cell will all the points.
  svtkIdType numPts = newPoints->GetNumberOfPoints();
  svtkIdType* ptIds = new svtkIdType[numPts];
  for (svtkIdType cc = 0; cc < numPts; cc++)
  {
    ptIds[cc] = cc;
  }
  output->AllocateEstimate(1, 1);
  output->InsertNextCell(SVTK_POLY_VERTEX, numPts, ptIds);
  delete[] ptIds;

  // Add all other columns as point data.
  for (int cc = 0; cc < input->GetNumberOfColumns(); cc++)
  {
    svtkAbstractArray* arr = input->GetColumn(cc);
    if (this->PreserveCoordinateColumnsAsDataArrays)
    {
      output->GetPointData()->AddArray(arr);
    }
    else if (arr != xarray && arr != yarray && arr != zarray)
    {
      output->GetPointData()->AddArray(arr);
    }
  }
  return 1;
}

//----------------------------------------------------------------------------
void svtkTableToPolyData::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "XColumn: " << (this->XColumn ? this->XColumn : "(none)") << endl;
  os << indent << "XComponent: " << this->XComponent << endl;
  os << indent << "XColumnIndex: " << this->XColumnIndex << endl;
  os << indent << "YColumn: " << (this->YColumn ? this->YColumn : "(none)") << endl;
  os << indent << "YComponent: " << this->YComponent << endl;
  os << indent << "YColumnIndex: " << this->YColumnIndex << endl;
  os << indent << "ZColumn: " << (this->ZColumn ? this->ZColumn : "(none)") << endl;
  os << indent << "ZComponent: " << this->ZComponent << endl;
  os << indent << "ZColumnIndex: " << this->ZColumnIndex << endl;
  os << indent << "Create2DPoints: " << (this->Create2DPoints ? "true" : "false") << endl;
  os << indent << "PreserveCoordinateColumnsAsDataArrays: "
     << (this->PreserveCoordinateColumnsAsDataArrays ? "true" : "false") << endl;
}
