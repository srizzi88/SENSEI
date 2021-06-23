// -*- c++ -*-
/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkTemporalStatistics.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/*
 * Copyright 2008 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

#include "svtkTemporalStatistics.h"

#include "svtkArrayDispatch.h"
#include "svtkCellData.h"
#include "svtkCompositeDataIterator.h"
#include "svtkCompositeDataSet.h"
#include "svtkDataArray.h"
#include "svtkDataArrayRange.h"
#include "svtkDataSet.h"
#include "svtkGraph.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkMultiBlockDataSet.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkStdString.h"
#include "svtkStreamingDemandDrivenPipeline.h"

#include "svtkSmartPointer.h"

#include <algorithm>
#include <functional>

//=============================================================================
svtkStandardNewMacro(svtkTemporalStatistics);

namespace
{

//=============================================================================
const char* const AVERAGE_SUFFIX = "average";
const char* const MINIMUM_SUFFIX = "minimum";
const char* const MAXIMUM_SUFFIX = "maximum";
const char* const STANDARD_DEVIATION_SUFFIX = "stddev";

inline svtkStdString svtkTemporalStatisticsMangleName(const char* originalName, const char* suffix)
{
  if (!originalName)
    return suffix;
  return svtkStdString(originalName) + "_" + svtkStdString(suffix);
}

//-----------------------------------------------------------------------------
struct AccumulateAverage
{
  template <typename InArrayT, typename OutArrayT>
  void operator()(InArrayT* inArray, OutArrayT* outArray) const
  {
    // These share APIType:
    using T = svtk::GetAPIType<InArrayT>;

    const auto in = svtk::DataArrayValueRange(inArray);
    auto out = svtk::DataArrayValueRange(outArray);

    std::transform(in.cbegin(), in.cend(), out.cbegin(), out.begin(), std::plus<T>{});
  }
};

struct AccumulateMinimum
{
  template <typename InArrayT, typename OutArrayT>
  void operator()(InArrayT* inArray, OutArrayT* outArray) const
  {
    // These share APIType:
    using T = svtk::GetAPIType<InArrayT>;

    const auto in = svtk::DataArrayValueRange(inArray);
    auto out = svtk::DataArrayValueRange(outArray);

    std::transform(in.cbegin(), in.cend(), out.cbegin(), out.begin(),
      [](T v1, T v2) -> T { return std::min(v1, v2); });
  }
};

struct AccumulateMaximum
{
  template <typename InArrayT, typename OutArrayT>
  void operator()(InArrayT* inArray, OutArrayT* outArray) const
  {
    // These share APIType:
    using T = svtk::GetAPIType<InArrayT>;

    const auto in = svtk::DataArrayValueRange(inArray);
    auto out = svtk::DataArrayValueRange(outArray);

    std::transform(in.cbegin(), in.cend(), out.cbegin(), out.begin(),
      [](T v1, T v2) -> T { return std::max(v1, v2); });
  }
};

// standard deviation one-pass algorithm from
// http://www.cs.berkeley.edu/~mhoemmen/cs194/Tutorials/variance.pdf
// this is numerically stable!
struct AccumulateStdDev
{
  template <typename InArrayT, typename OutArrayT, typename PrevArrayT>
  void operator()(InArrayT* inArray, OutArrayT* outArray, PrevArrayT* prevArray, int passIn) const
  {
    // All arrays have the same valuetype:
    using T = svtk::GetAPIType<InArrayT>;

    const double pass = static_cast<double>(passIn);

    const auto inValues = svtk::DataArrayValueRange(inArray);
    const auto prevValues = svtk::DataArrayValueRange(prevArray);
    auto outValues = svtk::DataArrayValueRange(outArray);

    for (svtkIdType i = 0; i < inValues.size(); ++i)
    {
      const double temp = inValues[i] - (prevValues[i] / pass);
      outValues[i] += static_cast<T>(pass * temp * temp / (pass + 1.));
    }
  }
};

//-----------------------------------------------------------------------------
struct FinishAverage
{
  template <typename ArrayT>
  void operator()(ArrayT* array, int sumSize) const
  {
    auto range = svtk::DataArrayValueRange(array);
    using RefT = typename decltype(range)::ReferenceType;
    for (RefT ref : range)
    {
      ref /= sumSize;
    }
  }
};

//-----------------------------------------------------------------------------
struct FinishStdDev
{
  template <typename ArrayT>
  void operator()(ArrayT* array, int sumSizeIn) const
  {
    const double sumSize = static_cast<double>(sumSizeIn);
    auto range = svtk::DataArrayValueRange(array);
    using RefT = typename decltype(range)::ReferenceType;
    using ValueT = typename decltype(range)::ValueType;
    for (RefT ref : range)
    {
      ref = static_cast<ValueT>(std::sqrt(static_cast<double>(ref) / sumSize));
    }
  }
};

} // end anon namespace

//=============================================================================
svtkTemporalStatistics::svtkTemporalStatistics()
{
  this->ComputeAverage = 1;
  this->ComputeMinimum = 1;
  this->ComputeMaximum = 1;
  this->ComputeStandardDeviation = 1;

  this->CurrentTimeIndex = 0;
  this->GeneratedChangingTopologyWarning = false;
}

svtkTemporalStatistics::~svtkTemporalStatistics() = default;

void svtkTemporalStatistics::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "ComputeAverage: " << this->ComputeAverage << endl;
  os << indent << "ComputeMinimum: " << this->ComputeMinimum << endl;
  os << indent << "ComputeMaximum: " << this->ComputeMaximum << endl;
  os << indent << "ComputeStandardDeviation: " << this->ComputeStandardDeviation << endl;
}

//-----------------------------------------------------------------------------
int svtkTemporalStatistics::FillInputPortInformation(int svtkNotUsed(port), svtkInformation* info)
{
  info->Remove(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE());
  info->Append(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkDataSet");
  info->Append(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkGraph");
  info->Append(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkCompositeDataSet");
  return 1;
}

//-----------------------------------------------------------------------------
int svtkTemporalStatistics::RequestInformation(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** svtkNotUsed(inputVector), svtkInformationVector* outputVector)
{
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  // The output data of this filter has no time associated with it.  It is the
  // result of computations that happen over all time.
  outInfo->Remove(svtkStreamingDemandDrivenPipeline::TIME_STEPS());
  outInfo->Remove(svtkStreamingDemandDrivenPipeline::TIME_RANGE());

  return 1;
}

//-----------------------------------------------------------------------------
int svtkTemporalStatistics::RequestDataObject(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  svtkDataObject* input = svtkDataObject::GetData(inInfo);
  svtkDataObject* output = svtkDataObject::GetData(outInfo);

  if (!input)
  {
    return 0;
  }

  svtkSmartPointer<svtkDataObject> newOutput;

  if (!output || !output->IsA(input->GetClassName()))
  {
    newOutput.TakeReference(input->NewInstance());
  }

  if (newOutput)
  {
    outInfo->Set(svtkDataObject::DATA_OBJECT(), newOutput);
  }

  return 1;
}

//-----------------------------------------------------------------------------
int svtkTemporalStatistics::RequestUpdateExtent(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* svtkNotUsed(outputVector))
{
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);

  // The RequestData method will tell the pipeline executive to iterate the
  // upstream pipeline to get each time step in order.  The executive in turn
  // will call this method to get the extent request for each iteration (in this
  // case the time step).
  double* inTimes = inInfo->Get(svtkStreamingDemandDrivenPipeline::TIME_STEPS());
  if (inTimes)
  {
    inInfo->Set(
      svtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP(), inTimes[this->CurrentTimeIndex]);
  }

  return 1;
}

//-----------------------------------------------------------------------------
int svtkTemporalStatistics::RequestData(
  svtkInformation* request, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  svtkDataObject* input = svtkDataObject::GetData(inInfo);
  svtkDataObject* output = svtkDataObject::GetData(outInfo);

  if (this->CurrentTimeIndex == 0)
  {
    // First execution, initialize arrays.
    this->InitializeStatistics(input, output);
  }
  else
  {
    // Subsequent execution, accumulate new data.
    this->AccumulateStatistics(input, output);
  }

  this->CurrentTimeIndex++;

  if (this->CurrentTimeIndex < inInfo->Length(svtkStreamingDemandDrivenPipeline::TIME_STEPS()))
  {
    // There is still more to do.
    request->Set(svtkStreamingDemandDrivenPipeline::CONTINUE_EXECUTING(), 1);
  }
  else
  {
    // We are done.  Finish up.
    this->PostExecute(input, output);
    request->Remove(svtkStreamingDemandDrivenPipeline::CONTINUE_EXECUTING());
    this->CurrentTimeIndex = 0;
  }

  return 1;
}

//-----------------------------------------------------------------------------
void svtkTemporalStatistics::InitializeStatistics(svtkDataObject* input, svtkDataObject* output)
{
  if (input->IsA("svtkDataSet"))
  {
    this->InitializeStatistics(svtkDataSet::SafeDownCast(input), svtkDataSet::SafeDownCast(output));
    return;
  }

  if (input->IsA("svtkGraph"))
  {
    this->InitializeStatistics(svtkGraph::SafeDownCast(input), svtkGraph::SafeDownCast(output));
    return;
  }

  if (input->IsA("svtkCompositeDataSet"))
  {
    this->InitializeStatistics(
      svtkCompositeDataSet::SafeDownCast(input), svtkCompositeDataSet::SafeDownCast(output));
    return;
  }

  svtkWarningMacro(<< "Unsupported input type: " << input->GetClassName());
}

//-----------------------------------------------------------------------------
void svtkTemporalStatistics::InitializeStatistics(svtkDataSet* input, svtkDataSet* output)
{
  output->CopyStructure(input);
  this->InitializeArrays(input->GetFieldData(), output->GetFieldData());
  this->InitializeArrays(input->GetPointData(), output->GetPointData());
  this->InitializeArrays(input->GetCellData(), output->GetCellData());
}

//-----------------------------------------------------------------------------
void svtkTemporalStatistics::InitializeStatistics(svtkGraph* input, svtkGraph* output)
{
  output->CopyStructure(input);
  this->InitializeArrays(input->GetFieldData(), output->GetFieldData());
  this->InitializeArrays(input->GetVertexData(), output->GetVertexData());
  this->InitializeArrays(input->GetEdgeData(), output->GetEdgeData());
}

//-----------------------------------------------------------------------------
void svtkTemporalStatistics::InitializeStatistics(
  svtkCompositeDataSet* input, svtkCompositeDataSet* output)
{
  output->CopyStructure(input);

  svtkSmartPointer<svtkCompositeDataIterator> inputItr;
  inputItr.TakeReference(input->NewIterator());

  for (inputItr->InitTraversal(); !inputItr->IsDoneWithTraversal(); inputItr->GoToNextItem())
  {
    svtkDataObject* inputObj = inputItr->GetCurrentDataObject();

    svtkSmartPointer<svtkDataObject> outputObj;
    outputObj.TakeReference(inputObj->NewInstance());

    this->InitializeStatistics(inputObj, outputObj);
    output->SetDataSet(inputItr, outputObj);
  }
}

//-----------------------------------------------------------------------------
void svtkTemporalStatistics::InitializeArrays(svtkFieldData* inFd, svtkFieldData* outFd)
{
  // Because we need to do mathematical operations, we require all arrays we
  // process to be numeric data (i.e. a svtkDataArray).  We also handle global
  // ids and petigree ids special (we just pass them).  Ideally would just let
  // svtkFieldData or svtkDataSetAttributes handle this for us, but no such method
  // that fits our needs here.  Thus, we pass data a bit differently then other
  // filters.  If I miss something important, it should be added here.

  outFd->Initialize();

  svtkDataSetAttributes* inDsa = svtkDataSetAttributes::SafeDownCast(inFd);
  svtkDataSetAttributes* outDsa = svtkDataSetAttributes::SafeDownCast(outFd);
  if (inDsa)
  {
    svtkDataArray* globalIds = inDsa->GetGlobalIds();
    svtkAbstractArray* pedigreeIds = inDsa->GetPedigreeIds();
    if (globalIds)
      outDsa->SetGlobalIds(globalIds);
    if (pedigreeIds)
      outDsa->SetPedigreeIds(pedigreeIds);
  }

  int numArrays = inFd->GetNumberOfArrays();
  for (int i = 0; i < numArrays; i++)
  {
    svtkDataArray* array = inFd->GetArray(i);
    if (!array)
      continue; // Array not numeric.
    if (outFd->HasArray(array->GetName()))
      continue; // Must be Ids.

    this->InitializeArray(array, outFd);
  }
}

//-----------------------------------------------------------------------------
void svtkTemporalStatistics::InitializeArray(svtkDataArray* array, svtkFieldData* outFd)
{
  if (this->ComputeAverage || this->ComputeStandardDeviation)
  {
    svtkSmartPointer<svtkDataArray> newArray;
    newArray.TakeReference(
      svtkArrayDownCast<svtkDataArray>(svtkAbstractArray::CreateArray(array->GetDataType())));
    newArray->DeepCopy(array);
    newArray->SetName(svtkTemporalStatisticsMangleName(array->GetName(), AVERAGE_SUFFIX));
    if (outFd->HasArray(newArray->GetName()))
    {
      svtkWarningMacro(<< "Input has two arrays named " << array->GetName()
                      << ".  Output statistics will probably be wrong.");
      return;
    }
    outFd->AddArray(newArray);
  }

  if (this->ComputeMinimum)
  {
    svtkSmartPointer<svtkDataArray> newArray;
    newArray.TakeReference(
      svtkArrayDownCast<svtkDataArray>(svtkAbstractArray::CreateArray(array->GetDataType())));
    newArray->DeepCopy(array);
    newArray->SetName(svtkTemporalStatisticsMangleName(array->GetName(), MINIMUM_SUFFIX));
    outFd->AddArray(newArray);
  }

  if (this->ComputeMaximum)
  {
    svtkSmartPointer<svtkDataArray> newArray;
    newArray.TakeReference(
      svtkArrayDownCast<svtkDataArray>(svtkAbstractArray::CreateArray(array->GetDataType())));
    newArray->DeepCopy(array);
    newArray->SetName(svtkTemporalStatisticsMangleName(array->GetName(), MAXIMUM_SUFFIX));
    outFd->AddArray(newArray);
  }

  if (this->ComputeStandardDeviation)
  {
    svtkSmartPointer<svtkDataArray> newArray;
    newArray.TakeReference(
      svtkArrayDownCast<svtkDataArray>(svtkAbstractArray::CreateArray(array->GetDataType())));
    newArray->SetName(svtkTemporalStatisticsMangleName(array->GetName(), STANDARD_DEVIATION_SUFFIX));

    newArray->SetNumberOfComponents(array->GetNumberOfComponents());
    newArray->CopyComponentNames(array);

    newArray->SetNumberOfTuples(array->GetNumberOfTuples());
    newArray->Fill(0.);
    outFd->AddArray(newArray);
  }
}

//-----------------------------------------------------------------------------
void svtkTemporalStatistics::AccumulateStatistics(svtkDataObject* input, svtkDataObject* output)
{
  if (input->IsA("svtkDataSet"))
  {
    this->AccumulateStatistics(svtkDataSet::SafeDownCast(input), svtkDataSet::SafeDownCast(output));
    return;
  }

  if (input->IsA("svtkGraph"))
  {
    this->AccumulateStatistics(svtkGraph::SafeDownCast(input), svtkGraph::SafeDownCast(output));
    return;
  }

  if (input->IsA("svtkCompositeDataSet"))
  {
    this->AccumulateStatistics(
      svtkCompositeDataSet::SafeDownCast(input), svtkCompositeDataSet::SafeDownCast(output));
  }
}

//-----------------------------------------------------------------------------
void svtkTemporalStatistics::AccumulateStatistics(svtkDataSet* input, svtkDataSet* output)
{
  this->AccumulateArrays(input->GetFieldData(), output->GetFieldData());
  this->AccumulateArrays(input->GetPointData(), output->GetPointData());
  this->AccumulateArrays(input->GetCellData(), output->GetCellData());
}

//-----------------------------------------------------------------------------
void svtkTemporalStatistics::AccumulateStatistics(svtkGraph* input, svtkGraph* output)
{
  this->AccumulateArrays(input->GetFieldData(), output->GetFieldData());
  this->AccumulateArrays(input->GetVertexData(), output->GetVertexData());
  this->AccumulateArrays(input->GetEdgeData(), output->GetEdgeData());
}

//-----------------------------------------------------------------------------
void svtkTemporalStatistics::AccumulateStatistics(
  svtkCompositeDataSet* input, svtkCompositeDataSet* output)
{
  svtkSmartPointer<svtkCompositeDataIterator> inputItr;
  inputItr.TakeReference(input->NewIterator());

  for (inputItr->InitTraversal(); !inputItr->IsDoneWithTraversal(); inputItr->GoToNextItem())
  {
    svtkDataObject* inputObj = inputItr->GetCurrentDataObject();
    svtkDataObject* outputObj = output->GetDataSet(inputItr);

    this->AccumulateStatistics(inputObj, outputObj);
  }
}

//-----------------------------------------------------------------------------
void svtkTemporalStatistics::AccumulateArrays(svtkFieldData* inFd, svtkFieldData* outFd)
{
  int numArrays = inFd->GetNumberOfArrays();
  for (int i = 0; i < numArrays; i++)
  {
    svtkDataArray* inArray = inFd->GetArray(i);
    svtkDataArray* outArray;
    if (!inArray)
    {
      continue;
    }

    outArray = this->GetArray(outFd, inArray, AVERAGE_SUFFIX);
    if (outArray)
    {

      svtkDataArray* stdevOutArray = this->GetArray(outFd, inArray, STANDARD_DEVIATION_SUFFIX);
      if (stdevOutArray)
      {
        using Dispatcher = svtkArrayDispatch::Dispatch3SameValueType;
        AccumulateStdDev worker;
        if (!Dispatcher::Execute(inArray, stdevOutArray, outArray, worker, this->CurrentTimeIndex))
        { // Fallback to slow path:
          worker(inArray, stdevOutArray, outArray, this->CurrentTimeIndex);
        }

        // Alert change in data.
        stdevOutArray->DataChanged();
      }

      using Dispatcher = svtkArrayDispatch::Dispatch2SameValueType;
      AccumulateAverage worker;
      if (!Dispatcher::Execute(inArray, outArray, worker))
      { // Fallback to slow path:
        worker(inArray, stdevOutArray);
      }

      // Alert change in data.
      outArray->DataChanged();
    }

    outArray = this->GetArray(outFd, inArray, MINIMUM_SUFFIX);
    if (outArray)
    {
      using Dispatcher = svtkArrayDispatch::Dispatch2SameValueType;
      AccumulateMinimum worker;
      if (!Dispatcher::Execute(inArray, outArray, worker))
      { // Fallback to slow path:
        worker(inArray, outArray);
      }

      // Alert change in data.
      outArray->DataChanged();
    }

    outArray = this->GetArray(outFd, inArray, MAXIMUM_SUFFIX);
    if (outArray)
    {
      using Dispatcher = svtkArrayDispatch::Dispatch2SameValueType;
      AccumulateMaximum worker;
      if (!Dispatcher::Execute(inArray, outArray, worker))
      { // Fallback to slow path:
        worker(inArray, outArray);
      }
      // Alert change in data.
      outArray->DataChanged();
    }
  }
}

//-----------------------------------------------------------------------------
void svtkTemporalStatistics::PostExecute(svtkDataObject* input, svtkDataObject* output)
{
  if (input->IsA("svtkDataSet"))
  {
    this->PostExecute(svtkDataSet::SafeDownCast(input), svtkDataSet::SafeDownCast(output));
    return;
  }

  if (input->IsA("svtkGraph"))
  {
    this->PostExecute(svtkGraph::SafeDownCast(input), svtkGraph::SafeDownCast(output));
    return;
  }

  if (input->IsA("svtkCompositeDataSet"))
  {
    this->PostExecute(
      svtkCompositeDataSet::SafeDownCast(input), svtkCompositeDataSet::SafeDownCast(output));
  }
}

//-----------------------------------------------------------------------------
void svtkTemporalStatistics::PostExecute(svtkDataSet* input, svtkDataSet* output)
{
  this->FinishArrays(input->GetFieldData(), output->GetFieldData());
  this->FinishArrays(input->GetPointData(), output->GetPointData());
  this->FinishArrays(input->GetCellData(), output->GetCellData());
}

//-----------------------------------------------------------------------------
void svtkTemporalStatistics::PostExecute(svtkGraph* input, svtkGraph* output)
{
  this->FinishArrays(input->GetFieldData(), output->GetFieldData());
  this->FinishArrays(input->GetVertexData(), output->GetVertexData());
  this->FinishArrays(input->GetEdgeData(), output->GetEdgeData());
}

//-----------------------------------------------------------------------------
void svtkTemporalStatistics::PostExecute(svtkCompositeDataSet* input, svtkCompositeDataSet* output)
{
  svtkSmartPointer<svtkCompositeDataIterator> inputItr;
  inputItr.TakeReference(input->NewIterator());

  for (inputItr->InitTraversal(); !inputItr->IsDoneWithTraversal(); inputItr->GoToNextItem())
  {
    svtkDataObject* inputObj = inputItr->GetCurrentDataObject();
    svtkDataObject* outputObj = output->GetDataSet(inputItr);

    this->PostExecute(inputObj, outputObj);
  }
}

//-----------------------------------------------------------------------------
void svtkTemporalStatistics::FinishArrays(svtkFieldData* inFd, svtkFieldData* outFd)
{
  using Dispatcher = svtkArrayDispatch::Dispatch;

  int numArrays = inFd->GetNumberOfArrays();
  for (int i = 0; i < numArrays; i++)
  {
    svtkDataArray* inArray = inFd->GetArray(i);
    svtkDataArray* outArray;
    if (!inArray)
      continue;

    outArray = this->GetArray(outFd, inArray, AVERAGE_SUFFIX);
    if (outArray)
    {
      FinishAverage worker;
      if (!Dispatcher::Execute(outArray, worker, this->CurrentTimeIndex))
      { // fall-back to slow path
        worker(outArray, this->CurrentTimeIndex);
      }
    }
    svtkDataArray* avgArray = outArray;

    // No post processing on minimum.
    // No post processing on maximum.

    outArray = this->GetArray(outFd, inArray, STANDARD_DEVIATION_SUFFIX);
    if (outArray)
    {
      if (!avgArray)
      {
        svtkWarningMacro(<< "Average not computed for " << inArray->GetName()
                        << ", standard deviation skipped.");
        outFd->RemoveArray(outArray->GetName());
      }
      else
      {
        FinishStdDev worker;
        if (!Dispatcher::Execute(outArray, worker, this->CurrentTimeIndex))
        { // fall-back to slow path
          worker(outArray, this->CurrentTimeIndex);
        }
        if (!this->ComputeAverage)
        {
          outFd->RemoveArray(avgArray->GetName());
        }
      }
    }
  }
}

//-----------------------------------------------------------------------------
svtkDataArray* svtkTemporalStatistics::GetArray(
  svtkFieldData* fieldData, svtkDataArray* inArray, const char* nameSuffix)
{
  svtkStdString outArrayName = svtkTemporalStatisticsMangleName(inArray->GetName(), nameSuffix);
  svtkDataArray* outArray = fieldData->GetArray(outArrayName.c_str());
  if (!outArray)
    return nullptr;

  if ((inArray->GetNumberOfComponents() != outArray->GetNumberOfComponents()) ||
    (inArray->GetNumberOfTuples() != outArray->GetNumberOfTuples()))
  {
    if (!this->GeneratedChangingTopologyWarning)
    {
      std::string fieldType = svtkCellData::SafeDownCast(fieldData) == nullptr ? "points" : "cells";
      svtkWarningMacro("The number of " << fieldType << " has changed between time "
                                       << "steps. No arrays of this type will be output since this "
                                       << "filter can not handle grids that change over time.");
      this->GeneratedChangingTopologyWarning = true;
    }
    fieldData->RemoveArray(outArray->GetName());
    return nullptr;
  }

  return outArray;
}
