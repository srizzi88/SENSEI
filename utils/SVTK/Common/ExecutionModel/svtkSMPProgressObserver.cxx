/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkSMPProgressObserver.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkSMPProgressObserver.h"

#include "svtkCommand.h"
#include "svtkObjectFactory.h"

svtkStandardNewMacro(svtkSMPProgressObserver);

svtkSMPProgressObserver::svtkSMPProgressObserver() = default;

svtkSMPProgressObserver::~svtkSMPProgressObserver() = default;

void svtkSMPProgressObserver::UpdateProgress(double progress)
{
  svtkProgressObserver* observer = this->Observers.Local();
  observer->UpdateProgress(progress);
}

void svtkSMPProgressObserver::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
