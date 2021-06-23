/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkInteractorStyleSwitchBase.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkInteractorStyleSwitchBase.h"

#include "svtkObjectFactory.h"

// This is largely here to confirm the approach works, and will be replaced
// with standard factory override logic in the modularized source tree.
//----------------------------------------------------------------------------
svtkObjectFactoryNewMacro(svtkInteractorStyleSwitchBase);

//----------------------------------------------------------------------------
svtkInteractorStyleSwitchBase::svtkInteractorStyleSwitchBase() = default;

//----------------------------------------------------------------------------
svtkInteractorStyleSwitchBase::~svtkInteractorStyleSwitchBase() = default;

//----------------------------------------------------------------------------
svtkRenderWindowInteractor* svtkInteractorStyleSwitchBase::GetInteractor()
{
  static bool warned = false;
  if (!warned && strcmp(this->GetClassName(), "svtkInteractorStyleSwitchBase") == 0)
  {
    svtkWarningMacro("Warning: Link to svtkInteractionStyle for default style selection.");
    warned = true;
  }
  return nullptr;
}

//----------------------------------------------------------------------------
void svtkInteractorStyleSwitchBase::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
