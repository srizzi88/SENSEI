/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkmPolyDataNormals.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkmPolyDataNormals.h"

#include "svtkCellData.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkPointData.h"
#include "svtkPolyData.h"
#include "svtkSmartPointer.h"

#include "svtkmlib/ArrayConverters.h"
#include "svtkmlib/PolyDataConverter.h"

#include "svtkmFilterPolicy.h"

#include "svtkm/filter/SurfaceNormals.h"

svtkStandardNewMacro(svtkmPolyDataNormals);

//------------------------------------------------------------------------------
void svtkmPolyDataNormals::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
svtkmPolyDataNormals::svtkmPolyDataNormals()
{
  // change defaults from parent
  this->Splitting = 0;
  this->Consistency = 0;
  this->FlipNormals = 0;
  this->ComputePointNormals = 1;
  this->ComputeCellNormals = 0;
  this->AutoOrientNormals = 0;
}

//------------------------------------------------------------------------------
svtkmPolyDataNormals::~svtkmPolyDataNormals() = default;

//------------------------------------------------------------------------------
int svtkmPolyDataNormals::RequestData(
  svtkInformation* request, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  svtkPolyData* input = svtkPolyData::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));
  svtkPolyData* output = svtkPolyData::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  try
  {
    // convert the input dataset to a svtkm::cont::DataSet
    auto in = tosvtkm::Convert(input, tosvtkm::FieldsFlag::None);

    svtkm::cont::DataSet result;

    // check for flags that svtkm filter cannot handle
    bool unsupported = this->Splitting != 0;
    if (!unsupported)
    {
      svtkmInputFilterPolicy policy;
      svtkm::filter::SurfaceNormals filter;
      filter.SetGenerateCellNormals((this->ComputeCellNormals != 0));
      filter.SetCellNormalsName("Normals");
      filter.SetGeneratePointNormals((this->ComputePointNormals != 0));
      filter.SetPointNormalsName("Normals");
      filter.SetAutoOrientNormals(this->AutoOrientNormals != 0);
      filter.SetFlipNormals(this->FlipNormals != 0);
      filter.SetConsistency(this->Consistency != 0);
      result = filter.Execute(in, policy);
    }
    else
    {
      svtkWarningMacro(<< "Unsupported options\n"
                      << "Falling back to svtkPolyDataNormals.");
      return this->Superclass::RequestData(request, inputVector, outputVector);
    }

    if (!fromsvtkm::Convert(result, output, input))
    {
      svtkErrorMacro(<< "Unable to convert SVTKm DataSet back to SVTK");
      return 0;
    }
  }
  catch (const svtkm::cont::Error& e)
  {
    svtkWarningMacro(<< "SVTK-m error: " << e.GetMessage() << "Falling back to svtkPolyDataNormals");
    return this->Superclass::RequestData(request, inputVector, outputVector);
  }

  svtkSmartPointer<svtkDataArray> pointNormals = output->GetPointData()->GetArray("Normals");
  svtkSmartPointer<svtkDataArray> cellNormals = output->GetCellData()->GetArray("Normals");

  output->GetPointData()->CopyNormalsOff();
  output->GetPointData()->PassData(input->GetPointData());
  output->GetCellData()->CopyNormalsOff();
  output->GetCellData()->PassData(input->GetPointData());

  if (pointNormals)
  {
    output->GetPointData()->SetNormals(pointNormals);
  }
  if (cellNormals)
  {
    output->GetCellData()->SetNormals(cellNormals);
  }

  return 1;
}
