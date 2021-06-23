/*=========================================================================

Program:   Visualization Toolkit
Module:    svtkOpenVRPropPicker.cxx

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkOpenVRPropPicker.h"

#include "svtkAssemblyNode.h"
#include "svtkAssemblyPath.h"
#include "svtkBox.h"
#include "svtkCamera.h"
#include "svtkCommand.h"
#include "svtkMath.h"
#include "svtkObjectFactory.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor3D.h"
#include "svtkRenderer.h"
#include "svtkTransform.h"

svtkStandardNewMacro(svtkOpenVRPropPicker);

svtkOpenVRPropPicker::svtkOpenVRPropPicker() {}

svtkOpenVRPropPicker::~svtkOpenVRPropPicker() {}

// set up for a pick
void svtkOpenVRPropPicker::Initialize()
{
#ifndef SVTK_LEGACY_SILENT
  svtkErrorMacro(
    "This class is deprecated: Please use svtkPropPicker directly instead of this class");
#endif
  this->svtkAbstractPropPicker::Initialize();
}

void svtkOpenVRPropPicker::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
