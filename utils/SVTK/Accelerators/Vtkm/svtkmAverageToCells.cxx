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
#include "svtkmAverageToCells.h"

#include "svtkCellData.h"
#include "svtkDataSet.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkObjectFactory.h"

#include "svtkmlib/ArrayConverters.h"
#include "svtkmlib/DataSetConverters.h"

#include "svtkmFilterPolicy.h"

#include <svtkm/filter/CellAverage.h>
#include <svtkm/filter/CellAverage.hxx>

svtkStandardNewMacro(svtkmAverageToCells);

//------------------------------------------------------------------------------
svtkmAverageToCells::svtkmAverageToCells() {}

//------------------------------------------------------------------------------
svtkmAverageToCells::~svtkmAverageToCells() {}

//------------------------------------------------------------------------------
int svtkmAverageToCells::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);
  svtkDataSet* input = svtkDataSet::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));

  svtkDataSet* output = svtkDataSet::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  output->ShallowCopy(input);

  // grab the input array to process to determine the field we want to average
  int association = this->GetInputArrayAssociation(0, inputVector);
  auto fieldArray = this->GetInputArrayToProcess(0, inputVector);
  if (association != svtkDataObject::FIELD_ASSOCIATION_POINTS || fieldArray == nullptr ||
    fieldArray->GetName() == nullptr || fieldArray->GetName()[0] == '\0')
  {
    svtkErrorMacro(<< "Invalid field: Requires a point field with a valid name.");
    return 0;
  }

  const char* fieldName = fieldArray->GetName();

  try
  {
    // convert the input dataset to a svtkm::cont::DataSet
    svtkm::cont::DataSet in = tosvtkm::Convert(input);
    auto field = tosvtkm::Convert(fieldArray, association);
    in.AddField(field);

    svtkmInputFilterPolicy policy;
    svtkm::filter::CellAverage filter;
    filter.SetActiveField(fieldName, svtkm::cont::Field::Association::POINTS);
    filter.SetOutputFieldName(fieldName); // should we expose this control?

    auto result = filter.Execute(in, policy);

    // convert back the dataset to SVTK, and add the field as a cell field
    svtkDataArray* resultingArray = fromsvtkm::Convert(result.GetCellField(fieldName));
    if (resultingArray == nullptr)
    {
      svtkErrorMacro(<< "Unable to convert result array from SVTK-m to SVTK");
      return 0;
    }

    output->GetCellData()->AddArray(resultingArray);
    resultingArray->FastDelete();
  }
  catch (const svtkm::cont::Error& e)
  {
    svtkErrorMacro(<< "SVTK-m error: " << e.GetMessage());
    return 0;
  }

  return 1;
}

//------------------------------------------------------------------------------
void svtkmAverageToCells::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
