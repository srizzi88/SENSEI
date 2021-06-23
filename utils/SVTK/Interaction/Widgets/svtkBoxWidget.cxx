/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkBoxWidget.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkBoxWidget.h"

#include "svtkActor.h"
#include "svtkAssemblyNode.h"
#include "svtkAssemblyPath.h"
#include "svtkCallbackCommand.h"
#include "svtkCamera.h"
#include "svtkCellArray.h"
#include "svtkCellPicker.h"
#include "svtkDoubleArray.h"
#include "svtkFloatArray.h"
#include "svtkMath.h"
#include "svtkObjectFactory.h"
#include "svtkPickingManager.h"
#include "svtkPlanes.h"
#include "svtkPolyData.h"
#include "svtkPolyDataMapper.h"
#include "svtkProperty.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkSphereSource.h"
#include "svtkTransform.h"

svtkStandardNewMacro(svtkBoxWidget);

svtkBoxWidget::svtkBoxWidget()
{
  this->State = svtkBoxWidget::Start;
  this->EventCallbackCommand->SetCallback(svtkBoxWidget::ProcessEvents);

  // Enable/disable the translation, rotation, and scaling of the widget
  this->TranslationEnabled = 1;
  this->RotationEnabled = 1;
  this->ScalingEnabled = 1;

  // Build the representation of the widget
  int i;

  // Control orientation of normals
  this->InsideOut = 0;
  this->OutlineFaceWires = 0;
  this->OutlineCursorWires = 1;

  // Construct the poly data representing the hex
  this->HexPolyData = svtkPolyData::New();
  this->HexMapper = svtkPolyDataMapper::New();
  this->HexMapper->SetInputData(HexPolyData);
  this->HexActor = svtkActor::New();
  this->HexActor->SetMapper(this->HexMapper);

  // Construct initial points
  this->Points = svtkPoints::New(SVTK_DOUBLE);
  this->Points->SetNumberOfPoints(15); // 8 corners; 6 faces; 1 center
  this->HexPolyData->SetPoints(this->Points);

  // Construct connectivity for the faces. These are used to perform
  // the picking.
  svtkIdType pts[4];
  svtkCellArray* cells = svtkCellArray::New();
  cells->AllocateEstimate(6, 4);
  pts[0] = 3;
  pts[1] = 0;
  pts[2] = 4;
  pts[3] = 7;
  cells->InsertNextCell(4, pts);
  pts[0] = 1;
  pts[1] = 2;
  pts[2] = 6;
  pts[3] = 5;
  cells->InsertNextCell(4, pts);
  pts[0] = 0;
  pts[1] = 1;
  pts[2] = 5;
  pts[3] = 4;
  cells->InsertNextCell(4, pts);
  pts[0] = 2;
  pts[1] = 3;
  pts[2] = 7;
  pts[3] = 6;
  cells->InsertNextCell(4, pts);
  pts[0] = 0;
  pts[1] = 3;
  pts[2] = 2;
  pts[3] = 1;
  cells->InsertNextCell(4, pts);
  pts[0] = 4;
  pts[1] = 5;
  pts[2] = 6;
  pts[3] = 7;
  cells->InsertNextCell(4, pts);
  this->HexPolyData->SetPolys(cells);
  cells->Delete();
  this->HexPolyData->BuildCells();

  // The face of the hexahedra
  cells = svtkCellArray::New();
  cells->AllocateEstimate(1, 4);
  cells->InsertNextCell(4, pts); // temporary, replaced later
  this->HexFacePolyData = svtkPolyData::New();
  this->HexFacePolyData->SetPoints(this->Points);
  this->HexFacePolyData->SetPolys(cells);
  this->HexFaceMapper = svtkPolyDataMapper::New();
  this->HexFaceMapper->SetInputData(HexFacePolyData);
  this->HexFace = svtkActor::New();
  this->HexFace->SetMapper(this->HexFaceMapper);
  cells->Delete();

  // Create the outline for the hex
  this->OutlinePolyData = svtkPolyData::New();
  this->OutlinePolyData->SetPoints(this->Points);
  this->OutlineMapper = svtkPolyDataMapper::New();
  this->OutlineMapper->SetInputData(this->OutlinePolyData);
  this->HexOutline = svtkActor::New();
  this->HexOutline->SetMapper(this->OutlineMapper);
  cells = svtkCellArray::New();
  cells->AllocateEstimate(15, 2);
  this->OutlinePolyData->SetLines(cells);
  cells->Delete();

  // Set up the initial properties
  this->CreateDefaultProperties();

  // Create the outline
  this->GenerateOutline();

  // Create the handles
  this->Handle = new svtkActor*[7];
  this->HandleMapper = new svtkPolyDataMapper*[7];
  this->HandleGeometry = new svtkSphereSource*[7];
  for (i = 0; i < 7; i++)
  {
    this->HandleGeometry[i] = svtkSphereSource::New();
    this->HandleGeometry[i]->SetThetaResolution(16);
    this->HandleGeometry[i]->SetPhiResolution(8);
    this->HandleMapper[i] = svtkPolyDataMapper::New();
    this->HandleMapper[i]->SetInputConnection(this->HandleGeometry[i]->GetOutputPort());
    this->Handle[i] = svtkActor::New();
    this->Handle[i]->SetMapper(this->HandleMapper[i]);
  }

  // Define the point coordinates
  double bounds[6];
  bounds[0] = -0.5;
  bounds[1] = 0.5;
  bounds[2] = -0.5;
  bounds[3] = 0.5;
  bounds[4] = -0.5;
  bounds[5] = 0.5;
  // Points 8-14 are down by PositionHandles();
  this->PlaceWidget(bounds);

  // Manage the picking stuff
  this->HandlePicker = svtkCellPicker::New();
  this->HandlePicker->SetTolerance(0.001);
  for (i = 0; i < 7; i++)
  {
    this->HandlePicker->AddPickList(this->Handle[i]);
  }
  this->HandlePicker->PickFromListOn();

  this->HexPicker = svtkCellPicker::New();
  this->HexPicker->SetTolerance(0.001);
  this->HexPicker->AddPickList(HexActor);
  this->HexPicker->PickFromListOn();

  this->CurrentHandle = nullptr;

  this->Transform = svtkTransform::New();
}

