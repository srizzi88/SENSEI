/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkFixedPointVolumeRayCastHelper.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkFixedPointVolumeRayCastHelper.h"
#include "svtkObjectFactory.h"

#include <cmath>

svtkStandardNewMacro(svtkFixedPointVolumeRayCastHelper);

svtkFixedPointVolumeRayCastHelper::svtkFixedPointVolumeRayCastHelper() = default;

svtkFixedPointVolumeRayCastHelper::~svtkFixedPointVolumeRayCastHelper() = default;

void svtkFixedPointVolumeRayCastHelper::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
