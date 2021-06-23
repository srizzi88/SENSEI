/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkExtractVOI.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkmExtractVOI.h"

#include "svtkCellData.h"
#include "svtkExtractStructuredGridHelper.h"
#include "svtkImageData.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkPointData.h"

#include "svtkmlib/ArrayConverters.h"
#include "svtkmlib/ImageDataConverter.h"

#include "svtkmFilterPolicy.h"

#include "svtkm/filter/ExtractStructured.h"
#include "svtkm/filter/ExtractStructured.hxx"

namespace
{

struct InputFilterPolicy : public svtkmInputFilterPolicy
{
  using StructuredCellSetList = svtkm::List<svtkm::cont::CellSetStructured<1>,
    svtkm::cont::CellSetStructured<2>, svtkm::cont::CellSetStructured<3> >;
};

}

svtkStandardNewMacro(svtkmExtractVOI);

//------------------------------------------------------------------------------
void svtkmExtractVOI::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
svtkmExtractVOI::svtkmExtractVOI() = default;
svtkmExtractVOI::~svtkmExtractVOI() = default;

//------------------------------------------------------------------------------
int svtkmExtractVOI::RequestData(
  svtkInformation* request, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  svtkImageData* input = svtkImageData::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));
  svtkImageData* output = svtkImageData::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  try
  {
    // convert the input dataset to a svtkm::cont::DataSet
    auto in = tosvtkm::Convert(input, tosvtkm::FieldsFlag::PointsAndCells);

    // transform VOI
    int inExtents[6], voi[6];
    input->GetExtent(inExtents);
    for (int i = 0; i < 6; i += 2)
    {
      voi[i] = this->VOI[i] - inExtents[i];
      voi[i + 1] = this->VOI[i + 1] - inExtents[i] + 1;
    }

    // apply the filter
    svtkm::filter::PolicyBase<InputFilterPolicy> policy;
    svtkm::filter::ExtractStructured filter;
    filter.SetVOI(voi[0], voi[1], voi[2], voi[3], voi[4], voi[5]);
    filter.SetSampleRate(this->SampleRate[0], this->SampleRate[1], this->SampleRate[2]);
    filter.SetIncludeBoundary((this->IncludeBoundary != 0));
    auto result = filter.Execute(in, policy);

    // convert back to svtkImageData
    int outExtents[6];
    this->Internal->GetOutputWholeExtent(outExtents);
    if (!fromsvtkm::Convert(result, outExtents, output, input))
    {
      svtkErrorMacro(<< "Unable to convert SVTKm DataSet back to SVTK");
      return 0;
    }
  }
  catch (const svtkm::cont::Error& e)
  {
    svtkErrorMacro(<< "SVTK-m error: " << e.GetMessage() << "Falling back to svtkExtractVOI");
    return this->Superclass::RequestData(request, inputVector, outputVector);
  }

  return 1;
}
