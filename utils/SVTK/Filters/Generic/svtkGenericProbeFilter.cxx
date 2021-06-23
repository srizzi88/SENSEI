/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkGenericProbeFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkGenericProbeFilter.h"

#include "svtkCell.h"
#include "svtkCellData.h"
#include "svtkDoubleArray.h"
#include "svtkGenericAdaptorCell.h"
#include "svtkGenericAttribute.h"
#include "svtkGenericAttributeCollection.h"
#include "svtkGenericCellIterator.h"
#include "svtkGenericDataSet.h"
#include "svtkIdTypeArray.h"
#include "svtkImageData.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkStreamingDemandDrivenPipeline.h"

svtkStandardNewMacro(svtkGenericProbeFilter);

//----------------------------------------------------------------------------
svtkGenericProbeFilter::svtkGenericProbeFilter()
{
  this->ValidPoints = svtkIdTypeArray::New();
  this->SetNumberOfInputPorts(2);
}

//----------------------------------------------------------------------------
svtkGenericProbeFilter::~svtkGenericProbeFilter()
{
  this->ValidPoints->Delete();
  this->ValidPoints = nullptr;
}

//----------------------------------------------------------------------------
void svtkGenericProbeFilter::SetSourceData(svtkGenericDataSet* input)
{
  this->SetInputData(1, input);
}

//----------------------------------------------------------------------------
svtkGenericDataSet* svtkGenericProbeFilter::GetSource()
{
  if (this->GetNumberOfInputConnections(1) < 1)
  {
    return nullptr;
  }

  return svtkGenericDataSet::SafeDownCast(this->GetExecutive()->GetInputData(1, 0));
}

//----------------------------------------------------------------------------
int svtkGenericProbeFilter::RequestInformation(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  svtkInformation* sourceInfo = inputVector[1]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  // A variation of the bug fix from John Biddiscombe.
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

//----------------------------------------------------------------------------
int svtkGenericProbeFilter::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* sourceInfo = inputVector[1]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  svtkDataSet* input = svtkDataSet::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));
  svtkDataSet* output = svtkDataSet::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  svtkGenericDataSet* source =
    svtkGenericDataSet::SafeDownCast(sourceInfo->Get(svtkDataObject::DATA_OBJECT()));

  svtkIdType ptId, numPts;
  double x[3], tol2;
  int subId;

  double pcoords[3];

  svtkDebugMacro(<< "Probing data");

  if (source == nullptr)
  {
    svtkErrorMacro(<< "Source is nullptr.");
    return 1;
  }

  // First, copy the input to the output as a starting point
  output->CopyStructure(input);

  numPts = input->GetNumberOfPoints();
  this->ValidPoints->Allocate(numPts);

  svtkPointData* outputPD = output->GetPointData();
  svtkCellData* outputCD = output->GetCellData();

  // prepare the output attributes
  svtkGenericAttributeCollection* attributes = source->GetAttributes();
  svtkGenericAttribute* attribute;
  svtkDataArray* attributeArray;

  int c = attributes->GetNumberOfAttributes();
  svtkDataSetAttributes* dsAttributes;

  int attributeType;

  double* tuples = new double[attributes->GetMaxNumberOfComponents()];

  for (int i = 0; i < c; ++i)
  {
    attribute = attributes->GetAttribute(i);
    attributeType = attribute->GetType();
    if (attribute->GetCentering() == svtkPointCentered)
    {
      dsAttributes = outputPD;
    }
    else // svtkCellCentered
    {
      dsAttributes = outputCD;
    }
    attributeArray = svtkDataArray::CreateDataArray(attribute->GetComponentType());
    attributeArray->SetNumberOfComponents(attribute->GetNumberOfComponents());
    attributeArray->SetName(attribute->GetName());
    dsAttributes->AddArray(attributeArray);
    attributeArray->Delete();

    if (dsAttributes->GetAttribute(attributeType) == nullptr)
    {
      dsAttributes->SetActiveAttribute(dsAttributes->GetNumberOfArrays() - 1, attributeType);
    }
  }

  // Use tolerance as a function of size of source data
  //
  tol2 = source->GetLength();
  tol2 = tol2 ? tol2 * tol2 / 1000.0 : 0.001;
  cout << "tol2=" << tol2 << endl;
  // Loop over all input points, interpolating source data
  //
  int abort = 0;

  // Need to use source to create a cellIt since this class is virtual
  svtkGenericCellIterator* cellIt = source->NewCellIterator();

  svtkIdType progressInterval = numPts / 20 + 1;
  for (ptId = 0; ptId < numPts && !abort; ptId++)
  {
    if (!(ptId % progressInterval))
    {
      this->UpdateProgress(static_cast<double>(ptId) / numPts);
      abort = GetAbortExecute();
    }

    // Get the xyz coordinate of the point in the input dataset
    input->GetPoint(ptId, x);

    // Find the cell that contains xyz and get it
    if (source->FindCell(x, cellIt, tol2, subId, pcoords))
    {
      svtkGenericAdaptorCell* cellProbe = cellIt->GetCell();

      // for each cell-centered attribute: copy the value
      for (int attrib = 0; attrib < c; ++attrib)
      {
        if (attributes->GetAttribute(attrib)->GetCentering() == svtkCellCentered)
        {
          svtkDataArray* array = outputCD->GetArray(attributes->GetAttribute(attrib)->GetName());
          double* values = attributes->GetAttribute(attrib)->GetTuple(cellProbe);
          array->InsertNextTuple(values);
        }
      }

      // for each point-centered attribute: interpolate the value
      int j = 0;
      for (int attribute_idx = 0; attribute_idx < c; ++attribute_idx)
      {
        svtkGenericAttribute* a = attributes->GetAttribute(attribute_idx);
        if (a->GetCentering() == svtkPointCentered)
        {
          cellProbe->InterpolateTuple(a, pcoords, tuples);
          outputPD->GetArray(j)->InsertTuple(ptId, tuples);
          ++j;
        }
      }
      this->ValidPoints->InsertNextValue(ptId);
    }
    else
    {
      outputPD->NullPoint(ptId);
    }
  }
  cellIt->Delete();
  delete[] tuples;

  return 1;
}

//----------------------------------------------------------------------------
void svtkGenericProbeFilter::PrintSelf(ostream& os, svtkIndent indent)
{
  svtkGenericDataSet* source = this->GetSource();

  this->Superclass::PrintSelf(os, indent);

  os << indent << "Source: " << source << "\n";
  os << indent << "ValidPoints: " << this->ValidPoints << "\n";
}

//----------------------------------------------------------------------------
int svtkGenericProbeFilter::FillInputPortInformation(int port, svtkInformation* info)
{
  if (!this->Superclass::FillInputPortInformation(port, info))
  {
    return 0;
  }
  if (port == 1)
  {
    info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkGenericDataSet");
  }
  else
  {
    info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkDataSet");
  }
  return 1;
}
