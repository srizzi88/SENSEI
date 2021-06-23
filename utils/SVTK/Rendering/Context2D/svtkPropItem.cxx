/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPropItem.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkPropItem.h"

#include "svtkContextScene.h"
#include "svtkObjectFactory.h"
#include "svtkProp.h"
#include "svtkProp3D.h"
#include "svtkRenderer.h"

svtkObjectFactoryNewMacro(svtkPropItem);
svtkCxxSetObjectMacro(svtkPropItem, PropObject, svtkProp);

//------------------------------------------------------------------------------
svtkPropItem::svtkPropItem()
  : PropObject(nullptr)
{
}

//------------------------------------------------------------------------------
svtkPropItem::~svtkPropItem()
{
  this->SetPropObject(nullptr);
}

//------------------------------------------------------------------------------
void svtkPropItem::UpdateTransforms()
{
  svtkErrorMacro(<< "Missing override in the rendering backend. Some items "
                   "may be rendered incorrectly.");
}

//------------------------------------------------------------------------------
void svtkPropItem::ResetTransforms()
{
  svtkErrorMacro(<< "Missing override in the rendering backend. Some items "
                   "may be rendered incorrectly.");
}

//------------------------------------------------------------------------------
bool svtkPropItem::Paint(svtkContext2D*)
{
  if (!this->PropObject)
  {
    return false;
  }

  this->UpdateTransforms();

  int result = this->PropObject->RenderOpaqueGeometry(this->Scene->GetRenderer());
  if (this->PropObject->HasTranslucentPolygonalGeometry())
  {
    result += this->PropObject->RenderTranslucentPolygonalGeometry(this->Scene->GetRenderer());
  }
  result += this->PropObject->RenderOverlay(this->Scene->GetRenderer());

  this->ResetTransforms();

  return result != 0;
}

//------------------------------------------------------------------------------
void svtkPropItem::ReleaseGraphicsResources()
{
  if (this->PropObject && this->Scene && this->Scene->GetRenderer())
  {
    this->PropObject->ReleaseGraphicsResources(this->Scene->GetRenderer()->GetSVTKWindow());
  }
}

//------------------------------------------------------------------------------
void svtkPropItem::PrintSelf(std::ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Prop:";
  if (this->PropObject)
  {
    os << "\n";
    this->PropObject->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << "(nullptr)\n";
  }
}
