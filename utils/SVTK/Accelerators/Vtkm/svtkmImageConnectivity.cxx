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
#include "svtkmImageConnectivity.h"

#include "svtkImageData.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkNew.h"
#include "svtkObjectFactory.h"

#include "svtkmlib/ArrayConverters.h"
#include "svtkmlib/DataSetConverters.h"
#include "svtkmlib/PolyDataConverter.h"

#include "svtkmFilterPolicy.h"

#include <svtkm/filter/ImageConnectivity.h>

svtkStandardNewMacro(svtkmImageConnectivity);

//------------------------------------------------------------------------------
svtkmImageConnectivity::svtkmImageConnectivity() {}

//------------------------------------------------------------------------------
svtkmImageConnectivity::~svtkmImageConnectivity() {}

//------------------------------------------------------------------------------
void svtkmImageConnectivity::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
int svtkmImageConnectivity::RequestData(
  svtkInformation*, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  svtkInformation* outInfo = outputVector->GetInformationObject(0);
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);

  svtkImageData* output = static_cast<svtkImageData*>(outInfo->Get(svtkDataObject::DATA_OBJECT()));
  svtkImageData* input = static_cast<svtkImageData*>(inInfo->Get(svtkDataObject::DATA_OBJECT()));

  // Find the scalar array:
  int association = this->GetInputArrayAssociation(0, inputVector);
  svtkDataArray* inputArray = this->GetInputArrayToProcess(0, inputVector);
  if (association != svtkDataObject::FIELD_ASSOCIATION_POINTS || inputArray == nullptr ||
    inputArray->GetName() == nullptr || inputArray->GetName()[0] == '\0')
  {
    svtkErrorMacro("Invalid scalar array; array missing or not a point array.");
    return 0;
  }

  try
  {
    svtkm::filter::ImageConnectivity filter;
    filter.SetActiveField(inputArray->GetName(), svtkm::cont::Field::Association::POINTS);
    // the field should be named 'RegionId'
    filter.SetOutputFieldName("RegionId");

    // explicitly convert just the field we need
    auto inData = tosvtkm::Convert(input, tosvtkm::FieldsFlag::None);
    auto inField = tosvtkm::Convert(inputArray, association);
    inData.AddField(inField);

    // don't pass this field
    filter.SetFieldsToPass(svtkm::filter::FieldSelection(svtkm::filter::FieldSelection::MODE_NONE));

    svtkm::cont::DataSet result;
    svtkmInputFilterPolicy policy;
    result = filter.Execute(inData, policy);

    // Make sure the output has all the fields / etc that the input has
    output->ShallowCopy(input);

    // convert back the regionId field to SVTK
    if (!fromsvtkm::ConvertArrays(result, output))
    {
      svtkWarningMacro(<< "Unable to convert SVTKm DataSet back to SVTK.\n"
                      << "Falling back to serial implementation.");
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
