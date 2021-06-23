/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkInteractorStyleImage.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkInteractorStyleImage.h"

#include "svtkAbstractPropPicker.h"
#include "svtkAssemblyPath.h"
#include "svtkPropCollection.h"

#include "svtkCallbackCommand.h"
#include "svtkCamera.h"
#include "svtkImageMapper3D.h"
#include "svtkImageProperty.h"
#include "svtkImageSlice.h"
#include "svtkMath.h"
#include "svtkObjectFactory.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"

svtkStandardNewMacro(svtkInteractorStyleImage);

//----------------------------------------------------------------------------
svtkInteractorStyleImage::svtkInteractorStyleImage()
{
  this->WindowLevelStartPosition[0] = 0;
  this->WindowLevelStartPosition[1] = 0;

  this->WindowLevelCurrentPosition[0] = 0;
  this->WindowLevelCurrentPosition[1] = 0;

  this->WindowLevelInitial[0] = 1.0; // Window
  this->WindowLevelInitial[1] = 0.5; // Level

  this->CurrentImageProperty = nullptr;
  this->CurrentImageNumber = -1;

  this->InteractionMode = SVTKIS_IMAGE2D;

  this->XViewRightVector[0] = 0;
  this->XViewRightVector[1] = 1;
  this->XViewRightVector[2] = 0;

  this->XViewUpVector[0] = 0;
  this->XViewUpVector[1] = 0;
  this->XViewUpVector[2] = -1;

  this->YViewRightVector[0] = 1;
  this->YViewRightVector[1] = 0;
  this->YViewRightVector[2] = 0;

  this->YViewUpVector[0] = 0;
  this->YViewUpVector[1] = 0;
  this->YViewUpVector[2] = -1;

  this->ZViewRightVector[0] = 1;
  this->ZViewRightVector[1] = 0;
  this->ZViewRightVector[2] = 0;

  this->ZViewUpVector[0] = 0;
  this->ZViewUpVector[1] = 1;
  this->ZViewUpVector[2] = 0;
}

//----------------------------------------------------------------------------
svtkInteractorStyleImage::~svtkInteractorStyleImage()
{
  if (this->CurrentImageProperty)
  {
    this->CurrentImageProperty->Delete();
  }
}

//----------------------------------------------------------------------------
void svtkInteractorStyleImage::StartWindowLevel()
{
  if (this->State != SVTKIS_NONE)
  {
    return;
  }
  this->StartState(SVTKIS_WINDOW_LEVEL);

  // Get the last (the topmost) image
  this->SetCurrentImageNumber(this->CurrentImageNumber);

  if (this->HandleObservers && this->HasObserver(svtkCommand::StartWindowLevelEvent))
  {
    this->InvokeEvent(svtkCommand::StartWindowLevelEvent, this);
  }
  else
  {
    if (this->CurrentImageProperty)
    {
      svtkImageProperty* property = this->CurrentImageProperty;
      this->WindowLevelInitial[0] = property->GetColorWindow();
      this->WindowLevelInitial[1] = property->GetColorLevel();
    }
  }
}

//----------------------------------------------------------------------------
void svtkInteractorStyleImage::EndWindowLevel()
{
  if (this->State != SVTKIS_WINDOW_LEVEL)
  {
    return;
  }
  if (this->HandleObservers)
  {
    this->InvokeEvent(svtkCommand::EndWindowLevelEvent, this);
  }
  this->StopState();
}

//----------------------------------------------------------------------------
void svtkInteractorStyleImage::StartPick()
{
  if (this->State != SVTKIS_NONE)
  {
    return;
  }
  this->StartState(SVTKIS_PICK);
  if (this->HandleObservers)
  {
    this->InvokeEvent(svtkCommand::StartPickEvent, this);
  }
}

//----------------------------------------------------------------------------
void svtkInteractorStyleImage::EndPick()
{
  if (this->State != SVTKIS_PICK)
  {
    return;
  }
  if (this->HandleObservers)
  {
    this->InvokeEvent(svtkCommand::EndPickEvent, this);
  }
  this->StopState();
}

