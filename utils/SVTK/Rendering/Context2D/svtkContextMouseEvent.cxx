/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkContextMouseEvent.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkContextMouseEvent.h"
#include "svtkRenderWindowInteractor.h" // AIX include order issues.

int svtkContextMouseEvent::GetModifiers() const
{
  int modifier = svtkContextMouseEvent::NO_MODIFIER;
  if (this->Interactor)
  {
    if (this->Interactor->GetAltKey() > 0)
    {
      modifier |= svtkContextMouseEvent::ALT_MODIFIER;
    }
    if (this->Interactor->GetShiftKey() > 0)
    {
      modifier |= svtkContextMouseEvent::SHIFT_MODIFIER;
    }
    if (this->Interactor->GetControlKey() > 0)
    {
      modifier |= svtkContextMouseEvent::CONTROL_MODIFIER;
    }
  }
  return modifier;
}
