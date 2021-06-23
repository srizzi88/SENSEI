/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkContextTransform.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkContextTransform.h"

#include "svtkCommand.h"
#include "svtkContext2D.h"
#include "svtkContextMouseEvent.h"
#include "svtkContextScenePrivate.h"
#include "svtkObjectFactory.h"
#include "svtkTransform2D.h"
#include "svtkVector.h"
#include "svtkVectorOperators.h"

svtkStandardNewMacro(svtkContextTransform);

//-----------------------------------------------------------------------------
svtkContextTransform::svtkContextTransform()
  : ZoomAnchor(0.0f, 0.0f)
{
  this->Transform = svtkSmartPointer<svtkTransform2D>::New();
  this->PanMouseButton = svtkContextMouseEvent::LEFT_BUTTON;
  this->PanModifier = svtkContextMouseEvent::NO_MODIFIER;
  this->ZoomMouseButton = svtkContextMouseEvent::RIGHT_BUTTON;
  this->ZoomModifier = svtkContextMouseEvent::NO_MODIFIER;
  this->SecondaryPanMouseButton = svtkContextMouseEvent::NO_BUTTON;
  this->SecondaryPanModifier = svtkContextMouseEvent::NO_MODIFIER;
  this->SecondaryZoomMouseButton = svtkContextMouseEvent::LEFT_BUTTON;
  this->SecondaryZoomModifier = svtkContextMouseEvent::SHIFT_MODIFIER;

  this->ZoomOnMouseWheel = true;
  this->PanYOnMouseWheel = false;

  this->Interactive = false;
}

//-----------------------------------------------------------------------------
svtkContextTransform::~svtkContextTransform() = default;

//-----------------------------------------------------------------------------
bool svtkContextTransform::Paint(svtkContext2D* painter)
{
  painter->PushMatrix();
  painter->AppendTransform(this->Transform);
  bool result = this->PaintChildren(painter);
  painter->PopMatrix();
  return result;
}

//-----------------------------------------------------------------------------
void svtkContextTransform::Identity()
{
  this->Transform->Identity();
}

//-----------------------------------------------------------------------------
void svtkContextTransform::Update() {}

//-----------------------------------------------------------------------------
void svtkContextTransform::Translate(float dx, float dy)
{
  float d[] = { dx, dy };
  this->Transform->Translate(d);
}

//-----------------------------------------------------------------------------
void svtkContextTransform::Scale(float dx, float dy)
{
  float d[] = { dx, dy };
  this->Transform->Scale(d);
}

//-----------------------------------------------------------------------------
void svtkContextTransform::Rotate(float angle)
{
  this->Transform->Rotate(double(angle));
}

//-----------------------------------------------------------------------------
svtkTransform2D* svtkContextTransform::GetTransform()
{
  return this->Transform;
}

//-----------------------------------------------------------------------------
svtkVector2f svtkContextTransform::MapToParent(const svtkVector2f& point)
{
  svtkVector2f p;
  this->Transform->TransformPoints(point.GetData(), p.GetData(), 1);
  return p;
}

//-----------------------------------------------------------------------------
svtkVector2f svtkContextTransform::MapFromParent(const svtkVector2f& point)
{
  svtkVector2f p;
  this->Transform->InverseTransformPoints(point.GetData(), p.GetData(), 1);
  return p;
}

//-----------------------------------------------------------------------------
bool svtkContextTransform::Hit(const svtkContextMouseEvent& svtkNotUsed(mouse))
{
  // If we are interactive, we want to catch anything that propagates to the
  // background, otherwise we do not want any mouse events.
  return this->Interactive;
}

//-----------------------------------------------------------------------------
bool svtkContextTransform::MouseButtonPressEvent(const svtkContextMouseEvent& mouse)
{
  if (!this->Interactive)
  {
    return svtkAbstractContextItem::MouseButtonPressEvent(mouse);
  }
  if ((this->ZoomMouseButton != svtkContextMouseEvent::NO_BUTTON &&
        mouse.GetButton() == this->ZoomMouseButton && mouse.GetModifiers() == this->ZoomModifier) ||
    (this->SecondaryZoomMouseButton != svtkContextMouseEvent::NO_BUTTON &&
      mouse.GetButton() == this->SecondaryZoomMouseButton &&
      mouse.GetModifiers() == this->SecondaryZoomModifier))
  {
    // Determine anchor to zoom in on
    svtkVector2d screenPos(mouse.GetScreenPos().Cast<double>().GetData());
    svtkVector2d pos(0.0, 0.0);
    svtkTransform2D* transform = this->GetTransform();
    transform->InverseTransformPoints(screenPos.GetData(), pos.GetData(), 1);
    this->ZoomAnchor = svtkVector2f(pos.Cast<float>().GetData());
    return true;
  }
  return false;
}

