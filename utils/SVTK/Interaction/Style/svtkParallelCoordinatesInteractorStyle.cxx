/*=========================================================================

Program:   Visualization Toolkit
Module:    svtkParallelCoordinatesInteractorStyle.cxx

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2009 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
  -------------------------------------------------------------------------*/
#include "svtkParallelCoordinatesInteractorStyle.h"

#include "svtkAbstractPropPicker.h"
#include "svtkAssemblyPath.h"
#include "svtkCallbackCommand.h"
#include "svtkMath.h"
#include "svtkObjectFactory.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkViewport.h"

svtkStandardNewMacro(svtkParallelCoordinatesInteractorStyle);

//----------------------------------------------------------------------------
svtkParallelCoordinatesInteractorStyle::svtkParallelCoordinatesInteractorStyle()
{
  this->CursorStartPosition[0] = 0;
  this->CursorStartPosition[1] = 0;

  this->CursorCurrentPosition[0] = 0;
  this->CursorCurrentPosition[1] = 0;

  this->CursorLastPosition[0] = 0;
  this->CursorLastPosition[1] = 0;

  this->State = INTERACT_HOVER;
}

//----------------------------------------------------------------------------
svtkParallelCoordinatesInteractorStyle::~svtkParallelCoordinatesInteractorStyle() = default;
//----------------------------------------------------------------------------
void svtkParallelCoordinatesInteractorStyle::OnMouseMove()
{
  int x = this->Interactor->GetEventPosition()[0];
  int y = this->Interactor->GetEventPosition()[1];

  this->FindPokedRenderer(x, y);

  this->CursorLastPosition[0] = this->CursorCurrentPosition[0];
  this->CursorLastPosition[1] = this->CursorCurrentPosition[1];
  this->CursorCurrentPosition[0] = x;
  this->CursorCurrentPosition[1] = y;

  switch (this->State)
  {
    case INTERACT_HOVER:
      this->InvokeEvent(svtkCommand::InteractionEvent, nullptr);
      break;
    case INTERACT_INSPECT:
      this->Inspect(x, y);
      break;
    case INTERACT_ZOOM:
      this->Zoom();
      break;
    case INTERACT_PAN:
      this->Pan();
      break;
    default:
      // Call parent to handle all other states and perform additional work
      // This seems like it should always be called, but it's creating duplicate
      // interaction events to get invoked.
      this->Superclass::OnMouseMove();
      break;
  }
}

//----------------------------------------------------------------------------
void svtkParallelCoordinatesInteractorStyle::OnLeftButtonDown()
{
  int x = this->Interactor->GetEventPosition()[0];
  int y = this->Interactor->GetEventPosition()[1];

  this->FindPokedRenderer(x, y);
  if (this->CurrentRenderer == nullptr)
  {
    return;
  }

  // Redefine this button to handle window/level
  this->GrabFocus(this->EventCallbackCommand);

  if (!this->Interactor->GetShiftKey() && !this->Interactor->GetControlKey())
  {
    this->CursorStartPosition[0] = x;
    this->CursorStartPosition[1] = y;
    this->CursorLastPosition[0] = x;
    this->CursorLastPosition[1] = y;
    this->CursorCurrentPosition[0] = x;
    this->CursorCurrentPosition[1] = y;
    this->StartInspect(x, y); // ManipulateAxes(x,y);
  }
  /*  else if (!this->Interactor->GetControlKey())
      {
      this->CursorStartPosition[0] = x;
      this->CursorStartPosition[1] = y;
      this->CursorLastPosition[0] = x;
      this->CursorLastPosition[1] = y;
      this->CursorCurrentPosition[0] = x;
      this->CursorCurrentPosition[1] = y;
      this->StartSelectData(x,y);
      }*/
  else // The rest of the button + key combinations remain the same
  {
    this->Superclass::OnLeftButtonDown();
  }
}

//----------------------------------------------------------------------------
void svtkParallelCoordinatesInteractorStyle::OnLeftButtonUp()
{
  if (this->State == INTERACT_INSPECT)
  {
    this->EndInspect(); // ManipulateAxes();

    if (this->Interactor)
    {
      this->ReleaseFocus();
    }
  }
  /*  else if (this->State == INTERACT_SELECT_DATA)
      {
      this->EndSelectData();

      if ( this->Interactor )
      {
      this->ReleaseFocus();
      }
      } */

  // Call parent to handle all other states and perform additional work
  this->Superclass::OnLeftButtonUp();
}

