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
#include "svtkmCleanGrid.h"

#include "svtkCellData.h"
#include "svtkDemandDrivenPipeline.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkUnstructuredGrid.h"

#include "svtkmlib/ArrayConverters.h"
#include "svtkmlib/DataSetConverters.h"
#include "svtkmlib/UnstructuredGridConverter.h"

#include "svtkmFilterPolicy.h"

#include <svtkm/filter/CleanGrid.h>
#include <svtkm/filter/CleanGrid.hxx>

svtkStandardNewMacro(svtkmCleanGrid);

//------------------------------------------------------------------------------
svtkmCleanGrid::svtkmCleanGrid()
  : CompactPoints(false)
{
}

//------------------------------------------------------------------------------
svtkmCleanGrid::~svtkmCleanGrid() {}

//------------------------------------------------------------------------------
void svtkmCleanGrid::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "CompactPoints: " << (this->CompactPoints ? "On" : "Off") << "\n";
}

//------------------------------------------------------------------------------
int svtkmCleanGrid::FillInputPortInformation(int, svtkInformation* info)
{
  info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkDataSet");
  return 1;
}

//------------------------------------------------------------------------------
int svtkmCleanGrid::RequestData(svtkInformation* svtkNotUsed(request),
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
    auto fieldsFlag = this->CompactPoints ? tosvtkm::FieldsFlag::Points : tosvtkm::FieldsFlag::None;
    svtkm::cont::DataSet in = tosvtkm::Convert(input, fieldsFlag);

    // apply the filter
    svtkmInputFilterPolicy policy;
    svtkm::filter::CleanGrid filter;
    filter.SetCompactPointFields(this->CompactPoints);
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

  // pass cell data
  if (!this->CompactPoints)
  {
    output->GetPointData()->PassData(input->GetPointData());
  }
  output->GetCellData()->PassData(input->GetCellData());

  return 1;
}
