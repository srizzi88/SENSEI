/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkXYPlotWidget.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkXYPlotWidget.h"
#include "svtkCallbackCommand.h"
#include "svtkCoordinate.h"
#include "svtkObjectFactory.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkXYPlotActor.h"

svtkStandardNewMacro(svtkXYPlotWidget);
svtkCxxSetObjectMacro(svtkXYPlotWidget, XYPlotActor, svtkXYPlotActor);

//-------------------------------------------------------------------------
svtkXYPlotWidget::svtkXYPlotWidget()
{
  this->XYPlotActor = svtkXYPlotActor::New();
  this->EventCallbackCommand->SetCallback(svtkXYPlotWidget::ProcessEvents);
  this->State = svtkXYPlotWidget::Outside;
  this->Priority = 0.55;
}

//-------------------------------------------------------------------------
svtkXYPlotWidget::~svtkXYPlotWidget()
{
  if (this->XYPlotActor)
  {
    this->XYPlotActor->Delete();
  }
}

//-------------------------------------------------------------------------
void svtkXYPlotWidget::SetEnabled(int enabling)
{
  if (!this->Interactor)
  {
    svtkErrorMacro(<< "The interactor must be set prior to enabling/disabling widget");
    return;
  }

  if (enabling)
  {
    svtkDebugMacro(<< "Enabling line widget");
    if (this->Enabled) // already enabled, just return
    {
      return;
    }

    if (!this->CurrentRenderer)
    {
      this->SetCurrentRenderer(this->Interactor->FindPokedRenderer(
        this->Interactor->GetLastEventPosition()[0], this->Interactor->GetLastEventPosition()[1]));
      if (this->CurrentRenderer == nullptr)
      {
        return;
      }
    }

    this->Enabled = 1;

    // listen for the following events
    svtkRenderWindowInteractor* i = this->Interactor;
    i->AddObserver(svtkCommand::MouseMoveEvent, this->EventCallbackCommand, this->Priority);
    i->AddObserver(svtkCommand::LeftButtonPressEvent, this->EventCallbackCommand, this->Priority);
    i->AddObserver(svtkCommand::LeftButtonReleaseEvent, this->EventCallbackCommand, this->Priority);

    // Add the xy plot
    this->CurrentRenderer->AddViewProp(this->XYPlotActor);
    this->InvokeEvent(svtkCommand::EnableEvent, nullptr);
  }
  else // disabling------------------------------------------
  {
    svtkDebugMacro(<< "Disabling line widget");
    if (!this->Enabled) // already disabled, just return
    {
      return;
    }
    this->Enabled = 0;

    // don't listen for events any more
    this->Interactor->RemoveObserver(this->EventCallbackCommand);

    // turn off the line
    this->CurrentRenderer->RemoveActor(this->XYPlotActor);
    this->InvokeEvent(svtkCommand::DisableEvent, nullptr);
    this->SetCurrentRenderer(nullptr);
  }

  this->Interactor->Render();
}