svtkBoxWidget::~svtkBoxWidget()
{
  this->HexActor->Delete();
  this->HexMapper->Delete();
  this->HexPolyData->Delete();
  this->Points->Delete();

  this->HexFace->Delete();
  this->HexFaceMapper->Delete();
  this->HexFacePolyData->Delete();

  this->HexOutline->Delete();
  this->OutlineMapper->Delete();
  this->OutlinePolyData->Delete();

  for (int i = 0; i < 7; i++)
  {
    this->HandleGeometry[i]->Delete();
    this->HandleMapper[i]->Delete();
    this->Handle[i]->Delete();
  }
  delete[] this->Handle;
  delete[] this->HandleMapper;
  delete[] this->HandleGeometry;

  this->HandlePicker->Delete();
  this->HexPicker->Delete();

  this->Transform->Delete();

  this->HandleProperty->Delete();
  this->SelectedHandleProperty->Delete();
  this->FaceProperty->Delete();
  this->SelectedFaceProperty->Delete();
  this->OutlineProperty->Delete();
  this->SelectedOutlineProperty->Delete();
}

void svtkBoxWidget::SetEnabled(int enabling)
{
  if (!this->Interactor)
  {
    svtkErrorMacro(<< "The interactor must be set prior to enabling/disabling widget");
    return;
  }

  if (enabling) //------------------------------------------------------------
  {
    svtkDebugMacro(<< "Enabling widget");

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

    // listen to the following events
    svtkRenderWindowInteractor* i = this->Interactor;
    i->AddObserver(svtkCommand::MouseMoveEvent, this->EventCallbackCommand, this->Priority);
    i->AddObserver(svtkCommand::LeftButtonPressEvent, this->EventCallbackCommand, this->Priority);
    i->AddObserver(svtkCommand::LeftButtonReleaseEvent, this->EventCallbackCommand, this->Priority);
    i->AddObserver(svtkCommand::MiddleButtonPressEvent, this->EventCallbackCommand, this->Priority);
    i->AddObserver(
      svtkCommand::MiddleButtonReleaseEvent, this->EventCallbackCommand, this->Priority);
    i->AddObserver(svtkCommand::RightButtonPressEvent, this->EventCallbackCommand, this->Priority);
    i->AddObserver(svtkCommand::RightButtonReleaseEvent, this->EventCallbackCommand, this->Priority);

    // Add the various actors
    // Add the outline
    this->CurrentRenderer->AddActor(this->HexActor);
    this->CurrentRenderer->AddActor(this->HexOutline);
    this->HexActor->SetProperty(this->OutlineProperty);
    this->HexOutline->SetProperty(this->OutlineProperty);

    // Add the hex face
    this->CurrentRenderer->AddActor(this->HexFace);
    this->HexFace->SetProperty(this->FaceProperty);

    // turn on the handles
    for (int j = 0; j < 7; j++)
    {
      this->CurrentRenderer->AddActor(this->Handle[j]);
      this->Handle[j]->SetProperty(this->HandleProperty);
    }
    this->RegisterPickers();

    this->InvokeEvent(svtkCommand::EnableEvent, nullptr);
  }

  else // disabling-------------------------------------------------------------
  {
    svtkDebugMacro(<< "Disabling widget");

    if (!this->Enabled) // already disabled, just return
    {
      return;
    }

    this->Enabled = 0;

    // don't listen for events any more
    this->Interactor->RemoveObserver(this->EventCallbackCommand);

    // turn off the outline
    this->CurrentRenderer->RemoveActor(this->HexActor);
    this->CurrentRenderer->RemoveActor(this->HexOutline);

    // turn off the hex face
    this->CurrentRenderer->RemoveActor(this->HexFace);

    // turn off the handles
    for (int i = 0; i < 7; i++)
    {
      this->CurrentRenderer->RemoveActor(this->Handle[i]);
    }

    this->CurrentHandle = nullptr;
    this->InvokeEvent(svtkCommand::DisableEvent, nullptr);
    this->SetCurrentRenderer(nullptr);
    this->UnRegisterPickers();
  }

  this->Interactor->Render();
}

void svtkBoxWidget::ProcessEvents(
  svtkObject* svtkNotUsed(object), unsigned long event, void* clientdata, void* svtkNotUsed(calldata))
{
  svtkBoxWidget* self = reinterpret_cast<svtkBoxWidget*>(clientdata);

  // okay, let's do the right thing
  switch (event)
  {
    case svtkCommand::LeftButtonPressEvent:
      self->OnLeftButtonDown();
      break;
    case svtkCommand::LeftButtonReleaseEvent:
      self->OnLeftButtonUp();
      break;
    case svtkCommand::MiddleButtonPressEvent:
      self->OnMiddleButtonDown();
      break;
    case svtkCommand::MiddleButtonReleaseEvent:
      self->OnMiddleButtonUp();
      break;
    case svtkCommand::RightButtonPressEvent:
      self->OnRightButtonDown();
      break;
    case svtkCommand::RightButtonReleaseEvent:
      self->OnRightButtonUp();
      break;
    case svtkCommand::MouseMoveEvent:
      self->OnMouseMove();
      break;
  }
}

