/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkAffineRepresentation.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkAffineRepresentation.h"
#include "svtkObjectFactory.h"
#include "svtkTransform.h"

//----------------------------------------------------------------------
svtkAffineRepresentation::svtkAffineRepresentation()
{
  this->InteractionState = svtkAffineRepresentation::Outside;
  this->Tolerance = 15;
  this->Transform = svtkTransform::New();
}

//----------------------------------------------------------------------
svtkAffineRepresentation::~svtkAffineRepresentation()
{
  this->Transform->Delete();
}

//----------------------------------------------------------------------
void svtkAffineRepresentation::ShallowCopy(svtkProp* prop)
{
  svtkAffineRepresentation* rep = svtkAffineRepresentation::SafeDownCast(prop);
  if (rep)
  {
    this->SetTolerance(rep->GetTolerance());
  }
  this->Superclass::ShallowCopy(prop);
}

//----------------------------------------------------------------------
void svtkAffineRepresentation::PrintSelf(ostream& os, svtkIndent indent)
{
  // Superclass typedef defined in svtkTypeMacro() found in svtkSetGet.h
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Tolerance: " << this->Tolerance << "\n";
}
