/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkLabelRenderStrategy.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkLabelRenderStrategy.h"

#include "svtkRenderer.h"
#include "svtkTextProperty.h"

svtkCxxSetObjectMacro(svtkLabelRenderStrategy, Renderer, svtkRenderer);
svtkCxxSetObjectMacro(svtkLabelRenderStrategy, DefaultTextProperty, svtkTextProperty);

//----------------------------------------------------------------------------
svtkLabelRenderStrategy::svtkLabelRenderStrategy()
{
  this->Renderer = nullptr;
  this->DefaultTextProperty = svtkTextProperty::New();
}

//----------------------------------------------------------------------------
svtkLabelRenderStrategy::~svtkLabelRenderStrategy()
{
  this->SetRenderer(nullptr);
  this->SetDefaultTextProperty(nullptr);
}

//----------------------------------------------------------------------------
void svtkLabelRenderStrategy::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Renderer: " << this->Renderer << endl;
  os << indent << "DefaultTextProperty: " << this->DefaultTextProperty << endl;
}
