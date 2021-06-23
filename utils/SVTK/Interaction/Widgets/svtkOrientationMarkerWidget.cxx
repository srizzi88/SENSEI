/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkOrientationMarkerWidget.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkOrientationMarkerWidget.h"

#include "svtkActor2D.h"
#include "svtkCallbackCommand.h"
#include "svtkCamera.h"
#include "svtkCoordinate.h"
#include "svtkObjectFactory.h"
#include "svtkPoints.h"
#include "svtkPolyData.h"
#include "svtkPolyDataMapper2D.h"
#include "svtkProp.h"
#include "svtkProperty2D.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"

svtkStandardNewMacro(svtkOrientationMarkerWidget);

svtkCxxSetObjectMacro(svtkOrientationMarkerWidget, OrientationMarker, svtkProp);

class svtkOrientationMarkerWidgetObserver : public svtkCommand
{
public:
  static svtkOrientationMarkerWidgetObserver* New()
  {
    return new svtkOrientationMarkerWidgetObserver;
  }

  svtkOrientationMarkerWidgetObserver() { this->OrientationMarkerWidget = nullptr; }

  void Execute(svtkObject* wdg, unsigned long event, void* calldata) override
  {
    if (this->OrientationMarkerWidget)
    {
      this->OrientationMarkerWidget->ExecuteCameraUpdateEvent(wdg, event, calldata);
    }
  }

  svtkOrientationMarkerWidget* OrientationMarkerWidget;
};

//-------------------------------------------------------------------------
svtkOrientationMarkerWidget::svtkOrientationMarkerWidget()
{
  this->StartEventObserverId = 0;
  this->EventCallbackCommand->SetCallback(svtkOrientationMarkerWidget::ProcessEvents);

  this->Observer = svtkOrientationMarkerWidgetObserver::New();
  this->Observer->OrientationMarkerWidget = this;

  this->Tolerance = 7;
  this->Moving = 0;

  this->Viewport[0] = 0.0;
  this->Viewport[1] = 0.0;
  this->Viewport[2] = 0.2;
  this->Viewport[3] = 0.2;

  this->Renderer = svtkRenderer::New();
  this->Renderer->SetLayer(1);
  this->Renderer->InteractiveOff();

  this->Priority = 0.55;
  this->OrientationMarker = nullptr;
  this->State = svtkOrientationMarkerWidget::Outside;
  this->Interactive = 1;

  this->Outline = svtkPolyData::New();
  this->Outline->AllocateExact(128, 128);
  svtkPoints* points = svtkPoints::New();
  svtkIdType ptIds[5];
  ptIds[4] = ptIds[0] = points->InsertNextPoint(1, 1, 0);
  ptIds[1] = points->InsertNextPoint(2, 1, 0);
  ptIds[2] = points->InsertNextPoint(2, 2, 0);
  ptIds[3] = points->InsertNextPoint(1, 2, 0);

  this->Outline->SetPoints(points);
  this->Outline->InsertNextCell(SVTK_POLY_LINE, 5, ptIds);

  svtkCoordinate* tcoord = svtkCoordinate::New();
  tcoord->SetCoordinateSystemToDisplay();

  svtkPolyDataMapper2D* mapper = svtkPolyDataMapper2D::New();
  mapper->SetInputData(this->Outline);
  mapper->SetTransformCoordinate(tcoord);

  this->OutlineActor = svtkActor2D::New();
  this->OutlineActor->SetMapper(mapper);
  this->OutlineActor->SetPosition(0, 0);
  this->OutlineActor->SetPosition2(1, 1);
  this->OutlineActor->VisibilityOff();

  points->Delete();
  mapper->Delete();
  tcoord->Delete();
}

//-------------------------------------------------------------------------
svtkOrientationMarkerWidget::~svtkOrientationMarkerWidget()
{
  if (this->Enabled)
  {
    this->TearDownWindowInteraction();
  }

  this->Observer->Delete();
  this->Observer = nullptr;
  this->Renderer->Delete();
  this->Renderer = nullptr;
  this->SetOrientationMarker(nullptr);
  this->OutlineActor->Delete();
  this->Outline->Delete();
}

