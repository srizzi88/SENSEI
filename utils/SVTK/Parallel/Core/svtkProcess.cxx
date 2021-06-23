/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkProcess.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkProcess.h"
#include "svtkMultiProcessController.h"

// ----------------------------------------------------------------------------
svtkProcess::svtkProcess()
{
  this->Controller = nullptr;
  this->ReturnValue = 0;
}

// ----------------------------------------------------------------------------
svtkMultiProcessController* svtkProcess::GetController()
{
  return this->Controller;
}

// ----------------------------------------------------------------------------
void svtkProcess::SetController(svtkMultiProcessController* aController)
{
  this->Controller = aController;
}

// ----------------------------------------------------------------------------
int svtkProcess::GetReturnValue()
{
  return this->ReturnValue;
}

//----------------------------------------------------------------------------
void svtkProcess::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "ReturnValue: " << this->ReturnValue << endl;
  os << indent << "Controller: ";
  if (this->Controller)
  {
    os << endl;
    this->Controller->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << "(none)" << endl;
  }
}