//----------------------------------------------------------------------------
void svtkInteractorStyleImage::StartSlice()
{
  if (this->State != SVTKIS_NONE)
  {
    return;
  }
  this->StartState(SVTKIS_SLICE);
}

//----------------------------------------------------------------------------
void svtkInteractorStyleImage::EndSlice()
{
  if (this->State != SVTKIS_SLICE)
  {
    return;
  }
  this->StopState();
}

//----------------------------------------------------------------------------
void svtkInteractorStyleImage::OnMouseMove()
{
  int x = this->Interactor->GetEventPosition()[0];
  int y = this->Interactor->GetEventPosition()[1];

  switch (this->State)
  {
    case SVTKIS_WINDOW_LEVEL:
      this->FindPokedRenderer(x, y);
      this->WindowLevel();
      this->InvokeEvent(svtkCommand::InteractionEvent, nullptr);
      break;

    case SVTKIS_PICK:
      this->FindPokedRenderer(x, y);
      this->Pick();
      this->InvokeEvent(svtkCommand::InteractionEvent, nullptr);
      break;

    case SVTKIS_SLICE:
      this->FindPokedRenderer(x, y);
      this->Slice();
      this->InvokeEvent(svtkCommand::InteractionEvent, nullptr);
      break;
  }

  // Call parent to handle all other states and perform additional work

  this->Superclass::OnMouseMove();
}

//----------------------------------------------------------------------------
void svtkInteractorStyleImage::OnLeftButtonDown()
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
    this->WindowLevelStartPosition[0] = x;
    this->WindowLevelStartPosition[1] = y;
    this->StartWindowLevel();
  }

  // If shift is held down, do a rotation
  else if (this->InteractionMode == SVTKIS_IMAGE3D && this->Interactor->GetShiftKey())
  {
    this->StartRotate();
  }

  // If ctrl is held down in slicing mode, slice the image
  else if (this->InteractionMode == SVTKIS_IMAGE_SLICING && this->Interactor->GetControlKey())
  {
    this->StartSlice();
  }

  // The rest of the button + key combinations remain the same

  else
  {
    this->Superclass::OnLeftButtonDown();
  }
}

//----------------------------------------------------------------------------
void svtkInteractorStyleImage::OnLeftButtonUp()
{
  switch (this->State)
  {
    case SVTKIS_WINDOW_LEVEL:
      this->EndWindowLevel();
      if (this->Interactor)
      {
        this->ReleaseFocus();
      }
      break;

    case SVTKIS_SLICE:
      this->EndSlice();
      if (this->Interactor)
      {
        this->ReleaseFocus();
      }
      break;
  }

  // Call parent to handle all other states and perform additional work

  this->Superclass::OnLeftButtonUp();
}

//----------------------------------------------------------------------------
void svtkInteractorStyleImage::OnMiddleButtonDown()
{
  this->FindPokedRenderer(
    this->Interactor->GetEventPosition()[0], this->Interactor->GetEventPosition()[1]);
  if (this->CurrentRenderer == nullptr)
  {
    return;
  }

  // If shift is held down, change the slice
  if ((this->InteractionMode == SVTKIS_IMAGE3D || this->InteractionMode == SVTKIS_IMAGE_SLICING) &&
    this->Interactor->GetShiftKey())
  {
    this->StartSlice();
  }

  // The rest of the button + key combinations remain the same

  else
  {
    this->Superclass::OnMiddleButtonDown();
  }
}

//----------------------------------------------------------------------------
void svtkInteractorStyleImage::OnMiddleButtonUp()
{
  switch (this->State)
  {
    case SVTKIS_SLICE:
      this->EndSlice();
      if (this->Interactor)
      {
        this->ReleaseFocus();
      }
      break;
  }

  // Call parent to handle all other states and perform additional work

  this->Superclass::OnMiddleButtonUp();
}

