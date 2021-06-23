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
#include "svtkmContour.h"

#include "svtkCellData.h"
#include "svtkDataSet.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkNew.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkPolyData.h"
#include "svtkUnstructuredGrid.h"

#include "svtkmlib/ArrayConverters.h"
#include "svtkmlib/DataSetConverters.h"
#include "svtkmlib/PolyDataConverter.h"

#include "svtkmFilterPolicy.h"

#include <svtkm/cont/RuntimeDeviceTracker.h>
#include <svtkm/filter/Contour.h>
#include <svtkm/filter/Contour.hxx>

svtkStandardNewMacro(svtkmContour);

//------------------------------------------------------------------------------
svtkmContour::svtkmContour() {}

//------------------------------------------------------------------------------
svtkmContour::~svtkmContour() {}

//------------------------------------------------------------------------------
void svtkmContour::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
int svtkmContour::RequestData(
  svtkInformation* request, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  svtkm::cont::ScopedRuntimeDeviceTracker tracker(
    svtkm::cont::DeviceAdapterTagCuda{}, svtkm::cont::RuntimeDeviceTrackerMode::Disable);

  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);
  svtkDataSet* input = svtkDataSet::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));

  svtkPolyData* output = svtkPolyData::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  // Find the scalar array:
  int association = this->GetInputArrayAssociation(0, inputVector);
  svtkDataArray* inputArray = this->GetInputArrayToProcess(0, inputVector);
  if (association != svtkDataObject::FIELD_ASSOCIATION_POINTS || inputArray == nullptr ||
    inputArray->GetName() == nullptr || inputArray->GetName()[0] == '\0')
  {
    svtkErrorMacro("Invalid scalar array; array missing or not a point array.");
    return 0;
  }

  const int numContours = this->GetNumberOfContours();
  if (numContours == 0)
  {
    return 1;
  }

  try
  {
    svtkm::filter::Contour filter;
    filter.SetActiveField(inputArray->GetName(), svtkm::cont::Field::Association::POINTS);
    filter.SetGenerateNormals(this->GetComputeNormals() != 0);
    filter.SetNumberOfIsoValues(numContours);
    for (int i = 0; i < numContours; ++i)
    {
      filter.SetIsoValue(i, this->GetValue(i));
    }

    // convert the input dataset to a svtkm::cont::DataSet
    svtkm::cont::DataSet in;
    if (this->ComputeScalars)
    {
      in = tosvtkm::Convert(input, tosvtkm::FieldsFlag::PointsAndCells);
    }
    else
    {
      in = tosvtkm::Convert(input, tosvtkm::FieldsFlag::None);
      // explicitly convert just the field we need
      auto inField = tosvtkm::Convert(inputArray, association);
      in.AddField(inField);
      // don't pass this field
      filter.SetFieldsToPass(svtkm::filter::FieldSelection(svtkm::filter::FieldSelection::MODE_NONE));
    }

    svtkm::cont::DataSet result;
    svtkmInputFilterPolicy policy;

    result = filter.Execute(in, policy);

    // convert back the dataset to SVTK
    if (!fromsvtkm::Convert(result, output, input))
    {
      svtkWarningMacro(<< "Unable to convert SVTKm DataSet back to SVTK.\n"
                      << "Falling back to serial implementation.");
      return this->Superclass::RequestData(request, inputVector, outputVector);
    }

    if (this->ComputeScalars)
    {
      output->GetPointData()->SetActiveScalars(inputArray->GetName());
    }
    if (this->ComputeNormals)
    {
      output->GetPointData()->SetActiveAttribute(
        filter.GetNormalArrayName().c_str(), svtkDataSetAttributes::NORMALS);
    }
  }
  catch (const svtkm::cont::Error& e)
  {
    svtkErrorMacro(<< "SVTK-m error: " << e.GetMessage());
    return 0;
  }

  // we got this far, everything is good
  return 1;
}
