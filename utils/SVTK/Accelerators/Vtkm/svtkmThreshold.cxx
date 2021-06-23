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
#include "svtkmThreshold.h"
#include "svtkmConfig.h"

#include "svtkCellData.h"
#include "svtkDataSet.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkUnstructuredGrid.h"

#include "svtkmlib/ArrayConverters.h"
#include "svtkmlib/DataSetConverters.h"
#include "svtkmlib/UnstructuredGridConverter.h"

#include "svtkmFilterPolicy.h"

#include <svtkm/filter/Threshold.h>
#include <svtkm/filter/Threshold.hxx>

svtkStandardNewMacro(svtkmThreshold);

//------------------------------------------------------------------------------
svtkmThreshold::svtkmThreshold() {}

//------------------------------------------------------------------------------
svtkmThreshold::~svtkmThreshold() {}

//------------------------------------------------------------------------------
int svtkmThreshold::RequestData(
  svtkInformation* request, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);
  svtkDataSet* input = svtkDataSet::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));

  svtkUnstructuredGrid* output =
    svtkUnstructuredGrid::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  svtkDataArray* inputArray = this->GetInputArrayToProcess(0, inputVector);
  if (inputArray == nullptr || inputArray->GetName() == nullptr || inputArray->GetName()[0] == '\0')
  {
    svtkErrorMacro("Invalid input array.");
    return 0;
  }

  try
  {
    // convert the input dataset to a svtkm::cont::DataSet
    auto in = tosvtkm::Convert(input, tosvtkm::FieldsFlag::PointsAndCells);

    svtkmInputFilterPolicy policy;
    svtkm::filter::Threshold filter;
    filter.SetActiveField(inputArray->GetName());
    filter.SetLowerThreshold(this->GetLowerThreshold());
    filter.SetUpperThreshold(this->GetUpperThreshold());
    auto result = filter.Execute(in, policy);

    // now we are done the algorithm and conversion of arrays so
    // convert back the dataset to SVTK
    if (!fromsvtkm::Convert(result, output, input))
    {
      svtkErrorMacro(<< "Unable to convert SVTKm DataSet back to SVTK");
      return 0;
    }
  }
  catch (const svtkm::cont::Error& e)
  {
    svtkWarningMacro(<< "SVTK-m error: " << e.GetMessage()
                    << "Falling back to serial implementation");
    return this->Superclass::RequestData(request, inputVector, outputVector);
  }

  return 1;
}

//------------------------------------------------------------------------------
void svtkmThreshold::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