void svtkBoxWidget::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  double* bounds = this->InitialBounds;
  os << indent << "Initial Bounds: "
     << "(" << bounds[0] << "," << bounds[1] << ") "
     << "(" << bounds[2] << "," << bounds[3] << ") "
     << "(" << bounds[4] << "," << bounds[5] << ")\n";

  if (this->HandleProperty)
  {
    os << indent << "Handle Property: " << this->HandleProperty << "\n";
  }
  else
  {
    os << indent << "Handle Property: (none)\n";
  }
  if (this->SelectedHandleProperty)
  {
    os << indent << "Selected Handle Property: " << this->SelectedHandleProperty << "\n";
  }
  else
  {
    os << indent << "SelectedHandle Property: (none)\n";
  }

  if (this->FaceProperty)
  {
    os << indent << "Face Property: " << this->FaceProperty << "\n";
  }
  else
  {
    os << indent << "Face Property: (none)\n";
  }
  if (this->SelectedFaceProperty)
  {
    os << indent << "Selected Face Property: " << this->SelectedFaceProperty << "\n";
  }
  else
  {
    os << indent << "Selected Face Property: (none)\n";
  }

  if (this->OutlineProperty)
  {
    os << indent << "Outline Property: " << this->OutlineProperty << "\n";
  }
  else
  {
    os << indent << "Outline Property: (none)\n";
  }
  if (this->SelectedOutlineProperty)
  {
    os << indent << "Selected Outline Property: " << this->SelectedOutlineProperty << "\n";
  }
  else
  {
    os << indent << "Selected Outline Property: (none)\n";
  }

  os << indent << "Outline Face Wires: " << (this->OutlineFaceWires ? "On\n" : "Off\n");
  os << indent << "Outline Cursor Wires: " << (this->OutlineCursorWires ? "On\n" : "Off\n");
  os << indent << "Inside Out: " << (this->InsideOut ? "On\n" : "Off\n");
  os << indent << "Translation Enabled: " << (this->TranslationEnabled ? "On\n" : "Off\n");
  os << indent << "Scaling Enabled: " << (this->ScalingEnabled ? "On\n" : "Off\n");
  os << indent << "Rotation Enabled: " << (this->RotationEnabled ? "On\n" : "Off\n");
}

#define SVTK_AVERAGE(a, b, c)                                                                       \
  c[0] = (a[0] + b[0]) / 2.0;                                                                      \
  c[1] = (a[1] + b[1]) / 2.0;                                                                      \
  c[2] = (a[2] + b[2]) / 2.0;

void svtkBoxWidget::PositionHandles()
{
  double* pts = static_cast<svtkDoubleArray*>(this->Points->GetData())->GetPointer(0);
  double* p0 = pts;
  double* p1 = pts + 3 * 1;
  double* p2 = pts + 3 * 2;
  double* p3 = pts + 3 * 3;
  // double *p4 = pts + 3*4;
  double* p5 = pts + 3 * 5;
  double* p6 = pts + 3 * 6;
  double* p7 = pts + 3 * 7;
  double x[3];

  SVTK_AVERAGE(p0, p7, x);
  this->Points->SetPoint(8, x);
  SVTK_AVERAGE(p1, p6, x);
  this->Points->SetPoint(9, x);
  SVTK_AVERAGE(p0, p5, x);
  this->Points->SetPoint(10, x);
  SVTK_AVERAGE(p2, p7, x);
  this->Points->SetPoint(11, x);
  SVTK_AVERAGE(p1, p3, x);
  this->Points->SetPoint(12, x);
  SVTK_AVERAGE(p5, p7, x);
  this->Points->SetPoint(13, x);
  SVTK_AVERAGE(p0, p6, x);
  this->Points->SetPoint(14, x);

  int i;
  for (i = 0; i < 7; ++i)
  {
    this->HandleGeometry[i]->SetCenter(this->Points->GetPoint(8 + i));
  }

  this->Points->GetData()->Modified();
  this->HexFacePolyData->Modified();
  this->HexPolyData->Modified();
  this->GenerateOutline();
}
#undef SVTK_AVERAGE

void svtkBoxWidget::HandlesOn()
{
  for (int i = 0; i < 7; i++)
  {
    this->Handle[i]->VisibilityOn();
  }
}

void svtkBoxWidget::HandlesOff()
{
  for (int i = 0; i < 7; i++)
  {
    this->Handle[i]->VisibilityOff();
  }
}

void svtkBoxWidget::SizeHandles()
{
  double radius = this->svtk3DWidget::SizeHandles(1.5);
  for (int i = 0; i < 7; i++)
  {
    this->HandleGeometry[i]->SetRadius(radius);
  }
}

int svtkBoxWidget::HighlightHandle(svtkProp* prop)
{
  // first unhighlight anything picked
  this->HighlightOutline(0);
  if (this->CurrentHandle)
  {
    this->CurrentHandle->SetProperty(this->HandleProperty);
  }

  this->CurrentHandle = static_cast<svtkActor*>(prop);

  if (this->CurrentHandle)
  {
    this->CurrentHandle->SetProperty(this->SelectedHandleProperty);
    for (int i = 0; i < 6; i++) // find attached face
    {
      if (this->CurrentHandle == this->Handle[i])
      {
        return i;
      }
    }
  }

  if (this->CurrentHandle == this->Handle[6])
  {
    this->HighlightOutline(1);
  }

  return -1;
}

void svtkBoxWidget::HighlightFace(int cellId)
{
  if (cellId >= 0)
  {
    svtkIdType npts;
    const svtkIdType* pts;
    svtkCellArray* cells = this->HexFacePolyData->GetPolys();
    this->HexPolyData->GetCellPoints(cellId, npts, pts);
    cells->ReplaceCellAtId(0, npts, pts);
    cells->Modified();
    this->CurrentHexFace = cellId;
    this->HexFace->SetProperty(this->SelectedFaceProperty);
    if (!this->CurrentHandle)
    {
      this->CurrentHandle = this->HexFace;
    }
  }
  else
  {
    this->HexFace->SetProperty(this->FaceProperty);
    this->CurrentHexFace = -1;
  }
}

void svtkBoxWidget::HighlightOutline(int highlight)
{
  if (highlight)
  {
    this->HexActor->SetProperty(this->SelectedOutlineProperty);
    this->HexOutline->SetProperty(this->SelectedOutlineProperty);
  }
  else
  {
    this->HexActor->SetProperty(this->OutlineProperty);
    this->HexOutline->SetProperty(this->OutlineProperty);
  }
}