//-------------------------------------------------------------------------
void svtkOrientationMarkerWidget::SetEnabled(int value)
{
  if (!this->Interactor)
  {
    svtkErrorMacro("The interactor must be set prior to enabling/disabling widget");
  }

  if (value != this->Enabled)
  {
    if (value)
    {
      if (!this->OrientationMarker)
      {
        svtkErrorMacro("An orientation marker must be set prior to enabling/disabling widget");
        return;
      }

      if (!this->CurrentRenderer)
      {
        int* pos = this->Interactor->GetLastEventPosition();
        this->SetCurrentRenderer(this->Interactor->FindPokedRenderer(pos[0], pos[1]));

        if (this->CurrentRenderer == nullptr)
        {
          return;
        }
      }

      this->UpdateInternalViewport();

      this->SetupWindowInteraction();
      this->Enabled = 1;
      this->InvokeEvent(svtkCommand::EnableEvent, nullptr);
    }
    else
    {
      this->InvokeEvent(svtkCommand::DisableEvent, nullptr);
      this->Enabled = 0;
      this->TearDownWindowInteraction();
      this->SetCurrentRenderer(nullptr);
    }
  }
}

//-------------------------------------------------------------------------
void svtkOrientationMarkerWidget::SetupWindowInteraction()
{
  svtkRenderWindow* renwin = this->CurrentRenderer->GetRenderWindow();
  renwin->AddRenderer(this->Renderer);
  if (renwin->GetNumberOfLayers() < 2)
  {
    renwin->SetNumberOfLayers(2);
  }

  this->CurrentRenderer->AddViewProp(this->OutlineActor);

  this->Renderer->AddViewProp(this->OrientationMarker);
  this->OrientationMarker->VisibilityOn();

  if (this->Interactive)
  {
    svtkRenderWindowInteractor* interactor = this->Interactor;
    if (this->EventCallbackCommand)
    {
      interactor->AddObserver(
        svtkCommand::MouseMoveEvent, this->EventCallbackCommand, this->Priority);
      interactor->AddObserver(
        svtkCommand::LeftButtonPressEvent, this->EventCallbackCommand, this->Priority);
      interactor->AddObserver(
        svtkCommand::LeftButtonReleaseEvent, this->EventCallbackCommand, this->Priority);
    }
  }

  svtkCamera* pcam = this->CurrentRenderer->GetActiveCamera();
  svtkCamera* cam = this->Renderer->GetActiveCamera();
  if (pcam && cam)
  {
    cam->SetParallelProjection(pcam->GetParallelProjection());
  }

  // We need to copy the camera before the compositing observer is called.
  // Compositing temporarily changes the camera to display an image.
  this->StartEventObserverId =
    this->CurrentRenderer->AddObserver(svtkCommand::StartEvent, this->Observer, 1);
}

//-------------------------------------------------------------------------
void svtkOrientationMarkerWidget::TearDownWindowInteraction()
{
  if (this->StartEventObserverId != 0)
  {
    this->CurrentRenderer->RemoveObserver(this->StartEventObserverId);
  }

  this->Interactor->RemoveObserver(this->EventCallbackCommand);

  this->OrientationMarker->VisibilityOff();
  this->Renderer->RemoveViewProp(this->OrientationMarker);

  this->CurrentRenderer->RemoveViewProp(this->OutlineActor);

  // if the render window is still around, remove our renderer from it
  svtkRenderWindow* renwin = this->CurrentRenderer->GetRenderWindow();
  if (renwin)
  {
    renwin->RemoveRenderer(this->Renderer);
  }
}

//-------------------------------------------------------------------------
void svtkOrientationMarkerWidget::ExecuteCameraUpdateEvent(
  svtkObject* svtkNotUsed(o), unsigned long svtkNotUsed(event), void* svtkNotUsed(calldata))
{
  if (!this->CurrentRenderer)
  {
    return;
  }

  svtkCamera* cam = this->CurrentRenderer->GetActiveCamera();
  double pos[3], fp[3], viewup[3];
  cam->GetPosition(pos);
  cam->GetFocalPoint(fp);
  cam->GetViewUp(viewup);

  cam = this->Renderer->GetActiveCamera();
  cam->SetPosition(pos);
  cam->SetFocalPoint(fp);
  cam->SetViewUp(viewup);
  this->Renderer->ResetCamera();

  this->UpdateOutline();
}

