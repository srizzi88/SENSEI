/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkLogoWidget.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkLogoWidget.h"
#include "svtkCallbackCommand.h"
#include "svtkLogoRepresentation.h"
#include "svtkObjectFactory.h"

svtkStandardNewMacro(svtkLogoWidget);

//-------------------------------------------------------------------------
svtkLogoWidget::svtkLogoWidget()
{
  this->Selectable = 0;
}

//-------------------------------------------------------------------------
svtkLogoWidget::~svtkLogoWidget() = default;

//----------------------------------------------------------------------
void svtkLogoWidget::CreateDefaultRepresentation()
{
  if (!this->WidgetRep)
  {
    this->WidgetRep = svtkLogoRepresentation::New();
  }
}

//-------------------------------------------------------------------------
void svtkLogoWidget::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
