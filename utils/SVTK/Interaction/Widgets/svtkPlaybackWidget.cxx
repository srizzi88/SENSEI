/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPlaybackWidget.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkPlaybackWidget.h"
#include "svtkCallbackCommand.h"
#include "svtkObjectFactory.h"
#include "svtkPlaybackRepresentation.h"

svtkStandardNewMacro(svtkPlaybackWidget);

//-------------------------------------------------------------------------
svtkPlaybackWidget::svtkPlaybackWidget() = default;

//-------------------------------------------------------------------------
svtkPlaybackWidget::~svtkPlaybackWidget() = default;

//----------------------------------------------------------------------
void svtkPlaybackWidget::CreateDefaultRepresentation()
{
  if (!this->WidgetRep)
  {
    this->WidgetRep = svtkPlaybackRepresentation::New();
  }
}

//-------------------------------------------------------------------------
void svtkPlaybackWidget::SelectRegion(double eventPos[2])
{
  if (!this->WidgetRep)
  {
    return;
  }

  double x = eventPos[0];
  if (x < 0.16667)
  {
    reinterpret_cast<svtkPlaybackRepresentation*>(this->WidgetRep)->JumpToBeginning();
  }
  else if (x <= 0.333333)
  {
    reinterpret_cast<svtkPlaybackRepresentation*>(this->WidgetRep)->BackwardOneFrame();
  }
  else if (x <= 0.500000)
  {
    reinterpret_cast<svtkPlaybackRepresentation*>(this->WidgetRep)->Stop();
  }
  else if (x < 0.666667)
  {
    reinterpret_cast<svtkPlaybackRepresentation*>(this->WidgetRep)->Play();
  }
  else if (x <= 0.833333)
  {
    reinterpret_cast<svtkPlaybackRepresentation*>(this->WidgetRep)->ForwardOneFrame();
  }
  else if (x <= 1.00000)
  {
    reinterpret_cast<svtkPlaybackRepresentation*>(this->WidgetRep)->JumpToEnd();
  }
}

//-------------------------------------------------------------------------
void svtkPlaybackWidget::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
