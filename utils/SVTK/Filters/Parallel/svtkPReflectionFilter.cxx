/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPReflectionFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkPReflectionFilter.h"

#include "svtkBoundingBox.h"
#include "svtkMultiProcessController.h"
#include "svtkObjectFactory.h"

svtkStandardNewMacro(svtkPReflectionFilter);
svtkCxxSetObjectMacro(svtkPReflectionFilter, Controller, svtkMultiProcessController);
//----------------------------------------------------------------------------
svtkPReflectionFilter::svtkPReflectionFilter()
{
  this->Controller = nullptr;
  this->SetController(svtkMultiProcessController::GetGlobalController());
}

//----------------------------------------------------------------------------
svtkPReflectionFilter::~svtkPReflectionFilter()
{
  this->SetController(nullptr);
}

//----------------------------------------------------------------------------
int svtkPReflectionFilter::ComputeBounds(svtkDataObject* input, double bounds[6])
{
  svtkBoundingBox bbox;

  if (this->Superclass::ComputeBounds(input, bounds))
  {
    bbox.SetBounds(bounds);
  }

  if (this->Controller)
  {
    this->Controller->GetCommunicator()->ComputeGlobalBounds(
      this->Controller->GetLocalProcessId(), this->Controller->GetNumberOfProcesses(), &bbox);
    bbox.GetBounds(bounds);
  }

  return 1;
}

//----------------------------------------------------------------------------
void svtkPReflectionFilter::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Controller: " << this->Controller << endl;
}
