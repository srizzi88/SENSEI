/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkAppendDataSets.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkAppendDataSets.h"

#include "svtkAppendFilter.h"
#include "svtkAppendPolyData.h"
#include "svtkBoundingBox.h"
#include "svtkCell.h"
#include "svtkCellData.h"
#include "svtkCleanPolyData.h"
#include "svtkDataObjectTypes.h"
#include "svtkDataSetCollection.h"
#include "svtkExecutive.h"
#include "svtkIncrementalOctreePointLocator.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkNew.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkPolyData.h"
#include "svtkSmartPointer.h"
#include "svtkStreamingDemandDrivenPipeline.h"
#include "svtkType.h"
#include "svtkUnstructuredGrid.h"

svtkStandardNewMacro(svtkAppendDataSets);

//----------------------------------------------------------------------------
svtkAppendDataSets::svtkAppendDataSets()
  : MergePoints(false)
  , Tolerance(0.0)
  , ToleranceIsAbsolute(true)
  , OutputDataSetType(SVTK_UNSTRUCTURED_GRID)
  , OutputPointsPrecision(DEFAULT_PRECISION)
{
}

//----------------------------------------------------------------------------
svtkAppendDataSets::~svtkAppendDataSets() {}

//----------------------------------------------------------------------------
svtkTypeBool svtkAppendDataSets::ProcessRequest(
  svtkInformation* request, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // Handle update requests
  if (request->Has(svtkStreamingDemandDrivenPipeline::REQUEST_UPDATE_EXTENT()))
  {
    return this->RequestUpdateExtent(request, inputVector, outputVector);
  }

  return this->Superclass::ProcessRequest(request, inputVector, outputVector);
}

//----------------------------------------------------------------------------
int svtkAppendDataSets::RequestDataObject(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  if (!inInfo)
  {
    return 0;
  }

  if (this->OutputDataSetType != SVTK_POLY_DATA && this->OutputDataSetType != SVTK_UNSTRUCTURED_GRID)
  {
    svtkErrorMacro(
      "Output type '" << svtkDataObjectTypes::GetClassNameFromTypeId(this->OutputDataSetType)
                      << "' is not supported.");
    return 0;
  }

  svtkDataObject* input = inInfo->Get(svtkDataObject::DATA_OBJECT());
  if (input)
  {
    svtkInformation* info = outputVector->GetInformationObject(0);
    svtkDataObject* output = info->Get(svtkDataObject::DATA_OBJECT());

    if (!output ||
      (svtkDataObjectTypes::GetTypeIdFromClassName(output->GetClassName()) !=
        this->OutputDataSetType))
    {
      svtkSmartPointer<svtkDataObject> newOutput;
      newOutput.TakeReference(svtkDataObjectTypes::NewDataObject(this->OutputDataSetType));
      info->Set(svtkDataObject::DATA_OBJECT(), newOutput);
      this->GetOutputPortInformation(0)->Set(
        svtkDataObject::DATA_EXTENT_TYPE(), newOutput->GetExtentType());
    }
    return 1;
  }

  return 0;
}

//----------------------------------------------------------------------------
// Append data sets into single unstructured grid
int svtkAppendDataSets::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the output info object
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the output
  svtkPointSet* output = svtkPointSet::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));
  svtkUnstructuredGrid* outputUG =
    svtkUnstructuredGrid::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));
  svtkPolyData* outputPD = svtkPolyData::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  svtkDebugMacro("Appending data together");

  if (outputUG)
  {
    svtkNew<svtkAppendFilter> appender;
    appender->SetOutputPointsPrecision(this->GetOutputPointsPrecision());
    appender->SetMergePoints(this->GetMergePoints());
    appender->SetToleranceIsAbsolute(this->GetToleranceIsAbsolute());
    appender->SetTolerance(this->GetTolerance());

    for (int cc = 0; cc < inputVector[0]->GetNumberOfInformationObjects(); cc++)
    {
      auto input = svtkDataSet::GetData(inputVector[0], cc);
      appender->AddInputData(input);
    }
    if (appender->GetNumberOfInputConnections(0) > 0)
    {
      appender->Update();
      outputUG->ShallowCopy(appender->GetOutput());
    }
  }
  else if (outputPD)
  {
    svtkNew<svtkAppendPolyData> appender;
    appender->SetOutputPointsPrecision(this->GetOutputPointsPrecision());
    for (int cc = 0; cc < inputVector[0]->GetNumberOfInformationObjects(); cc++)
    {
      auto input = svtkPolyData::GetData(inputVector[0], cc);
      if (input)
      {
        appender->AddInputData(input);
      }
    }
    if (this->MergePoints)
    {
      if (appender->GetNumberOfInputConnections(0) > 0)
      {
        svtkNew<svtkCleanPolyData> cleaner;
        cleaner->SetInputConnection(appender->GetOutputPort());
        cleaner->PointMergingOn();
        cleaner->ConvertLinesToPointsOff();
        cleaner->ConvertPolysToLinesOff();
        cleaner->ConvertStripsToPolysOff();
        if (this->GetToleranceIsAbsolute())
        {
          cleaner->SetAbsoluteTolerance(this->GetTolerance());
          cleaner->ToleranceIsAbsoluteOn();
        }
        else
        {
          cleaner->SetTolerance(this->GetTolerance());
          cleaner->ToleranceIsAbsoluteOff();
        }
        cleaner->Update();
        output->ShallowCopy(cleaner->GetOutput());
      }
    }
    else
    {
      if (appender->GetNumberOfInputConnections(0) > 0)
      {
        appender->Update();
        output->ShallowCopy(appender->GetOutput());
      }
    }
  }
  else
  {
    svtkErrorMacro("Unsupported output type.");
    return 0;
  }

  return 1;
}

//----------------------------------------------------------------------------
int svtkAppendDataSets::RequestUpdateExtent(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* svtkNotUsed(outputVector))
{
  int numInputConnections = this->GetNumberOfInputConnections(0);

  // Let downstream request a subset of connection 0, for connections >= 1
  // send their WHOLE_EXTENT as UPDATE_EXTENT.
  for (int idx = 1; idx < numInputConnections; ++idx)
  {
    svtkInformation* inputInfo = inputVector[0]->GetInformationObject(idx);
    if (inputInfo->Has(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()))
    {
      int ext[6];
      inputInfo->Get(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), ext);
      inputInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), ext, 6);
    }
  }

  return 1;
}

//----------------------------------------------------------------------------
int svtkAppendDataSets::FillInputPortInformation(int, svtkInformation* info)
{
  info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkDataSet");
  info->Set(svtkAlgorithm::INPUT_IS_REPEATABLE(), 1);
  return 1;
}

//----------------------------------------------------------------------------
void svtkAppendDataSets::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "MergePoints:" << (this->MergePoints ? "On" : "Off") << "\n";
  os << indent << "Tolerance: " << this->Tolerance << "\n";
  os << indent
     << "OutputDataSetType: " << svtkDataObjectTypes::GetClassNameFromTypeId(this->OutputDataSetType)
     << "\n";
  os << indent << "OutputPointsPrecision: " << this->OutputPointsPrecision << "\n";
}
