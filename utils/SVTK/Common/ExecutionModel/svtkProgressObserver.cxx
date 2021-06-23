/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkProgressObserver.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkProgressObserver.h"

#include "svtkCommand.h"
#include "svtkObjectFactory.h"

svtkStandardNewMacro(svtkProgressObserver);

svtkProgressObserver::svtkProgressObserver() = default;

svtkProgressObserver::~svtkProgressObserver() = default;

void svtkProgressObserver::UpdateProgress(double amount)
{
  this->Progress = amount;
  this->InvokeEvent(svtkCommand::ProgressEvent, static_cast<void*>(&amount));
}

void svtkProgressObserver::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
