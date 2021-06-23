/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkTextWidget.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkTextWidget.h"
#include "svtkCommand.h"
#include "svtkObjectFactory.h"
#include "svtkTextRepresentation.h"

svtkStandardNewMacro(svtkTextWidget);

//-------------------------------------------------------------------------
svtkTextWidget::svtkTextWidget() = default;

//-------------------------------------------------------------------------
svtkTextWidget::~svtkTextWidget() = default;

//-------------------------------------------------------------------------
void svtkTextWidget::SetTextActor(svtkTextActor* textActor)
{
  svtkTextRepresentation* textRep = reinterpret_cast<svtkTextRepresentation*>(this->WidgetRep);
  if (!textRep)
  {
    this->CreateDefaultRepresentation();
    textRep = reinterpret_cast<svtkTextRepresentation*>(this->WidgetRep);
  }

  if (textRep->GetTextActor() != textActor)
  {
    textRep->SetTextActor(textActor);
    this->Modified();
  }
}

//-------------------------------------------------------------------------
svtkTextActor* svtkTextWidget::GetTextActor()
{
  svtkTextRepresentation* textRep = reinterpret_cast<svtkTextRepresentation*>(this->WidgetRep);
  if (!textRep)
  {
    return nullptr;
  }
  else
  {
    return textRep->GetTextActor();
  }
}

//----------------------------------------------------------------------
void svtkTextWidget::CreateDefaultRepresentation()
{
  if (!this->WidgetRep)
  {
    this->WidgetRep = svtkTextRepresentation::New();
  }
}

//-------------------------------------------------------------------------
void svtkTextWidget::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
