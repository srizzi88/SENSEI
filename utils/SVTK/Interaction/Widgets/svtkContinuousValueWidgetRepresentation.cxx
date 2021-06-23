/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkContinuousValueWidgetRepresentation.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/

#include "svtkContinuousValueWidgetRepresentation.h"
#include "svtkCommand.h"
#include "svtkEvent.h"
#include "svtkInteractorObserver.h"
#include "svtkMath.h"
#include "svtkObjectFactory.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkWindow.h"

//----------------------------------------------------------------------
svtkContinuousValueWidgetRepresentation::svtkContinuousValueWidgetRepresentation()
{
  this->Value = 0;
}

//----------------------------------------------------------------------
svtkContinuousValueWidgetRepresentation::~svtkContinuousValueWidgetRepresentation() = default;

//----------------------------------------------------------------------
void svtkContinuousValueWidgetRepresentation::PlaceWidget(double* svtkNotUsed(bds[6]))
{
  // Position the handles at the end of the lines
  this->BuildRepresentation();
}

void svtkContinuousValueWidgetRepresentation::SetValue(double) {}

//----------------------------------------------------------------------
void svtkContinuousValueWidgetRepresentation::PrintSelf(ostream& os, svtkIndent indent)
{
  // Superclass typedef defined in svtkTypeMacro() found in svtkSetGet.h
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Value: " << this->GetValue() << "\n";
}
