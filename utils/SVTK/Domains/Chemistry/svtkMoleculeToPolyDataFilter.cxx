/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkMoleculeToPolyDataFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkMoleculeToPolyDataFilter.h"

#include "svtkInformation.h"
#include "svtkMolecule.h"

//----------------------------------------------------------------------------
svtkMoleculeToPolyDataFilter::svtkMoleculeToPolyDataFilter()
{
  this->SetNumberOfInputPorts(1);
}

//----------------------------------------------------------------------------
svtkMoleculeToPolyDataFilter::~svtkMoleculeToPolyDataFilter() = default;

//----------------------------------------------------------------------------
svtkMolecule* svtkMoleculeToPolyDataFilter::GetInput()
{
  return svtkMolecule::SafeDownCast(this->Superclass::GetInput(0));
}

//----------------------------------------------------------------------------
int svtkMoleculeToPolyDataFilter::FillInputPortInformation(int port, svtkInformation* info)
{
  if (!this->Superclass::FillInputPortInformation(port, info))
  {
    return 0;
  }
  info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkMolecule");
  return 1;
}

//----------------------------------------------------------------------------
void svtkMoleculeToPolyDataFilter::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