//----------------------------------------------------------------------------
void svtkParallelCoordinatesInteractorStyle::OnMiddleButtonDown()
{
  int x = this->Interactor->GetEventPosition()[0];
  int y = this->Interactor->GetEventPosition()[1];

  this->FindPokedRenderer(x, y);
  if (this->CurrentRenderer == nullptr)
  {
    return;
  }

  // Redefine this button + shift to handle pick
  this->GrabFocus(this->EventCallbackCommand);

  if (!this->Interactor->GetShiftKey() && !this->Interactor->GetControlKey())
  {
    this->CursorStartPosition[0] = x;
    this->CursorStartPosition[1] = y;
    this->CursorLastPosition[0] = x;
    this->CursorLastPosition[1] = y;
    this->CursorCurrentPosition[0] = x;
    this->CursorCurrentPosition[1] = y;
    this->StartPan();
  }
  // The rest of the button + key combinations remain the same
  else
  {
    this->Superclass::OnMiddleButtonDown();
  }
}

//----------------------------------------------------------------------------
void svtkParallelCoordinatesInteractorStyle::OnMiddleButtonUp()
{
  if (this->State == INTERACT_PAN)
  {
    this->EndPan();
    if (this->Interactor)
    {
      this->ReleaseFocus();
    }
  }

  // Call parent to handle all other states and perform additional work

  this->Superclass::OnMiddleButtonUp();
}

//----------------------------------------------------------------------------
void svtkParallelCoordinatesInteractorStyle::OnRightButtonDown()
{
  int x = this->Interactor->GetEventPosition()[0];
  int y = this->Interactor->GetEventPosition()[1];

  this->FindPokedRenderer(x, y);
  if (this->CurrentRenderer == nullptr)
  {
    return;
  }

  // Redefine this button + shift to handle pick
  this->GrabFocus(this->EventCallbackCommand);

  if (!this->Interactor->GetShiftKey() && !this->Interactor->GetControlKey())
  {
    this->CursorStartPosition[0] = x;
    this->CursorStartPosition[1] = y;
    this->CursorLastPosition[0] = x;
    this->CursorLastPosition[1] = y;
    this->CursorCurrentPosition[0] = x;
    this->CursorCurrentPosition[1] = y;
    this->StartZoom();
  }
  // The rest of the button + key combinations remain the same
  else
  {
    this->Superclass::OnRightButtonDown();
  }
}

//----------------------------------------------------------------------------
void svtkParallelCoordinatesInteractorStyle::OnRightButtonUp()
{
  if (this->State == INTERACT_ZOOM)
  {
    this->EndZoom();
    if (this->Interactor)
    {
      this->ReleaseFocus();
    }
  }

  // Call parent to handle all other states and perform additional work

  this->Superclass::OnRightButtonUp();
}

//----------------------------------------------------------------------------
void svtkParallelCoordinatesInteractorStyle::OnLeave()
{
  int x = this->Interactor->GetEventPosition()[0];
  int y = this->Interactor->GetEventPosition()[1];

  this->FindPokedRenderer(x, y);

  this->CursorLastPosition[0] = this->CursorCurrentPosition[0];
  this->CursorLastPosition[1] = this->CursorCurrentPosition[1];
  this->CursorCurrentPosition[0] = x;
  this->CursorCurrentPosition[1] = y;

  switch (this->State)
  {
    case INTERACT_HOVER:
      this->InvokeEvent(svtkCommand::InteractionEvent, nullptr);
      break;
    case INTERACT_INSPECT:
      this->Inspect(x, y);
      break;
    case INTERACT_ZOOM:
      this->Zoom();
      break;
    case INTERACT_PAN:
      this->Pan();
      break;
    default:
      // Call parent to handle all other states and perform additional work
      // This seems like it should always be called, but it's creating duplicate
      // interaction events to get invoked.
      this->Superclass::OnLeave();
      break;
  }
}

