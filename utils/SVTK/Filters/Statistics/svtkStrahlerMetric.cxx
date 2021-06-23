/*=========================================================================

Program:   Visualization Toolkit
Module:    svtkStrahlerMetric.cxx

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

#include "svtkStrahlerMetric.h"

#include "svtkDataSetAttributes.h"
#include "svtkFloatArray.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkObjectFactory.h"
#include "svtkOutEdgeIterator.h"
#include "svtkSmartPointer.h"

//--------------------------------------------------------------------------

svtkStandardNewMacro(svtkStrahlerMetric);

svtkStrahlerMetric::svtkStrahlerMetric()
{
  this->MaxStrahler = 0;
  this->Normalize = 0;
  this->MetricArrayName = nullptr;
  this->SetMetricArrayName("Strahler");
}

svtkStrahlerMetric::~svtkStrahlerMetric()
{
  this->SetMetricArrayName(nullptr);
}

//----------------------------------------------------------------------------

float svtkStrahlerMetric::CalculateStrahler(svtkIdType root, svtkFloatArray* metric, svtkTree* tree)
{
  float strahler, maxStrahler;
  bool same;
  svtkSmartPointer<svtkOutEdgeIterator> children = svtkSmartPointer<svtkOutEdgeIterator>::New();

  svtkIdType nrChildren = tree->GetNumberOfChildren(root);

  std::vector<float> childStrahler(nrChildren);

  // A leaf node has a Strahler value of 1.
  if (nrChildren == 0)
  {
    strahler = 1.0;
  }
  else
  {
    // Non-leaf node: find the Strahler values of the children.
    tree->GetOutEdges(root, children);
    for (svtkIdType i = 0; i < nrChildren; i++)
    {
      childStrahler[i] = this->CalculateStrahler(children->Next().Target, metric, tree);
    }
    // Determine if the children have the same strahler values
    same = true;
    maxStrahler = childStrahler[0];
    for (svtkIdType j = 1; j < nrChildren; j++)
    {
      same = same && (maxStrahler == childStrahler[j]);
      if (maxStrahler < childStrahler[j])
      {
        maxStrahler = childStrahler[j];
      }
    }
    // Calculate the strahler value for this node
    strahler = (same) ? maxStrahler + nrChildren - 1 : maxStrahler + nrChildren - 2;
  }
  // Record the strahler value within the array.
  metric->InsertValue(root, strahler);
  if (strahler > this->MaxStrahler)
  {
    this->MaxStrahler = strahler;
  }
  return strahler;
}

int svtkStrahlerMetric::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  svtkDebugMacro(<< "StrahlerMetric executing.");

  // get the input and output
  svtkTree* input = svtkTree::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));
  svtkTree* output = svtkTree::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  // The output model should share the data of the input.
  output->ShallowCopy(input);

  // Create a new array to hold the metric.
  svtkFloatArray* metric = svtkFloatArray::New();
  metric->SetName(this->MetricArrayName);
  metric->SetNumberOfValues(input->GetNumberOfVertices());

  this->MaxStrahler = 1.0;

  this->CalculateStrahler(input->GetRoot(), metric, input);

  if (this->Normalize)
  {
    for (svtkIdType i = 0; i < input->GetNumberOfVertices(); i++)
    {
      metric->SetValue(i, metric->GetValue(i) / this->MaxStrahler);
    }
  }

  output->GetVertexData()->AddArray(metric);
  metric->Delete();

  svtkDebugMacro(<< "StrahlerMetric done.");
  return 1;
}

void svtkStrahlerMetric::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Normalize: " << this->Normalize << endl;
  os << indent << "MaxStrahler: " << this->MaxStrahler << endl;
  os << indent << "MetricArrayName: " << (this->MetricArrayName ? this->MetricArrayName : "(none)")
     << endl;
}
