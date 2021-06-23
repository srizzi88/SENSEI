/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPolyDataContourLineInterpolator.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkPolyDataContourLineInterpolator.h"

#include "svtkContourRepresentation.h"
#include "svtkMath.h"
#include "svtkObjectFactory.h"
#include "svtkPolyData.h"
#include "svtkPolyDataCollection.h"

//----------------------------------------------------------------------
svtkPolyDataContourLineInterpolator::svtkPolyDataContourLineInterpolator()
{
  this->Polys = svtkPolyDataCollection::New();
}

//----------------------------------------------------------------------
svtkPolyDataContourLineInterpolator::~svtkPolyDataContourLineInterpolator()
{
  this->Polys->Delete();
}

//----------------------------------------------------------------------
void svtkPolyDataContourLineInterpolator::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Polys: \n";
  this->Polys->PrintSelf(os, indent.GetNextIndent());
}
