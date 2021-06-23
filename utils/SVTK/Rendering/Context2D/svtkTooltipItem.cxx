/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkTooltipItem.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkTooltipItem.h"

#include "svtkBrush.h"
#include "svtkContext2D.h"
#include "svtkContextScene.h"
#include "svtkPen.h"
#include "svtkTextProperty.h"
#include "svtkTransform2D.h"

#include "svtkNew.h"
#include "svtkStdString.h"
#include <sstream>

#include "svtkObjectFactory.h"

//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
svtkStandardNewMacro(svtkTooltipItem);

//-----------------------------------------------------------------------------
svtkTooltipItem::svtkTooltipItem()
  : PositionVector(0, 0)
{
  this->Position = this->PositionVector.GetData();
  this->TextProperties = svtkTextProperty::New();
  this->TextProperties->SetVerticalJustificationToBottom();
  this->TextProperties->SetJustificationToLeft();
  this->TextProperties->SetColor(0.0, 0.0, 0.0);
  this->Pen = svtkPen::New();
  this->Pen->SetColor(0, 0, 0);
  this->Pen->SetWidth(1.0);
  this->Brush = svtkBrush::New();
  this->Brush->SetColor(242, 242, 242);
}

//-----------------------------------------------------------------------------
svtkTooltipItem::~svtkTooltipItem()
{
  this->Pen->Delete();
  this->Brush->Delete();
  this->TextProperties->Delete();
}

//-----------------------------------------------------------------------------
void svtkTooltipItem::SetPosition(const svtkVector2f& pos)
{
  this->PositionVector = pos;
}

//-----------------------------------------------------------------------------
svtkVector2f svtkTooltipItem::GetPositionVector()
{
  return this->PositionVector;
}

//-----------------------------------------------------------------------------
void svtkTooltipItem::SetText(const svtkStdString& text)
{
  if (this->Text != text)
  {
    this->Text = text;
    this->Modified();
  }
}

//-----------------------------------------------------------------------------
svtkStdString svtkTooltipItem::GetText()
{
  return this->Text;
}

//-----------------------------------------------------------------------------
void svtkTooltipItem::Update() {}

//-----------------------------------------------------------------------------
bool svtkTooltipItem::Paint(svtkContext2D* painter)
{
  // This is where everything should be drawn, or dispatched to other methods.
  svtkDebugMacro(<< "Paint event called in svtkTooltipItem.");

  if (!this->Visible || !this->Text)
  {
    return false;
  }

  // save painter settings
  svtkNew<svtkPen> previousPen;
  previousPen->DeepCopy(painter->GetPen());
  svtkNew<svtkBrush> previousBrush;
  previousBrush->DeepCopy(painter->GetBrush());
  svtkNew<svtkTextProperty> previousTextProp;
  previousTextProp->ShallowCopy(painter->GetTextProp());

  painter->ApplyPen(this->Pen);
  painter->ApplyBrush(this->Brush);
  painter->ApplyTextProp(this->TextProperties);

  // Compute the bounds, then make a few adjustments to the size we will use
  svtkVector2f bounds[2];
  painter->ComputeStringBounds(this->Text, bounds[0].GetData());
  if (bounds[1].GetX() == 0.0f && bounds[1].GetY() == 0.0f)
  {
    // This signals only non-renderable characters, so return
    return false;
  }
  float scale[2];
  float position[2];
  painter->GetTransform()->GetScale(scale);
  painter->GetTransform()->GetPosition(position);
  bounds[0] = svtkVector2f(
    this->PositionVector.GetX() - 5 / scale[0], this->PositionVector.GetY() - 3 / scale[1]);
  bounds[1].Set(bounds[1].GetX() + 10 / scale[0], bounds[1].GetY() + 10 / scale[1]);
  // Pull the tooltip back in if it will go off the edge of the screen.
  float maxX = (this->Scene->GetViewWidth() - position[0]) / scale[0];
  if (bounds[0].GetX() >= maxX - bounds[1].GetX())
  {
    bounds[0].SetX(maxX - bounds[1].GetX());
  }
  float maxY = (this->Scene->GetViewHeight() - position[1]) / scale[1];
  if (bounds[0].GetY() >= maxY - bounds[1].GetY())
  {
    bounds[0].SetY(maxY - bounds[1].GetY());
  }

  // Draw a rectangle as background, and then center our text in there
  painter->DrawRect(bounds[0].GetX(), bounds[0].GetY(), bounds[1].GetX(), bounds[1].GetY());
  painter->DrawString(bounds[0].GetX() + 5 / scale[0], bounds[0].GetY() + 3 / scale[1], this->Text);

  // restore painter settings
  painter->ApplyPen(previousPen);
  painter->ApplyBrush(previousBrush);
  painter->ApplyTextProp(previousTextProp);

  return true;
}

//-----------------------------------------------------------------------------
void svtkTooltipItem::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
