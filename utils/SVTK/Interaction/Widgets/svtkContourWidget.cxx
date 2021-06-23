/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkContourWidget.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkContourWidget.h"
#include "svtkCallbackCommand.h"
#include "svtkCommand.h"
#include "svtkEvent.h"
#include "svtkObjectFactory.h"
#include "svtkOrientedGlyphContourRepresentation.h"
#include "svtkPolyData.h"
#include "svtkProperty.h"
#include "svtkProperty2D.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkSphereSource.h"
#include "svtkWidgetCallbackMapper.h"
#include "svtkWidgetEvent.h"

svtkStandardNewMacro(svtkContourWidget);

//----------------------------------------------------------------------
svtkContourWidget::svtkContourWidget()
{
  this->ManagesCursor = 0;
  this->WidgetState = svtkContourWidget::Start;
  this->CurrentHandle = 0;
  this->AllowNodePicking = 0;
  this->FollowCursor = 0;
  this->ContinuousDraw = 0;
  this->ContinuousActive = 0;

  // These are the event callbacks supported by this widget
  this->CallbackMapper->SetCallbackMethod(
    svtkCommand::LeftButtonPressEvent, svtkWidgetEvent::Select, this, svtkContourWidget::SelectAction);
  this->CallbackMapper->SetCallbackMethod(svtkCommand::RightButtonPressEvent,
    svtkWidgetEvent::AddFinalPoint, this, svtkContourWidget::AddFinalPointAction);
  this->CallbackMapper->SetCallbackMethod(
    svtkCommand::MouseMoveEvent, svtkWidgetEvent::Move, this, svtkContourWidget::MoveAction);
  this->CallbackMapper->SetCallbackMethod(svtkCommand::LeftButtonReleaseEvent,
    svtkWidgetEvent::EndSelect, this, svtkContourWidget::EndSelectAction);
  this->CallbackMapper->SetCallbackMethod(svtkCommand::KeyPressEvent, svtkEvent::NoModifier, 127, 1,
    "Delete", svtkWidgetEvent::Delete, this, svtkContourWidget::DeleteAction);
  this->CallbackMapper->SetCallbackMethod(svtkCommand::KeyPressEvent, svtkEvent::NoModifier, 8, 1,
    "BackSpace", svtkWidgetEvent::Delete, this, svtkContourWidget::DeleteAction);
  this->CallbackMapper->SetCallbackMethod(svtkCommand::KeyPressEvent, svtkEvent::ShiftModifier, 127,
    1, "Delete", svtkWidgetEvent::Reset, this, svtkContourWidget::ResetAction);
  this->CallbackMapper->SetCallbackMethod(svtkCommand::MiddleButtonPressEvent,
    svtkWidgetEvent::Translate, this, svtkContourWidget::TranslateContourAction);
  this->CallbackMapper->SetCallbackMethod(svtkCommand::MiddleButtonReleaseEvent,
    svtkWidgetEvent::EndTranslate, this, svtkContourWidget::EndSelectAction);
  this->CallbackMapper->SetCallbackMethod(svtkCommand::RightButtonPressEvent, svtkWidgetEvent::Scale,
    this, svtkContourWidget::ScaleContourAction);
  this->CallbackMapper->SetCallbackMethod(svtkCommand::RightButtonReleaseEvent,
    svtkWidgetEvent::EndScale, this, svtkContourWidget::EndSelectAction);

  this->CreateDefaultRepresentation();
}

//----------------------------------------------------------------------
svtkContourWidget::~svtkContourWidget() = default;

//----------------------------------------------------------------------
void svtkContourWidget::CreateDefaultRepresentation()
{
  if (!this->WidgetRep)
  {
    svtkOrientedGlyphContourRepresentation* rep = svtkOrientedGlyphContourRepresentation::New();

    this->WidgetRep = rep;

    svtkSphereSource* ss = svtkSphereSource::New();
    ss->SetRadius(0.5);
    ss->Update();
    rep->SetActiveCursorShape(ss->GetOutput());
    ss->Delete();

    rep->GetProperty()->SetColor(0.25, 1.0, 0.25);

    svtkProperty* property = svtkProperty::SafeDownCast(rep->GetActiveProperty());
    if (property)
    {
      property->SetRepresentationToSurface();
      property->SetAmbient(0.1);
      property->SetDiffuse(0.9);
      property->SetSpecular(0.0);
    }
  }
}

//----------------------------------------------------------------------
void svtkContourWidget::CloseLoop()
{
  svtkContourRepresentation* rep = reinterpret_cast<svtkContourRepresentation*>(this->WidgetRep);
  if (!rep->GetClosedLoop() && rep->GetNumberOfNodes() > 1)
  {
    this->WidgetState = svtkContourWidget::Manipulate;
    rep->ClosedLoopOn();
    this->Render();
  }
}