//-------------------------------------------------------------------------
int svtkOrientationMarkerWidget::ComputeStateBasedOnPosition(int X, int Y, int* pos1, int* pos2)
{
  if (X < (pos1[0] - this->Tolerance) || (pos2[0] + this->Tolerance) < X ||
    Y < (pos1[1] - this->Tolerance) || (pos2[1] + this->Tolerance) < Y)
  {
    return svtkOrientationMarkerWidget::Outside;
  }

  // if we are not outside and the left mouse button wasn't clicked,
  // then we are inside, otherwise we are moving

  int result =
    this->Moving ? svtkOrientationMarkerWidget::Translating : svtkOrientationMarkerWidget::Inside;

  int e1 = 0;
  int e2 = 0;
  int e3 = 0;
  int e4 = 0;
  if (X - pos1[0] < this->Tolerance)
  {
    e1 = 1;
  }
  if (pos2[0] - X < this->Tolerance)
  {
    e3 = 1;
  }
  if (Y - pos1[1] < this->Tolerance)
  {
    e2 = 1;
  }
  if (pos2[1] - Y < this->Tolerance)
  {
    e4 = 1;
  }

  // are we on a corner or an edge?
  if (e1)
  {
    if (e2)
    {
      result = svtkOrientationMarkerWidget::AdjustingP1; // lower left
    }
    if (e4)
    {
      result = svtkOrientationMarkerWidget::AdjustingP4; // upper left
    }
  }
  if (e3)
  {
    if (e2)
    {
      result = svtkOrientationMarkerWidget::AdjustingP2; // lower right
    }
    if (e4)
    {
      result = svtkOrientationMarkerWidget::AdjustingP3; // upper right
    }
  }

  return result;
}

//-------------------------------------------------------------------------
void svtkOrientationMarkerWidget::SetCursor(int state)
{
  switch (state)
  {
    case svtkOrientationMarkerWidget::AdjustingP1:
      this->RequestCursorShape(SVTK_CURSOR_SIZESW);
      break;
    case svtkOrientationMarkerWidget::AdjustingP3:
      this->RequestCursorShape(SVTK_CURSOR_SIZENE);
      break;
    case svtkOrientationMarkerWidget::AdjustingP2:
      this->RequestCursorShape(SVTK_CURSOR_SIZESE);
      break;
    case svtkOrientationMarkerWidget::AdjustingP4:
      this->RequestCursorShape(SVTK_CURSOR_SIZENW);
      break;
    case svtkOrientationMarkerWidget::Translating:
      this->RequestCursorShape(SVTK_CURSOR_SIZEALL);
      break;
    case svtkOrientationMarkerWidget::Inside:
      this->RequestCursorShape(SVTK_CURSOR_SIZEALL);
      break;
    case svtkOrientationMarkerWidget::Outside:
      this->RequestCursorShape(SVTK_CURSOR_DEFAULT);
      break;
  }
}

//-------------------------------------------------------------------------
void svtkOrientationMarkerWidget::ProcessEvents(
  svtkObject* svtkNotUsed(object), unsigned long event, void* clientdata, void* svtkNotUsed(calldata))
{
  svtkOrientationMarkerWidget* self = reinterpret_cast<svtkOrientationMarkerWidget*>(clientdata);

  if (!self->GetInteractive())
  {
    return;
  }

  switch (event)
  {
    case svtkCommand::LeftButtonPressEvent:
      self->OnLeftButtonDown();
      break;
    case svtkCommand::LeftButtonReleaseEvent:
      self->OnLeftButtonUp();
      break;
    case svtkCommand::MouseMoveEvent:
      self->OnMouseMove();
      break;
  }
}

