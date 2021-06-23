/*=========================================================================

 Program:   Visualization Toolkit
 Module:    svtkOverlappingAMRAlgorithm.cxx

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/
#include "svtkOverlappingAMRAlgorithm.h"
#include "svtkCompositeDataPipeline.h"
#include "svtkInformation.h"
#include "svtkObjectFactory.h"
#include "svtkOverlappingAMR.h"

svtkStandardNewMacro(svtkOverlappingAMRAlgorithm);

//------------------------------------------------------------------------------
svtkOverlappingAMRAlgorithm::svtkOverlappingAMRAlgorithm()
{
  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(1);
}

//------------------------------------------------------------------------------
svtkOverlappingAMRAlgorithm::~svtkOverlappingAMRAlgorithm() = default;

//------------------------------------------------------------------------------
void svtkOverlappingAMRAlgorithm::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
svtkOverlappingAMR* svtkOverlappingAMRAlgorithm::GetOutput()
{
  return this->GetOutput(0);
}

//------------------------------------------------------------------------------
svtkOverlappingAMR* svtkOverlappingAMRAlgorithm::GetOutput(int port)
{
  svtkDataObject* output =
    svtkCompositeDataPipeline::SafeDownCast(this->GetExecutive())->GetCompositeOutputData(port);
  return (svtkOverlappingAMR::SafeDownCast(output));
}

//------------------------------------------------------------------------------
int svtkOverlappingAMRAlgorithm::FillOutputPortInformation(
  int svtkNotUsed(port), svtkInformation* info)
{
  info->Set(svtkDataObject::DATA_TYPE_NAME(), "svtkOverlappingAMR");
  return 1;
}

//------------------------------------------------------------------------------
int svtkOverlappingAMRAlgorithm::FillInputPortInformation(int svtkNotUsed(port), svtkInformation* info)
{
  info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkOverlappingAMR");
  return 1;
}
