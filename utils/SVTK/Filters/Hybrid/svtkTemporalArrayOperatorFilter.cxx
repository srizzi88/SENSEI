/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkTemporalArrayOperatorFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

    This software is distributed WITHOUT ANY WARRANTY; without even
    the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
    PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkTemporalArrayOperatorFilter.h"

#include "svtkArrayDispatch.h"
#include "svtkCellData.h"
#include "svtkDataArray.h"
#include "svtkDataArrayRange.h"
#include "svtkDataObjectTreeIterator.h"
#include "svtkDataSet.h"
#include "svtkGraph.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkMultiBlockDataSet.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkStreamingDemandDrivenPipeline.h"
#include "svtkTable.h"

#include <algorithm>
#include <functional>

svtkStandardNewMacro(svtkTemporalArrayOperatorFilter);

//------------------------------------------------------------------------------
svtkTemporalArrayOperatorFilter::svtkTemporalArrayOperatorFilter()
{
  this->Operator = OperatorType::ADD;
  this->NumberTimeSteps = 0;
  this->FirstTimeStepIndex = 0;
  this->SecondTimeStepIndex = 0;
  this->OutputArrayNameSuffix = nullptr;

  // Set the default input data array that the algorithm will process (point scalars)
  this->SetInputArrayToProcess(
    0, 0, 0, svtkDataObject::FIELD_ASSOCIATION_POINTS, svtkDataSetAttributes::SCALARS);

  this->SetNumberOfOutputPorts(1);
}

//------------------------------------------------------------------------------
svtkTemporalArrayOperatorFilter::~svtkTemporalArrayOperatorFilter()
{
  this->SetOutputArrayNameSuffix(nullptr);
}

//------------------------------------------------------------------------------
void svtkTemporalArrayOperatorFilter::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Operator: " << this->Operator << endl;
  os << indent << "First time step: " << this->FirstTimeStepIndex << endl;
  os << indent << "Second time step: " << this->SecondTimeStepIndex << endl;
  os << indent << "Output array name suffix: "
     << (this->OutputArrayNameSuffix ? this->OutputArrayNameSuffix : "") << endl;
  os << indent << "Field association: "
     << svtkDataObject::GetAssociationTypeAsString(this->GetInputArrayAssociation()) << endl;
}

//------------------------------------------------------------------------------
int svtkTemporalArrayOperatorFilter::FillInputPortInformation(
  int svtkNotUsed(port), svtkInformation* info)
{
  info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkDataObject");
  return 1;
}

//------------------------------------------------------------------------------
int svtkTemporalArrayOperatorFilter::FillOutputPortInformation(
  int svtkNotUsed(port), svtkInformation* info)
{
  info->Set(svtkDataObject::DATA_TYPE_NAME(), "svtkDataObject");
  return 1;
}

//------------------------------------------------------------------------------
int svtkTemporalArrayOperatorFilter::RequestDataObject(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputInfoVector, svtkInformationVector* outputInfoVector)
{
  svtkDataObject* inputObj = svtkDataObject::GetData(inputInfoVector[0]);
  if (inputObj != nullptr)
  {
    svtkDataObject* outputObj = svtkDataObject::GetData(outputInfoVector);
    if (!outputObj || !outputObj->IsA(inputObj->GetClassName()))
    {
      svtkDataObject* newOutputObj = inputObj->NewInstance();
      svtkInformation* outputInfo = outputInfoVector->GetInformationObject(0);
      outputInfo->Set(svtkDataObject::DATA_OBJECT(), newOutputObj);
      newOutputObj->Delete();
    }
    return 1;
  }
  return 0;
}

//------------------------------------------------------------------------------
int svtkTemporalArrayOperatorFilter::RequestInformation(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputInfoVector, svtkInformationVector* svtkNotUsed(outputInfoVector))
{
  // Get input and output information objects
  svtkInformation* inputInfo = inputInfoVector[0]->GetInformationObject(0);

  // Check for presence more than one time step
  if (inputInfo->Has(svtkStreamingDemandDrivenPipeline::TIME_STEPS()))
  {
    // Find time on input
    this->NumberTimeSteps = inputInfo->Length(svtkStreamingDemandDrivenPipeline::TIME_STEPS());
    if (this->NumberTimeSteps < 2)
    {
      svtkErrorMacro(<< "Not enough numbers of time steps: " << this->NumberTimeSteps);
      return 0;
    }
  }
  else
  {
    svtkErrorMacro(<< "No time steps in input data.");
    return 0;
  }

  return 1;
}

