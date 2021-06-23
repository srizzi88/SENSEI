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
#include "svtkmPointElevation.h"
#include "svtkmConfig.h"

#include "svtkCellData.h"
#include "svtkDataSet.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkPointData.h"
#include "svtkUnstructuredGrid.h"

#include "svtkmlib/ArrayConverters.h"
#include "svtkmlib/DataSetConverters.h"

#include "svtkmFilterPolicy.h"

#include <svtkm/filter/PointElevation.h>

svtkStandardNewMacro(svtkmPointElevation);

//------------------------------------------------------------------------------
svtkmPointElevation::svtkmPointElevation() {}

//------------------------------------------------------------------------------
svtkmPointElevation::~svtkmPointElevation() {}

//------------------------------------------------------------------------------
int svtkmPointElevation::RequestData(
  svtkInformation* request, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // Get the input and output data objects.
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);
  svtkDataSet* input = svtkDataSet::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));

  svtkDataSet* output = svtkDataSet::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  output->ShallowCopy(input);
  // Check the size of the input.
  svtkIdType numPts = input->GetNumberOfPoints();
  if (numPts < 1)
  {
    svtkDebugMacro("No input!");
    return 1;
  }

  try
  {
    // Convert the input dataset to a svtkm::cont::DataSet
    auto in = tosvtkm::Convert(input, tosvtkm::FieldsFlag::Points);

    svtkmInputFilterPolicy policy;
    // Setup input
    svtkm::filter::PointElevation filter;
    filter.SetLowPoint(this->LowPoint[0], this->LowPoint[1], this->LowPoint[2]);
    filter.SetHighPoint(this->HighPoint[0], this->HighPoint[1], this->HighPoint[2]);
    filter.SetRange(this->ScalarRange[0], this->ScalarRange[1]);
    filter.SetOutputFieldName("elevation");
    filter.SetUseCoordinateSystemAsField(true);
    auto result = filter.Execute(in, policy);

    // Convert the result back
    svtkDataArray* resultingArray = fromsvtkm::Convert(result.GetField("elevation"));
    if (resultingArray == nullptr)
    {
      svtkErrorMacro(<< "Unable to convert result array from SVTK-m to SVTK");
      return 0;
    }
    output->GetPointData()->AddArray(resultingArray);
    output->GetPointData()->SetActiveScalars("elevation");
    resultingArray->FastDelete();
  }
  catch (const svtkm::cont::Error& e)
  {
    svtkErrorMacro(<< "SVTK-m error: " << e.GetMessage() << "Falling back to serial implementation");
    return this->Superclass::RequestData(request, inputVector, outputVector);
  }
  return 1;
}

//------------------------------------------------------------------------------
void svtkmPointElevation::PrintSelf(std::ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