//----------------------------------------------------------------------
void svtkContourWidget::SetEnabled(int enabling)
{
  // The handle widgets are not actually enabled until they are placed.
  // The handle widgets take their representation from the svtkContourRepresentation.
  if (enabling)
  {
    if (this->WidgetState == svtkContourWidget::Start)
    {
      reinterpret_cast<svtkContourRepresentation*>(this->WidgetRep)->VisibilityOff();
    }
    else
    {
      reinterpret_cast<svtkContourRepresentation*>(this->WidgetRep)->VisibilityOn();
    }
  }

  this->Superclass::SetEnabled(enabling);
}

// The following methods are the callbacks that the contour widget responds to.
//-------------------------------------------------------------------------
void svtkContourWidget::SelectAction(svtkAbstractWidget* w)
{
  svtkContourWidget* self = reinterpret_cast<svtkContourWidget*>(w);
  svtkContourRepresentation* rep = reinterpret_cast<svtkContourRepresentation*>(self->WidgetRep);

  int X = self->Interactor->GetEventPosition()[0];
  int Y = self->Interactor->GetEventPosition()[1];
  double pos[2];
  pos[0] = X;
  pos[1] = Y;

  if (self->ContinuousDraw)
  {
    self->ContinuousActive = 0;
  }

  switch (self->WidgetState)
  {
    case svtkContourWidget::Start:
    case svtkContourWidget::Define:
    {
      // If we are following the cursor, let's add 2 nodes rightaway, on the
      // first click. The second node is the one that follows the cursor
      // around.
      if ((self->FollowCursor || self->ContinuousDraw) && (rep->GetNumberOfNodes() == 0))
      {
        self->AddNode();
      }
      self->AddNode();
      if (self->ContinuousDraw)
      {
        self->ContinuousActive = 1;
      }
      break;
    }

    case svtkContourWidget::Manipulate:
    {
      if (rep->ActivateNode(X, Y))
      {
        self->Superclass::StartInteraction();
        self->InvokeEvent(svtkCommand::StartInteractionEvent, nullptr);
        self->StartInteraction();
        rep->SetCurrentOperationToTranslate();
        rep->StartWidgetInteraction(pos);
        self->EventCallbackCommand->SetAbortFlag(1);
      }
      else if (rep->AddNodeOnContour(X, Y))
      {
        if (rep->ActivateNode(X, Y))
        {
          rep->SetCurrentOperationToTranslate();
          rep->StartWidgetInteraction(pos);
        }
        self->EventCallbackCommand->SetAbortFlag(1);
      }
      else if (!rep->GetNeedToRender())
      {
        rep->SetRebuildLocator(true);
      }
      break;
    }
  }

  if (rep->GetNeedToRender())
  {
    self->Render();
    rep->NeedToRenderOff();
  }
}

//-------------------------------------------------------------------------
void svtkContourWidget::AddFinalPointAction(svtkAbstractWidget* w)
{
  svtkContourWidget* self = reinterpret_cast<svtkContourWidget*>(w);
  svtkContourRepresentation* rep = reinterpret_cast<svtkContourRepresentation*>(self->WidgetRep);

  if (self->WidgetState != svtkContourWidget::Manipulate && rep->GetNumberOfNodes() >= 1)
  {
    // In follow cursor and continuous draw mode, the "extra" node
    // has already been added for us.
    if (!self->FollowCursor && !self->ContinuousDraw)
    {
      self->AddNode();
    }

    if (self->ContinuousDraw)
    {
      self->ContinuousActive = 0;
    }

    self->WidgetState = svtkContourWidget::Manipulate;
    self->EventCallbackCommand->SetAbortFlag(1);
    self->InvokeEvent(svtkCommand::EndInteractionEvent, nullptr);
  }

  if (rep->GetNeedToRender())
  {
    self->Render();
    rep->NeedToRenderOff();
  }
}

