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
#include "svtkmNDHistogram.h"
#include "svtkmConfig.h"

#include "svtkCellData.h"
#include "svtkDataSet.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkSparseArray.h"
#include "svtkTable.h"
#include "svtkmFilterPolicy.h"

#include "svtkmlib/ArrayConverters.h"
#include "svtkmlib/DataSetConverters.h"

#include <svtkm/filter/NDHistogram.h>

svtkStandardNewMacro(svtkmNDHistogram);

//------------------------------------------------------------------------------
void svtkmNDHistogram::PrintSelf(std::ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "FieldNames: "
     << "\n";
  for (const auto& fieldName : FieldNames)
  {
    os << indent << fieldName << " ";
  }
  os << indent << "\n";
  os << indent << "NumberOfBins: "
     << "\n";
  for (const auto& nob : NumberOfBins)
  {
    os << indent << nob << " ";
  }
  os << indent << "\n";
  os << indent << "BinDeltas: "
     << "\n";
  for (const auto& bd : BinDeltas)
  {
    os << indent << bd << " ";
  }
  os << indent << "\n";
  os << indent << "DataRanges: "
     << "\n";
  for (const auto& dr : DataRanges)
  {
    os << indent << dr.first << " " << dr.second << " ";
  }
  os << indent << "\n";
}

//------------------------------------------------------------------------------
svtkmNDHistogram::svtkmNDHistogram() {}

//------------------------------------------------------------------------------
svtkmNDHistogram::~svtkmNDHistogram() {}

//------------------------------------------------------------------------------
void svtkmNDHistogram::AddFieldAndBin(const std::string& fieldName, const svtkIdType& numberOfBins)
{
  this->FieldNames.push_back(fieldName);
  this->NumberOfBins.push_back(numberOfBins);
  this->SetInputArrayToProcess(static_cast<int>(this->FieldNames.size()), 0, 0,
    svtkDataObject::FIELD_ASSOCIATION_POINTS, fieldName.c_str());
}

//------------------------------------------------------------------------------
double svtkmNDHistogram::GetBinDelta(size_t fieldIndex)
{
  return this->BinDeltas[fieldIndex];
}

//------------------------------------------------------------------------------
std::pair<double, double> svtkmNDHistogram::GetDataRange(size_t fieldIndex)
{
  return this->DataRanges[fieldIndex];
}

//------------------------------------------------------------------------------
int svtkmNDHistogram::FillInputPortInformation(int port, svtkInformation* info)
{
  this->Superclass::FillInputPortInformation(port, info);

  info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkDataObject");
  return 1;
}

//------------------------------------------------------------------------------
int svtkmNDHistogram::GetFieldIndexFromFieldName(const std::string& fieldName)
{
  auto iter = std::find(this->FieldNames.begin(), this->FieldNames.end(), fieldName);
  return (iter == std::end(this->FieldNames)) ? -1
                                              : static_cast<int>(iter - this->FieldNames.begin());
}

//------------------------------------------------------------------------------
int svtkmNDHistogram::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkDataSet* input = svtkDataSet::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));

  svtkArrayData* output = svtkArrayData::GetData(outputVector, 0);
  output->ClearArrays();

  try
  {
    svtkm::cont::DataSet in = tosvtkm::Convert(input, tosvtkm::FieldsFlag::PointsAndCells);

    svtkmInputFilterPolicy policy;
    svtkm::filter::NDHistogram filter;
    for (size_t i = 0; i < this->FieldNames.size(); i++)
    {
      filter.AddFieldAndBin(this->FieldNames[i], this->NumberOfBins[i]);
    }
    svtkm::cont::DataSet out = filter.Execute(in, policy);

    svtkm::Id numberOfFields = out.GetNumberOfFields();
    this->BinDeltas.clear();
    this->DataRanges.clear();
    this->BinDeltas.reserve(static_cast<size_t>(numberOfFields));
    this->DataRanges.reserve(static_cast<size_t>(numberOfFields));

    // Fetch the field array out of the svtkm filter result
    size_t index = 0;
    std::vector<svtkDataArray*> fArrays;
    for (auto& fn : this->FieldNames)
    {
      svtkDataArray* fnArray = fromsvtkm::Convert(out.GetField(fn));
      fnArray->SetName(fn.c_str());
      fArrays.push_back(fnArray);
      this->BinDeltas.push_back(filter.GetBinDelta(index));
      this->DataRanges.push_back(
        std::make_pair(filter.GetDataRange(index).Min, filter.GetDataRange(index).Max));
      index++;
    }
    svtkDataArray* frequencyArray = fromsvtkm::Convert(out.GetField("Frequency"));
    frequencyArray->SetName("Frequency");

    // Create the sparse array
    svtkSparseArray<double>* sparseArray = svtkSparseArray<double>::New();
    svtkArrayExtents sae; // sparse array extent
    size_t ndims(fArrays.size());
    sae.SetDimensions(static_cast<svtkArrayExtents::DimensionT>(ndims));
    for (size_t i = 0; i < ndims; i++)
    {
      sae[static_cast<svtkArrayExtents::DimensionT>(i)] =
        svtkArrayRange(0, fArrays[i]->GetNumberOfValues());
    }
    sparseArray->Resize(sae);

    // Set the dimension label
    for (size_t i = 0; i < ndims; i++)
    {
      sparseArray->SetDimensionLabel(static_cast<svtkIdType>(i), fArrays[i]->GetName());
    }
    // Fill in the sparse array
    for (svtkIdType i = 0; i < frequencyArray->GetNumberOfValues(); i++)
    {
      svtkArrayCoordinates coords;
      coords.SetDimensions(static_cast<svtkArrayCoordinates::DimensionT>(ndims));
      for (size_t j = 0; j < ndims; j++)
      {
        coords[static_cast<svtkArrayCoordinates::DimensionT>(j)] = fArrays[j]->GetComponent(i, 0);
      }
      sparseArray->SetValue(coords, frequencyArray->GetComponent(i, 0));
    }
    output->AddArray(sparseArray);

    // Clean up the memory
    for (auto& fArray : fArrays)
    {
      fArray->FastDelete();
    }
    frequencyArray->FastDelete();
    sparseArray->FastDelete();
  }
  catch (const svtkm::cont::Error& e)
  {
    svtkErrorMacro(<< "SVTK-m error: " << e.GetMessage());
    return 0;
  }
  return 1;
}
