/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkCastToConcrete.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkCastToConcrete.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkObjectFactory.h"
#include "svtkPolyData.h"
#include "svtkRectilinearGrid.h"
#include "svtkStreamingDemandDrivenPipeline.h"
#include "svtkStructuredGrid.h"
#include "svtkStructuredPoints.h"
#include "svtkUnstructuredGrid.h"

svtkStandardNewMacro(svtkCastToConcrete);

int svtkCastToConcrete::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  svtkDataSet* input = svtkDataSet::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));
  svtkDataSet* output = svtkDataSet::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  svtkDebugMacro(<< "Casting to concrete type...");

  output->ShallowCopy(input);

  return 1;
}

int svtkCastToConcrete::RequestInformation(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  outInfo->Set(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),
    inInfo->Get(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()), 6);

  return 1;
}

//----------------------------------------------------------------------------
void svtkCastToConcrete::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