//------------------------------------------------------------------------
void svtkContourWidget::AddNode()
{
  int X = this->Interactor->GetEventPosition()[0];
  int Y = this->Interactor->GetEventPosition()[1];

  // If the rep already has at least 2 nodes, check how close we are to
  // the first
  svtkContourRepresentation* rep = reinterpret_cast<svtkContourRepresentation*>(this->WidgetRep);

  int numNodes = rep->GetNumberOfNodes();
  if (numNodes > 1)
  {
    int pixelTolerance = rep->GetPixelTolerance();
    int pixelTolerance2 = pixelTolerance * pixelTolerance;

    double displayPos[2];
    if (!rep->GetNthNodeDisplayPosition(0, displayPos))
    {
      svtkErrorMacro("Can't get first node display position!");
      return;
    }

    // if in continuous draw mode, we don't want to close the loop until we are at least
    // numNodes > pixelTolerance away

    int distance2 = static_cast<int>(
      (X - displayPos[0]) * (X - displayPos[0]) + (Y - displayPos[1]) * (Y - displayPos[1]));

    if ((distance2 < pixelTolerance2 && numNodes > 2) ||
      (this->ContinuousDraw && numNodes > pixelTolerance && distance2 < pixelTolerance2))
    {
      // yes - we have made a loop. Stop defining and switch to
      // manipulate mode
      this->WidgetState = svtkContourWidget::Manipulate;
      rep->ClosedLoopOn();
      this->Render();
      this->EventCallbackCommand->SetAbortFlag(1);
      this->InvokeEvent(svtkCommand::EndInteractionEvent, nullptr);
      return;
    }
  }

  if (rep->AddNodeAtDisplayPosition(X, Y))
  {
    if (this->WidgetState == svtkContourWidget::Start)
    {
      this->InvokeEvent(svtkCommand::StartInteractionEvent, nullptr);
    }

    this->WidgetState = svtkContourWidget::Define;
    rep->VisibilityOn();
    this->EventCallbackCommand->SetAbortFlag(1);
    this->InvokeEvent(svtkCommand::InteractionEvent, nullptr);
  }
}

//-------------------------------------------------------------------------
// Note that if you select the contour at a location that is not moused over
// a control point, the translate action makes the closest contour node
// jump to the current mouse location. Perhaps we should either
// (a) Disable translations when not moused over a control point
// (b) Fix the jumping behaviour by calculating motion vectors from the start
//     of the interaction.
void svtkContourWidget::TranslateContourAction(svtkAbstractWidget* w)
{
  svtkContourWidget* self = reinterpret_cast<svtkContourWidget*>(w);

  if (self->WidgetState != svtkContourWidget::Manipulate)
  {
    return;
  }

  svtkContourRepresentation* rep = reinterpret_cast<svtkContourRepresentation*>(self->WidgetRep);

  int X = self->Interactor->GetEventPosition()[0];
  int Y = self->Interactor->GetEventPosition()[1];
  double pos[2];
  pos[0] = X;
  pos[1] = Y;

  if (rep->ActivateNode(X, Y))
  {
    self->Superclass::StartInteraction();
    self->InvokeEvent(svtkCommand::StartInteractionEvent, nullptr);
    self->StartInteraction();
    rep->SetCurrentOperationToShift(); // Here
    rep->StartWidgetInteraction(pos);
    self->EventCallbackCommand->SetAbortFlag(1);
  }
  else
  {
    double p[3];
    int idx;
    if (rep->FindClosestPointOnContour(X, Y, p, &idx))
    {
      rep->GetNthNodeDisplayPosition(idx, pos);
      rep->ActivateNode(pos);
      self->Superclass::StartInteraction();
      self->InvokeEvent(svtkCommand::StartInteractionEvent, nullptr);
      self->StartInteraction();
      rep->SetCurrentOperationToShift(); // Here
      rep->StartWidgetInteraction(pos);
      self->EventCallbackCommand->SetAbortFlag(1);
    }
  }

  if (rep->GetNeedToRender())
  {
    self->Render();
    rep->NeedToRenderOff();
  }
}
//-------------------------------------------------------------------------
// Note that if you select the contour at a location that is not moused over
// a control point, the scale action makes the closest contour node
// jump to the current mouse location. Perhaps we should either
// (a) Disable scaling when not moused over a control point
// (b) Fix the jumping behaviour by calculating motion vectors from the start
//     of the interaction.
void svtkContourWidget::ScaleContourAction(svtkAbstractWidget* w)
{
  svtkContourWidget* self = reinterpret_cast<svtkContourWidget*>(w);

  if (self->WidgetState != svtkContourWidget::Manipulate)
    return;

  svtkContourRepresentation* rep = reinterpret_cast<svtkContourRepresentation*>(self->WidgetRep);

  int X = self->Interactor->GetEventPosition()[0];
  int Y = self->Interactor->GetEventPosition()[1];
  double pos[2];
  pos[0] = X;
  pos[1] = Y;

  if (rep->ActivateNode(X, Y))
  {
    self->Superclass::StartInteraction();
    self->InvokeEvent(svtkCommand::StartInteractionEvent, nullptr);
    self->StartInteraction();
    rep->SetCurrentOperationToScale(); // Here
    rep->StartWidgetInteraction(pos);
    self->EventCallbackCommand->SetAbortFlag(1);
  }
  else
  {
    double p[3];
    int idx;
    if (rep->FindClosestPointOnContour(X, Y, p, &idx))
    {
      rep->GetNthNodeDisplayPosition(idx, pos);
      rep->ActivateNode(pos);
      self->Superclass::StartInteraction();
      self->InvokeEvent(svtkCommand::StartInteractionEvent, nullptr);
      self->StartInteraction();
      rep->SetCurrentOperationToScale(); // Here
      rep->StartWidgetInteraction(pos);
      self->EventCallbackCommand->SetAbortFlag(1);
    }
  }

  if (rep->GetNeedToRender())
  {
    self->Render();
    rep->NeedToRenderOff();
  }
}

