/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkHardwareWindow.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkHardwareWindow.h"

#include "svtkObjectFactory.h"

//----------------------------------------------------------------------------
svtkObjectFactoryNewMacro(svtkHardwareWindow);

svtkHardwareWindow::svtkHardwareWindow()
{
  this->Borders = 1;
#ifdef SVTK_DEFAULT_RENDER_WINDOW_OFFSCREEN
  this->ShowWindow = false;
  this->UseOffScreenBuffers = true;
#endif
}

//----------------------------------------------------------------------------
svtkHardwareWindow::~svtkHardwareWindow() {}

//-----------------------------------------------------------------------------
void svtkHardwareWindow::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Borders: " << this->Borders << "\n";
}