//------------------------------------------------------------------------------
int svtkTemporalArrayOperatorFilter::RequestUpdateExtent(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputInfoVector, svtkInformationVector* outputInfoVector)
{
  if (this->FirstTimeStepIndex < 0 || this->SecondTimeStepIndex < 0 ||
    this->FirstTimeStepIndex >= this->NumberTimeSteps ||
    this->SecondTimeStepIndex >= this->NumberTimeSteps)
  {
    svtkErrorMacro(<< "Specified timesteps (" << this->FirstTimeStepIndex << " and "
                  << this->SecondTimeStepIndex
                  << "are outside the range of"
                     " available time steps ("
                  << this->NumberTimeSteps << ")");
    return 0;
  }

  if (this->FirstTimeStepIndex == this->SecondTimeStepIndex)
  {
    svtkWarningMacro(<< "First and second time steps are the same.");
  }

  svtkInformation* outputInfo = outputInfoVector->GetInformationObject(0);
  // Find the required input time steps and request them
  if (outputInfo->Has(svtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP()))
  {
    svtkInformation* inputInfo = inputInfoVector[0]->GetInformationObject(0);
    // Get the available input times
    double* inputTime = inputInfo->Get(svtkStreamingDemandDrivenPipeline::TIME_STEPS());
    if (inputTime)
    {
      // Request the two time steps upstream
      double inputUpdateTimes[2] = { inputTime[this->FirstTimeStepIndex],
        inputTime[this->SecondTimeStepIndex] };

      inputInfo->Set(svtkMultiTimeStepAlgorithm::UPDATE_TIME_STEPS(), inputUpdateTimes, 2);
    }
  }
  return 1;
}

//------------------------------------------------------------------------------
int svtkTemporalArrayOperatorFilter::RequestData(svtkInformation* svtkNotUsed(theRequest),
  svtkInformationVector** inputInfoVector, svtkInformationVector* outputInfoVector)
{
  svtkMultiBlockDataSet* inputMultiBlock = svtkMultiBlockDataSet::GetData(inputInfoVector[0]);

  int numberTimeSteps = inputMultiBlock->GetNumberOfBlocks();
  if (numberTimeSteps != 2)
  {
    svtkErrorMacro(<< "The number of time blocks is incorrect.");
    return 0;
  }

  svtkDataObject* data0 = inputMultiBlock->GetBlock(0);
  svtkDataObject* data1 = inputMultiBlock->GetBlock(1);
  if (!data0 || !data1)
  {
    svtkErrorMacro(<< "Unable to retrieve data objects.");
    return 0;
  }

  svtkSmartPointer<svtkDataObject> newOutData;
  newOutData.TakeReference(this->Process(data0, data1));

  svtkInformation* outInfo = outputInfoVector->GetInformationObject(0);
  svtkDataObject* outData = svtkDataObject::GetData(outInfo);
  outData->ShallowCopy(newOutData);

  return newOutData != nullptr ? 1 : 0;
}

//------------------------------------------------------------------------------
int svtkTemporalArrayOperatorFilter::GetInputArrayAssociation()
{
  svtkInformation* inputArrayInfo =
    this->GetInformation()->Get(INPUT_ARRAYS_TO_PROCESS())->GetInformationObject(0);
  return inputArrayInfo->Get(svtkDataObject::FIELD_ASSOCIATION());
}

