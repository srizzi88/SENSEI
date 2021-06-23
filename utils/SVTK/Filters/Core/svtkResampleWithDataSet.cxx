/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkResampleWithDataSet.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkResampleWithDataSet.h"

#include "svtkCellData.h"
#include "svtkCharArray.h"
#include "svtkCompositeDataProbeFilter.h"
#include "svtkCompositeDataSet.h"
#include "svtkCompositeDataSetRange.h"
#include "svtkDataObject.h"
#include "svtkDataSet.h"
#include "svtkIdList.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkNew.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkSMPThreadLocalObject.h"
#include "svtkSMPTools.h"
#include "svtkSmartPointer.h"
#include "svtkStreamingDemandDrivenPipeline.h"
#include "svtkUnsignedCharArray.h"

svtkObjectFactoryNewMacro(svtkResampleWithDataSet);

//-----------------------------------------------------------------------------
svtkResampleWithDataSet::svtkResampleWithDataSet()
  : MarkBlankPointsAndCells(true)
{
  this->SetNumberOfInputPorts(2);
  this->SetNumberOfOutputPorts(1);
}

//-----------------------------------------------------------------------------
svtkResampleWithDataSet::~svtkResampleWithDataSet() = default;

//-----------------------------------------------------------------------------
void svtkResampleWithDataSet::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  this->Prober->PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void svtkResampleWithDataSet::SetSourceConnection(svtkAlgorithmOutput* algOutput)
{
  this->SetInputConnection(1, algOutput);
}

//----------------------------------------------------------------------------
void svtkResampleWithDataSet::SetSourceData(svtkDataObject* input)
{
  this->SetInputData(1, input);
}

//----------------------------------------------------------------------------
void svtkResampleWithDataSet::SetCategoricalData(bool arg)
{
  this->Prober->SetCategoricalData(arg);
}

bool svtkResampleWithDataSet::GetCategoricalData()
{
  // work around for Visual Studio warning C4800:
  // 'int' : forcing value to bool 'true' or 'false' (performance warning)
  return this->Prober->GetCategoricalData() ? true : false;
}

void svtkResampleWithDataSet::SetPassCellArrays(bool arg)
{
  this->Prober->SetPassCellArrays(arg);
}

bool svtkResampleWithDataSet::GetPassCellArrays()
{
  // work around for Visual Studio warning C4800:
  // 'int' : forcing value to bool 'true' or 'false' (performance warning)
  return this->Prober->GetPassCellArrays() ? true : false;
}

void svtkResampleWithDataSet::SetPassPointArrays(bool arg)
{
  this->Prober->SetPassPointArrays(arg);
}

bool svtkResampleWithDataSet::GetPassPointArrays()
{
  return this->Prober->GetPassPointArrays() ? true : false;
}

void svtkResampleWithDataSet::SetPassFieldArrays(bool arg)
{
  this->Prober->SetPassFieldArrays(arg);
}

bool svtkResampleWithDataSet::GetPassFieldArrays()
{
  return this->Prober->GetPassFieldArrays() ? true : false;
}

void svtkResampleWithDataSet::SetCellLocatorPrototype(svtkAbstractCellLocator* locator)
{
  this->Prober->SetCellLocatorPrototype(locator);
}

svtkAbstractCellLocator* svtkResampleWithDataSet::GetCellLocatorPrototype() const
{
  return this->Prober->GetCellLocatorPrototype();
}

//----------------------------------------------------------------------------
void svtkResampleWithDataSet::SetTolerance(double arg)
{
  this->Prober->SetTolerance(arg);
}

double svtkResampleWithDataSet::GetTolerance()
{
  return this->Prober->GetTolerance();
}

void svtkResampleWithDataSet::SetComputeTolerance(bool arg)
{
  this->Prober->SetComputeTolerance(arg);
}

bool svtkResampleWithDataSet::GetComputeTolerance()
{
  return this->Prober->GetComputeTolerance();
}

//----------------------------------------------------------------------------
svtkMTimeType svtkResampleWithDataSet::GetMTime()
{
  return std::max(this->Superclass::GetMTime(), this->Prober->GetMTime());
}

//----------------------------------------------------------------------------
int svtkResampleWithDataSet::RequestInformation(
  svtkInformation*, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* sourceInfo = inputVector[1]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  outInfo->CopyEntry(sourceInfo, svtkStreamingDemandDrivenPipeline::TIME_STEPS());
  outInfo->CopyEntry(sourceInfo, svtkStreamingDemandDrivenPipeline::TIME_RANGE());

  return 1;
}

//-----------------------------------------------------------------------------
int svtkResampleWithDataSet::RequestUpdateExtent(
  svtkInformation*, svtkInformationVector** inputVector, svtkInformationVector*)
{
  svtkInformation* sourceInfo = inputVector[1]->GetInformationObject(0);

  sourceInfo->Remove(svtkStreamingDemandDrivenPipeline::UPDATE_EXTENT());
  if (sourceInfo->Has(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()))
  {
    sourceInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(),
      sourceInfo->Get(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()), 6);
  }

  sourceInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(), 0);
  sourceInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(), 1);
  sourceInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(), 0);

  return 1;
}