//----------------------------------------------------------------------------
void svtkInteractorStyleImage::OnRightButtonDown()
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
  if (this->Interactor->GetShiftKey())
  {
    this->StartPick();
  }

  else if (this->InteractionMode == SVTKIS_IMAGE3D && this->Interactor->GetControlKey())
  {
    this->StartSlice();
  }
  else if (this->InteractionMode == SVTKIS_IMAGE_SLICING && this->Interactor->GetControlKey())
  {
    this->StartSpin();
  }

  // The rest of the button + key combinations remain the same

  else
  {
    this->Superclass::OnRightButtonDown();
  }
}

//----------------------------------------------------------------------------
void svtkInteractorStyleImage::OnRightButtonUp()
{
  switch (this->State)
  {
    case SVTKIS_PICK:
      this->EndPick();
      if (this->Interactor)
      {
        this->ReleaseFocus();
      }
      break;

    case SVTKIS_SLICE:
      this->EndSlice();
      if (this->Interactor)
      {
        this->ReleaseFocus();
      }
      break;

    case SVTKIS_SPIN:
      if (this->Interactor)
      {
        this->EndSpin();
      }
      break;
  }

  // Call parent to handle all other states and perform additional work

  this->Superclass::OnRightButtonUp();
}

//----------------------------------------------------------------------------
void svtkInteractorStyleImage::OnChar()
{
  svtkRenderWindowInteractor* rwi = this->Interactor;

  switch (rwi->GetKeyCode())
  {
    case 'f':
    case 'F':
    {
      this->AnimState = SVTKIS_ANIM_ON;
      svtkAssemblyPath* path = nullptr;
      this->FindPokedRenderer(rwi->GetEventPosition()[0], rwi->GetEventPosition()[1]);
      rwi->GetPicker()->Pick(
        rwi->GetEventPosition()[0], rwi->GetEventPosition()[1], 0.0, this->CurrentRenderer);
      svtkAbstractPropPicker* picker;
      if ((picker = svtkAbstractPropPicker::SafeDownCast(rwi->GetPicker())))
      {
        path = picker->GetPath();
      }
      if (path != nullptr)
      {
        rwi->FlyToImage(this->CurrentRenderer, picker->GetPickPosition());
      }
      this->AnimState = SVTKIS_ANIM_OFF;
      break;
    }

    case 'r':
    case 'R':
      // Allow either shift/ctrl to trigger the usual 'r' binding
      // otherwise trigger reset window level event
      if (rwi->GetShiftKey() || rwi->GetControlKey())
      {
        this->Superclass::OnChar();
      }
      else if (this->HandleObservers && this->HasObserver(svtkCommand::ResetWindowLevelEvent))
      {
        this->InvokeEvent(svtkCommand::ResetWindowLevelEvent, this);
      }
      else if (this->CurrentImageProperty)
      {
        svtkImageProperty* property = this->CurrentImageProperty;
        property->SetColorWindow(this->WindowLevelInitial[0]);
        property->SetColorLevel(this->WindowLevelInitial[1]);
        this->Interactor->Render();
      }
      break;

    case 'x':
    case 'X':
    {
      this->SetImageOrientation(this->XViewRightVector, this->XViewUpVector);
      this->Interactor->Render();
    }
    break;

    case 'y':
    case 'Y':
    {
      this->SetImageOrientation(this->YViewRightVector, this->YViewUpVector);
      this->Interactor->Render();
    }
    break;

    case 'z':
    case 'Z':
    {
      this->SetImageOrientation(this->ZViewRightVector, this->ZViewUpVector);
      this->Interactor->Render();
    }
    break;

    default:
      this->Superclass::OnChar();
      break;
  }
}

