/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkElevationFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkmHistogram.h"

#include "svtkCellData.h"
#include "svtkDataSet.h"
#include "svtkDoubleArray.h"
#include "svtkFieldData.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkPointData.h"
#include "svtkTable.h"
#include "svtkUnstructuredGrid.h"

#include "svtkmlib/ArrayConverters.h"
#include "svtkmlib/DataSetConverters.h"

#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkSmartPointer.h"

#include "svtkmFilterPolicy.h"
#include <svtkm/cont/ArrayRangeCompute.hxx>
#include <svtkm/filter/Histogram.h>

svtkStandardNewMacro(svtkmHistogram);

//------------------------------------------------------------------------------
svtkmHistogram::svtkmHistogram()
{
  this->CustomBinRange[0] = 0;
  this->CustomBinRange[0] = 100;
  this->UseCustomBinRanges = false;
  this->CenterBinsAroundMinAndMax = false;
  this->NumberOfBins = 10;
}

//------------------------------------------------------------------------------
svtkmHistogram::~svtkmHistogram() {}

//-----------------------------------------------------------------------------
int svtkmHistogram::FillInputPortInformation(int port, svtkInformation* info)
{
  this->Superclass::FillInputPortInformation(port, info);

  info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkDataObject");
  return 1;
}

//------------------------------------------------------------------------------
int svtkmHistogram::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkDataSet* input = svtkDataSet::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));

  svtkTable* output = svtkTable::GetData(outputVector, 0);
  output->Initialize();

  // These are the mid-points for each of the bins
  svtkSmartPointer<svtkDoubleArray> binExtents = svtkSmartPointer<svtkDoubleArray>::New();
  binExtents->SetNumberOfComponents(1);
  binExtents->SetNumberOfTuples(static_cast<svtkIdType>(this->NumberOfBins));
  binExtents->SetName("bin_extents");
  binExtents->FillComponent(0, 0.0);

  // Grab the input array to process to determine the field we want to apply histogram
  int association = this->GetInputArrayAssociation(0, inputVector);
  auto fieldArray = this->GetInputArrayToProcess(0, inputVector);
  if ((association != svtkDataObject::FIELD_ASSOCIATION_POINTS &&
        association != svtkDataObject::FIELD_ASSOCIATION_CELLS) ||
    fieldArray == nullptr || fieldArray->GetName() == nullptr || fieldArray->GetName()[0] == '\0')
  {
    svtkErrorMacro(<< "Invalid field: Requires a point or cell field with a valid name.");
    return 0;
  }

  const char* fieldName = fieldArray->GetName();

  try
  {
    svtkm::cont::DataSet in = tosvtkm::Convert(input);
    auto field = tosvtkm::Convert(fieldArray, association);
    in.AddField(field);

    svtkmInputFilterPolicy policy;
    svtkm::filter::Histogram filter;

    filter.SetNumberOfBins(static_cast<svtkm::Id>(this->NumberOfBins));
    filter.SetActiveField(fieldName, field.GetAssociation());
    if (this->UseCustomBinRanges)
    {
      if (this->CustomBinRange[0] > this->CustomBinRange[1])
      {
        svtkWarningMacro("Custom bin range adjusted to keep min <= max value");
        double min = this->CustomBinRange[1];
        double max = this->CustomBinRange[0];
        this->CustomBinRange[0] = min;
        this->CustomBinRange[1] = max;
      }
      filter.SetRange(svtkm::Range(this->CustomBinRange[0], this->CustomBinRange[1]));
    }
    auto result = filter.Execute(in, policy);
    this->BinDelta = filter.GetBinDelta();
    this->ComputedRange[0] = filter.GetComputedRange().Min;
    this->ComputedRange[1] = filter.GetComputedRange().Max;

    // Convert the result back
    svtkDataArray* resultingArray = fromsvtkm::Convert(result.GetField("histogram"));
    resultingArray->SetName("bin_values");
    if (resultingArray == nullptr)
    {
      svtkErrorMacro(<< "Unable to convert result array from SVTK-m to SVTK");
      return 0;
    }
    this->FillBinExtents(binExtents);
    output->GetRowData()->AddArray(binExtents);
    output->GetRowData()->AddArray(resultingArray);

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
void svtkmHistogram::PrintSelf(std::ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "NumberOfBins: " << NumberOfBins << "\n";
  os << indent << "UseCustomBinRanges: " << UseCustomBinRanges << "\n";
  os << indent << "CenterBinsAroundMinAndMax: " << CenterBinsAroundMinAndMax << "\n";
  os << indent << "CustomBinRange: " << CustomBinRange[0] << ", " << CustomBinRange[1] << "\n";
}

//------------------------------------------------------------------------------
void svtkmHistogram::FillBinExtents(svtkDoubleArray* binExtents)
{
  binExtents->SetNumberOfComponents(1);
  binExtents->SetNumberOfTuples(static_cast<svtkIdType>(this->NumberOfBins));
  double binDelta = this->CenterBinsAroundMinAndMax
    ? ((this->ComputedRange[1] - this->ComputedRange[0]) / (this->NumberOfBins - 1))
    : this->BinDelta;
  double halfBinDelta = binDelta / 2.0;
  for (svtkIdType i = 0; i < static_cast<svtkIdType>(this->NumberOfBins); i++)
  {
    binExtents->SetValue(i,
      this->ComputedRange[0] + (i * binDelta) +
        (this->CenterBinsAroundMinAndMax ? 0.0 : halfBinDelta));
  }
}