//-------------------------------------------------------------------------
void svtkContourWidget::DeleteAction(svtkAbstractWidget* w)
{
  svtkContourWidget* self = reinterpret_cast<svtkContourWidget*>(w);

  if (self->WidgetState == svtkContourWidget::Start)
  {
    return;
  }

  svtkContourRepresentation* rep = reinterpret_cast<svtkContourRepresentation*>(self->WidgetRep);

  if (self->WidgetState == svtkContourWidget::Define)
  {
    if (rep->DeleteLastNode())
    {
      self->InvokeEvent(svtkCommand::InteractionEvent, nullptr);
    }
  }
  else
  {
    int X = self->Interactor->GetEventPosition()[0];
    int Y = self->Interactor->GetEventPosition()[1];
    rep->ActivateNode(X, Y);
    if (rep->DeleteActiveNode())
    {
      self->InvokeEvent(svtkCommand::InteractionEvent, nullptr);
    }
    rep->ActivateNode(X, Y);
    int numNodes = rep->GetNumberOfNodes();
    if (numNodes < 3)
    {
      rep->ClosedLoopOff();
      if (numNodes < 2)
      {
        self->WidgetState = svtkContourWidget::Define;
      }
    }
  }

  if (rep->GetNeedToRender())
  {
    self->Render();
    rep->NeedToRenderOff();
  }
}

//-------------------------------------------------------------------------
void svtkContourWidget::MoveAction(svtkAbstractWidget* w)
{
  svtkContourWidget* self = reinterpret_cast<svtkContourWidget*>(w);

  if (self->WidgetState == svtkContourWidget::Start)
  {
    return;
  }

  int X = self->Interactor->GetEventPosition()[0];
  int Y = self->Interactor->GetEventPosition()[1];
  svtkContourRepresentation* rep = reinterpret_cast<svtkContourRepresentation*>(self->WidgetRep);

  if (self->WidgetState == svtkContourWidget::Define)
  {
    if (self->FollowCursor || self->ContinuousDraw)
    {
      // Have the last node follow the mouse in this case...
      const int numNodes = rep->GetNumberOfNodes();

      // First check if the last node is near the first node, if so, we intend
      // closing the loop.
      if (numNodes > 1)
      {
        double displayPos[2];
        int pixelTolerance = rep->GetPixelTolerance();
        int pixelTolerance2 = pixelTolerance * pixelTolerance;

        rep->GetNthNodeDisplayPosition(0, displayPos);

        int distance2 = static_cast<int>(
          (X - displayPos[0]) * (X - displayPos[0]) + (Y - displayPos[1]) * (Y - displayPos[1]));

        const bool mustCloseLoop = (distance2 < pixelTolerance2 && numNodes > 2) ||
          (self->ContinuousDraw && numNodes > pixelTolerance && distance2 < pixelTolerance2);

        if (mustCloseLoop != (rep->GetClosedLoop() == 1))
        {
          if (rep->GetClosedLoop())
          {
            // We need to open the closed loop.
            // We do this by adding a node at (X,Y). If by chance the point
            // placer says that (X,Y) is invalid, we'll add it at the location
            // of the first control point (which we know is valid).

            if (!rep->AddNodeAtDisplayPosition(X, Y))
            {
              double closedLoopPoint[3];
              rep->GetNthNodeWorldPosition(0, closedLoopPoint);
              rep->AddNodeAtDisplayPosition(closedLoopPoint);
            }
            rep->ClosedLoopOff();
          }
          else
          {
            // We need to close the open loop. Delete the node that's following
            // the mouse cursor and close the loop between the previous node and
            // the first node.
            rep->DeleteLastNode();
            rep->ClosedLoopOn();
          }
        }
        else if (rep->GetClosedLoop() == 0)
        {
          if (self->ContinuousDraw && self->ContinuousActive)
          {
            rep->AddNodeAtDisplayPosition(X, Y);
          }
          else
          {
            // If we aren't changing the loop topology, simply update the position
            // of the latest node to follow the mouse cursor position (X,Y).
            rep->SetNthNodeDisplayPosition(numNodes - 1, X, Y);
          }
        }
      }
    }
    else
    {
      return;
    }
  }

  if (rep->GetCurrentOperation() == svtkContourRepresentation::Inactive)
  {
    rep->ComputeInteractionState(X, Y);
    rep->ActivateNode(X, Y);
  }
  else
  {
    double pos[2];
    pos[0] = X;
    pos[1] = Y;
    self->WidgetRep->WidgetInteraction(pos);
    self->InvokeEvent(svtkCommand::InteractionEvent, nullptr);
  }

  if (self->WidgetRep->GetNeedToRender())
  {
    self->Render();
    self->WidgetRep->NeedToRenderOff();
  }
}

