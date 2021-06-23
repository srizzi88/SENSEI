/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkReebGraphSimplificationFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkReebGraphSimplificationFilter.h"

#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkObjectFactory.h"
#include "svtkReebGraph.h"

svtkStandardNewMacro(svtkReebGraphSimplificationFilter);

//----------------------------------------------------------------------------
svtkReebGraphSimplificationFilter::svtkReebGraphSimplificationFilter()
{
  this->SetNumberOfInputPorts(1);
  this->SimplificationThreshold = 0;
  this->SimplificationMetric = nullptr;
}

//----------------------------------------------------------------------------
svtkReebGraphSimplificationFilter::~svtkReebGraphSimplificationFilter() {}

//----------------------------------------------------------------------------
void svtkReebGraphSimplificationFilter::SetSimplificationMetric(
  svtkReebGraphSimplificationMetric* simplificationMetric)
{
  if (simplificationMetric != this->SimplificationMetric)
  {
    this->SimplificationMetric = simplificationMetric;
    this->Modified();
  }
}

//----------------------------------------------------------------------------
int svtkReebGraphSimplificationFilter::FillInputPortInformation(int portNumber, svtkInformation* info)
{
  if (!portNumber)
  {
    info->Remove(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE());
    info->Append(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkReebGraph");
  }
  return 1;
}

//----------------------------------------------------------------------------
int svtkReebGraphSimplificationFilter::FillOutputPortInformation(int, svtkInformation* info)
{
  info->Set(svtkDirectedGraph::DATA_TYPE_NAME(), "svtkReebGraph");
  return 1;
}

//----------------------------------------------------------------------------
void svtkReebGraphSimplificationFilter::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Simplification Threshold: " << this->SimplificationThreshold << "\n";
}

//----------------------------------------------------------------------------
svtkReebGraph* svtkReebGraphSimplificationFilter::GetOutput()
{
  return svtkReebGraph::SafeDownCast(this->GetOutputDataObject(0));
}

//----------------------------------------------------------------------------
int svtkReebGraphSimplificationFilter::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{

  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);

  svtkReebGraph* input = svtkReebGraph::SafeDownCast(inInfo->Get(svtkReebGraph::DATA_OBJECT()));

  svtkInformation* outInfo = outputVector->GetInformationObject(0);
  svtkReebGraph* output = svtkReebGraph::SafeDownCast(outInfo->Get(svtkReebGraph::DATA_OBJECT()));

  output->DeepCopy(input);
  output->Simplify(this->SimplificationThreshold, this->SimplificationMetric);

  return 1;
}
