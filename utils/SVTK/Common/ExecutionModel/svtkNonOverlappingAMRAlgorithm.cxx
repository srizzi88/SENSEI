/*=========================================================================

 Program:   Visualization Toolkit
 Module:    svtkNonOverlappingAMRAlgorithm.cxx

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/
#include "svtkNonOverlappingAMRAlgorithm.h"
#include "svtkCompositeDataPipeline.h"
#include "svtkInformation.h"
#include "svtkNonOverlappingAMR.h"
#include "svtkObjectFactory.h"

svtkStandardNewMacro(svtkNonOverlappingAMRAlgorithm);

//------------------------------------------------------------------------------
svtkNonOverlappingAMRAlgorithm::svtkNonOverlappingAMRAlgorithm()
{
  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(1);
}

//------------------------------------------------------------------------------
svtkNonOverlappingAMRAlgorithm::~svtkNonOverlappingAMRAlgorithm() = default;

//------------------------------------------------------------------------------
void svtkNonOverlappingAMRAlgorithm::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
svtkNonOverlappingAMR* svtkNonOverlappingAMRAlgorithm::GetOutput()
{
  return (this->GetOutput(0));
}

//------------------------------------------------------------------------------
svtkNonOverlappingAMR* svtkNonOverlappingAMRAlgorithm::GetOutput(int port)
{
  svtkDataObject* output =
    svtkCompositeDataPipeline::SafeDownCast(this->GetExecutive())->GetCompositeOutputData(port);
  return (svtkNonOverlappingAMR::SafeDownCast(output));
}

//------------------------------------------------------------------------------
int svtkNonOverlappingAMRAlgorithm::FillOutputPortInformation(
  int svtkNotUsed(port), svtkInformation* info)
{
  info->Set(svtkDataObject::DATA_TYPE_NAME(), "svtkNonOverlappingAMR");
  return 1;
}

//------------------------------------------------------------------------------
int svtkNonOverlappingAMRAlgorithm::FillInputPortInformation(
  int svtkNotUsed(port), svtkInformation* info)
{
  info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkNonOverlappingAMR");
  return 1;
}