//-------------------------------------------------------------------------
void svtkContourWidget::EndSelectAction(svtkAbstractWidget* w)
{
  svtkContourWidget* self = reinterpret_cast<svtkContourWidget*>(w);
  svtkContourRepresentation* rep = reinterpret_cast<svtkContourRepresentation*>(self->WidgetRep);

  if (self->ContinuousDraw)
  {
    self->ContinuousActive = 0;
  }

  // Do nothing if inactive
  if (rep->GetCurrentOperation() == svtkContourRepresentation::Inactive)
  {
    rep->SetRebuildLocator(true);
    return;
  }

  rep->SetCurrentOperationToInactive();
  self->EventCallbackCommand->SetAbortFlag(1);
  self->Superclass::EndInteraction();
  self->InvokeEvent(svtkCommand::EndInteractionEvent, nullptr);

  // Node picking
  if (self->AllowNodePicking && self->Interactor->GetControlKey() &&
    self->WidgetState == svtkContourWidget::Manipulate)
  {
    rep->ToggleActiveNodeSelected();
  }

  if (self->WidgetRep->GetNeedToRender())
  {
    self->Render();
    self->WidgetRep->NeedToRenderOff();
  }
}

//-------------------------------------------------------------------------
void svtkContourWidget::ResetAction(svtkAbstractWidget* w)
{
  svtkContourWidget* self = reinterpret_cast<svtkContourWidget*>(w);
  self->Initialize(nullptr);
}

//----------------------------------------------------------------------
void svtkContourWidget::Initialize(svtkPolyData* pd, int state, svtkIdList* idList)
{
  if (!this->GetEnabled())
  {
    svtkErrorMacro(<< "Enable widget before initializing");
  }

  if (this->WidgetRep)
  {
    svtkContourRepresentation* rep = reinterpret_cast<svtkContourRepresentation*>(this->WidgetRep);

    if (pd == nullptr)
    {
      while (rep->DeleteLastNode())
      {
        ;
      }
      rep->ClosedLoopOff();
      this->Render();
      rep->NeedToRenderOff();
      rep->VisibilityOff();
      this->WidgetState = svtkContourWidget::Start;
    }
    else
    {
      rep->Initialize(pd, idList);
      this->WidgetState = (rep->GetClosedLoop() || state == 1) ? svtkContourWidget::Manipulate
                                                               : svtkContourWidget::Define;
    }
  }
}

//----------------------------------------------------------------------
void svtkContourWidget::SetAllowNodePicking(svtkTypeBool val)
{
  if (this->AllowNodePicking == val)
  {
    return;
  }
  this->AllowNodePicking = val;
  if (this->AllowNodePicking)
  {
    svtkContourRepresentation* rep = reinterpret_cast<svtkContourRepresentation*>(this->WidgetRep);
    rep->SetShowSelectedNodes(this->AllowNodePicking);
  }
}

//----------------------------------------------------------------------
void svtkContourWidget::PrintSelf(ostream& os, svtkIndent indent)
{
  // Superclass typedef defined in svtkTypeMacro() found in svtkSetGet.h
  this->Superclass::PrintSelf(os, indent);

  os << indent << "WidgetState: " << this->WidgetState << endl;
  os << indent << "CurrentHandle: " << this->CurrentHandle << endl;
  os << indent << "AllowNodePicking: " << this->AllowNodePicking << endl;
  os << indent << "FollowCursor: " << (this->FollowCursor ? "On" : "Off") << endl;
  os << indent << "ContinuousDraw: " << (this->ContinuousDraw ? "On" : "Off") << endl;
}
