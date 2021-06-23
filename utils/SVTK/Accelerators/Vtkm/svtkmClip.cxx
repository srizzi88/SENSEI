/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkmClip.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkmClip.h"

#include "svtkCellIterator.h"
#include "svtkDataArray.h"
#include "svtkDataSet.h"
#include "svtkDemandDrivenPipeline.h"
#include "svtkImplicitFunction.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkPolyData.h"
#include "svtkTableBasedClipDataSet.h"
#include "svtkUnstructuredGrid.h"

#include "svtkmlib/ArrayConverters.h"
#include "svtkmlib/DataSetConverters.h"
#include "svtkmlib/ImplicitFunctionConverter.h"
#include "svtkmlib/PolyDataConverter.h"
#include "svtkmlib/UnstructuredGridConverter.h"

#include "svtkmFilterPolicy.h"

#include <svtkm/cont/RuntimeDeviceTracker.h>

#include <svtkm/filter/ClipWithField.h>
#include <svtkm/filter/ClipWithField.hxx>
#include <svtkm/filter/ClipWithImplicitFunction.h>
#include <svtkm/filter/ClipWithImplicitFunction.hxx>

#include <algorithm>

svtkStandardNewMacro(svtkmClip);

//------------------------------------------------------------------------------
void svtkmClip::PrintSelf(std::ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "ClipValue: " << this->ClipValue << "\n";
  os << indent << "ClipFunction: \n";
  this->ClipFunction->PrintSelf(os, indent.GetNextIndent());
  os << indent << "ComputeScalars: " << this->ComputeScalars << "\n";
}

//------------------------------------------------------------------------------
svtkmClip::svtkmClip()
  : ClipValue(0.)
  , ComputeScalars(true)
  , ClipFunction(nullptr)
  , ClipFunctionConverter(new tosvtkm::ImplicitFunctionConverter)
{
  // Clip active point scalars by default
  this->SetInputArrayToProcess(
    0, 0, 0, svtkDataObject::FIELD_ASSOCIATION_POINTS, svtkDataSetAttributes::SCALARS);
}

//------------------------------------------------------------------------------
svtkmClip::~svtkmClip() {}

//----------------------------------------------------------------------------
svtkMTimeType svtkmClip::GetMTime()
{
  svtkMTimeType mTime = this->Superclass::GetMTime();
  if (this->ClipFunction)
  {
    mTime = std::max(mTime, this->ClipFunction->GetMTime());
  }
  return mTime;
}

//----------------------------------------------------------------------------
void svtkmClip::SetClipFunction(svtkImplicitFunction* clipFunction)
{
  if (this->ClipFunction != clipFunction)
  {
    this->ClipFunction = clipFunction;
    this->ClipFunctionConverter->Set(clipFunction);
    this->Modified();
  }
}

//------------------------------------------------------------------------------
int svtkmClip::RequestData(
  svtkInformation*, svtkInformationVector** inInfoVec, svtkInformationVector* outInfoVec)
{
  svtkm::cont::ScopedRuntimeDeviceTracker tracker(
    svtkm::cont::DeviceAdapterTagCuda{}, svtkm::cont::RuntimeDeviceTrackerMode::Disable);

  svtkInformation* inInfo = inInfoVec[0]->GetInformationObject(0);
  svtkInformation* outInfo = outInfoVec->GetInformationObject(0);

  // Extract data objects from info:
  svtkDataSet* input = svtkDataSet::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));
  svtkUnstructuredGrid* output =
    svtkUnstructuredGrid::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  // Find the scalar array:
  int assoc = this->GetInputArrayAssociation(0, inInfoVec);
  svtkDataArray* scalars = this->GetInputArrayToProcess(0, inInfoVec);
  if (!this->ClipFunction &&
    (assoc != svtkDataObject::FIELD_ASSOCIATION_POINTS || scalars == nullptr ||
      scalars->GetName() == nullptr || scalars->GetName()[0] == '\0'))
  {
    svtkErrorMacro("Invalid scalar array; array missing or not a point array.");
    return 0;
  }

  // Validate input objects:
  if (input->GetNumberOfPoints() == 0 || input->GetNumberOfCells() == 0)
  {
    return 1; // nothing to do
  }

  try
  {
    // Convert inputs to svtkm objects:
    auto fieldsFlag =
      this->ComputeScalars ? tosvtkm::FieldsFlag::PointsAndCells : tosvtkm::FieldsFlag::None;
    auto in = tosvtkm::Convert(input, fieldsFlag);

    // Run filter:
    svtkm::cont::DataSet result;
    svtkmInputFilterPolicy policy;
    if (this->ClipFunction)
    {
      svtkm::filter::ClipWithImplicitFunction functionFilter;
      auto function = this->ClipFunctionConverter->Get();
      if (function.GetValid())
      {
        functionFilter.SetImplicitFunction(function);
        result = functionFilter.Execute(in, policy);
      }
    }
    else
    {
      svtkm::filter::ClipWithField fieldFilter;
      if (!this->ComputeScalars)
      {
        // explicitly convert just the field we need
        auto inField = tosvtkm::Convert(scalars, assoc);
        in.AddField(inField);
        // don't pass this field
        fieldFilter.SetFieldsToPass(
          svtkm::filter::FieldSelection(svtkm::filter::FieldSelection::MODE_NONE));
      }

      fieldFilter.SetActiveField(scalars->GetName(), svtkm::cont::Field::Association::POINTS);
      fieldFilter.SetClipValue(this->ClipValue);
      result = fieldFilter.Execute(in, policy);
    }

    // Convert result to output:
    if (!fromsvtkm::Convert(result, output, input))
    {
      svtkErrorMacro("Error generating svtkUnstructuredGrid from svtkm's result.");
      return 0;
    }

    if (!this->ClipFunction && this->ComputeScalars)
    {
      output->GetPointData()->SetActiveScalars(scalars->GetName());
    }

    return 1;
  }
  catch (const svtkm::cont::Error& e)
  {
    svtkWarningMacro(<< "SVTK-m error: " << e.GetMessage()
                    << "Falling back to serial implementation.");

    svtkNew<svtkTableBasedClipDataSet> filter;
    filter->SetClipFunction(this->ClipFunction);
    filter->SetValue(this->ClipValue);
    filter->SetInputData(input);
    filter->Update();
    output->ShallowCopy(filter->GetOutput());
    return 1;
  }
}

//------------------------------------------------------------------------------
int svtkmClip::FillInputPortInformation(int, svtkInformation* info)
{
  // These are the types supported by tosvtkm::Convert:
  info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkPolyData");
  info->Append(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkUnstructuredGrid");
  info->Append(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkStructuredGrid");
  info->Append(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkUniformGrid");
  info->Append(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkImageData");
  return 1;
}
