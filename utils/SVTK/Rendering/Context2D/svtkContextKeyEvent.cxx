/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkContextScene.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkContextKeyEvent.h"

#include "svtkRenderWindowInteractor.h"

#include <cassert>

svtkContextKeyEvent::svtkContextKeyEvent() {}

void svtkContextKeyEvent::SetInteractor(svtkRenderWindowInteractor* interactor)
{
  this->Interactor = interactor;
}

svtkRenderWindowInteractor* svtkContextKeyEvent::GetInteractor() const
{
  return this->Interactor;
}

char svtkContextKeyEvent::GetKeyCode() const
{
  if (this->Interactor)
  {
    return this->Interactor->GetKeyCode();
  }
  else
  {
    // This should never happen, perhaps there is a better return value?
    return 0;
  }
}
