/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImageIterateFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkImageIterateFilter.h"

#include "svtkDataSetAttributes.h"
#include "svtkImageData.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkStreamingDemandDrivenPipeline.h"
#include "svtkTrivialProducer.h"

//----------------------------------------------------------------------------
svtkImageIterateFilter::svtkImageIterateFilter()
{
  // for filters that execute multiple times
  this->Iteration = 0;
  this->NumberOfIterations = 0;
  this->IterationData = nullptr;
  this->SetNumberOfIterations(1);
  this->InputVector = svtkInformationVector::New();
  this->OutputVector = svtkInformationVector::New();
}

//----------------------------------------------------------------------------
svtkImageIterateFilter::~svtkImageIterateFilter()
{
  this->SetNumberOfIterations(0);
  this->InputVector->Delete();
  this->OutputVector->Delete();
}

//----------------------------------------------------------------------------
void svtkImageIterateFilter::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "NumberOfIterations: " << this->NumberOfIterations << "\n";

  // This variable is included here to pass the PrintSelf test.
  // The variable is public to get around a compiler issue.
  // this->Iteration
}

//----------------------------------------------------------------------------
int svtkImageIterateFilter ::RequestInformation(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  svtkInformation* in = inInfo;
  for (int i = 0; i < this->NumberOfIterations; ++i)
  {
    this->Iteration = i;

    int next = i + 1;
    svtkInformation* out;
    if (next == this->NumberOfIterations)
    {
      out = outInfo;
    }
    else
    {
      out = this->IterationData[next]->GetOutputInformation(0);
    }
    out->CopyEntry(in, svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT());

    out->CopyEntry(in, svtkDataObject::ORIGIN());
    out->CopyEntry(in, svtkDataObject::SPACING());

    svtkInformation* scalarInfo = svtkDataObject::GetActiveFieldInformation(
      in, svtkDataObject::FIELD_ASSOCIATION_POINTS, svtkDataSetAttributes::SCALARS);
    if (scalarInfo)
    {
      int scalarType = SVTK_DOUBLE;
      if (scalarInfo->Has(svtkDataObject::FIELD_ARRAY_TYPE()))
      {
        scalarType = scalarInfo->Get(svtkDataObject::FIELD_ARRAY_TYPE());
      }
      int numComp = 1;
      if (scalarInfo->Has(svtkDataObject::FIELD_NUMBER_OF_COMPONENTS()))
      {
        numComp = scalarInfo->Get(svtkDataObject::FIELD_NUMBER_OF_COMPONENTS());
      }
      svtkDataObject::SetPointDataActiveScalarInfo(out, scalarType, numComp);
    }

    if (!this->IterativeRequestInformation(in, out))
    {
      return 0;
    }

    in = out;
  }

  return 1;
}

//----------------------------------------------------------------------------
int svtkImageIterateFilter ::RequestUpdateExtent(
  svtkInformation*, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* out = outputVector->GetInformationObject(0);
  for (int i = this->NumberOfIterations - 1; i >= 0; --i)
  {
    this->Iteration = i;

    svtkInformation* in;
    if (i == 0)
    {
      in = inInfo;
    }
    else
    {
      in = this->IterationData[i]->GetOutputInformation(0);
    }
    in->CopyEntry(out, svtkStreamingDemandDrivenPipeline::UPDATE_EXTENT());

    if (!this->IterativeRequestUpdateExtent(in, out))
    {
      return 0;
    }

    out = in;
  }

  return 1;
}

//----------------------------------------------------------------------------
int svtkImageIterateFilter::RequestData(
  svtkInformation* request, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  svtkInformation* in = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  for (int i = 0; i < this->NumberOfIterations; ++i)
  {
    this->Iteration = i;

    int next = i + 1;
    svtkInformation* out;
    if (next == this->NumberOfIterations)
    {
      out = outInfo;
    }
    else
    {
      out = this->IterationData[next]->GetOutputInformation(0);
    }

    this->InputVector->SetInformationObject(0, in);
    this->OutputVector->SetInformationObject(0, out);
    if (!this->IterativeRequestData(request, &this->InputVector, this->OutputVector))
    {
      return 0;
    }

    if (in->Get(svtkDemandDrivenPipeline::RELEASE_DATA()))
    {
      svtkDataObject* inData = in->Get(svtkDataObject::DATA_OBJECT());
      inData->ReleaseData();
    }

    in = out;
  }
  this->InputVector->SetNumberOfInformationObjects(0);
  this->OutputVector->SetNumberOfInformationObjects(0);

  return 1;
}

//----------------------------------------------------------------------------
// Called by the above for each decomposition.  Subclass can modify
// the defaults by implementing this method.
int svtkImageIterateFilter::IterativeRequestInformation(svtkInformation*, svtkInformation*)
{
  return 1;
}

//----------------------------------------------------------------------------
// Called by the above for each decomposition.  Subclass can modify
// the defaults by implementing this method.
int svtkImageIterateFilter::IterativeRequestUpdateExtent(svtkInformation*, svtkInformation*)
{
  return 1;
}

//----------------------------------------------------------------------------
// Called by the above for each decomposition.  Subclass can modify
// the defaults by implementing this method.
int svtkImageIterateFilter::IterativeRequestData(
  svtkInformation* request, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  return this->Superclass::RequestData(request, inputVector, outputVector);
}

//----------------------------------------------------------------------------
// Filters that execute multiple times per update use this internal method.
void svtkImageIterateFilter::SetNumberOfIterations(int num)
{
  int idx;

  if (num == this->NumberOfIterations)
  {
    return;
  }

  // delete previous temporary caches
  // (first and last are global input and output)
  if (this->IterationData)
  {
    for (idx = 1; idx < this->NumberOfIterations; ++idx)
    {
      this->IterationData[idx]->Delete();
      this->IterationData[idx] = nullptr;
    }
    delete[] this->IterationData;
    this->IterationData = nullptr;
  }

  // special case for destructor
  if (num == 0)
  {
    return;
  }

  // create new ones (first and last set later to input and output)
  this->IterationData = reinterpret_cast<svtkAlgorithm**>(new void*[num + 1]);
  this->IterationData[0] = this->IterationData[num] = nullptr;
  for (idx = 1; idx < num; ++idx)
  {
    svtkImageData* cache = svtkImageData::New();
    svtkTrivialProducer* tp = svtkTrivialProducer::New();
    tp->ReleaseDataFlagOn();
    tp->SetOutput(cache);
    this->IterationData[idx] = tp;
    cache->Delete();
  }

  this->NumberOfIterations = num;
  this->Modified();
}
