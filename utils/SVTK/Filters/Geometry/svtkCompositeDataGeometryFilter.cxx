/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkCompositeDataGeometryFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkCompositeDataGeometryFilter.h"

#include "svtkAppendPolyData.h"
#include "svtkCompositeDataIterator.h"
#include "svtkCompositeDataPipeline.h"
#include "svtkCompositeDataSet.h"
#include "svtkDataSet.h"
#include "svtkDataSetSurfaceFilter.h"
#include "svtkImageData.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkNew.h"
#include "svtkObjectFactory.h"
#include "svtkSmartPointer.h"

svtkStandardNewMacro(svtkCompositeDataGeometryFilter);

//-----------------------------------------------------------------------------
svtkCompositeDataGeometryFilter::svtkCompositeDataGeometryFilter() = default;

//-----------------------------------------------------------------------------
svtkCompositeDataGeometryFilter::~svtkCompositeDataGeometryFilter() = default;

//-----------------------------------------------------------------------------
int svtkCompositeDataGeometryFilter::FillInputPortInformation(
  int svtkNotUsed(port), svtkInformation* info)
{
  // now add our info
  info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkCompositeDataSet");
  return 1;
}

//-----------------------------------------------------------------------------
svtkTypeBool svtkCompositeDataGeometryFilter::ProcessRequest(
  svtkInformation* request, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // generate the data
  if (request->Has(svtkCompositeDataPipeline::REQUEST_DATA()))
  {
    int retVal = this->RequestCompositeData(request, inputVector, outputVector);
    return retVal;
  }

  return this->Superclass::ProcessRequest(request, inputVector, outputVector);
}

//-----------------------------------------------------------------------------
int svtkCompositeDataGeometryFilter::RequestCompositeData(
  svtkInformation*, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  svtkCompositeDataSet* input = svtkCompositeDataSet::GetData(inputVector[0], 0);
  if (!input)
  {
    svtkErrorMacro("No input composite dataset provided.");
    return 0;
  }

  svtkPolyData* output = svtkPolyData::GetData(outputVector, 0);
  if (!output)
  {
    svtkErrorMacro("No output polydata provided.");
    return 0;
  }

  svtkNew<svtkAppendPolyData> append;
  svtkSmartPointer<svtkCompositeDataIterator> iter;
  iter.TakeReference(input->NewIterator());

  for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
  {
    svtkDataSet* ds = svtkDataSet::SafeDownCast(iter->GetCurrentDataObject());
    if (ds && ds->GetNumberOfPoints() > 0)
    {
      svtkNew<svtkDataSetSurfaceFilter> dssf;
      dssf->SetInputData(ds);
      dssf->Update();
      append->AddInputDataObject(dssf->GetOutputDataObject(0));
    }
  }
  if (append->GetNumberOfInputConnections(0) > 0)
  {
    append->Update();
    output->ShallowCopy(append->GetOutput());
  }

  return 1;
}

//-----------------------------------------------------------------------------
svtkExecutive* svtkCompositeDataGeometryFilter::CreateDefaultExecutive()
{
  return svtkCompositeDataPipeline::New();
}

//-----------------------------------------------------------------------------
void svtkCompositeDataGeometryFilter::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