void svtkBoxWidget::OnLeftButtonDown()
{
  int X = this->Interactor->GetEventPosition()[0];
  int Y = this->Interactor->GetEventPosition()[1];

  // Okay, we can process this. Try to pick handles first;
  // if no handles picked, then pick the bounding box.
  if (!this->CurrentRenderer || !this->CurrentRenderer->IsInViewport(X, Y))
  {
    this->State = svtkBoxWidget::Outside;
    return;
  }

  svtkAssemblyPath* path = this->GetAssemblyPath(X, Y, 0., this->HandlePicker);

  if (path != nullptr)
  {
    this->State = svtkBoxWidget::Moving;
    this->HighlightFace(this->HighlightHandle(path->GetFirstNode()->GetViewProp()));
    this->HandlePicker->GetPickPosition(this->LastPickPosition);
    this->ValidPick = 1;
  }
  else
  {
    path = this->GetAssemblyPath(X, Y, 0., this->HexPicker);

    if (path != nullptr)
    {
      this->State = svtkBoxWidget::Moving;
      this->HexPicker->GetPickPosition(this->LastPickPosition);
      this->ValidPick = 1;
      if (!this->Interactor->GetShiftKey())
      {
        this->HighlightHandle(nullptr);
        this->HighlightFace(this->HexPicker->GetCellId());
      }
      else
      {
        this->CurrentHandle = this->Handle[6];
        this->HighlightOutline(1);
      }
    }
    else
    {
      this->HighlightFace(this->HighlightHandle(nullptr));
      this->State = svtkBoxWidget::Outside;
      return;
    }
  }

  this->EventCallbackCommand->SetAbortFlag(1);
  this->StartInteraction();
  this->InvokeEvent(svtkCommand::StartInteractionEvent, nullptr);
  this->Interactor->Render();
}

void svtkBoxWidget::OnLeftButtonUp()
{
  if (this->State == svtkBoxWidget::Outside || this->State == svtkBoxWidget::Start)
  {
    return;
  }

  this->State = svtkBoxWidget::Start;
  this->HighlightFace(this->HighlightHandle(nullptr));
  this->SizeHandles();

  this->EventCallbackCommand->SetAbortFlag(1);
  this->EndInteraction();
  this->InvokeEvent(svtkCommand::EndInteractionEvent, nullptr);
  this->Interactor->Render();
}

void svtkBoxWidget::OnMiddleButtonDown()
{
  int X = this->Interactor->GetEventPosition()[0];
  int Y = this->Interactor->GetEventPosition()[1];

  // Okay, we can process this. Try to pick handles first;
  // if no handles picked, then pick the bounding box.
  if (!this->CurrentRenderer || !this->CurrentRenderer->IsInViewport(X, Y))
  {
    this->State = svtkBoxWidget::Outside;
    return;
  }

  svtkAssemblyPath* path = this->GetAssemblyPath(X, Y, 0., this->HandlePicker);

  if (path != nullptr)
  {
    this->State = svtkBoxWidget::Moving;
    this->CurrentHandle = this->Handle[6];
    this->HighlightOutline(1);
    this->HandlePicker->GetPickPosition(this->LastPickPosition);
    this->ValidPick = 1;
  }
  else
  {
    path = this->GetAssemblyPath(X, Y, 0., this->HexPicker);

    if (path != nullptr)
    {
      this->State = svtkBoxWidget::Moving;
      this->CurrentHandle = this->Handle[6];
      this->HighlightOutline(1);
      this->HexPicker->GetPickPosition(this->LastPickPosition);
      this->ValidPick = 1;
    }
    else
    {
      this->HighlightFace(this->HighlightHandle(nullptr));
      this->State = svtkBoxWidget::Outside;
      return;
    }
  }

  this->EventCallbackCommand->SetAbortFlag(1);
  this->StartInteraction();
  this->InvokeEvent(svtkCommand::StartInteractionEvent, nullptr);
  this->Interactor->Render();
}

void svtkBoxWidget::OnMiddleButtonUp()
{
  if (this->State == svtkBoxWidget::Outside || this->State == svtkBoxWidget::Start)
  {
    return;
  }

  this->State = svtkBoxWidget::Start;
  this->HighlightFace(this->HighlightHandle(nullptr));
  this->SizeHandles();

  this->EventCallbackCommand->SetAbortFlag(1);
  this->EndInteraction();
  this->InvokeEvent(svtkCommand::EndInteractionEvent, nullptr);
  this->Interactor->Render();
}

void svtkBoxWidget::OnRightButtonDown()
{
  int X = this->Interactor->GetEventPosition()[0];
  int Y = this->Interactor->GetEventPosition()[1];

  // Okay, we can process this. Try to pick handles first;
  // if no handles picked, then pick the bounding box.
  if (!this->CurrentRenderer || !this->CurrentRenderer->IsInViewport(X, Y))
  {
    this->State = svtkBoxWidget::Outside;
    return;
  }

  svtkAssemblyPath* path = this->GetAssemblyPath(X, Y, 0., this->HandlePicker);

  if (path != nullptr)
  {
    this->State = svtkBoxWidget::Scaling;
    this->HighlightOutline(1);
    this->HandlePicker->GetPickPosition(this->LastPickPosition);
    this->ValidPick = 1;
  }
  else
  {
    path = this->GetAssemblyPath(X, Y, 0., this->HexPicker);

    if (path != nullptr)
    {
      this->State = svtkBoxWidget::Scaling;
      this->HighlightOutline(1);
      this->HexPicker->GetPickPosition(this->LastPickPosition);
      this->ValidPick = 1;
    }
    else
    {
      this->State = svtkBoxWidget::Outside;
      return;
    }
  }

  this->EventCallbackCommand->SetAbortFlag(1);
  this->StartInteraction();
  this->InvokeEvent(svtkCommand::StartInteractionEvent, nullptr);
  this->Interactor->Render();
}

void svtkBoxWidget::OnRightButtonUp()
{
  if (this->State == svtkBoxWidget::Outside)
  {
    return;
  }

  this->State = svtkBoxWidget::Start;
  this->HighlightOutline(0);
  this->SizeHandles();

  this->EventCallbackCommand->SetAbortFlag(1);
  this->EndInteraction();
  this->InvokeEvent(svtkCommand::EndInteractionEvent, nullptr);
  this->Interactor->Render();
}