//-------------------------------------------------------------------------
void svtkOrientationMarkerWidget::OnLeftButtonDown()
{
  // We're only here if we are enabled
  int X = this->Interactor->GetEventPosition()[0];
  int Y = this->Interactor->GetEventPosition()[1];

  // are we over the widget?
  double vp[4];
  this->Renderer->GetViewport(vp);

  this->Renderer->NormalizedDisplayToDisplay(vp[0], vp[1]);
  this->Renderer->NormalizedDisplayToDisplay(vp[2], vp[3]);

  int pos1[2] = { static_cast<int>(vp[0]), static_cast<int>(vp[1]) };
  int pos2[2] = { static_cast<int>(vp[2]), static_cast<int>(vp[3]) };

  this->StartPosition[0] = X;
  this->StartPosition[1] = Y;

  // flag that we are attempting to adjust or move the outline
  this->Moving = 1;
  this->State = this->ComputeStateBasedOnPosition(X, Y, pos1, pos2);
  this->SetCursor(this->State);

  if (this->State == svtkOrientationMarkerWidget::Outside)
  {
    this->Moving = 0;
    return;
  }

  this->EventCallbackCommand->SetAbortFlag(1);
  this->StartInteraction();
  this->InvokeEvent(svtkCommand::StartInteractionEvent, nullptr);
}

//-------------------------------------------------------------------------
void svtkOrientationMarkerWidget::OnLeftButtonUp()
{
  if (this->State == svtkOrientationMarkerWidget::Outside)
  {
    return;
  }

  // finalize any corner adjustments
  this->SquareRenderer();
  this->UpdateOutline();

  // stop adjusting
  this->State = svtkOrientationMarkerWidget::Outside;
  this->Moving = 0;

  this->RequestCursorShape(SVTK_CURSOR_DEFAULT);
  this->EndInteraction();
  this->InvokeEvent(svtkCommand::EndInteractionEvent, nullptr);
  this->Interactor->Render();
}

//-------------------------------------------------------------------------
void svtkOrientationMarkerWidget::SquareRenderer()
{
  int* size = this->Renderer->GetSize();
  if (size[0] == 0 || size[1] == 0)
  {
    return;
  }

  double vp[4];
  this->Renderer->GetViewport(vp);

  this->Renderer->NormalizedDisplayToDisplay(vp[0], vp[1]);
  this->Renderer->NormalizedDisplayToDisplay(vp[2], vp[3]);

  // get the minimum viewport edge size
  //
  double dx = vp[2] - vp[0];
  double dy = vp[3] - vp[1];

  if (dx != dy)
  {
    double delta = dx < dy ? dx : dy;

    switch (this->State)
    {
      case svtkOrientationMarkerWidget::AdjustingP1:
        vp[2] = vp[0] + delta;
        vp[3] = vp[1] + delta;
        break;
      case svtkOrientationMarkerWidget::AdjustingP2:
        vp[0] = vp[2] - delta;
        vp[3] = vp[1] + delta;
        break;
      case svtkOrientationMarkerWidget::AdjustingP3:
        vp[0] = vp[2] - delta;
        vp[1] = vp[3] - delta;
        break;
      case svtkOrientationMarkerWidget::AdjustingP4:
        vp[2] = vp[0] + delta;
        vp[1] = vp[3] - delta;
        break;
      case svtkOrientationMarkerWidget::Translating:
        delta = (dx + dy) * 0.5;
        vp[0] = ((vp[0] + vp[2]) - delta) * 0.5;
        vp[1] = ((vp[1] + vp[3]) - delta) * 0.5;
        vp[2] = vp[0] + delta;
        vp[3] = vp[1] + delta;
        break;
    }
    this->Renderer->DisplayToNormalizedDisplay(vp[0], vp[1]);
    this->Renderer->DisplayToNormalizedDisplay(vp[2], vp[3]);
    this->Renderer->SetViewport(vp);
    this->UpdateViewport();
  }
}

//-------------------------------------------------------------------------
void svtkOrientationMarkerWidget::UpdateOutline()
{
  double vp[4];
  this->Renderer->GetViewport(vp);

  this->Renderer->NormalizedDisplayToDisplay(vp[0], vp[1]);
  this->Renderer->NormalizedDisplayToDisplay(vp[2], vp[3]);

  svtkPoints* points = this->Outline->GetPoints();

  points->SetPoint(0, vp[0] + 1, vp[1] + 1, 0);
  points->SetPoint(1, vp[2] - 1, vp[1] + 1, 0);
  points->SetPoint(2, vp[2] - 1, vp[3] - 1, 0);
  points->SetPoint(3, vp[0] + 1, vp[3] - 1, 0);
  this->Outline->Modified();
}

