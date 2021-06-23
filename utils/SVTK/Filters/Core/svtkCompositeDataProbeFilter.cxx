/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkCompositeDataProbeFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkCompositeDataProbeFilter.h"

#include "svtkCellData.h"
#include "svtkCompositeDataIterator.h"
#include "svtkCompositeDataPipeline.h"
#include "svtkCompositeDataSet.h"
#include "svtkDataArray.h"
#include "svtkDataSet.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkMath.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkSmartPointer.h"

svtkStandardNewMacro(svtkCompositeDataProbeFilter);
//----------------------------------------------------------------------------
svtkCompositeDataProbeFilter::svtkCompositeDataProbeFilter()
{
  this->PassPartialArrays = false;
}

//----------------------------------------------------------------------------
svtkCompositeDataProbeFilter::~svtkCompositeDataProbeFilter() = default;

//----------------------------------------------------------------------------
int svtkCompositeDataProbeFilter::FillInputPortInformation(int port, svtkInformation* info)
{
  this->Superclass::FillInputPortInformation(port, info);
  if (port == 1)
  {
    // We have to save svtkDataObject since this filter can work on svtkDataSet
    // and svtkCompositeDataSet consisting of svtkDataSet leaf nodes.
    info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkCompositeDataSet");
    info->Append(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkDataSet");
  }
  return 1;
}

//----------------------------------------------------------------------------
svtkExecutive* svtkCompositeDataProbeFilter::CreateDefaultExecutive()
{
  return svtkCompositeDataPipeline::New();
}

//----------------------------------------------------------------------------
int svtkCompositeDataProbeFilter::RequestData(
  svtkInformation* request, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* sourceInfo = inputVector[1]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  svtkDataSet* input = svtkDataSet::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));

  svtkDataSet* sourceDS = svtkDataSet::SafeDownCast(sourceInfo->Get(svtkDataObject::DATA_OBJECT()));
  svtkCompositeDataSet* sourceComposite =
    svtkCompositeDataSet::SafeDownCast(sourceInfo->Get(svtkDataObject::DATA_OBJECT()));
  svtkDataSet* output = svtkDataSet::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  if (!input)
  {
    return 0;
  }

  if (!sourceDS && !sourceComposite)
  {
    svtkErrorMacro("svtkDataSet or svtkCompositeDataSet is expected as the input "
                  "on port 1");
    return 0;
  }

  if (sourceDS)
  {
    // Superclass knowns exactly what to do.
    return this->Superclass::RequestData(request, inputVector, outputVector);
  }

  // First, copy the input to the output as a starting point
  output->CopyStructure(input);

  if (this->BuildFieldList(sourceComposite))
  {
    this->InitializeForProbing(input, output);

    svtkSmartPointer<svtkCompositeDataIterator> iter;
    iter.TakeReference(sourceComposite->NewIterator());
    // We do reverse traversal, so that for hierarchical datasets, we traverse the
    // higher resolution blocks first.
    int idx = 0;
    for (iter->InitReverseTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
    {
      sourceDS = svtkDataSet::SafeDownCast(iter->GetCurrentDataObject());
      if (!sourceDS)
      {
        svtkErrorMacro("All leaves in the multiblock dataset must be svtkDataSet.");
        return 0;
      }

      if (sourceDS->GetNumberOfPoints() == 0)
      {
        continue;
      }

      this->DoProbing(input, idx, sourceDS, output);
      idx++;
    }
  }

  this->PassAttributeData(input, sourceComposite, output);
  return 1;
}

//----------------------------------------------------------------------------
void svtkCompositeDataProbeFilter::InitializeOutputArrays(svtkPointData* outPD, svtkIdType numPts)
{
  if (!this->PassPartialArrays)
  {
    this->Superclass::InitializeOutputArrays(outPD, numPts);
  }
  else
  {
    for (int cc = 0; cc < outPD->GetNumberOfArrays(); cc++)
    {
      svtkDataArray* da = outPD->GetArray(cc);
      if (da)
      {
        da->SetNumberOfTuples(numPts);
        double null_value = 0.0;
        if (da->IsA("svtkDoubleArray") || da->IsA("svtkFloatArray"))
        {
          null_value = svtkMath::Nan();
        }
        da->Fill(null_value);
      }
    }
  }
}

//----------------------------------------------------------------------------
int svtkCompositeDataProbeFilter::BuildFieldList(svtkCompositeDataSet* source)
{
  delete this->PointList;
  delete this->CellList;
  this->PointList = nullptr;
  this->CellList = nullptr;

  svtkSmartPointer<svtkCompositeDataIterator> iter;
  iter.TakeReference(source->NewIterator());

  int numDatasets = 0;
  for (iter->InitReverseTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
  {
    svtkDataSet* sourceDS = svtkDataSet::SafeDownCast(iter->GetCurrentDataObject());
    if (!sourceDS)
    {
      svtkErrorMacro("All leaves in the multiblock dataset must be svtkDataSet.");
      return 0;
    }
    if (sourceDS->GetNumberOfPoints() == 0)
    {
      continue;
    }
    numDatasets++;
  }

  this->PointList = new svtkDataSetAttributes::FieldList(numDatasets);
  this->CellList = new svtkDataSetAttributes::FieldList(numDatasets);

  bool initializedPD = false;
  bool initializedCD = false;
  for (iter->InitReverseTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
  {
    svtkDataSet* sourceDS = svtkDataSet::SafeDownCast(iter->GetCurrentDataObject());
    if (sourceDS->GetNumberOfPoints() == 0)
    {
      continue;
    }
    if (!initializedPD)
    {
      this->PointList->InitializeFieldList(sourceDS->GetPointData());
      initializedPD = true;
    }
    else
    {
      if (this->PassPartialArrays)
      {
        this->PointList->UnionFieldList(sourceDS->GetPointData());
      }
      else
      {
        this->PointList->IntersectFieldList(sourceDS->GetPointData());
      }
    }

    if (sourceDS->GetNumberOfCells() > 0)
    {
      if (!initializedCD)
      {
        this->CellList->InitializeFieldList(sourceDS->GetCellData());
        initializedCD = true;
      }
      else
      {
        if (this->PassPartialArrays)
        {
          this->CellList->UnionFieldList(sourceDS->GetCellData());
        }
        else
        {
          this->CellList->IntersectFieldList(sourceDS->GetCellData());
        }
      }
    }
  }
  return 1;
}

//----------------------------------------------------------------------------
void svtkCompositeDataProbeFilter::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << "PassPartialArrays: " << this->PassPartialArrays << endl;
}