//-----------------------------------------------------------------------------
bool svtkContextTransform::MouseMoveEvent(const svtkContextMouseEvent& mouse)
{
  if (!this->Interactive)
  {
    return svtkAbstractContextItem::MouseButtonPressEvent(mouse);
  }
  if ((this->PanMouseButton != svtkContextMouseEvent::NO_BUTTON &&
        mouse.GetButton() == this->PanMouseButton && mouse.GetModifiers() == this->PanModifier) ||
    (this->SecondaryPanMouseButton != svtkContextMouseEvent::NO_BUTTON &&
      mouse.GetButton() == this->SecondaryPanMouseButton &&
      mouse.GetModifiers() == this->SecondaryPanModifier))
  {
    // Figure out how much the mouse has moved by in plot coordinates - pan
    svtkVector2d screenPos(mouse.GetScreenPos().Cast<double>().GetData());
    svtkVector2d lastScreenPos(mouse.GetLastScreenPos().Cast<double>().GetData());
    svtkVector2d pos(0.0, 0.0);
    svtkVector2d last(0.0, 0.0);

    // Go from screen to scene coordinates to work out the delta
    svtkTransform2D* transform = this->GetTransform();
    transform->InverseTransformPoints(screenPos.GetData(), pos.GetData(), 1);
    transform->InverseTransformPoints(lastScreenPos.GetData(), last.GetData(), 1);
    svtkVector2f delta((last - pos).Cast<float>().GetData());
    this->Translate(-delta[0], -delta[1]);

    // Mark the scene as dirty
    this->Scene->SetDirty(true);

    this->InvokeEvent(svtkCommand::InteractionEvent);
    return true;
  }
  if ((this->ZoomMouseButton != svtkContextMouseEvent::NO_BUTTON &&
        mouse.GetButton() == this->ZoomMouseButton && mouse.GetModifiers() == this->ZoomModifier) ||
    (this->SecondaryZoomMouseButton != svtkContextMouseEvent::NO_BUTTON &&
      mouse.GetButton() == this->SecondaryZoomMouseButton &&
      mouse.GetModifiers() == this->SecondaryZoomModifier))
  {
    // Figure out how much the mouse has moved and scale accordingly
    float delta = 0.0f;
    if (this->Scene->GetSceneHeight() > 0)
    {
      delta = static_cast<float>(mouse.GetLastScreenPos()[1] - mouse.GetScreenPos()[1]) /
        this->Scene->GetSceneHeight();
    }

    // Dragging full screen height zooms 4x.
    float scaling = pow(4.0f, delta);

    // Zoom in on anchor position
    this->Translate(this->ZoomAnchor[0], this->ZoomAnchor[1]);
    this->Scale(scaling, scaling);
    this->Translate(-this->ZoomAnchor[0], -this->ZoomAnchor[1]);

    // Mark the scene as dirty
    this->Scene->SetDirty(true);

    this->InvokeEvent(svtkCommand::InteractionEvent);
    return true;
  }
  return false;
}

//-----------------------------------------------------------------------------
bool svtkContextTransform::MouseWheelEvent(const svtkContextMouseEvent& mouse, int delta)
{
  if (!this->Interactive)
  {
    return svtkAbstractContextItem::MouseButtonPressEvent(mouse);
  }
  if (this->ZoomOnMouseWheel)
  {
    // Determine current position to zoom in on
    svtkVector2d screenPos(mouse.GetScreenPos().Cast<double>().GetData());
    svtkVector2d pos(0.0, 0.0);
    svtkTransform2D* transform = this->GetTransform();
    transform->InverseTransformPoints(screenPos.GetData(), pos.GetData(), 1);
    svtkVector2f zoomAnchor = svtkVector2f(pos.Cast<float>().GetData());

    // Ten "wheels" to double/halve zoom level
    float scaling = pow(2.0f, delta / 10.0f);

    // Zoom in on current position
    this->Translate(zoomAnchor[0], zoomAnchor[1]);
    this->Scale(scaling, scaling);
    this->Translate(-zoomAnchor[0], -zoomAnchor[1]);

    // Mark the scene as dirty
    this->Scene->SetDirty(true);

    this->InvokeEvent(svtkCommand::InteractionEvent);
    return true;
  }
  if (this->PanYOnMouseWheel)
  {
    // Ten "wheels" to scroll a screen
    this->Translate(0.0f, delta / 10.0f * this->Scene->GetSceneHeight());

    // Mark the scene as dirty
    this->Scene->SetDirty(true);

    this->InvokeEvent(svtkCommand::InteractionEvent);
    return true;
  }
  return false;
}

//-----------------------------------------------------------------------------
void svtkContextTransform::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Transform:\n";
  this->Transform->PrintSelf(os, indent.GetNextIndent());
}