void svtkBoxWidget::OnMouseMove()
{
  // See whether we're active
  if (this->State == svtkBoxWidget::Outside || this->State == svtkBoxWidget::Start)
  {
    return;
  }

  int X = this->Interactor->GetEventPosition()[0];
  int Y = this->Interactor->GetEventPosition()[1];

  // Do different things depending on state
  // Calculations everybody does
  double focalPoint[4], pickPoint[4], prevPickPoint[4];
  double z, vpn[3];

  svtkCamera* camera = this->CurrentRenderer->GetActiveCamera();
  if (!camera)
  {
    return;
  }

  // Compute the two points defining the motion vector
  this->ComputeWorldToDisplay(
    this->LastPickPosition[0], this->LastPickPosition[1], this->LastPickPosition[2], focalPoint);
  z = focalPoint[2];
  this->ComputeDisplayToWorld(double(this->Interactor->GetLastEventPosition()[0]),
    double(this->Interactor->GetLastEventPosition()[1]), z, prevPickPoint);
  this->ComputeDisplayToWorld(double(X), double(Y), z, pickPoint);

  // Process the motion
  if (this->State == svtkBoxWidget::Moving)
  {
    // Okay to process
    if (this->CurrentHandle)
    {
      if (this->RotationEnabled && this->CurrentHandle == this->HexFace)
      {
        camera->GetViewPlaneNormal(vpn);
        this->Rotate(X, Y, prevPickPoint, pickPoint, vpn);
      }
      else if (this->TranslationEnabled && this->CurrentHandle == this->Handle[6])
      {
        this->Translate(prevPickPoint, pickPoint);
      }
      else if (this->TranslationEnabled && this->ScalingEnabled)
      {
        if (this->CurrentHandle == this->Handle[0])
        {
          this->MoveMinusXFace(prevPickPoint, pickPoint);
        }
        else if (this->CurrentHandle == this->Handle[1])
        {
          this->MovePlusXFace(prevPickPoint, pickPoint);
        }
        else if (this->CurrentHandle == this->Handle[2])
        {
          this->MoveMinusYFace(prevPickPoint, pickPoint);
        }
        else if (this->CurrentHandle == this->Handle[3])
        {
          this->MovePlusYFace(prevPickPoint, pickPoint);
        }
        else if (this->CurrentHandle == this->Handle[4])
        {
          this->MoveMinusZFace(prevPickPoint, pickPoint);
        }
        else if (this->CurrentHandle == this->Handle[5])
        {
          this->MovePlusZFace(prevPickPoint, pickPoint);
        }
      }
    }
  }
  else if (this->ScalingEnabled && this->State == svtkBoxWidget::Scaling)
  {
    this->Scale(prevPickPoint, pickPoint, X, Y);
  }

  // Interact, if desired
  this->EventCallbackCommand->SetAbortFlag(1);
  this->InvokeEvent(svtkCommand::InteractionEvent, nullptr);
  this->Interactor->Render();
}

void svtkBoxWidget::MoveFace(
  double* p1, double* p2, double* dir, double* x1, double* x2, double* x3, double* x4, double* x5)
{
  int i;
  double v[3], v2[3];

  for (i = 0; i < 3; i++)
  {
    v[i] = p2[i] - p1[i];
    v2[i] = dir[i];
  }

  svtkMath::Normalize(v2);
  double f = svtkMath::Dot(v, v2);

  for (i = 0; i < 3; i++)
  {
    v[i] = f * v2[i];

    x1[i] += v[i];
    x2[i] += v[i];
    x3[i] += v[i];
    x4[i] += v[i];
    x5[i] += v[i];
  }
  this->PositionHandles();
}

void svtkBoxWidget::GetDirection(
  const double Nx[3], const double Ny[3], const double Nz[3], double dir[3])
{
  double dotNy, dotNz;
  double y[3];

  if (svtkMath::Dot(Nx, Nx) != 0)
  {
    dir[0] = Nx[0];
    dir[1] = Nx[1];
    dir[2] = Nx[2];
  }
  else
  {
    dotNy = svtkMath::Dot(Ny, Ny);
    dotNz = svtkMath::Dot(Nz, Nz);
    if (dotNy != 0 && dotNz != 0)
    {
      svtkMath::Cross(Ny, Nz, dir);
    }
    else if (dotNy != 0)
    {
      // dir must have been initialized to the
      // corresponding coordinate direction before calling
      // this method
      svtkMath::Cross(Ny, dir, y);
      svtkMath::Cross(y, Ny, dir);
    }
    else if (dotNz != 0)
    {
      // dir must have been initialized to the
      // corresponding coordinate direction before calling
      // this method
      svtkMath::Cross(Nz, dir, y);
      svtkMath::Cross(y, Nz, dir);
    }
  }
}
void svtkBoxWidget::MovePlusXFace(double* p1, double* p2)
{
  double* pts = static_cast<svtkDoubleArray*>(this->Points->GetData())->GetPointer(0);

  double* h1 = pts + 3 * 9;

  double* x1 = pts + 3 * 1;
  double* x2 = pts + 3 * 2;
  double* x3 = pts + 3 * 5;
  double* x4 = pts + 3 * 6;

  double dir[3] = { 1, 0, 0 };
  this->ComputeNormals();
  this->GetDirection(this->N[1], this->N[3], this->N[5], dir);
  this->MoveFace(p1, p2, dir, x1, x2, x3, x4, h1);
}

void svtkBoxWidget::MoveMinusXFace(double* p1, double* p2)
{
  double* pts = static_cast<svtkDoubleArray*>(this->Points->GetData())->GetPointer(0);

  double* h1 = pts + 3 * 8;

  double* x1 = pts + 3 * 0;
  double* x2 = pts + 3 * 3;
  double* x3 = pts + 3 * 4;
  double* x4 = pts + 3 * 7;

  double dir[3] = { -1, 0, 0 };
  this->ComputeNormals();
  this->GetDirection(this->N[0], this->N[4], this->N[2], dir);

  this->MoveFace(p1, p2, dir, x1, x2, x3, x4, h1);
}

