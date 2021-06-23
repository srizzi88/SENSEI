/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkProbeSelectedLocations.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkProbeSelectedLocations.h"

#include "svtkDataArray.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkObjectFactory.h"
#include "svtkPoints.h"
#include "svtkProbeFilter.h"
#include "svtkSelection.h"
#include "svtkSelectionNode.h"
#include "svtkStreamingDemandDrivenPipeline.h"
#include "svtkTrivialProducer.h"
#include "svtkUnstructuredGrid.h"

svtkStandardNewMacro(svtkProbeSelectedLocations);
//----------------------------------------------------------------------------
svtkProbeSelectedLocations::svtkProbeSelectedLocations() = default;

//----------------------------------------------------------------------------
svtkProbeSelectedLocations::~svtkProbeSelectedLocations() = default;

//----------------------------------------------------------------------------
int svtkProbeSelectedLocations::RequestDataObject(
  svtkInformation* request, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  if (this->PreserveTopology)
  {
    svtkWarningMacro("This filter does not support PreserveTopology.");
    this->PreserveTopology = 0;
  }
  return this->Superclass::RequestDataObject(request, inputVector, outputVector);
}

//----------------------------------------------------------------------------
int svtkProbeSelectedLocations::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  svtkInformation* selInfo = inputVector[1]->GetInformationObject(0);
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  if (!selInfo)
  {
    // When selection is not provided, quietly select nothing.
    return 1;
  }

  svtkSelection* selInput = svtkSelection::GetData(selInfo);
  svtkDataSet* dataInput = svtkDataSet::GetData(inInfo);
  svtkDataSet* output = svtkDataSet::GetData(outInfo);

  svtkSelectionNode* node = nullptr;
  if (selInput->GetNumberOfNodes() == 1)
  {
    node = selInput->GetNode(0);
  }
  if (!node)
  {
    svtkErrorMacro("Selection must have a single node.");
    return 0;
  }

  if (node->GetContentType() != svtkSelectionNode::LOCATIONS)
  {
    svtkErrorMacro("Missing or incompatible CONTENT_TYPE. "
                  "svtkSelection::LOCATIONS required.");
    return 0;
  }

  // From the indicates locations in the selInput, create a unstructured grid to
  // probe with.
  svtkUnstructuredGrid* tempInput = svtkUnstructuredGrid::New();
  svtkPoints* points = svtkPoints::New();
  tempInput->SetPoints(points);
  points->Delete();

  svtkDataArray* dA = svtkArrayDownCast<svtkDataArray>(node->GetSelectionList());
  if (!dA)
  {
    // no locations to probe, quietly quit.
    return 1;
  }

  if (dA->GetNumberOfComponents() != 3)
  {
    svtkErrorMacro("SelectionList must be a 3 component list with point locations.");
    return 0;
  }

  svtkIdType numTuples = dA->GetNumberOfTuples();
  points->SetDataTypeToDouble();
  points->SetNumberOfPoints(numTuples);

  for (svtkIdType cc = 0; cc < numTuples; cc++)
  {
    points->SetPoint(cc, dA->GetTuple(cc));
  }

  svtkDataSet* inputClone = dataInput->NewInstance();
  inputClone->ShallowCopy(dataInput);

  svtkProbeFilter* subFilter = svtkProbeFilter::New();
  svtkTrivialProducer* tp = svtkTrivialProducer::New();
  tp->SetOutput(inputClone);
  subFilter->SetInputConnection(1, tp->GetOutputPort());
  inputClone->Delete();
  tp->Delete();

  tp = svtkTrivialProducer::New();
  tp->SetOutput(tempInput);
  subFilter->SetInputConnection(0, tp->GetOutputPort());
  tempInput->Delete();
  tp->Delete();
  tp = nullptr;

  svtkDebugMacro(<< "Preparing subfilter to extract from dataset");
  // pass all required information to the helper filter
  int piece = 0;
  int npieces = 1;
  int* uExtent = nullptr;
  if (outInfo->Has(svtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER()))
  {
    piece = outInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
    npieces = outInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());
  }
  if (outInfo->Has(svtkStreamingDemandDrivenPipeline::UPDATE_EXTENT()))
  {
    uExtent = outInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_EXTENT());
  }

  subFilter->UpdatePiece(piece, npieces, 0, uExtent);
  output->ShallowCopy(subFilter->GetOutput());
  subFilter->Delete();

  return 1;
}

//----------------------------------------------------------------------------
void svtkProbeSelectedLocations::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