//----------------------------------------------------------------------------
void svtkInteractorStyleImage::WindowLevel()
{
  svtkRenderWindowInteractor* rwi = this->Interactor;

  this->WindowLevelCurrentPosition[0] = rwi->GetEventPosition()[0];
  this->WindowLevelCurrentPosition[1] = rwi->GetEventPosition()[1];

  if (this->HandleObservers && this->HasObserver(svtkCommand::WindowLevelEvent))
  {
    this->InvokeEvent(svtkCommand::WindowLevelEvent, this);
  }
  else if (this->CurrentImageProperty)
  {
    const int* size = this->CurrentRenderer->GetSize();

    double window = this->WindowLevelInitial[0];
    double level = this->WindowLevelInitial[1];

    // Compute normalized delta

    double dx =
      (this->WindowLevelCurrentPosition[0] - this->WindowLevelStartPosition[0]) * 4.0 / size[0];
    double dy =
      (this->WindowLevelStartPosition[1] - this->WindowLevelCurrentPosition[1]) * 4.0 / size[1];

    // Scale by current values

    if (fabs(window) > 0.01)
    {
      dx = dx * window;
    }
    else
    {
      dx = dx * (window < 0 ? -0.01 : 0.01);
    }
    if (fabs(level) > 0.01)
    {
      dy = dy * level;
    }
    else
    {
      dy = dy * (level < 0 ? -0.01 : 0.01);
    }

    // Abs so that direction does not flip

    if (window < 0.0)
    {
      dx = -1 * dx;
    }
    if (level < 0.0)
    {
      dy = -1 * dy;
    }

    // Compute new window level

    double newWindow = dx + window;
    double newLevel = level - dy;

    if (newWindow < 0.01)
    {
      newWindow = 0.01;
    }

    this->CurrentImageProperty->SetColorWindow(newWindow);
    this->CurrentImageProperty->SetColorLevel(newLevel);

    this->Interactor->Render();
  }
}

//----------------------------------------------------------------------------
void svtkInteractorStyleImage::Pick()
{
  this->InvokeEvent(svtkCommand::PickEvent, this);
}

//----------------------------------------------------------------------------
void svtkInteractorStyleImage::Slice()
{
  if (this->CurrentRenderer == nullptr)
  {
    return;
  }

  svtkRenderWindowInteractor* rwi = this->Interactor;
  int dy = rwi->GetEventPosition()[1] - rwi->GetLastEventPosition()[1];

  svtkCamera* camera = this->CurrentRenderer->GetActiveCamera();
  double* range = camera->GetClippingRange();
  double distance = camera->GetDistance();

  // scale the interaction by the height of the viewport
  double viewportHeight = 0.0;
  if (camera->GetParallelProjection())
  {
    viewportHeight = camera->GetParallelScale();
  }
  else
  {
    double angle = svtkMath::RadiansFromDegrees(camera->GetViewAngle());
    viewportHeight = 2.0 * distance * tan(0.5 * angle);
  }

  const int* size = this->CurrentRenderer->GetSize();
  double delta = dy * viewportHeight / size[1];
  distance += delta;

  // clamp the distance to the clipping range
  if (distance < range[0])
  {
    distance = range[0] + viewportHeight * 1e-3;
  }
  if (distance > range[1])
  {
    distance = range[1] - viewportHeight * 1e-3;
  }
  camera->SetDistance(distance);

  rwi->Render();
}

//----------------------------------------------------------------------------
void svtkInteractorStyleImage::SetImageOrientation(
  const double leftToRight[3], const double viewUp[3])
{
  if (this->CurrentRenderer)
  {
    // the cross product points out of the screen
    double vector[3];
    svtkMath::Cross(leftToRight, viewUp, vector);
    double focus[3];
    svtkCamera* camera = this->CurrentRenderer->GetActiveCamera();
    camera->GetFocalPoint(focus);
    double d = camera->GetDistance();
    camera->SetPosition(
      focus[0] + d * vector[0], focus[1] + d * vector[1], focus[2] + d * vector[2]);
    camera->SetFocalPoint(focus);
    camera->SetViewUp(viewUp);
  }
}