//----------------------------------------------------------------------------
int svtkResampleWithDataSet::FillInputPortInformation(int svtkNotUsed(port), svtkInformation* info)
{
  info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkDataSet");
  info->Append(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkCompositeDataSet");
  return 1;
}

//----------------------------------------------------------------------------
int svtkResampleWithDataSet::FillOutputPortInformation(int svtkNotUsed(port), svtkInformation* info)
{
  info->Set(svtkDataObject::DATA_TYPE_NAME(), "svtkDataObject");
  return 1;
}

//-----------------------------------------------------------------------------
const char* svtkResampleWithDataSet::GetMaskArrayName() const
{
  return this->Prober->GetValidPointMaskArrayName();
}

//-----------------------------------------------------------------------------
namespace
{

class MarkHiddenPoints
{
public:
  MarkHiddenPoints(char* maskArray, svtkUnsignedCharArray* pointGhostArray)
    : MaskArray(maskArray)
    , PointGhostArray(pointGhostArray)
  {
  }

  void operator()(svtkIdType begin, svtkIdType end)
  {
    for (svtkIdType i = begin; i < end; ++i)
    {
      if (!this->MaskArray[i])
      {
        this->PointGhostArray->SetValue(
          i, this->PointGhostArray->GetValue(i) | svtkDataSetAttributes::HIDDENPOINT);
      }
    }
  }

private:
  char* MaskArray;
  svtkUnsignedCharArray* PointGhostArray;
};

class MarkHiddenCells
{
public:
  MarkHiddenCells(svtkDataSet* data, char* maskArray, svtkUnsignedCharArray* cellGhostArray)
    : Data(data)
    , MaskArray(maskArray)
    , CellGhostArray(cellGhostArray)
  {
  }

  void operator()(svtkIdType begin, svtkIdType end)
  {
    svtkIdList* cellPoints = this->PointIds.Local();

    for (svtkIdType i = begin; i < end; ++i)
    {
      this->Data->GetCellPoints(i, cellPoints);
      svtkIdType npts = cellPoints->GetNumberOfIds();
      for (svtkIdType j = 0; j < npts; ++j)
      {
        svtkIdType ptid = cellPoints->GetId(j);
        if (!this->MaskArray[ptid])
        {
          this->CellGhostArray->SetValue(
            i, this->CellGhostArray->GetValue(i) | svtkDataSetAttributes::HIDDENPOINT);
          break;
        }
      }
    }
  }

private:
  svtkDataSet* Data;
  char* MaskArray;
  svtkUnsignedCharArray* CellGhostArray;

  svtkSMPThreadLocalObject<svtkIdList> PointIds;
};

} // anonymous namespace

void svtkResampleWithDataSet::SetBlankPointsAndCells(svtkDataSet* dataset)
{
  if (dataset->GetNumberOfPoints() <= 0)
  {
    return;
  }

  svtkPointData* pd = dataset->GetPointData();
  svtkCharArray* maskArray = svtkArrayDownCast<svtkCharArray>(pd->GetArray(this->GetMaskArrayName()));
  char* mask = maskArray->GetPointer(0);

  dataset->AllocatePointGhostArray();
  svtkUnsignedCharArray* pointGhostArray = dataset->GetPointGhostArray();

  svtkIdType numPoints = dataset->GetNumberOfPoints();
  MarkHiddenPoints pointWorklet(mask, pointGhostArray);
  svtkSMPTools::For(0, numPoints, pointWorklet);

  dataset->AllocateCellGhostArray();
  svtkUnsignedCharArray* cellGhostArray = dataset->GetCellGhostArray();

  svtkIdType numCells = dataset->GetNumberOfCells();
  // GetCellPoints needs to be called once from a single thread for safe
  // multi-threaded calls
  svtkNew<svtkIdList> cpts;
  dataset->GetCellPoints(0, cpts);

  MarkHiddenCells cellWorklet(dataset, mask, cellGhostArray);
  svtkSMPTools::For(0, numCells, cellWorklet);
}

//-----------------------------------------------------------------------------
int svtkResampleWithDataSet::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* sourceInfo = inputVector[1]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  svtkDataObject* source = sourceInfo->Get(svtkDataObject::DATA_OBJECT());

  svtkDataObject* inDataObject = inInfo->Get(svtkDataObject::DATA_OBJECT());
  svtkDataObject* outDataObject = outInfo->Get(svtkDataObject::DATA_OBJECT());
  if (inDataObject->IsA("svtkDataSet"))
  {
    svtkDataSet* input = svtkDataSet::SafeDownCast(inDataObject);
    svtkDataSet* output = svtkDataSet::SafeDownCast(outDataObject);

    this->Prober->SetInputData(input);
    this->Prober->SetSourceData(source);
    this->Prober->Update();
    output->ShallowCopy(this->Prober->GetOutput());
    if (this->MarkBlankPointsAndCells)
    {
      this->SetBlankPointsAndCells(output);
    }
  }
  else if (inDataObject->IsA("svtkCompositeDataSet"))
  {
    svtkCompositeDataSet* input = svtkCompositeDataSet::SafeDownCast(inDataObject);
    svtkCompositeDataSet* output = svtkCompositeDataSet::SafeDownCast(outDataObject);
    output->CopyStructure(input);

    this->Prober->SetSourceData(source);

    using Opts = svtk::CompositeDataSetOptions;
    for (auto node : svtk::Range(input, Opts::SkipEmptyNodes))
    {
      svtkDataSet* ds = static_cast<svtkDataSet*>(node.GetDataObject());
      if (ds)
      {
        this->Prober->SetInputData(ds);
        this->Prober->Update();
        svtkDataSet* result = this->Prober->GetOutput();

        svtkDataSet* block = result->NewInstance();
        block->ShallowCopy(result);
        if (this->MarkBlankPointsAndCells)
        {
          this->SetBlankPointsAndCells(block);
        }
        node.SetDataObject(output, block);
        block->Delete();
      }
    }
  }

  return 1;
}