//------------------------------------------------------------------------------
svtkDataObject* svtkTemporalArrayOperatorFilter::Process(
  svtkDataObject* inputData0, svtkDataObject* inputData1)
{
  if (inputData0->IsA("svtkCompositeDataSet"))
  {
    // We suppose input data are same type and have same structure (they should!)
    svtkCompositeDataSet* compositeDataSet0 = svtkCompositeDataSet::SafeDownCast(inputData0);
    svtkCompositeDataSet* compositeDataSet1 = svtkCompositeDataSet::SafeDownCast(inputData1);

    svtkCompositeDataSet* outputCompositeDataSet = compositeDataSet0->NewInstance();
    outputCompositeDataSet->ShallowCopy(inputData0);

    svtkSmartPointer<svtkCompositeDataIterator> iter;
    iter.TakeReference(compositeDataSet0->NewIterator());
    for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
    {
      svtkDataObject* dataObj0 = iter->GetCurrentDataObject();
      svtkDataObject* dataObj1 = compositeDataSet1->GetDataSet(iter);
      if (!dataObj0 || !dataObj1)
      {
        svtkWarningMacro("The composite datasets have different structure.");
        continue;
      }

      svtkSmartPointer<svtkDataObject> resultDataObj;
      resultDataObj.TakeReference(this->ProcessDataObject(dataObj0, dataObj1));
      if (!resultDataObj)
      {
        return nullptr;
      }
      outputCompositeDataSet->SetDataSet(iter, resultDataObj);
    }
    return outputCompositeDataSet;
  }

  return this->ProcessDataObject(inputData0, inputData1);
}

//------------------------------------------------------------------------------
svtkDataObject* svtkTemporalArrayOperatorFilter::ProcessDataObject(
  svtkDataObject* inputData0, svtkDataObject* inputData1)
{
  svtkDataArray* inputArray0 = this->GetInputArrayToProcess(0, inputData0);
  svtkDataArray* inputArray1 = this->GetInputArrayToProcess(0, inputData1);
  if (!inputArray0 || !inputArray1)
  {
    svtkErrorMacro(<< "Unable to retrieve data arrays to process.");
    return nullptr;
  }

  if (inputArray0->GetDataType() != inputArray1->GetDataType())
  {
    svtkErrorMacro(<< "Array type in each time step are different.");
    return nullptr;
  }

  if (strcmp(inputArray0->GetName(), inputArray1->GetName()))
  {
    svtkErrorMacro(<< "Array name in each time step are different.");
    return nullptr;
  }

  if (inputArray0->GetNumberOfComponents() != inputArray1->GetNumberOfComponents())
  {
    svtkErrorMacro(<< "The number of components of the array in each time "
                     "step are different.");
    return nullptr;
  }

  if (inputArray0->GetNumberOfTuples() != inputArray1->GetNumberOfTuples())
  {
    svtkErrorMacro(<< "The number of tuples of the array in each time step"
                     "are different.");
    return nullptr;
  }

  // Copy input structure into output
  svtkDataObject* outputDataObject = inputData0->NewInstance();
  outputDataObject->ShallowCopy(inputData1);

  svtkDataSet* outputDataSet = svtkDataSet::SafeDownCast(outputDataObject);
  svtkGraph* outputGraph = svtkGraph::SafeDownCast(outputDataObject);
  svtkTable* outputTable = svtkTable::SafeDownCast(outputDataObject);

  svtkSmartPointer<svtkDataArray> outputArray;
  outputArray.TakeReference(this->ProcessDataArray(inputArray0, inputArray1));

  switch (this->GetInputArrayAssociation())
  {
    case svtkDataObject::FIELD_ASSOCIATION_CELLS:
      if (!outputDataSet)
      {
        svtkErrorMacro(<< "Bad input association for input data object.");
        return nullptr;
      }
      outputDataSet->GetCellData()->AddArray(outputArray);
      break;
    case svtkDataObject::FIELD_ASSOCIATION_NONE:
      outputDataObject->GetFieldData()->AddArray(outputArray);
      break;
    case svtkDataObject::FIELD_ASSOCIATION_VERTICES:
      if (!outputGraph)
      {
        svtkErrorMacro(<< "Bad input association for input data object.");
        return nullptr;
      }
      outputGraph->GetVertexData()->AddArray(outputArray);
      break;
    case svtkDataObject::FIELD_ASSOCIATION_EDGES:
      if (!outputGraph)
      {
        svtkErrorMacro(<< "Bad input association for input data object.");
        return nullptr;
      }
      outputGraph->GetEdgeData()->AddArray(outputArray);
      break;
    case svtkDataObject::FIELD_ASSOCIATION_ROWS:
      if (!outputTable)
      {
        svtkErrorMacro(<< "Bad input association for input data object.");
        return nullptr;
      }
      outputTable->GetRowData()->AddArray(outputArray);
      break;
    case svtkDataObject::FIELD_ASSOCIATION_POINTS:
    default:
      if (!outputDataSet)
      {
        svtkErrorMacro(<< "Bad input association for input data object.");
        return nullptr;
      }
      outputDataSet->GetPointData()->AddArray(outputArray);
      break;
  }

  return outputDataObject;
}

