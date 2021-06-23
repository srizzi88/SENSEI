//=============================================================================
//
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2012 Sandia Corporation.
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//=============================================================================
#include "svtkmExternalFaces.h"

#include "svtkCellData.h"
#include "svtkDemandDrivenPipeline.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkNew.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkUnsignedCharArray.h"
#include "svtkUnstructuredGrid.h"

#include "svtkmlib/ArrayConverters.h"
#include "svtkmlib/CellSetConverters.h"
#include "svtkmlib/DataSetConverters.h"
#include "svtkmlib/UnstructuredGridConverter.h"

#include "svtkmFilterPolicy.h"

#include <svtkm/filter/ExternalFaces.h>
#include <svtkm/filter/ExternalFaces.hxx>

svtkStandardNewMacro(svtkmExternalFaces);

//------------------------------------------------------------------------------
svtkmExternalFaces::svtkmExternalFaces()
  : CompactPoints(false)
{
  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(1);
}

//------------------------------------------------------------------------------
svtkmExternalFaces::~svtkmExternalFaces() {}

//------------------------------------------------------------------------------
void svtkmExternalFaces::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
void svtkmExternalFaces::SetInputData(svtkUnstructuredGrid* ds)
{
  this->SetInputDataObject(0, ds);
}

//------------------------------------------------------------------------------
svtkUnstructuredGrid* svtkmExternalFaces::GetOutput()
{
  return svtkUnstructuredGrid::SafeDownCast(this->GetOutputDataObject(0));
}

//------------------------------------------------------------------------------
int svtkmExternalFaces::FillInputPortInformation(int, svtkInformation* info)
{
  info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkUnstructuredGrid");
  info->Append(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkImageData");
  info->Append(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkStructuredGrid");
  info->Append(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkRectilinearGrid");
  return 1;
}

//------------------------------------------------------------------------------
int svtkmExternalFaces::FillOutputPortInformation(int svtkNotUsed(port), svtkInformation* info)
{
  info->Set(svtkDataObject::DATA_TYPE_NAME(), "svtkUnstructuredGrid");
  return 1;
}

//------------------------------------------------------------------------------
svtkTypeBool svtkmExternalFaces::ProcessRequest(
  svtkInformation* request, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // generate the data
  if (request->Has(svtkDemandDrivenPipeline::REQUEST_DATA()))
  {
    return this->RequestData(request, inputVector, outputVector);
  }

  return this->Superclass::ProcessRequest(request, inputVector, outputVector);
}

//------------------------------------------------------------------------------
int svtkmExternalFaces::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  svtkDataSet* input = svtkDataSet::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));
  svtkUnstructuredGrid* output =
    svtkUnstructuredGrid::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  try
  {
    // convert the input dataset to a svtkm::cont::DataSet
    auto in = tosvtkm::Convert(input, tosvtkm::FieldsFlag::PointsAndCells);

    // apply the filter
    svtkmInputFilterPolicy policy;
    svtkm::filter::ExternalFaces filter;
    filter.SetCompactPoints(this->CompactPoints);
    filter.SetPassPolyData(true);
    auto result = filter.Execute(in, policy);

    // convert back to svtkDataSet (svtkUnstructuredGrid)
    if (!fromsvtkm::Convert(result, output, input))
    {
      svtkErrorMacro(<< "Unable to convert SVTKm DataSet back to SVTK");
      return 0;
    }
  }
  catch (const svtkm::cont::Error& e)
  {
    svtkErrorMacro(<< "SVTK-m error: " << e.GetMessage());
    return 0;
  }

  return 1;
}
