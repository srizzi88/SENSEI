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
#include "svtkmProbe.h"

#include "svtkCellData.h"
#include "svtkDataSet.h"
#include "svtkExecutive.h"
#include "svtkImageData.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkStreamingDemandDrivenPipeline.h"

#include "svtkmlib/ArrayConverters.h"
#include "svtkmlib/DataSetConverters.h"

#include "svtkmFilterPolicy.h"

#include "svtkm/filter/Probe.h"

svtkStandardNewMacro(svtkmProbe);

//------------------------------------------------------------------------------
svtkmProbe::svtkmProbe()
{
  this->SetNumberOfInputPorts(2);
  this->PassCellArrays = false;
  this->PassPointArrays = false;
  this->PassFieldArrays = true;
  this->ValidPointMaskArrayName = "svtkValidPointMask";
  this->ValidCellMaskArrayName = "svtkValidCellMask";
}

//------------------------------------------------------------------------------
void svtkmProbe::SetSourceData(svtkDataObject* input)
{
  this->SetInputData(1, input);
}

//------------------------------------------------------------------------------
svtkDataObject* svtkmProbe::GetSource()
{
  if (this->GetNumberOfInputConnections(1) < 1)
  {
    return nullptr;
  }
  return this->GetExecutive()->GetInputData(1, 0);
}

//------------------------------------------------------------------------------
void svtkmProbe::SetSourceConnection(svtkAlgorithmOutput* algOutput)
{
  this->SetInputConnection(1, algOutput);
}

//------------------------------------------------------------------------------
int svtkmProbe::RequestData(svtkInformation* svtkNotUsed(request), svtkInformationVector** inputVector,
  svtkInformationVector* outputVector)
{
  // Get the info objects
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* sourceInfo = inputVector[1]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  // Get the input and output
  svtkDataSet* input = svtkDataSet::SafeDownCast(inInfo->Get(svtkDataSet::DATA_OBJECT()));
  svtkDataSet* source = svtkDataSet::SafeDownCast(sourceInfo->Get(svtkDataSet::DATA_OBJECT()));
  svtkDataSet* output = svtkDataSet::SafeDownCast(outInfo->Get(svtkDataSet::DATA_OBJECT()));

  // Copy the input to the output as a starting point
  output->CopyStructure(input);

  try
  {
    // Convert the input dataset to a svtkm::cont::DataSet
    svtkm::cont::DataSet in = tosvtkm::Convert(input);
    // SVTK-m's probe filter requires the source to have at least a cellSet.
    svtkm::cont::DataSet so = tosvtkm::Convert(source, tosvtkm::FieldsFlag::PointsAndCells);
    if (so.GetNumberOfCells() <= 0)
    {
      svtkErrorMacro(<< "The source geometry does not have any cell set,"
                       "aborting svtkmProbe filter");
      return 0;
    }

    svtkmInputFilterPolicy policy;
    svtkm::filter::Probe probe;
    // The input in SVTK is the geometry in SVTKM and the source in SVTK is the input
    // in SVTKM.
    probe.SetGeometry(in);

    auto result = probe.Execute(so, policy);
    for (svtkm::Id i = 0; i < result.GetNumberOfFields(); i++)
    {
      const svtkm::cont::Field& field = result.GetField(i);
      svtkDataArray* fieldArray = fromsvtkm::Convert(field);
      if (field.GetAssociation() == svtkm::cont::Field::Association::POINTS)
      {
        if (strcmp(fieldArray->GetName(), "HIDDEN") == 0)
        {
          fieldArray->SetName(this->ValidPointMaskArrayName.c_str());
        }
        output->GetPointData()->AddArray(fieldArray);
      }
      else if (field.GetAssociation() == svtkm::cont::Field::Association::CELL_SET)
      {
        if (strcmp(fieldArray->GetName(), "HIDDEN") == 0)
        {
          fieldArray->SetName(this->ValidCellMaskArrayName.c_str());
        }
        output->GetCellData()->AddArray(fieldArray);
      }
      fieldArray->FastDelete();
    }
  }
  catch (const svtkm::cont::Error& e)
  {
    svtkErrorMacro(<< "SVTK-m error: " << e.GetMessage());
    return 0;
  }

  this->PassAttributeData(input, source, output);

  return 1;
}

