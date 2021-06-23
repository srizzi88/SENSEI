/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkRendererDelegate.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkRendererDelegate.h"

svtkRendererDelegate::svtkRendererDelegate()
{
  this->Used = false;
}

svtkRendererDelegate::~svtkRendererDelegate() = default;

void svtkRendererDelegate::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Used: ";
  if (this->Used)
  {
    os << "On";
  }
  else
  {
    os << "Off";
  }
  os << endl;
}
