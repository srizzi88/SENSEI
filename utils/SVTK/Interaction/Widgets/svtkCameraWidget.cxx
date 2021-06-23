/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkCameraWidget.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkCameraWidget.h"
#include "svtkCallbackCommand.h"
#include "svtkCameraInterpolator.h"
#include "svtkCameraRepresentation.h"
#include "svtkObjectFactory.h"

svtkStandardNewMacro(svtkCameraWidget);

//-------------------------------------------------------------------------
svtkCameraWidget::svtkCameraWidget() = default;

//-------------------------------------------------------------------------
svtkCameraWidget::~svtkCameraWidget() = default;

//----------------------------------------------------------------------
void svtkCameraWidget::CreateDefaultRepresentation()
{
  if (!this->WidgetRep)
  {
    this->WidgetRep = svtkCameraRepresentation::New();
  }
}

//-------------------------------------------------------------------------
void svtkCameraWidget::SelectRegion(double eventPos[2])
{
  if (!this->WidgetRep)
  {
    return;
  }

  double x = eventPos[0];
  if (x < 0.3333)
  {
    reinterpret_cast<svtkCameraRepresentation*>(this->WidgetRep)->AddCameraToPath();
  }
  else if (x < 0.666667)
  {
    reinterpret_cast<svtkCameraRepresentation*>(this->WidgetRep)->AnimatePath(this->Interactor);
  }
  else if (x < 1.0)
  {
    reinterpret_cast<svtkCameraRepresentation*>(this->WidgetRep)->InitializePath();
  }

  this->Superclass::SelectRegion(eventPos);
}

//-------------------------------------------------------------------------
void svtkCameraWidget::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
