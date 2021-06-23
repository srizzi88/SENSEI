/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPolyDataToReebGraphFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkPolyDataToReebGraphFilter.h"

#include "svtkElevationFilter.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkPolyData.h"
#include "svtkReebGraph.h"

svtkStandardNewMacro(svtkPolyDataToReebGraphFilter);

//----------------------------------------------------------------------------
svtkPolyDataToReebGraphFilter::svtkPolyDataToReebGraphFilter()
{
  FieldId = 0;
  this->SetNumberOfInputPorts(1);
}

//----------------------------------------------------------------------------
svtkPolyDataToReebGraphFilter::~svtkPolyDataToReebGraphFilter() = default;

//----------------------------------------------------------------------------
int svtkPolyDataToReebGraphFilter::FillInputPortInformation(
  int svtkNotUsed(portNumber), svtkInformation* info)
{
  info->Remove(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE());
  info->Append(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkPolyData");
  return 1;
}

//----------------------------------------------------------------------------
int svtkPolyDataToReebGraphFilter::FillOutputPortInformation(int, svtkInformation* info)
{
  info->Set(svtkDirectedGraph::DATA_TYPE_NAME(), "svtkReebGraph");
  return 1;
}

//----------------------------------------------------------------------------
void svtkPolyDataToReebGraphFilter::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Field Id: " << this->FieldId << "\n";
}

//----------------------------------------------------------------------------
svtkReebGraph* svtkPolyDataToReebGraphFilter::GetOutput()
{
  return svtkReebGraph::SafeDownCast(this->GetOutputDataObject(0));
}

//----------------------------------------------------------------------------
int svtkPolyDataToReebGraphFilter::RequestData(
  svtkInformation*, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{

  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);

  svtkPolyData* input = svtkPolyData::SafeDownCast(inInfo->Get(svtkPolyData::DATA_OBJECT()));

  svtkInformation* outInfo = outputVector->GetInformationObject(0);
  svtkReebGraph* output = svtkReebGraph::SafeDownCast(outInfo->Get(svtkReebGraph::DATA_OBJECT()));

  // check for the presence of a scalar field
  svtkDataArray* scalarField = input->GetPointData()->GetArray(FieldId);
  if (!scalarField)
  {
    svtkElevationFilter* eFilter = svtkElevationFilter::New();
    eFilter->SetInputData(input);
    eFilter->Update();
    output->Build(svtkPolyData::SafeDownCast(eFilter->GetOutput()), "Elevation");
    eFilter->Delete();
  }
  else
  {
    output->Build(input, FieldId);
  }
  if (scalarField)
  {
  }
  else
  {
  }

  return 1;
}