//----------------------------------------------------------------------------
// This is a way of dealing with images as if they were layers.
// It looks through the renderer's list of props and sets the
// interactor ivars from the Nth image that it finds.  You can
// also use negative numbers, i.e. -1 will return the last image,
// -2 will return the second-to-last image, etc.
void svtkInteractorStyleImage::SetCurrentImageNumber(int i)
{
  this->CurrentImageNumber = i;

  if (!this->CurrentRenderer)
  {
    return;
  }

  svtkPropCollection* props = this->CurrentRenderer->GetViewProps();
  svtkProp* prop = nullptr;
  svtkAssemblyPath* path;
  svtkImageSlice* imageProp = nullptr;
  svtkCollectionSimpleIterator pit;

  for (int k = 0; k < 2; k++)
  {
    int j = 0;
    for (props->InitTraversal(pit); (prop = props->GetNextProp(pit));)
    {
      bool foundImageProp = false;
      for (prop->InitPathTraversal(); (path = prop->GetNextPath());)
      {
        svtkProp* tryProp = path->GetLastNode()->GetViewProp();
        imageProp = svtkImageSlice::SafeDownCast(tryProp);
        if (imageProp)
        {
          if (j == i && imageProp->GetPickable())
          {
            foundImageProp = true;
            break;
          }
          imageProp = nullptr;
          j++;
        }
      }
      if (foundImageProp)
      {
        break;
      }
    }
    if (i < 0)
    {
      i += j;
    }
  }

  svtkImageProperty* property = nullptr;
  if (imageProp)
  {
    property = imageProp->GetProperty();
  }

  if (property != this->CurrentImageProperty)
  {
    if (this->CurrentImageProperty)
    {
      this->CurrentImageProperty->Delete();
    }

    this->CurrentImageProperty = property;

    if (this->CurrentImageProperty)
    {
      this->CurrentImageProperty->Register(this);
    }
  }
}

//----------------------------------------------------------------------------
void svtkInteractorStyleImage::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Window Level Current Position: (" << this->WindowLevelCurrentPosition[0] << ", "
     << this->WindowLevelCurrentPosition[1] << ")\n";

  os << indent << "Window Level Start Position: (" << this->WindowLevelStartPosition[0] << ", "
     << this->WindowLevelStartPosition[1] << ")\n";

  os << indent << "Interaction Mode: ";
  if (this->InteractionMode == SVTKIS_IMAGE2D)
  {
    os << "Image2D\n";
  }
  else if (this->InteractionMode == SVTKIS_IMAGE3D)
  {
    os << "Image3D\n";
  }
  else if (this->InteractionMode == SVTKIS_IMAGE_SLICING)
  {
    os << "ImageSlicing\n";
  }
  else
  {
    os << "Unknown\n";
  }

  os << indent << "X View Right Vector: (" << this->XViewRightVector[0] << ", "
     << this->XViewRightVector[1] << ", " << this->XViewRightVector[2] << ")\n";

  os << indent << "X View Up Vector: (" << this->XViewUpVector[0] << ", " << this->XViewUpVector[1]
     << ", " << this->XViewUpVector[2] << ")\n";

  os << indent << "Y View Right Vector: (" << this->YViewRightVector[0] << ", "
     << this->YViewRightVector[1] << ", " << this->YViewRightVector[2] << ")\n";

  os << indent << "Y View Up Vector: (" << this->YViewUpVector[0] << ", " << this->YViewUpVector[1]
     << ", " << this->YViewUpVector[2] << ")\n";

  os << indent << "Z View Right Vector: (" << this->ZViewRightVector[0] << ", "
     << this->ZViewRightVector[1] << ", " << this->ZViewRightVector[2] << ")\n";

  os << indent << "Z View Up Vector: (" << this->ZViewUpVector[0] << ", " << this->ZViewUpVector[1]
     << ", " << this->ZViewUpVector[2] << ")\n";
}