//-------------------------------------------------------------------------
void svtkOrientationMarkerWidget::SetInteractive(svtkTypeBool interact)
{
  if (this->Interactor && this->Enabled)
  {
    if (this->Interactive == interact)
    {
      return;
    }
    if (interact)
    {
      svtkRenderWindowInteractor* i = this->Interactor;
      if (this->EventCallbackCommand)
      {
        i->AddObserver(svtkCommand::MouseMoveEvent, this->EventCallbackCommand, this->Priority);
        i->AddObserver(
          svtkCommand::LeftButtonPressEvent, this->EventCallbackCommand, this->Priority);
        i->AddObserver(
          svtkCommand::LeftButtonReleaseEvent, this->EventCallbackCommand, this->Priority);
      }
    }
    else
    {
      this->Interactor->RemoveObserver(this->EventCallbackCommand);
    }
    this->Interactive = interact;
    this->Interactor->Render();
  }
  else
  {
    svtkGenericWarningMacro("Set interactor and Enabled before changing \
      interaction.");
  }
}

//-------------------------------------------------------------------------
void svtkOrientationMarkerWidget::OnMouseMove()
{
  // compute some info we need for all cases
  int X = this->Interactor->GetEventPosition()[0];
  int Y = this->Interactor->GetEventPosition()[1];

  double vp[4];
  this->Renderer->GetViewport(vp);

  // compute display bounds of the widget to see if we are inside or outside
  this->Renderer->NormalizedDisplayToDisplay(vp[0], vp[1]);
  this->Renderer->NormalizedDisplayToDisplay(vp[2], vp[3]);

  int pos1[2] = { static_cast<int>(vp[0]), static_cast<int>(vp[1]) };
  int pos2[2] = { static_cast<int>(vp[2]), static_cast<int>(vp[3]) };

  int state = this->ComputeStateBasedOnPosition(X, Y, pos1, pos2);
  this->State = this->Moving ? this->State : state;
  this->SetCursor(this->State);
  this->OutlineActor->SetVisibility(this->State);

  if (this->State == svtkOrientationMarkerWidget::Outside || !this->Moving)
  {
    this->Interactor->Render();
    return;
  }

  // based on the state set when the left mouse button is clicked,
  // adjust the renderer's viewport
  switch (this->State)
  {
    case svtkOrientationMarkerWidget::AdjustingP1:
      this->ResizeBottomLeft(X, Y);
      break;
    case svtkOrientationMarkerWidget::AdjustingP2:
      this->ResizeBottomRight(X, Y);
      break;
    case svtkOrientationMarkerWidget::AdjustingP3:
      this->ResizeTopRight(X, Y);
      break;
    case svtkOrientationMarkerWidget::AdjustingP4:
      this->ResizeTopLeft(X, Y);
      break;
    case svtkOrientationMarkerWidget::Translating:
      this->MoveWidget(X, Y);
      break;
  }

  this->UpdateOutline();
  this->EventCallbackCommand->SetAbortFlag(1);
  this->InvokeEvent(svtkCommand::InteractionEvent, nullptr);
  this->Interactor->Render();
}