//-------------------------------------------------------------------------
void svtkXYPlotWidget::ProcessEvents(
  svtkObject* svtkNotUsed(object), unsigned long event, void* clientdata, void* svtkNotUsed(calldata))
{
  svtkXYPlotWidget* self = reinterpret_cast<svtkXYPlotWidget*>(clientdata);

  // okay, let's do the right thing
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
int svtkXYPlotWidget::ComputeStateBasedOnPosition(int X, int Y, int* pos1, int* pos2)
{
  int Result;

  // what are we modifying? The position, or size?
  // if size what piece?
  // if we are within 7 pixels of an edge...
  int e1 = 0;
  int e2 = 0;
  int e3 = 0;
  int e4 = 0;
  if (X - pos1[0] < 7)
  {
    e1 = 1;
  }
  if (pos2[0] - X < 7)
  {
    e3 = 1;
  }
  if (Y - pos1[1] < 7)
  {
    e2 = 1;
  }
  if (pos2[1] - Y < 7)
  {
    e4 = 1;
  }

  // assume we are moving
  Result = svtkXYPlotWidget::Moving;
  // unless we are on a corner or edges
  if (e2)
  {
    Result = svtkXYPlotWidget::AdjustingE2;
  }
  if (e4)
  {
    Result = svtkXYPlotWidget::AdjustingE4;
  }
  if (e1)
  {
    Result = svtkXYPlotWidget::AdjustingE1;
    if (e2)
    {
      Result = svtkXYPlotWidget::AdjustingP1;
    }
    if (e4)
    {
      Result = svtkXYPlotWidget::AdjustingP4;
    }
  }
  if (e3)
  {
    Result = svtkXYPlotWidget::AdjustingE3;
    if (e2)
    {
      Result = svtkXYPlotWidget::AdjustingP2;
    }
    if (e4)
    {
      Result = svtkXYPlotWidget::AdjustingP3;
    }
  }

  return Result;
}

//-------------------------------------------------------------------------
void svtkXYPlotWidget::SetCursor(int cState)
{
  switch (cState)
  {
    case svtkXYPlotWidget::AdjustingP1:
      this->RequestCursorShape(SVTK_CURSOR_SIZESW);
      break;
    case svtkXYPlotWidget::AdjustingP3:
      this->RequestCursorShape(SVTK_CURSOR_SIZENE);
      break;
    case svtkXYPlotWidget::AdjustingP2:
      this->RequestCursorShape(SVTK_CURSOR_SIZESE);
      break;
    case svtkXYPlotWidget::AdjustingP4:
      this->RequestCursorShape(SVTK_CURSOR_SIZENW);
      break;
    case svtkXYPlotWidget::AdjustingE1:
    case svtkXYPlotWidget::AdjustingE3:
      this->RequestCursorShape(SVTK_CURSOR_SIZEWE);
      break;
    case svtkXYPlotWidget::AdjustingE2:
    case svtkXYPlotWidget::AdjustingE4:
      this->RequestCursorShape(SVTK_CURSOR_SIZENS);
      break;
    case svtkXYPlotWidget::Moving:
      this->RequestCursorShape(SVTK_CURSOR_SIZEALL);
      break;
  }
}

//-------------------------------------------------------------------------
void svtkXYPlotWidget::OnLeftButtonDown()
{
  // We're only here is we are enabled
  int X = this->Interactor->GetEventPosition()[0];
  int Y = this->Interactor->GetEventPosition()[1];

  // are we over the widget?
  // this->Interactor->FindPokedRenderer(X,Y);
  int* pos1 =
    this->XYPlotActor->GetPositionCoordinate()->GetComputedDisplayValue(this->CurrentRenderer);
  int* pos2 =
    this->XYPlotActor->GetPosition2Coordinate()->GetComputedDisplayValue(this->CurrentRenderer);

  // are we not over the xy plot, ignore
  if (X < pos1[0] || X > pos2[0] || Y < pos1[1] || Y > pos2[1])
  {
    return;
  }

  // start a drag, store the normalized view coords
  double X2 = X;
  double Y2 = Y;
  // convert to normalized viewport coordinates
  this->CurrentRenderer->DisplayToNormalizedDisplay(X2, Y2);
  this->CurrentRenderer->NormalizedDisplayToViewport(X2, Y2);
  this->CurrentRenderer->ViewportToNormalizedViewport(X2, Y2);
  this->StartPosition[0] = X2;
  this->StartPosition[1] = Y2;

  this->State = this->ComputeStateBasedOnPosition(X, Y, pos1, pos2);
  this->SetCursor(this->State);

  this->EventCallbackCommand->SetAbortFlag(1);
  this->StartInteraction();
  this->InvokeEvent(svtkCommand::StartInteractionEvent, nullptr);
}

//-------------------------------------------------------------------------
void svtkXYPlotWidget::OnMouseMove()
{
  // compute some info we need for all cases
  int X = this->Interactor->GetEventPosition()[0];
  int Y = this->Interactor->GetEventPosition()[1];

  // compute the display bounds of the xy plot if we are inside or outside
  int *pos1, *pos2;
  if (this->State == svtkXYPlotWidget::Outside || this->State == svtkXYPlotWidget::Inside)
  {
    pos1 =
      this->XYPlotActor->GetPositionCoordinate()->GetComputedDisplayValue(this->CurrentRenderer);
    pos2 =
      this->XYPlotActor->GetPosition2Coordinate()->GetComputedDisplayValue(this->CurrentRenderer);

    if (this->State == svtkXYPlotWidget::Outside)
    {
      // if we are not over the xy plot, ignore
      if (X < pos1[0] || X > pos2[0] || Y < pos1[1] || Y > pos2[1])
      {
        return;
      }
      // otherwise change our state to inside
      this->State = svtkXYPlotWidget::Inside;
    }

    // if inside, set the cursor to the correct shape
    if (this->State == svtkXYPlotWidget::Inside)
    {
      // if we have left then change cursor back to default
      if (X < pos1[0] || X > pos2[0] || Y < pos1[1] || Y > pos2[1])
      {
        this->State = svtkXYPlotWidget::Outside;
        this->RequestCursorShape(SVTK_CURSOR_DEFAULT);
        return;
      }
      // adjust the cursor based on our position
      this->SetCursor(this->ComputeStateBasedOnPosition(X, Y, pos1, pos2));
      return;
    }
  }

  double XF = X;
  double YF = Y;
  // convert to normalized viewport coordinates
  this->CurrentRenderer->DisplayToNormalizedDisplay(XF, YF);
  this->CurrentRenderer->NormalizedDisplayToViewport(XF, YF);
  this->CurrentRenderer->ViewportToNormalizedViewport(XF, YF);

  // there are four parameters that can be adjusted
  double* fpos1 = this->XYPlotActor->GetPositionCoordinate()->GetValue();
  double* fpos2 = this->XYPlotActor->GetPosition2Coordinate()->GetValue();
  float par1[2];
  float par2[2];
  par1[0] = fpos1[0];
  par1[1] = fpos1[1];
  par2[0] = fpos1[0] + fpos2[0];
  par2[1] = fpos1[1] + fpos2[1];

  // based on the state, adjust the xy plot parameters
  switch (this->State)
  {
    case svtkXYPlotWidget::AdjustingP1:
      par1[0] = par1[0] + XF - this->StartPosition[0];
      par1[1] = par1[1] + YF - this->StartPosition[1];
      break;
    case svtkXYPlotWidget::AdjustingP2:
      par2[0] = par2[0] + XF - this->StartPosition[0];
      par1[1] = par1[1] + YF - this->StartPosition[1];
      break;
    case svtkXYPlotWidget::AdjustingP3:
      par2[0] = par2[0] + XF - this->StartPosition[0];
      par2[1] = par2[1] + YF - this->StartPosition[1];
      break;
    case svtkXYPlotWidget::AdjustingP4:
      par1[0] = par1[0] + XF - this->StartPosition[0];
      par2[1] = par2[1] + YF - this->StartPosition[1];
      break;
    case svtkXYPlotWidget::AdjustingE1:
      par1[0] = par1[0] + XF - this->StartPosition[0];
      break;
    case svtkXYPlotWidget::AdjustingE2:
      par1[1] = par1[1] + YF - this->StartPosition[1];
      break;
    case svtkXYPlotWidget::AdjustingE3:
      par2[0] = par2[0] + XF - this->StartPosition[0];
      break;
    case svtkXYPlotWidget::AdjustingE4:
      par2[1] = par2[1] + YF - this->StartPosition[1];
      break;
    case svtkXYPlotWidget::Moving:
      // first apply the move
      par1[0] = par1[0] + XF - this->StartPosition[0];
      par1[1] = par1[1] + YF - this->StartPosition[1];
      par2[0] = par2[0] + XF - this->StartPosition[0];
      par2[1] = par2[1] + YF - this->StartPosition[1];
      // then check for an orientation change if the xy plot moves so that
      // its center is closer to a different edge that its current edge by
      // 0.2 then swap orientation
      float centerX = (par1[0] + par2[0]) / 2.0;
      float centerY = (par1[1] + par2[1]) / 2.0;
      // what edge is it closest to
      if (fabs(centerX - 0.5) > fabs(centerY - 0.5))
      {
        // is it far enough in to consider a change in orientation?
        if (fabs(centerX - 0.5) > 0.2 + fabs(centerY - 0.5))
        {
          // do we need to change orientation
          if (!this->XYPlotActor->GetExchangeAxes())
          {
            this->XYPlotActor->SetExchangeAxes(1);
            // also change the corners
            par2[0] = centerX + centerY - par1[1];
            par2[1] = centerY + centerX - par1[0];
            par1[0] = 2 * centerX - par2[0];
            par1[1] = 2 * centerY - par2[1];
          }
        }
      }
      else
      {
        // is it far enough in to consider a change in orientation?
        if (fabs(centerY - 0.5) > 0.2 + fabs(centerX - 0.5))
        {
          // do we need to change orientation
          if (this->XYPlotActor->GetExchangeAxes())
          {
            this->XYPlotActor->SetExchangeAxes(0);
            // also change the corners
            par2[0] = centerX + centerY - par1[1];
            par2[1] = centerY + centerX - par1[0];
            par1[0] = 2 * centerX - par2[0];
            par1[1] = 2 * centerY - par2[1];
          }
        }
      }
      break;
  }

  // push the change out to the xy plot
  // make sure the xy plot doesn't shrink to nothing
  if (par2[0] > par1[0] && par2[1] > par1[1])
  {
    this->XYPlotActor->GetPositionCoordinate()->SetValue(par1[0], par1[1]);
    this->XYPlotActor->GetPosition2Coordinate()->SetValue(par2[0] - par1[0], par2[1] - par1[1]);
    this->StartPosition[0] = XF;
    this->StartPosition[1] = YF;
  }

  // start a drag
  this->EventCallbackCommand->SetAbortFlag(1);
  this->InvokeEvent(svtkCommand::InteractionEvent, nullptr);
  this->Interactor->Render();
}

//-------------------------------------------------------------------------
void svtkXYPlotWidget::OnLeftButtonUp()
{
  if (this->State == svtkXYPlotWidget::Outside)
  {
    return;
  }

  // stop adjusting
  this->State = svtkXYPlotWidget::Outside;
  this->EventCallbackCommand->SetAbortFlag(1);
  this->RequestCursorShape(SVTK_CURSOR_DEFAULT);
  this->EndInteraction();
  this->InvokeEvent(svtkCommand::EndInteractionEvent, nullptr);
  this->Interactor->Render();
}

//-------------------------------------------------------------------------
void svtkXYPlotWidget::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "XYPlotActor: " << this->XYPlotActor << "\n";
}
