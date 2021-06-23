/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkRemoveHiddenData.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/
#include "svtkRemoveHiddenData.h"

#include "svtkAnnotation.h"
#include "svtkAnnotationLayers.h"
#include "svtkCellData.h"
#include "svtkConvertSelection.h"
#include "svtkDataSet.h"
#include "svtkDoubleArray.h"
#include "svtkExtractSelectedGraph.h"
#include "svtkExtractSelectedRows.h"
#include "svtkGraph.h"
#include "svtkIdTypeArray.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkScalarsToColors.h"
#include "svtkSelection.h"
#include "svtkSelectionNode.h"
#include "svtkSmartPointer.h"
#include "svtkTable.h"
#include "svtkUnsignedCharArray.h"

svtkStandardNewMacro(svtkRemoveHiddenData);

svtkRemoveHiddenData::svtkRemoveHiddenData()
{
  this->ExtractGraph = svtkSmartPointer<svtkExtractSelectedGraph>::New();
  this->ExtractGraph->SetRemoveIsolatedVertices(false);

  this->ExtractTable = svtkSmartPointer<svtkExtractSelectedRows>::New();

  this->SetNumberOfInputPorts(2);
}

svtkRemoveHiddenData::~svtkRemoveHiddenData() = default;

int svtkRemoveHiddenData::FillInputPortInformation(int port, svtkInformation* info)
{
  if (port == 0)
  {
    info->Remove(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE());
    info->Append(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkGraph");
    info->Append(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkTable");
  }
  else if (port == 1)
  {
    info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkAnnotationLayers");
    info->Set(svtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
  }
  return 1;
}

int svtkRemoveHiddenData::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* annotationsInfo = inputVector[1]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  svtkDataObject* input = inInfo->Get(svtkDataObject::DATA_OBJECT());
  svtkDataObject* output = outInfo->Get(svtkDataObject::DATA_OBJECT());

  svtkAnnotationLayers* annotations = nullptr;
  if (annotationsInfo)
  {
    annotations =
      svtkAnnotationLayers::SafeDownCast(annotationsInfo->Get(svtkDataObject::DATA_OBJECT()));
  }

  // Nothing to do if no input annotations
  if (!annotations)
  {
    output->ShallowCopy(input);
    return 1;
  }

  svtkGraph* graph = svtkGraph::SafeDownCast(output);
  svtkTable* table = svtkTable::SafeDownCast(output);

  svtkSmartPointer<svtkSelection> selection = svtkSmartPointer<svtkSelection>::New();
  unsigned int numAnnotations = annotations->GetNumberOfAnnotations();
  int numHiddenAnnotations = 0;
  for (unsigned int a = 0; a < numAnnotations; ++a)
  {
    svtkAnnotation* ann = annotations->GetAnnotation(a);

    // Only if the annotation is both enabled AND hidden will
    // its selection get added
    if (ann->GetInformation()->Has(svtkAnnotation::ENABLE()) &&
      ann->GetInformation()->Get(svtkAnnotation::ENABLE()) == 1 &&
      ann->GetInformation()->Has(svtkAnnotation::HIDE()) &&
      ann->GetInformation()->Get(svtkAnnotation::HIDE()) == 1)
    {
      selection->Union(ann->GetSelection());
      numHiddenAnnotations++;
    }
  }

  // Nothing to do if no hidden annotations
  if (numHiddenAnnotations == 0)
  {
    output->ShallowCopy(input);
    return 1;
  }

  // We want to output the visible data, so the hidden annotation
  // selections need to be inverted before sent to the extraction filter:
  for (unsigned int i = 0; i < selection->GetNumberOfNodes(); ++i)
  {
    svtkSelectionNode* node = selection->GetNode(i);
    node->GetProperties()->Set(svtkSelectionNode::INVERSE(), 1);
  }

  if (graph)
  {
    this->ExtractGraph->SetInputData(input);
    this->ExtractGraph->SetInputData(1, selection);
    this->ExtractGraph->Update();
    output->ShallowCopy(this->ExtractGraph->GetOutput());
  }
  else if (table)
  {
    this->ExtractTable->SetInputData(input);
    this->ExtractTable->SetInputData(1, selection);
    this->ExtractTable->Update();
    output->ShallowCopy(this->ExtractTable->GetOutput());
  }
  else
  {
    svtkErrorMacro("Unsupported input data type.");
    return 0;
  }

  return 1;
}

void svtkRemoveHiddenData::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
