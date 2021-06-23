/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkOTFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkOTFilter.h"

#include "svtkDataArray.h"
#include "svtkDataObject.h"
#include "svtkDataSetAttributes.h"
#include "svtkInformation.h"
#include "svtkSmartPointer.h"
#include "svtkTable.h"

#include "svtkOTIncludes.h"
#include "svtkOTUtilities.h"

using namespace OT;

//-----------------------------------------------------------------------------
svtkOTFilter::svtkOTFilter()
{
  this->SetInputArrayToProcess(
    0, 0, 0, svtkDataObject::FIELD_ASSOCIATION_POINTS_THEN_CELLS, svtkDataSetAttributes::SCALARS);
}

//-----------------------------------------------------------------------------
svtkOTFilter::~svtkOTFilter() {}

//-----------------------------------------------------------------------------
int svtkOTFilter::FillInputPortInformation(int port, svtkInformation* info)
{
  this->Superclass::FillInputPortInformation(port, info);
  info->Remove(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE());
  info->Append(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkTable");
  return 1;
}

//-----------------------------------------------------------------------------
void svtkOTFilter::AddToOutput(Sample* ns, const std::string& name)
{
  svtkSmartPointer<svtkDataArray> outArray =
    svtkSmartPointer<svtkDataArray>::Take(svtkOTUtilities::SampleToArray(ns));
  outArray->SetName(name.c_str());
  this->Output->AddColumn(outArray);
}

//-----------------------------------------------------------------------------
int svtkOTFilter::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  this->Output = svtkTable::GetData(outputVector, 0);
  this->Output->Initialize();

  svtkDataArray* dataArray = this->GetInputArrayToProcess(0, inputVector);
  Sample* ns = svtkOTUtilities::ArrayToSample(dataArray);

  int ret = 1;
  if (ns)
  {
    ret = this->Process(ns);
    delete ns;
  }
  return ret;
}

//-----------------------------------------------------------------------------
void svtkOTFilter::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
