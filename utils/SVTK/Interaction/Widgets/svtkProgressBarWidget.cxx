/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkProgressBarWidget.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

    This software is distributed WITHOUT ANY WARRANTY; without even
    the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
    PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkProgressBarWidget.h"
#include "svtkCallbackCommand.h"
#include "svtkObjectFactory.h"
#include "svtkProgressBarRepresentation.h"

svtkStandardNewMacro(svtkProgressBarWidget);

//-------------------------------------------------------------------------
svtkProgressBarWidget::svtkProgressBarWidget()
{
  this->Selectable = 0;
}

//-------------------------------------------------------------------------
svtkProgressBarWidget::~svtkProgressBarWidget() = default;

//----------------------------------------------------------------------
void svtkProgressBarWidget::CreateDefaultRepresentation()
{
  if (!this->WidgetRep)
  {
    this->WidgetRep = svtkProgressBarRepresentation::New();
  }
}

//-------------------------------------------------------------------------
void svtkProgressBarWidget::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