//-------------------------------------------------------------------------
void svtkOrientationMarkerWidget::MoveWidget(int X, int Y)
{
  int dx = X - this->StartPosition[0];
  int dy = Y - this->StartPosition[1];

  this->StartPosition[0] = X;
  this->StartPosition[1] = Y;

  double currentViewport[4];
  this->CurrentRenderer->GetViewport(currentViewport);
  this->CurrentRenderer->NormalizedDisplayToDisplay(currentViewport[0], currentViewport[1]);
  this->CurrentRenderer->NormalizedDisplayToDisplay(currentViewport[2], currentViewport[3]);

  double vp[4];
  this->Renderer->GetViewport(vp);
  this->Renderer->NormalizedDisplayToDisplay(vp[0], vp[1]);
  this->Renderer->NormalizedDisplayToDisplay(vp[2], vp[3]);

  double newPos[4] = { vp[0] + dx, vp[1] + dy, vp[2] + dx, vp[3] + dy };

  if (newPos[0] < currentViewport[0])
  {
    newPos[0] = currentViewport[0];
    newPos[2] = currentViewport[0] + (vp[2] - vp[0]);
    this->StartPosition[0] = static_cast<int>((newPos[2] - 0.5 * (vp[2] - vp[0])));
  }
  if (newPos[1] < currentViewport[1])
  {
    newPos[1] = currentViewport[1];
    newPos[3] = currentViewport[1] + (vp[3] - vp[1]);
    this->StartPosition[1] = static_cast<int>((newPos[3] - 0.5 * (vp[3] - vp[1])));
  }
  if (newPos[2] >= currentViewport[2])
  {
    newPos[2] = currentViewport[2];
    newPos[0] = currentViewport[2] - (vp[2] - vp[0]);
    this->StartPosition[0] = static_cast<int>((newPos[0] + 0.5 * (vp[2] - vp[0])));
  }
  if (newPos[3] >= currentViewport[3])
  {
    newPos[3] = currentViewport[3];
    newPos[1] = currentViewport[3] - (vp[3] - vp[1]);
    this->StartPosition[1] = static_cast<int>((newPos[1] + 0.5 * (vp[3] - vp[1])));
  }

  this->Renderer->DisplayToNormalizedDisplay(newPos[0], newPos[1]);
  this->Renderer->DisplayToNormalizedDisplay(newPos[2], newPos[3]);

  this->Renderer->SetViewport(newPos);
  this->UpdateViewport();
}

//-------------------------------------------------------------------------
void svtkOrientationMarkerWidget::ResizeTopLeft(int X, int Y)
{
  int dx = X - this->StartPosition[0];
  int dy = Y - this->StartPosition[1];
  int delta = (abs(dx) + abs(dy)) / 2;

  if (dx <= 0 && dy >= 0) // make bigger
  {
    dx = -delta;
    dy = delta;
  }
  else if (dx >= 0 && dy <= 0) // make smaller
  {
    dx = delta;
    dy = -delta;
  }
  else
  {
    return;
  }

  double currentViewport[4];
  this->CurrentRenderer->GetViewport(currentViewport);
  this->CurrentRenderer->NormalizedDisplayToDisplay(currentViewport[0], currentViewport[1]);
  this->CurrentRenderer->NormalizedDisplayToDisplay(currentViewport[2], currentViewport[3]);

  double vp[4];
  this->Renderer->GetViewport(vp);
  this->Renderer->NormalizedDisplayToDisplay(vp[0], vp[1]);
  this->Renderer->NormalizedDisplayToDisplay(vp[2], vp[3]);

  double newPos[4] = { vp[0] + dx, vp[1], vp[2], vp[3] + dy };

  if (newPos[0] < currentViewport[0])
  {
    newPos[0] = currentViewport[0];
  }
  if (newPos[0] > newPos[2] - this->Tolerance) // keep from making it too small
  {
    newPos[0] = newPos[2] - this->Tolerance;
  }
  if (newPos[3] > currentViewport[3])
  {
    newPos[3] = currentViewport[3];
  }
  if (newPos[3] < newPos[1] + this->Tolerance)
  {
    newPos[3] = newPos[1] + this->Tolerance;
  }

  this->StartPosition[0] = static_cast<int>(newPos[0]);
  this->StartPosition[1] = static_cast<int>(newPos[3]);

  this->Renderer->DisplayToNormalizedDisplay(newPos[0], newPos[1]);
  this->Renderer->DisplayToNormalizedDisplay(newPos[2], newPos[3]);

  this->Renderer->SetViewport(newPos);
  this->UpdateViewport();
}

