/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkBlockItem.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkBlockItem.h"

// Get my new commands
#include "svtkCommand.h"

#include "svtkBrush.h"
#include "svtkContext2D.h"
#include "svtkContextMouseEvent.h"
#include "svtkContextScene.h"
#include "svtkPen.h"
#include "svtkStdString.h"
#include "svtkTextProperty.h"
#include "svtkVectorOperators.h"

#include "svtkObjectFactory.h"

//-----------------------------------------------------------------------------
svtkStandardNewMacro(svtkBlockItem);

//-----------------------------------------------------------------------------
svtkBlockItem::svtkBlockItem()
{
  this->MouseOver = false;
  this->scalarFunction = nullptr;
  this->Dimensions[0] = 0;
  this->Dimensions[1] = 0;
  this->Dimensions[2] = 0;
  this->Dimensions[3] = 0;
}

//-----------------------------------------------------------------------------
svtkBlockItem::~svtkBlockItem() = default;

//-----------------------------------------------------------------------------
bool svtkBlockItem::Paint(svtkContext2D* painter)
{
  painter->GetTextProp()->SetVerticalJustificationToCentered();
  painter->GetTextProp()->SetJustificationToCentered();
  painter->GetTextProp()->SetColor(0.0, 0.0, 0.0);
  painter->GetTextProp()->SetFontSize(24);
  painter->GetPen()->SetColor(0, 0, 0);

  if (this->MouseOver)
  {
    painter->GetBrush()->SetColor(255, 0, 0);
  }
  else
  {
    painter->GetBrush()->SetColor(0, 255, 0);
  }
  painter->DrawRect(
    this->Dimensions[0], this->Dimensions[1], this->Dimensions[2], this->Dimensions[3]);

  float x = this->Dimensions[0] + 0.5 * this->Dimensions[2];
  float y = this->Dimensions[1] + 0.5 * this->Dimensions[3];
  if (this->Label)
  {
    painter->DrawString(x, y, this->Label);
  }

  if (this->scalarFunction)
  {
    // We have a function pointer - do something...
    ;
  }

  this->PaintChildren(painter);
  return true;
}

//-----------------------------------------------------------------------------
bool svtkBlockItem::Hit(const svtkContextMouseEvent& mouse)
{
  svtkVector2f pos = mouse.GetPos();
  if (pos[0] > this->Dimensions[0] && pos[0] < this->Dimensions[0] + this->Dimensions[2] &&
    pos[1] > this->Dimensions[1] && pos[1] < this->Dimensions[1] + this->Dimensions[3])
  {
    return true;
  }
  else
  {
    return this->svtkAbstractContextItem::Hit(mouse);
  }
}

//-----------------------------------------------------------------------------
bool svtkBlockItem::MouseEnterEvent(const svtkContextMouseEvent&)
{
  this->MouseOver = true;
  this->GetScene()->SetDirty(true);
  return true;
}

//-----------------------------------------------------------------------------
bool svtkBlockItem::MouseMoveEvent(const svtkContextMouseEvent& mouse)
{
  svtkVector2f delta = mouse.GetPos() - mouse.GetLastPos();

  if (mouse.GetButton() == svtkContextMouseEvent::LEFT_BUTTON)
  {
    // Move the block by this amount
    this->Dimensions[0] += delta.GetX();
    this->Dimensions[1] += delta.GetY();

    this->GetScene()->SetDirty(true);
    return true;
  }
  else if (mouse.GetButton() == mouse.MIDDLE_BUTTON)
  {
    // Resize the block by this amount
    this->Dimensions[0] += delta.GetX();
    this->Dimensions[1] += delta.GetY();
    this->Dimensions[2] -= delta.GetX();
    this->Dimensions[3] -= delta.GetY();

    this->GetScene()->SetDirty(true);
    return true;
  }
  else if (mouse.GetButton() == mouse.RIGHT_BUTTON)
  {
    // Resize the block by this amount
    this->Dimensions[2] += delta.GetX();
    this->Dimensions[3] += delta.GetY();

    this->GetScene()->SetDirty(true);
    return true;
  }
  return false;
}

//-----------------------------------------------------------------------------
bool svtkBlockItem::MouseLeaveEvent(const svtkContextMouseEvent&)
{
  this->MouseOver = false;
  this->GetScene()->SetDirty(true);
  return true;
}

//-----------------------------------------------------------------------------
bool svtkBlockItem::MouseButtonPressEvent(const svtkContextMouseEvent&)
{
  return true;
}

//-----------------------------------------------------------------------------
bool svtkBlockItem::MouseButtonReleaseEvent(const svtkContextMouseEvent&)
{
  return true;
}

//-----------------------------------------------------------------------------
void svtkBlockItem::SetLabel(const svtkStdString& label)
{
  if (this->Label != label)
  {
    this->Label = label;
    this->Modified();
  }
}

//-----------------------------------------------------------------------------
svtkStdString svtkBlockItem::GetLabel()
{
  return this->Label;
}

//-----------------------------------------------------------------------------
void svtkBlockItem::SetScalarFunctor(double (*ScalarFunction)(double, double))
{
  this->scalarFunction = ScalarFunction;
}

//-----------------------------------------------------------------------------
void svtkBlockItem::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