void svtkBoxWidget::MovePlusYFace(double* p1, double* p2)
{
  double* pts = static_cast<svtkDoubleArray*>(this->Points->GetData())->GetPointer(0);

  double* h1 = pts + 3 * 11;

  double* x1 = pts + 3 * 2;
  double* x2 = pts + 3 * 3;
  double* x3 = pts + 3 * 6;
  double* x4 = pts + 3 * 7;

  double dir[3] = { 0, 1, 0 };
  this->ComputeNormals();
  this->GetDirection(this->N[3], this->N[5], this->N[1], dir);

  this->MoveFace(p1, p2, dir, x1, x2, x3, x4, h1);
}

void svtkBoxWidget::MoveMinusYFace(double* p1, double* p2)
{
  double* pts = static_cast<svtkDoubleArray*>(this->Points->GetData())->GetPointer(0);

  double* h1 = pts + 3 * 10;

  double* x1 = pts + 3 * 0;
  double* x2 = pts + 3 * 1;
  double* x3 = pts + 3 * 4;
  double* x4 = pts + 3 * 5;

  double dir[3] = { 0, -1, 0 };
  this->ComputeNormals();
  this->GetDirection(this->N[2], this->N[0], this->N[4], dir);

  this->MoveFace(p1, p2, dir, x1, x2, x3, x4, h1);
}

void svtkBoxWidget::MovePlusZFace(double* p1, double* p2)
{
  double* pts = static_cast<svtkDoubleArray*>(this->Points->GetData())->GetPointer(0);

  double* h1 = pts + 3 * 13;

  double* x1 = pts + 3 * 4;
  double* x2 = pts + 3 * 5;
  double* x3 = pts + 3 * 6;
  double* x4 = pts + 3 * 7;

  double dir[3] = { 0, 0, 1 };
  this->ComputeNormals();
  this->GetDirection(this->N[5], this->N[1], this->N[3], dir);

  this->MoveFace(p1, p2, dir, x1, x2, x3, x4, h1);
}

void svtkBoxWidget::MoveMinusZFace(double* p1, double* p2)
{
  double* pts = static_cast<svtkDoubleArray*>(this->Points->GetData())->GetPointer(0);

  double* h1 = pts + 3 * 12;

  double* x1 = pts + 3 * 0;
  double* x2 = pts + 3 * 1;
  double* x3 = pts + 3 * 2;
  double* x4 = pts + 3 * 3;

  double dir[3] = { 0, 0, -1 };
  this->ComputeNormals();
  this->GetDirection(this->N[4], this->N[2], this->N[0], dir);

  this->MoveFace(p1, p2, dir, x1, x2, x3, x4, h1);
}

// Loop through all points and translate them
void svtkBoxWidget::Translate(double* p1, double* p2)
{
  double* pts = static_cast<svtkDoubleArray*>(this->Points->GetData())->GetPointer(0);
  double v[3];

  v[0] = p2[0] - p1[0];
  v[1] = p2[1] - p1[1];
  v[2] = p2[2] - p1[2];

  // Move the corners
  for (int i = 0; i < 8; i++)
  {
    *pts++ += v[0];
    *pts++ += v[1];
    *pts++ += v[2];
  }
  this->PositionHandles();
}

void svtkBoxWidget::Scale(double* svtkNotUsed(p1), double* svtkNotUsed(p2), int svtkNotUsed(X), int Y)
{
  double* pts = static_cast<svtkDoubleArray*>(this->Points->GetData())->GetPointer(0);
  double* center = static_cast<svtkDoubleArray*>(this->Points->GetData())->GetPointer(3 * 14);
  double sf;

  if (Y > this->Interactor->GetLastEventPosition()[1])
  {
    sf = 1.03;
  }
  else
  {
    sf = 0.97;
  }

  // Move the corners
  for (int i = 0; i < 8; i++, pts += 3)
  {
    pts[0] = sf * (pts[0] - center[0]) + center[0];
    pts[1] = sf * (pts[1] - center[1]) + center[1];
    pts[2] = sf * (pts[2] - center[2]) + center[2];
  }
  this->PositionHandles();
}

void svtkBoxWidget::ComputeNormals()
{
  double* pts = static_cast<svtkDoubleArray*>(this->Points->GetData())->GetPointer(0);
  double* p0 = pts;
  double* px = pts + 3 * 1;
  double* py = pts + 3 * 3;
  double* pz = pts + 3 * 4;
  int i;

  for (i = 0; i < 3; i++)
  {
    this->N[0][i] = p0[i] - px[i];
    this->N[2][i] = p0[i] - py[i];
    this->N[4][i] = p0[i] - pz[i];
  }
  svtkMath::Normalize(this->N[0]);
  svtkMath::Normalize(this->N[2]);
  svtkMath::Normalize(this->N[4]);
  for (i = 0; i < 3; i++)
  {
    this->N[1][i] = -this->N[0][i];
    this->N[3][i] = -this->N[2][i];
    this->N[5][i] = -this->N[4][i];
  }
}

void svtkBoxWidget::GetPlanes(svtkPlanes* planes)
{
  if (!planes)
  {
    return;
  }

  this->ComputeNormals();

  svtkPoints* pts = svtkPoints::New(SVTK_DOUBLE);
  pts->SetNumberOfPoints(6);

  svtkDoubleArray* normals = svtkDoubleArray::New();
  normals->SetNumberOfComponents(3);
  normals->SetNumberOfTuples(6);

  // Set the normals and coordinate values
  double factor = (this->InsideOut ? -1.0 : 1.0);
  for (int i = 0; i < 6; i++)
  {
    pts->SetPoint(i, this->Points->GetPoint(8 + i));
    normals->SetTuple3(i, factor * this->N[i][0], factor * this->N[i][1], factor * this->N[i][2]);
  }

  planes->SetPoints(pts);
  planes->SetNormals(normals);

  pts->Delete();
  normals->Delete();
}