//-------------------------------------------------------------------------
void svtkOrientationMarkerWidget::ResizeTopRight(int X, int Y)
{
  int dx = X - this->StartPosition[0];
  int dy = Y - this->StartPosition[1];
  int delta = (abs(dx) + abs(dy)) / 2;

  if (dx >= 0 && dy >= 0) // make bigger
  {
    dx = delta;
    dy = delta;
  }
  else if (dx <= 0 && dy <= 0) // make smaller
  {
    dx = -delta;
    dy = -delta;
  }
  else
  {
    return;
  }

  double currentViewport[4];
  this->CurrentRenderer->GetViewport(currentViewport);
  this->CurrentRenderer->NormalizedDisplayToDisplay(currentViewport[0], currentViewport[1]);
  this->CurrentRenderer->NormalizedDisplayToDisplay(currentViewport[2], currentViewport[3]);

  double vp[4];
  this->Renderer->GetViewport(vp);
  this->Renderer->NormalizedDisplayToDisplay(vp[0], vp[1]);
  this->Renderer->NormalizedDisplayToDisplay(vp[2], vp[3]);

  double newPos[4] = { vp[0], vp[1], vp[2] + dx, vp[3] + dy };

  if (newPos[2] > currentViewport[2])
  {
    newPos[2] = currentViewport[2];
  }
  if (newPos[2] < newPos[0] + this->Tolerance) // keep from making it too small
  {
    newPos[2] = newPos[0] + this->Tolerance;
  }
  if (newPos[3] > currentViewport[3])
  {
    newPos[3] = currentViewport[3];
  }
  if (newPos[3] < newPos[1] + this->Tolerance)
  {
    newPos[3] = newPos[1] + this->Tolerance;
  }

  this->StartPosition[0] = static_cast<int>(newPos[2]);
  this->StartPosition[1] = static_cast<int>(newPos[3]);

  this->Renderer->DisplayToNormalizedDisplay(newPos[0], newPos[1]);
  this->Renderer->DisplayToNormalizedDisplay(newPos[2], newPos[3]);

  this->Renderer->SetViewport(newPos);
  this->UpdateViewport();
}

//-------------------------------------------------------------------------
void svtkOrientationMarkerWidget::ResizeBottomRight(int X, int Y)
{
  int dx = X - this->StartPosition[0];
  int dy = Y - this->StartPosition[1];
  int delta = (abs(dx) + abs(dy)) / 2;

  if (dx >= 0 && dy <= 0) // make bigger
  {
    dx = delta;
    dy = -delta;
  }
  else if (dx <= 0 && dy >= 0) // make smaller
  {
    dx = -delta;
    dy = delta;
  }
  else
  {
    return;
  }

  double currentViewport[4];
  this->CurrentRenderer->GetViewport(currentViewport);
  this->CurrentRenderer->NormalizedDisplayToDisplay(currentViewport[0], currentViewport[1]);
  this->CurrentRenderer->NormalizedDisplayToDisplay(currentViewport[2], currentViewport[3]);

  double vp[4];
  this->Renderer->GetViewport(vp);
  this->Renderer->NormalizedDisplayToDisplay(vp[0], vp[1]);
  this->Renderer->NormalizedDisplayToDisplay(vp[2], vp[3]);

  double newPos[4] = { vp[0], vp[1] + dy, vp[2] + dx, vp[3] };

  if (newPos[2] > currentViewport[2])
  {
    newPos[2] = currentViewport[2];
  }
  if (newPos[2] < newPos[0] + this->Tolerance) // keep from making it too small
  {
    newPos[2] = newPos[0] + this->Tolerance;
  }
  if (newPos[1] < currentViewport[1])
  {
    newPos[1] = currentViewport[1];
  }
  if (newPos[1] > newPos[3] - this->Tolerance)
  {
    newPos[1] = newPos[3] - this->Tolerance;
  }

  this->StartPosition[0] = static_cast<int>(newPos[2]);
  this->StartPosition[1] = static_cast<int>(newPos[1]);

  this->Renderer->DisplayToNormalizedDisplay(newPos[0], newPos[1]);
  this->Renderer->DisplayToNormalizedDisplay(newPos[2], newPos[3]);

  this->Renderer->SetViewport(newPos);
  this->UpdateViewport();
}