//------------------------------------------------------------------------------
int svtkmProbe::RequestInformation(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // Update the whole extent in the output
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* sourceInfo = inputVector[1]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);
  int wholeExtent[6];
  if (inInfo && outInfo)
  {
    outInfo->CopyEntry(sourceInfo, svtkStreamingDemandDrivenPipeline::TIME_STEPS());
    outInfo->CopyEntry(sourceInfo, svtkStreamingDemandDrivenPipeline::TIME_RANGE());
    inInfo->Get(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), wholeExtent);
    outInfo->Set(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), wholeExtent, 6);

    // Make sure that the scalar type and number of components
    // are propagated from the source not the input.
    if (svtkImageData::HasScalarType(sourceInfo))
    {
      svtkImageData::SetScalarType(svtkImageData::GetScalarType(sourceInfo), outInfo);
    }
    if (svtkImageData::HasNumberOfScalarComponents(sourceInfo))
    {
      svtkImageData::SetNumberOfScalarComponents(
        svtkImageData::GetNumberOfScalarComponents(sourceInfo), outInfo);
    }
    return 1;
  }
  svtkErrorMacro("Missing input or output info!");
  return 0;
}

//------------------------------------------------------------------------------
int svtkmProbe::RequestUpdateExtent(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* sourceInfo = inputVector[1]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  if (inInfo && outInfo)
  { // Source's update exetent should be independent of the resampling extent
    inInfo->Set(svtkStreamingDemandDrivenPipeline::EXACT_EXTENT(), 1);
    sourceInfo->Remove(svtkStreamingDemandDrivenPipeline::UPDATE_EXTENT());
    if (sourceInfo->Has(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()))
    {
      sourceInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(),
        sourceInfo->Get(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()), 6);
    }
    return 1;
  }
  svtkErrorMacro("Missing input or output info!");
  return 0;
}

//------------------------------------------------------------------------------
void svtkmProbe::PassAttributeData(
  svtkDataSet* input, svtkDataObject* svtkNotUsed(source), svtkDataSet* output)
{
  if (this->PassPointArrays)
  { // Copy point data arrays
    int numPtArrays = input->GetPointData()->GetNumberOfArrays();
    for (int i = 0; i < numPtArrays; i++)
    {
      svtkDataArray* da = input->GetPointData()->GetArray(i);
      if (da && !output->GetPointData()->HasArray(da->GetName()))
      {
        output->GetPointData()->AddArray(da);
      }
    }

    // Set active attributes in the output to the active attributes in the input
    for (int i = 0; i < svtkDataSetAttributes::NUM_ATTRIBUTES; ++i)
    {
      svtkAbstractArray* da = input->GetPointData()->GetAttribute(i);
      if (da && da->GetName() && !output->GetPointData()->GetAttribute(i))
      {
        output->GetPointData()->SetAttribute(da, i);
      }
    }
  }

  // copy cell data arrays
  if (this->PassCellArrays)
  {
    int numCellArrays = input->GetCellData()->GetNumberOfArrays();
    for (int i = 0; i < numCellArrays; ++i)
    {
      svtkDataArray* da = input->GetCellData()->GetArray(i);
      if (!output->GetCellData()->HasArray(da->GetName()))
      {
        output->GetCellData()->AddArray(da);
      }
    }

    // Set active attributes in the output to the active attributes in the input
    for (int i = 0; i < svtkDataSetAttributes::NUM_ATTRIBUTES; ++i)
    {
      svtkAbstractArray* da = input->GetCellData()->GetAttribute(i);
      if (da && da->GetName() && !output->GetCellData()->GetAttribute(i))
      {
        output->GetCellData()->SetAttribute(da, i);
      }
    }
  }

  if (this->PassFieldArrays)
  {
    // nothing to do, svtkDemandDrivenPipeline takes care of that.
  }
  else
  {
    output->GetFieldData()->Initialize();
  }
}

//------------------------------------------------------------------------------
void svtkmProbe::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "PassPointArrays: " << this->PassPointArrays << "\n";
  os << indent << "PassCellArrays: " << this->PassCellArrays << "\n";
  os << indent << "PassFieldArray: " << this->PassFieldArrays << "\n";
}