void svtkBoxWidget::Rotate(int X, int Y, double* p1, double* p2, double* vpn)
{
  double* pts = static_cast<svtkDoubleArray*>(this->Points->GetData())->GetPointer(0);
  double* center = static_cast<svtkDoubleArray*>(this->Points->GetData())->GetPointer(3 * 14);
  double v[3];    // vector of motion
  double axis[3]; // axis of rotation
  double theta;   // rotation angle
  int i;

  v[0] = p2[0] - p1[0];
  v[1] = p2[1] - p1[1];
  v[2] = p2[2] - p1[2];

  // Create axis of rotation and angle of rotation
  svtkMath::Cross(vpn, v, axis);
  if (svtkMath::Normalize(axis) == 0.0)
  {
    return;
  }
  const int* size = this->CurrentRenderer->GetSize();
  double l2 = (X - this->Interactor->GetLastEventPosition()[0]) *
      (X - this->Interactor->GetLastEventPosition()[0]) +
    (Y - this->Interactor->GetLastEventPosition()[1]) *
      (Y - this->Interactor->GetLastEventPosition()[1]);
  theta = 360.0 * sqrt(l2 / (size[0] * size[0] + size[1] * size[1]));

  // Manipulate the transform to reflect the rotation
  this->Transform->Identity();
  this->Transform->Translate(center[0], center[1], center[2]);
  this->Transform->RotateWXYZ(theta, axis);
  this->Transform->Translate(-center[0], -center[1], -center[2]);

  // Set the corners
  svtkPoints* newPts = svtkPoints::New(SVTK_DOUBLE);
  this->Transform->TransformPoints(this->Points, newPts);

  for (i = 0; i < 8; i++, pts += 3)
  {
    this->Points->SetPoint(i, newPts->GetPoint(i));
  }

  newPts->Delete();
  this->PositionHandles();
}

void svtkBoxWidget::CreateDefaultProperties()
{
  // Handle properties
  this->HandleProperty = svtkProperty::New();
  this->HandleProperty->SetColor(1, 1, 1);

  this->SelectedHandleProperty = svtkProperty::New();
  this->SelectedHandleProperty->SetColor(1, 0, 0);

  // Face properties
  this->FaceProperty = svtkProperty::New();
  this->FaceProperty->SetColor(1, 1, 1);
  this->FaceProperty->SetOpacity(0.0);

  this->SelectedFaceProperty = svtkProperty::New();
  this->SelectedFaceProperty->SetColor(1, 1, 0);
  this->SelectedFaceProperty->SetOpacity(0.25);

  // Outline properties
  this->OutlineProperty = svtkProperty::New();
  this->OutlineProperty->SetRepresentationToWireframe();
  this->OutlineProperty->SetAmbient(1.0);
  this->OutlineProperty->SetAmbientColor(1.0, 1.0, 1.0);
  this->OutlineProperty->SetLineWidth(2.0);

  this->SelectedOutlineProperty = svtkProperty::New();
  this->SelectedOutlineProperty->SetRepresentationToWireframe();
  this->SelectedOutlineProperty->SetAmbient(1.0);
  this->SelectedOutlineProperty->SetAmbientColor(0.0, 1.0, 0.0);
  this->SelectedOutlineProperty->SetLineWidth(2.0);
}

void svtkBoxWidget::PlaceWidget(double bds[6])
{
  int i;
  double bounds[6], center[3];

  this->AdjustBounds(bds, bounds, center);

  this->Points->SetPoint(0, bounds[0], bounds[2], bounds[4]);
  this->Points->SetPoint(1, bounds[1], bounds[2], bounds[4]);
  this->Points->SetPoint(2, bounds[1], bounds[3], bounds[4]);
  this->Points->SetPoint(3, bounds[0], bounds[3], bounds[4]);
  this->Points->SetPoint(4, bounds[0], bounds[2], bounds[5]);
  this->Points->SetPoint(5, bounds[1], bounds[2], bounds[5]);
  this->Points->SetPoint(6, bounds[1], bounds[3], bounds[5]);
  this->Points->SetPoint(7, bounds[0], bounds[3], bounds[5]);

  for (i = 0; i < 6; i++)
  {
    this->InitialBounds[i] = bounds[i];
  }
  this->InitialLength = sqrt((bounds[1] - bounds[0]) * (bounds[1] - bounds[0]) +
    (bounds[3] - bounds[2]) * (bounds[3] - bounds[2]) +
    (bounds[5] - bounds[4]) * (bounds[5] - bounds[4]));

  this->PositionHandles();
  this->ComputeNormals();
  this->SizeHandles();
}

void svtkBoxWidget::GetTransform(svtkTransform* t)
{
  double* pts = static_cast<svtkDoubleArray*>(this->Points->GetData())->GetPointer(0);
  double* p0 = pts;
  double* p1 = pts + 3 * 1;
  double* p3 = pts + 3 * 3;
  double* p4 = pts + 3 * 4;
  double* p14 = pts + 3 * 14;
  double center[3], translate[3], scale[3], scaleVec[3][3];
  double InitialCenter[3];
  int i;

  // The transformation is relative to the initial bounds.
  // Initial bounds are set when PlaceWidget() is invoked.
  t->Identity();

  // Translation
  for (i = 0; i < 3; i++)
  {
    InitialCenter[i] = (this->InitialBounds[2 * i + 1] + this->InitialBounds[2 * i]) / 2.0;
    center[i] = p14[i] - InitialCenter[i];
  }
  translate[0] = center[0] + InitialCenter[0];
  translate[1] = center[1] + InitialCenter[1];
  translate[2] = center[2] + InitialCenter[2];
  t->Translate(translate[0], translate[1], translate[2]);

  // Orientation
  svtkMatrix4x4* matrix = svtkMatrix4x4::New();
  this->PositionHandles();
  this->ComputeNormals();
  for (i = 0; i < 3; i++)
  {
    matrix->SetElement(i, 0, this->N[1][i]);
    matrix->SetElement(i, 1, this->N[3][i]);
    matrix->SetElement(i, 2, this->N[5][i]);
  }
  t->Concatenate(matrix);
  matrix->Delete();

  // Scale
  for (i = 0; i < 3; i++)
  {
    scaleVec[0][i] = (p1[i] - p0[i]);
    scaleVec[1][i] = (p3[i] - p0[i]);
    scaleVec[2][i] = (p4[i] - p0[i]);
  }

  scale[0] = svtkMath::Norm(scaleVec[0]);
  if (this->InitialBounds[1] != this->InitialBounds[0])
  {
    scale[0] = scale[0] / (this->InitialBounds[1] - this->InitialBounds[0]);
  }
  scale[1] = svtkMath::Norm(scaleVec[1]);
  if (this->InitialBounds[3] != this->InitialBounds[2])
  {
    scale[1] = scale[1] / (this->InitialBounds[3] - this->InitialBounds[2]);
  }
  scale[2] = svtkMath::Norm(scaleVec[2]);
  if (this->InitialBounds[5] != this->InitialBounds[4])
  {
    scale[2] = scale[2] / (this->InitialBounds[5] - this->InitialBounds[4]);
  }
  t->Scale(scale[0], scale[1], scale[2]);

  // Add back in the contribution due to non-origin center
  t->Translate(-InitialCenter[0], -InitialCenter[1], -InitialCenter[2]);
}