//-------------------------------------------------------------------------
void svtkOrientationMarkerWidget::ResizeBottomLeft(int X, int Y)
{
  int dx = X - this->StartPosition[0];
  int dy = Y - this->StartPosition[1];
  int delta = (abs(dx) + abs(dy)) / 2;

  if (dx <= 0 && dy <= 0) // make bigger
  {
    dx = -delta;
    dy = -delta;
  }
  else if (dx >= 0 && dy >= 0) // make smaller
  {
    dx = delta;
    dy = delta;
  }
  else
  {
    return;
  }

  double currentViewport[4];
  this->CurrentRenderer->GetViewport(currentViewport);
  this->CurrentRenderer->NormalizedDisplayToDisplay(currentViewport[0], currentViewport[1]);
  this->CurrentRenderer->NormalizedDisplayToDisplay(currentViewport[2], currentViewport[3]);

  double vp[4];
  this->Renderer->GetViewport(vp);
  this->Renderer->NormalizedDisplayToDisplay(vp[0], vp[1]);
  this->Renderer->NormalizedDisplayToDisplay(vp[2], vp[3]);

  double newPos[4] = { vp[0] + dx, vp[1] + dy, vp[2], vp[3] };

  if (newPos[0] < currentViewport[0])
  {
    newPos[0] = currentViewport[0];
  }
  if (newPos[0] > newPos[2] - this->Tolerance) // keep from making it too small
  {
    newPos[0] = newPos[2] - this->Tolerance;
  }
  if (newPos[1] < currentViewport[1])
  {
    newPos[1] = currentViewport[1];
  }
  if (newPos[1] > newPos[3] - this->Tolerance)
  {
    newPos[1] = newPos[3] - this->Tolerance;
  }

  this->StartPosition[0] = static_cast<int>(newPos[0]);
  this->StartPosition[1] = static_cast<int>(newPos[1]);

  this->Renderer->DisplayToNormalizedDisplay(newPos[0], newPos[1]);
  this->Renderer->DisplayToNormalizedDisplay(newPos[2], newPos[3]);

  this->Renderer->SetViewport(newPos);
  this->UpdateViewport();
}

//-------------------------------------------------------------------------
void svtkOrientationMarkerWidget::SetOutlineColor(double r, double g, double b)
{
  this->OutlineActor->GetProperty()->SetColor(r, g, b);
  if (this->Interactor)
  {
    this->Interactor->Render();
  }
}

//-------------------------------------------------------------------------
double* svtkOrientationMarkerWidget::GetOutlineColor()
{
  return this->OutlineActor->GetProperty()->GetColor();
}

//-------------------------------------------------------------------------
void svtkOrientationMarkerWidget::UpdateViewport()
{
  if (!this->CurrentRenderer)
  {
    return;
  }
  double currentViewport[4];
  this->CurrentRenderer->GetViewport(currentViewport);

  double vp[4];
  this->Renderer->GetViewport(vp);

  double cvpRange[2];
  for (int i = 0; i < 2; ++i)
  {
    cvpRange[i] = currentViewport[i + 2] - currentViewport[i];
    this->Viewport[i] = (vp[i] - currentViewport[i]) / cvpRange[i];
    this->Viewport[i + 2] = (vp[i + 2] - currentViewport[i]) / cvpRange[i];
  }
}

//-------------------------------------------------------------------------
void svtkOrientationMarkerWidget::UpdateInternalViewport()
{
  if (!this->Renderer || !this->GetCurrentRenderer())
  {
    return;
  }

  // Compute the viewport for the widget w.r.t. to the current renderer
  double currentViewport[4];
  this->CurrentRenderer->GetViewport(currentViewport);
  double vp[4], currentViewportRange[2];
  for (int i = 0; i < 2; ++i)
  {
    currentViewportRange[i] = currentViewport[i + 2] - currentViewport[i];
    vp[i] = this->Viewport[i] * currentViewportRange[i] + currentViewport[i];
    vp[i + 2] = this->Viewport[i + 2] * currentViewportRange[i] + currentViewport[i];
  }
  this->Renderer->SetViewport(vp);
}

//-------------------------------------------------------------------------
void svtkOrientationMarkerWidget::Modified()
{
  this->UpdateInternalViewport();
  this->svtkInteractorObserver::Modified();
}

//-------------------------------------------------------------------------
void svtkOrientationMarkerWidget::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "OrientationMarker: " << this->OrientationMarker << endl;
  os << indent << "Interactive: " << this->Interactive << endl;
  os << indent << "Tolerance: " << this->Tolerance << endl;
  os << indent << "Viewport: (" << this->Viewport[0] << ", " << this->Viewport[1] << ", "
     << this->Viewport[2] << ", " << this->Viewport[3] << ")\n";
}