//------------------------------------------------------------------------------
struct TemporalDataOperatorWorker
{
  TemporalDataOperatorWorker(int op)
    : Operator(op)
  {
  }

  template <typename Array1T, typename Array2T, typename Array3T>
  void operator()(Array1T* src1, Array2T* src2, Array3T* dst)
  {
    using T = svtk::GetAPIType<Array3T>;

    SVTK_ASSUME(src1->GetNumberOfComponents() == dst->GetNumberOfComponents());
    SVTK_ASSUME(src2->GetNumberOfComponents() == dst->GetNumberOfComponents());

    const auto srcRange1 = svtk::DataArrayValueRange(src1);
    const auto srcRange2 = svtk::DataArrayValueRange(src2);
    auto dstRange = svtk::DataArrayValueRange(dst);

    switch (this->Operator)
    {
      case svtkTemporalArrayOperatorFilter::ADD:
        std::transform(srcRange1.cbegin(), srcRange1.cend(), srcRange2.cbegin(), dstRange.begin(),
          std::plus<T>{});
        break;
      case svtkTemporalArrayOperatorFilter::SUB:
        std::transform(srcRange1.cbegin(), srcRange1.cend(), srcRange2.cbegin(), dstRange.begin(),
          std::minus<T>{});
        break;
      case svtkTemporalArrayOperatorFilter::MUL:
        std::transform(srcRange1.cbegin(), srcRange1.cend(), srcRange2.cbegin(), dstRange.begin(),
          std::multiplies<T>{});
        break;
      case svtkTemporalArrayOperatorFilter::DIV:
        std::transform(srcRange1.cbegin(), srcRange1.cend(), srcRange2.cbegin(), dstRange.begin(),
          std::divides<T>{});
        break;
      default:
        std::copy(srcRange1.cbegin(), srcRange1.cend(), dstRange.begin());
        break;
    }
  }

  int Operator;
};

//------------------------------------------------------------------------------
svtkDataArray* svtkTemporalArrayOperatorFilter::ProcessDataArray(
  svtkDataArray* inputArray0, svtkDataArray* inputArray1)
{
  svtkAbstractArray* outputArray = svtkAbstractArray::CreateArray(inputArray0->GetDataType());
  svtkDataArray* outputDataArray = svtkDataArray::SafeDownCast(outputArray);

  outputDataArray->SetNumberOfComponents(inputArray0->GetNumberOfComponents());
  outputDataArray->SetNumberOfTuples(inputArray0->GetNumberOfTuples());
  outputDataArray->CopyComponentNames(inputArray0);

  std::string s = inputArray0->GetName() ? inputArray0->GetName() : "input_array";
  if (this->OutputArrayNameSuffix && strlen(this->OutputArrayNameSuffix) != 0)
  {
    s += this->OutputArrayNameSuffix;
  }
  else
  {
    switch (this->Operator)
    {
      case SUB:
        s += "_sub";
        break;
      case MUL:
        s += "_mul";
        break;
      case DIV:
        s += "_div";
        break;
      case ADD:
      default:
        s += "_add";
        break;
    }
  }
  outputDataArray->SetName(s.c_str());

  // Let's perform the operation on the array
  TemporalDataOperatorWorker worker(this->Operator);

  if (!svtkArrayDispatch::Dispatch3SameValueType::Execute(
        inputArray0, inputArray1, outputDataArray, worker))
  {
    worker(inputArray0, inputArray1, outputDataArray); // svtkDataArray fallback
  }

  return outputDataArray;
}
