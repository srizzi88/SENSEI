/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkLinearContourLineInterpolator.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkLinearContourLineInterpolator.h"
#include "svtkObjectFactory.h"

svtkStandardNewMacro(svtkLinearContourLineInterpolator);

//----------------------------------------------------------------------
svtkLinearContourLineInterpolator::svtkLinearContourLineInterpolator() = default;

//----------------------------------------------------------------------
svtkLinearContourLineInterpolator::~svtkLinearContourLineInterpolator() = default;

//----------------------------------------------------------------------
int svtkLinearContourLineInterpolator::InterpolateLine(svtkRenderer* svtkNotUsed(ren),
  svtkContourRepresentation* svtkNotUsed(rep), int svtkNotUsed(idx1), int svtkNotUsed(idx2))
{
  return 1;
}

//----------------------------------------------------------------------
void svtkLinearContourLineInterpolator::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
