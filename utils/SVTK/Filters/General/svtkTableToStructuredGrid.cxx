/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkTableToStructuredGrid.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkTableToStructuredGrid.h"

#include "svtkDoubleArray.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkPoints.h"
#include "svtkStreamingDemandDrivenPipeline.h"
#include "svtkTable.h"

svtkStandardNewMacro(svtkTableToStructuredGrid);
//----------------------------------------------------------------------------
svtkTableToStructuredGrid::svtkTableToStructuredGrid()
{
  this->XColumn = nullptr;
  this->YColumn = nullptr;
  this->ZColumn = nullptr;
  this->XComponent = 0;
  this->YComponent = 0;
  this->ZComponent = 0;
  this->WholeExtent[0] = this->WholeExtent[1] = this->WholeExtent[2] = this->WholeExtent[3] =
    this->WholeExtent[4] = this->WholeExtent[5] = 0;
}

//----------------------------------------------------------------------------
svtkTableToStructuredGrid::~svtkTableToStructuredGrid()
{
  this->SetXColumn(nullptr);
  this->SetYColumn(nullptr);
  this->SetZColumn(nullptr);
}

//----------------------------------------------------------------------------
int svtkTableToStructuredGrid::FillInputPortInformation(int svtkNotUsed(port), svtkInformation* info)
{
  info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkTable");
  return 1;
}

// ----------------------------------------------------------------------------
int svtkTableToStructuredGrid::RequestInformation(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** svtkNotUsed(inputVector), svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* outInfo = outputVector->GetInformationObject(0);
  outInfo->Set(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), this->WholeExtent, 6);

  return 1;
}

//----------------------------------------------------------------------------
int svtkTableToStructuredGrid::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  svtkStructuredGrid* output = svtkStructuredGrid::GetData(outputVector, 0);
  svtkTable* input = svtkTable::GetData(inputVector[0], 0);

  svtkStreamingDemandDrivenPipeline* sddp =
    svtkStreamingDemandDrivenPipeline::SafeDownCast(this->GetExecutive());
  int extent[6];
  sddp->GetOutputInformation(0)->Get(svtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), extent);
  return this->Convert(input, output, extent);
}

//----------------------------------------------------------------------------
int svtkTableToStructuredGrid::Convert(svtkTable* input, svtkStructuredGrid* output, int extent[6])
{
  int num_values =
    (extent[1] - extent[0] + 1) * (extent[3] - extent[2] + 1) * (extent[5] - extent[4] + 1);

  if (input->GetNumberOfRows() != num_values)
  {
    svtkErrorMacro("The input table must have exactly " << num_values << " rows. Currently it has "
                                                       << input->GetNumberOfRows() << " rows.");
    return 0;
  }

  svtkDataArray* xarray = svtkArrayDownCast<svtkDataArray>(input->GetColumnByName(this->XColumn));
  svtkDataArray* yarray = svtkArrayDownCast<svtkDataArray>(input->GetColumnByName(this->YColumn));
  svtkDataArray* zarray = svtkArrayDownCast<svtkDataArray>(input->GetColumnByName(this->ZColumn));
  if (!xarray || !yarray || !zarray)
  {
    svtkErrorMacro("Failed to locate the columns to use for the point"
                  " coordinates");
    return 0;
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
    for (svtkIdType cc = 0; cc < numtuples; cc++)
    {
      newData->SetComponent(cc, 0, xarray->GetComponent(cc, this->XComponent));
      newData->SetComponent(cc, 1, yarray->GetComponent(cc, this->YComponent));
      newData->SetComponent(cc, 2, zarray->GetComponent(cc, this->ZComponent));
    }
    newPoints->SetData(newData);
    newData->Delete();
  }

  output->SetExtent(extent);
  output->SetPoints(newPoints);
  newPoints->Delete();

  // Add all other columns as point data.
  for (int cc = 0; cc < input->GetNumberOfColumns(); cc++)
  {
    svtkAbstractArray* arr = input->GetColumn(cc);
    if (arr != xarray && arr != yarray && arr != zarray)
    {
      output->GetPointData()->AddArray(arr);
    }
  }
  return 1;
}

//----------------------------------------------------------------------------
void svtkTableToStructuredGrid::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "WholeExtent: " << this->WholeExtent[0] << ", " << this->WholeExtent[1] << ", "
     << this->WholeExtent[2] << ", " << this->WholeExtent[3] << ", " << this->WholeExtent[4] << ", "
     << this->WholeExtent[5] << endl;
  os << indent << "XColumn: " << (this->XColumn ? this->XColumn : "(none)") << endl;
  os << indent << "XComponent: " << this->XComponent << endl;
  os << indent << "YColumn: " << (this->YColumn ? this->YColumn : "(none)") << endl;
  os << indent << "YComponent: " << this->YComponent << endl;
  os << indent << "ZColumn: " << (this->ZColumn ? this->ZColumn : "(none)") << endl;
  os << indent << "ZComponent: " << this->ZComponent << endl;
}
