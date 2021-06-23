/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkUnstructuredGridVolumeMapper.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkUnstructuredGridVolumeMapper.h"

#include "svtkDataSet.h"
#include "svtkExecutive.h"
#include "svtkInformation.h"
#include "svtkUnstructuredGrid.h"

// Construct a svtkUnstructuredGridVolumeMapper with empty scalar input and
// clipping off.
svtkUnstructuredGridVolumeMapper::svtkUnstructuredGridVolumeMapper()
{
  this->BlendMode = svtkUnstructuredGridVolumeMapper::COMPOSITE_BLEND;
}

svtkUnstructuredGridVolumeMapper::~svtkUnstructuredGridVolumeMapper() = default;

void svtkUnstructuredGridVolumeMapper::SetInputData(svtkDataSet* genericInput)
{
  svtkUnstructuredGridBase* input = svtkUnstructuredGridBase::SafeDownCast(genericInput);

  if (input)
  {
    this->SetInputData(input);
  }
  else
  {
    svtkErrorMacro("The SetInput method of this mapper requires "
                  "svtkUnstructuredGridBase as input");
  }
}

void svtkUnstructuredGridVolumeMapper::SetInputData(svtkUnstructuredGridBase* input)
{
  this->SetInputDataInternal(0, input);
}

svtkUnstructuredGridBase* svtkUnstructuredGridVolumeMapper::GetInput()
{
  if (this->GetNumberOfInputConnections(0) < 1)
  {
    return nullptr;
  }
  return svtkUnstructuredGridBase::SafeDownCast(this->GetExecutive()->GetInputData(0, 0));
}

// Print the svtkUnstructuredGridVolumeMapper
void svtkUnstructuredGridVolumeMapper::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Blend Mode: " << this->BlendMode << endl;
}

//----------------------------------------------------------------------------
int svtkUnstructuredGridVolumeMapper::FillInputPortInformation(
  int svtkNotUsed(port), svtkInformation* info)
{
  info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkUnstructuredGridBase");
  return 1;
}