void svtkBoxWidget::SetTransform(svtkTransform* t)
{
  if (!t)
  {
    svtkErrorMacro(<< "svtkTransform t must be non-nullptr");
    return;
  }

  double* pts = static_cast<svtkDoubleArray*>(this->Points->GetData())->GetPointer(0);
  double xIn[3];
  // make sure the transform is up-to-date before using it
  t->Update();

  // Position the eight points of the box and then update the
  // position of the other handles.
  double* bounds = this->InitialBounds;

  xIn[0] = bounds[0];
  xIn[1] = bounds[2];
  xIn[2] = bounds[4];
  t->InternalTransformPoint(xIn, pts);

  xIn[0] = bounds[1];
  xIn[1] = bounds[2];
  xIn[2] = bounds[4];
  t->InternalTransformPoint(xIn, pts + 3);

  xIn[0] = bounds[1];
  xIn[1] = bounds[3];
  xIn[2] = bounds[4];
  t->InternalTransformPoint(xIn, pts + 6);

  xIn[0] = bounds[0];
  xIn[1] = bounds[3];
  xIn[2] = bounds[4];
  t->InternalTransformPoint(xIn, pts + 9);

  xIn[0] = bounds[0];
  xIn[1] = bounds[2];
  xIn[2] = bounds[5];
  t->InternalTransformPoint(xIn, pts + 12);

  xIn[0] = bounds[1];
  xIn[1] = bounds[2];
  xIn[2] = bounds[5];
  t->InternalTransformPoint(xIn, pts + 15);

  xIn[0] = bounds[1];
  xIn[1] = bounds[3];
  xIn[2] = bounds[5];
  t->InternalTransformPoint(xIn, pts + 18);

  xIn[0] = bounds[0];
  xIn[1] = bounds[3];
  xIn[2] = bounds[5];
  t->InternalTransformPoint(xIn, pts + 21);

  this->PositionHandles();
}

void svtkBoxWidget::GetPolyData(svtkPolyData* pd)
{
  pd->SetPoints(this->HexPolyData->GetPoints());
  pd->SetPolys(this->HexPolyData->GetPolys());
}

void svtkBoxWidget::SetOutlineFaceWires(int newValue)
{
  if (this->OutlineFaceWires != newValue)
  {
    this->OutlineFaceWires = newValue;
    this->Modified();
    // the outline is dependent on this value, so we have to regen
    this->GenerateOutline();
  }
}

void svtkBoxWidget::SetOutlineCursorWires(int newValue)
{
  if (this->OutlineCursorWires != newValue)
  {
    this->OutlineCursorWires = newValue;
    this->Modified();
    // the outline is dependent on this value, so we have to regen
    this->GenerateOutline();
  }
}

void svtkBoxWidget::GenerateOutline()
{
  // Whatever the case may be, we have to reset the Lines of the
  // OutlinePolyData (i.e. nuke all current line data)
  svtkCellArray* cells = this->OutlinePolyData->GetLines();
  cells->Reset();

  // Now the outline lines
  if (!this->OutlineFaceWires && !this->OutlineCursorWires)
  {
    return;
  }

  svtkIdType pts[2];

  if (this->OutlineFaceWires)
  {
    pts[0] = 0;
    pts[1] = 7; // the -x face
    cells->InsertNextCell(2, pts);
    pts[0] = 3;
    pts[1] = 4;
    cells->InsertNextCell(2, pts);
    pts[0] = 1;
    pts[1] = 6; // the +x face
    cells->InsertNextCell(2, pts);
    pts[0] = 2;
    pts[1] = 5;
    cells->InsertNextCell(2, pts);
    pts[0] = 1;
    pts[1] = 4; // the -y face
    cells->InsertNextCell(2, pts);
    pts[0] = 0;
    pts[1] = 5;
    cells->InsertNextCell(2, pts);
    pts[0] = 3;
    pts[1] = 6; // the +y face
    cells->InsertNextCell(2, pts);
    pts[0] = 2;
    pts[1] = 7;
    cells->InsertNextCell(2, pts);
    pts[0] = 0;
    pts[1] = 2; // the -z face
    cells->InsertNextCell(2, pts);
    pts[0] = 1;
    pts[1] = 3;
    cells->InsertNextCell(2, pts);
    pts[0] = 4;
    pts[1] = 6; // the +Z face
    cells->InsertNextCell(2, pts);
    pts[0] = 5;
    pts[1] = 7;
    cells->InsertNextCell(2, pts);
  }
  if (this->OutlineCursorWires)
  {
    pts[0] = 8;
    pts[1] = 9; // the x cursor line
    cells->InsertNextCell(2, pts);
    pts[0] = 10;
    pts[1] = 11; // the y cursor line
    cells->InsertNextCell(2, pts);
    pts[0] = 12;
    pts[1] = 13; // the z cursor line
    cells->InsertNextCell(2, pts);
  }
  this->OutlinePolyData->Modified();
  if (this->OutlineProperty)
  {
    this->OutlineProperty->SetRepresentationToWireframe();
    this->SelectedOutlineProperty->SetRepresentationToWireframe();
  }
}

//------------------------------------------------------------------------------
void svtkBoxWidget::RegisterPickers()
{
  svtkPickingManager* pm = this->GetPickingManager();
  if (!pm)
  {
    return;
  }
  pm->AddPicker(this->HandlePicker, this);
  pm->AddPicker(this->HexPicker, this);
}
