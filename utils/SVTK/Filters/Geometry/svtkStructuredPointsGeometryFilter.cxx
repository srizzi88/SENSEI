/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkStructuredPointsGeometryFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkStructuredPointsGeometryFilter.h"
#include "svtkObjectFactory.h"

svtkStandardNewMacro(svtkStructuredPointsGeometryFilter);

svtkStructuredPointsGeometryFilter::svtkStructuredPointsGeometryFilter()
{
  svtkErrorMacro("svtkStructuredPointsGeometryFilter will be deprecated in"
    << endl
    << "the next release after SVTK 4.0. Please use" << endl
    << "svtkImageDataGeometryFilter instead");
}