//----------------------------------------------------------------------------
void svtkParallelCoordinatesInteractorStyle::OnChar()
{
  svtkRenderWindowInteractor* rwi = this->Interactor;

  switch (rwi->GetKeyCode())
  {
    case 'f':
    case 'F':
      break;

    case 'r':
    case 'R':
      this->InvokeEvent(svtkCommand::UpdateEvent, nullptr);
      break;

    default:
      this->Superclass::OnChar();
      break;
  }
}

//----------------------------------------------------------------------------
void svtkParallelCoordinatesInteractorStyle::StartInspect(int svtkNotUsed(x), int svtkNotUsed(y))
{
  this->State = INTERACT_INSPECT;
  this->InvokeEvent(svtkCommand::StartInteractionEvent, nullptr);
}

//----------------------------------------------------------------------------
void svtkParallelCoordinatesInteractorStyle::Inspect(int svtkNotUsed(x), int svtkNotUsed(y))
{
  this->InvokeEvent(svtkCommand::InteractionEvent, nullptr);
}

//----------------------------------------------------------------------------
void svtkParallelCoordinatesInteractorStyle::EndInspect()
{
  this->InvokeEvent(svtkCommand::EndInteractionEvent, nullptr);
  this->State = INTERACT_HOVER;
}

//----------------------------------------------------------------------------
void svtkParallelCoordinatesInteractorStyle::StartZoom()
{
  this->State = INTERACT_ZOOM;
  this->InvokeEvent(svtkCommand::StartInteractionEvent, nullptr);
}

//----------------------------------------------------------------------------
void svtkParallelCoordinatesInteractorStyle::Zoom()
{
  this->InvokeEvent(svtkCommand::InteractionEvent, nullptr);
}

//----------------------------------------------------------------------------
void svtkParallelCoordinatesInteractorStyle::EndZoom()
{
  this->InvokeEvent(svtkCommand::EndInteractionEvent, nullptr);
  this->State = INTERACT_HOVER;
}

//----------------------------------------------------------------------------
void svtkParallelCoordinatesInteractorStyle::StartPan()
{
  this->State = INTERACT_PAN;
  this->InvokeEvent(svtkCommand::StartInteractionEvent, nullptr);
}

//----------------------------------------------------------------------------
void svtkParallelCoordinatesInteractorStyle::Pan()
{
  this->InvokeEvent(svtkCommand::InteractionEvent, nullptr);
}

//----------------------------------------------------------------------------
void svtkParallelCoordinatesInteractorStyle::EndPan()
{
  this->InvokeEvent(svtkCommand::EndInteractionEvent, nullptr);
  this->State = INTERACT_HOVER;
}

//----------------------------------------------------------------------------
void svtkParallelCoordinatesInteractorStyle::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Cursor Current Position: (" << this->CursorCurrentPosition[0] << ", "
     << this->CursorCurrentPosition[1] << ")" << endl;

  os << indent << "Cursor Start Position: (" << this->CursorStartPosition[0] << ", "
     << this->CursorStartPosition[1] << ")" << endl;

  os << indent << "Cursor Last Position: (" << this->CursorLastPosition[0] << ", "
     << this->CursorLastPosition[1] << ")" << endl;
}
void svtkParallelCoordinatesInteractorStyle::GetCursorStartPosition(
  svtkViewport* viewport, double pos[2])
{
  const int* size = viewport->GetSize();
  pos[0] = static_cast<double>(this->CursorStartPosition[0]) / size[0];
  pos[1] = static_cast<double>(this->CursorStartPosition[1]) / size[1];
}

void svtkParallelCoordinatesInteractorStyle::GetCursorCurrentPosition(
  svtkViewport* viewport, double pos[2])
{
  const int* size = viewport->GetSize();
  pos[0] = static_cast<double>(this->CursorCurrentPosition[0]) / size[0];
  pos[1] = static_cast<double>(this->CursorCurrentPosition[1]) / size[1];
}

void svtkParallelCoordinatesInteractorStyle::GetCursorLastPosition(
  svtkViewport* viewport, double pos[2])
{
  const int* size = viewport->GetSize();
  pos[0] = static_cast<double>(this->CursorLastPosition[0]) / size[0];
  pos[1] = static_cast<double>(this->CursorLastPosition[1]) / size[1];
}
