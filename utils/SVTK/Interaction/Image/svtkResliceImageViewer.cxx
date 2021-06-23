/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkResliceImageViewer.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkResliceImageViewer.h"

#include "svtkBoundedPlanePointPlacer.h"
#include "svtkCamera.h"
#include "svtkCommand.h"
#include "svtkImageActor.h"
#include "svtkImageData.h"
#include "svtkImageMapToWindowLevelColors.h"
#include "svtkImageReslice.h"
#include "svtkInteractorStyleImage.h"
#include "svtkMath.h"
#include "svtkObjectFactory.h"
#include "svtkPlane.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkResliceCursor.h"
#include "svtkResliceCursorActor.h"
#include "svtkResliceCursorLineRepresentation.h"
#include "svtkResliceCursorPolyDataAlgorithm.h"
#include "svtkResliceCursorThickLineRepresentation.h"
#include "svtkResliceCursorWidget.h"
#include "svtkResliceImageViewerMeasurements.h"
#include "svtkScalarsToColors.h"
#include "svtkSmartPointer.h"

svtkStandardNewMacro(svtkResliceImageViewer);

//----------------------------------------------------------------------------
// This class is used to scroll slices with the scroll bar. In the case of MPR
// view, it moves one "normalized spacing" in the direction of the normal to
// the resliced plane, provided the new center will continue to lie within the
// volume.
class svtkResliceImageViewerScrollCallback : public svtkCommand
{
public:
  static svtkResliceImageViewerScrollCallback* New()
  {
    return new svtkResliceImageViewerScrollCallback;
  }

  void Execute(svtkObject*, unsigned long ev, void*) override
  {
    if (!this->Viewer->GetSliceScrollOnMouseWheel())
    {
      return;
    }

    // Do not process if any modifiers are ON
    if (this->Viewer->GetInteractor()->GetShiftKey() ||
      this->Viewer->GetInteractor()->GetControlKey() || this->Viewer->GetInteractor()->GetAltKey())
    {
      return;
    }

    // forwards or backwards
    int sign = (ev == svtkCommand::MouseWheelForwardEvent) ? 1 : -1;
    this->Viewer->IncrementSlice(sign);

    // Abort further event processing for the scroll.
    this->SetAbortFlag(1);
  }

  svtkResliceImageViewerScrollCallback()
    : Viewer(nullptr)
  {
  }
  svtkResliceImageViewer* Viewer;
};

//----------------------------------------------------------------------------
svtkResliceImageViewer::svtkResliceImageViewer()
{
  // Default is to not use the reslice cursor widget, ie use fast
  // 3D texture mapping to display slices.
  this->ResliceMode = svtkResliceImageViewer::RESLICE_AXIS_ALIGNED;

  // Set up the reslice cursor widget, should it be used.

  this->ResliceCursorWidget = svtkResliceCursorWidget::New();

  svtkSmartPointer<svtkResliceCursor> resliceCursor = svtkSmartPointer<svtkResliceCursor>::New();
  resliceCursor->SetThickMode(0);
  resliceCursor->SetThickness(10, 10, 10);

  svtkSmartPointer<svtkResliceCursorLineRepresentation> resliceCursorRep =
    svtkSmartPointer<svtkResliceCursorLineRepresentation>::New();
  resliceCursorRep->GetResliceCursorActor()->GetCursorAlgorithm()->SetResliceCursor(resliceCursor);
  resliceCursorRep->GetResliceCursorActor()->GetCursorAlgorithm()->SetReslicePlaneNormal(
    this->SliceOrientation);
  this->ResliceCursorWidget->SetRepresentation(resliceCursorRep);

  this->PointPlacer = svtkBoundedPlanePointPlacer::New();

  this->Measurements = svtkResliceImageViewerMeasurements::New();
  this->Measurements->SetResliceImageViewer(this);

  this->ScrollCallback = svtkResliceImageViewerScrollCallback::New();
  this->ScrollCallback->Viewer = this;
  this->SliceScrollOnMouseWheel = 1;

  this->InstallPipeline();
}

//----------------------------------------------------------------------------
svtkResliceImageViewer::~svtkResliceImageViewer()
{
  this->Measurements->Delete();

  if (this->ResliceCursorWidget)
  {
    this->ResliceCursorWidget->Delete();
    this->ResliceCursorWidget = nullptr;
  }

  this->PointPlacer->Delete();
  this->ScrollCallback->Delete();
}

//----------------------------------------------------------------------------
void svtkResliceImageViewer::SetThickMode(int t)
{
  svtkSmartPointer<svtkResliceCursor> rc = this->GetResliceCursor();

  if (t == this->GetThickMode())
  {
    return;
  }

  svtkSmartPointer<svtkResliceCursorLineRepresentation> resliceCursorRepOld =
    svtkResliceCursorLineRepresentation::SafeDownCast(
      this->ResliceCursorWidget->GetRepresentation());
  svtkSmartPointer<svtkResliceCursorLineRepresentation> resliceCursorRepNew;

  this->GetResliceCursor()->SetThickMode(t);

  if (t)
  {
    resliceCursorRepNew = svtkSmartPointer<svtkResliceCursorThickLineRepresentation>::New();
  }
  else
  {
    resliceCursorRepNew = svtkSmartPointer<svtkResliceCursorLineRepresentation>::New();
  }

  int e = this->ResliceCursorWidget->GetEnabled();
  this->ResliceCursorWidget->SetEnabled(0);

  resliceCursorRepNew->GetResliceCursorActor()->GetCursorAlgorithm()->SetResliceCursor(rc);
  resliceCursorRepNew->GetResliceCursorActor()->GetCursorAlgorithm()->SetReslicePlaneNormal(
    this->SliceOrientation);
  this->ResliceCursorWidget->SetRepresentation(resliceCursorRepNew);
  resliceCursorRepNew->SetLookupTable(resliceCursorRepOld->GetLookupTable());

  resliceCursorRepNew->SetWindowLevel(
    resliceCursorRepOld->GetWindow(), resliceCursorRepOld->GetLevel(), 1);

  this->ResliceCursorWidget->SetEnabled(e);
}

//----------------------------------------------------------------------------
void svtkResliceImageViewer::SetResliceCursor(svtkResliceCursor* rc)
{
  svtkResliceCursorRepresentation* rep = svtkResliceCursorRepresentation::SafeDownCast(
    this->GetResliceCursorWidget()->GetRepresentation());
  rep->GetCursorAlgorithm()->SetResliceCursor(rc);

  // Rehook the observer to this reslice cursor.
  this->Measurements->SetResliceImageViewer(this);
}

//----------------------------------------------------------------------------
int svtkResliceImageViewer::GetThickMode()
{
  return (svtkResliceCursorThickLineRepresentation::SafeDownCast(
           this->ResliceCursorWidget->GetRepresentation()))
    ? 1
    : 0;
}

//----------------------------------------------------------------------------
void svtkResliceImageViewer::SetLookupTable(svtkScalarsToColors* l)
{
  if (svtkResliceCursorRepresentation* rep = svtkResliceCursorRepresentation::SafeDownCast(
        this->ResliceCursorWidget->GetRepresentation()))
  {
    rep->SetLookupTable(l);
  }

  if (this->WindowLevel)
  {
    this->WindowLevel->SetLookupTable(l);
    this->WindowLevel->SetOutputFormatToRGBA();
    this->WindowLevel->PassAlphaToOutputOn();
  }
}

//----------------------------------------------------------------------------
svtkScalarsToColors* svtkResliceImageViewer::GetLookupTable()
{
  if (svtkResliceCursorRepresentation* rep = svtkResliceCursorRepresentation::SafeDownCast(
        this->ResliceCursorWidget->GetRepresentation()))
  {
    return rep->GetLookupTable();
  }

  return nullptr;
}

//----------------------------------------------------------------------------
void svtkResliceImageViewer::UpdateOrientation()
{
  // Set the camera position

  svtkCamera* cam = this->Renderer ? this->Renderer->GetActiveCamera() : nullptr;
  if (cam)
  {
    switch (this->SliceOrientation)
    {
      case svtkImageViewer2::SLICE_ORIENTATION_XY:
        cam->SetFocalPoint(0, 0, 0);
        cam->SetPosition(0, 0, 1); // -1 if medical ?
        cam->SetViewUp(0, 1, 0);
        break;

      case svtkImageViewer2::SLICE_ORIENTATION_XZ:
        cam->SetFocalPoint(0, 0, 0);
        cam->SetPosition(0, -1, 0); // 1 if medical ?
        cam->SetViewUp(0, 0, 1);
        break;

      case svtkImageViewer2::SLICE_ORIENTATION_YZ:
        cam->SetFocalPoint(0, 0, 0);
        cam->SetPosition(1, 0, 0); // -1 if medical ?
        cam->SetViewUp(0, 0, 1);
        break;
    }
  }
}

//----------------------------------------------------------------------------
void svtkResliceImageViewer::UpdateDisplayExtent()
{
  // Only update the display extent in axis aligned mode

  if (this->ResliceMode == RESLICE_AXIS_ALIGNED)
  {
    this->Superclass::UpdateDisplayExtent();
  }
}

//----------------------------------------------------------------------------
void svtkResliceImageViewer::InstallPipeline()
{
  this->Superclass::InstallPipeline();

  if (this->Interactor)
  {
    this->ResliceCursorWidget->SetInteractor(this->Interactor);

    // Observe the scroll for slice manipulation at a higher priority
    // than the interactor style.
    this->Interactor->RemoveObserver(this->ScrollCallback);
    this->Interactor->AddObserver(svtkCommand::MouseWheelForwardEvent, this->ScrollCallback, 0.55);
    this->Interactor->AddObserver(svtkCommand::MouseWheelBackwardEvent, this->ScrollCallback, 0.55);
  }

  if (this->Renderer)
  {
    this->ResliceCursorWidget->SetDefaultRenderer(this->Renderer);
    svtkCamera* cam = this->Renderer->GetActiveCamera();
    cam->ParallelProjectionOn();
  }

  if (this->ResliceMode == RESLICE_OBLIQUE)
  {
    this->ResliceCursorWidget->SetEnabled(1);
    this->ImageActor->SetVisibility(0);
    this->UpdateOrientation();

    double bounds[6] = { 0, 1, 0, 1, 0, 1 };

    svtkCamera* cam = this->Renderer->GetActiveCamera();
    double onespacing[3] = { 1, 1, 1 };
    double* spacing = onespacing;
    if (this->GetResliceCursor()->GetImage())
    {
      this->GetResliceCursor()->GetImage()->GetBounds(bounds);
      spacing = this->GetResliceCursor()->GetImage()->GetSpacing();
    }
    double avg_spacing = (spacing[0] + spacing[1] + spacing[2]) / 3.0;
    cam->SetClippingRange(bounds[this->SliceOrientation * 2] - 100 * avg_spacing,
      bounds[this->SliceOrientation * 2 + 1] + 100 * avg_spacing);
  }
  else
  {
    this->ResliceCursorWidget->SetEnabled(0);
    this->ImageActor->SetVisibility(1);
    this->UpdateOrientation();
  }

  if (this->WindowLevel)
  {
    this->WindowLevel->SetLookupTable(this->GetLookupTable());
  }
}

//----------------------------------------------------------------------------
void svtkResliceImageViewer::UnInstallPipeline()
{
  this->ResliceCursorWidget->SetEnabled(0);

  if (this->Interactor)
  {
    this->Interactor->RemoveObserver(this->ScrollCallback);
  }

  this->Superclass::UnInstallPipeline();
}

//----------------------------------------------------------------------------
void svtkResliceImageViewer::UpdatePointPlacer()
{
  if (this->ResliceMode == RESLICE_OBLIQUE)
  {
    this->PointPlacer->SetProjectionNormalToOblique();
    if (svtkResliceCursorRepresentation* rep = svtkResliceCursorRepresentation::SafeDownCast(
          this->ResliceCursorWidget->GetRepresentation()))
    {
      const int planeOrientation = rep->GetCursorAlgorithm()->GetReslicePlaneNormal();
      svtkPlane* plane = this->GetResliceCursor()->GetPlane(planeOrientation);
      this->PointPlacer->SetObliquePlane(plane);
    }
  }
  else
  {

    if (!this->WindowLevel->GetInput())
    {
      return;
    }

    svtkImageData* input = this->ImageActor->GetInput();
    if (!input)
    {
      return;
    }

    double spacing[3];
    input->GetSpacing(spacing);

    double origin[3];
    input->GetOrigin(origin);

    double bounds[6];
    this->ImageActor->GetBounds(bounds);

    int displayExtent[6];
    this->ImageActor->GetDisplayExtent(displayExtent);

    int axis = svtkBoundedPlanePointPlacer::XAxis;
    double position = 0.0;
    if (displayExtent[0] == displayExtent[1])
    {
      axis = svtkBoundedPlanePointPlacer::XAxis;
      position = origin[0] + displayExtent[0] * spacing[0];
    }
    else if (displayExtent[2] == displayExtent[3])
    {
      axis = svtkBoundedPlanePointPlacer::YAxis;
      position = origin[1] + displayExtent[2] * spacing[1];
    }
    else if (displayExtent[4] == displayExtent[5])
    {
      axis = svtkBoundedPlanePointPlacer::ZAxis;
      position = origin[2] + displayExtent[4] * spacing[2];
    }

    this->PointPlacer->SetProjectionNormal(axis);
    this->PointPlacer->SetProjectionPosition(position);
  }
}

//----------------------------------------------------------------------------
void svtkResliceImageViewer::Render()
{
  if (!this->WindowLevel->GetInput())
  {
    return;
  }

  this->UpdatePointPlacer();

  this->Superclass::Render();
}

//----------------------------------------------------------------------------
svtkResliceCursor* svtkResliceImageViewer::GetResliceCursor()
{
  if (svtkResliceCursorRepresentation* rep = svtkResliceCursorRepresentation::SafeDownCast(
        this->ResliceCursorWidget->GetRepresentation()))
  {
    return rep->GetResliceCursor();
  }

  return nullptr;
}

//----------------------------------------------------------------------------
void svtkResliceImageViewer::SetInputData(svtkImageData* in)
{
  if (!in)
  {
    return;
  }

  this->WindowLevel->SetInputData(in);
  this->GetResliceCursor()->SetImage(in);
  this->GetResliceCursor()->SetCenter(in->GetCenter());
  this->UpdateDisplayExtent();

  double range[2];
  in->GetScalarRange(range);
  if (svtkResliceCursorRepresentation* rep = svtkResliceCursorRepresentation::SafeDownCast(
        this->ResliceCursorWidget->GetRepresentation()))
  {
    if (svtkImageReslice* reslice = svtkImageReslice::SafeDownCast(rep->GetReslice()))
    {
      // default background color is the min value of the image scalar range
      reslice->SetBackgroundColor(range[0], range[0], range[0], range[0]);
      this->SetColorWindow(range[1] - range[0]);
      this->SetColorLevel((range[0] + range[1]) / 2.0);
    }
  }
}

//----------------------------------------------------------------------------
void svtkResliceImageViewer::SetInputConnection(svtkAlgorithmOutput* input)
{
  svtkErrorMacro(<< "Use SetInputData instead. ");
  this->WindowLevel->SetInputConnection(input);
  this->UpdateDisplayExtent();
}

//----------------------------------------------------------------------------
void svtkResliceImageViewer::SetResliceMode(int r)
{
  if (r == this->ResliceMode)
  {
    return;
  }

  this->ResliceMode = r;
  this->Modified();

  this->InstallPipeline();
}

//----------------------------------------------------------------------------
void svtkResliceImageViewer::SetColorWindow(double w)
{
  double rmin = this->GetColorLevel() - 0.5 * fabs(w);
  double rmax = rmin + fabs(w);
  this->GetLookupTable()->SetRange(rmin, rmax);

  this->WindowLevel->SetWindow(w);
  if (svtkResliceCursorRepresentation* rep = svtkResliceCursorRepresentation::SafeDownCast(
        this->ResliceCursorWidget->GetRepresentation()))
  {
    rep->SetWindowLevel(w, rep->GetLevel(), 1);
  }
}

//----------------------------------------------------------------------------
void svtkResliceImageViewer::SetColorLevel(double w)
{
  double rmin = w - 0.5 * fabs(this->GetColorWindow());
  double rmax = rmin + fabs(this->GetColorWindow());
  this->GetLookupTable()->SetRange(rmin, rmax);

  this->WindowLevel->SetLevel(w);
  if (svtkResliceCursorRepresentation* rep = svtkResliceCursorRepresentation::SafeDownCast(
        this->ResliceCursorWidget->GetRepresentation()))
  {
    rep->SetWindowLevel(rep->GetWindow(), w, 1);
  }
}

//----------------------------------------------------------------------------
void svtkResliceImageViewer::Reset()
{
  this->ResliceCursorWidget->ResetResliceCursor();
}

//----------------------------------------------------------------------------
svtkPlane* svtkResliceImageViewer::GetReslicePlane()
{
  // Get the reslice plane
  if (svtkResliceCursorRepresentation* rep = svtkResliceCursorRepresentation::SafeDownCast(
        this->ResliceCursorWidget->GetRepresentation()))
  {
    const int planeOrientation = rep->GetCursorAlgorithm()->GetReslicePlaneNormal();
    svtkPlane* plane = this->GetResliceCursor()->GetPlane(planeOrientation);
    return plane;
  }

  return nullptr;
}

//----------------------------------------------------------------------------
double svtkResliceImageViewer::GetInterSliceSpacingInResliceMode()
{
  double n[3], imageSpacing[3], resliceSpacing = 0;

  if (svtkPlane* plane = this->GetReslicePlane())
  {
    plane->GetNormal(n);
    this->GetResliceCursor()->GetImage()->GetSpacing(imageSpacing);
    resliceSpacing = fabs(svtkMath::Dot(n, imageSpacing));
  }

  return resliceSpacing;
}

//----------------------------------------------------------------------------
void svtkResliceImageViewer::IncrementSlice(int inc)
{
  if (this->GetResliceMode() == svtkResliceImageViewer::RESLICE_AXIS_ALIGNED)
  {
    int oldSlice = this->GetSlice();
    this->SetSlice(this->GetSlice() + inc);
    if (this->GetSlice() != oldSlice)
    {
      this->InvokeEvent(svtkResliceImageViewer::SliceChangedEvent, nullptr);
      this->InvokeEvent(svtkCommand::InteractionEvent, nullptr);
    }
  }
  else
  {
    if (svtkPlane* p = this->GetReslicePlane())
    {
      double n[3], c[3], bounds[6];
      p->GetNormal(n);
      const double spacing = this->GetInterSliceSpacingInResliceMode() * inc;
      this->GetResliceCursor()->GetCenter(c);
      svtkMath::MultiplyScalar(n, spacing);
      c[0] += n[0];
      c[1] += n[1];
      c[2] += n[2];

      // If the new center is inside, put it there...
      if (svtkImageData* image = this->GetResliceCursor()->GetImage())
      {
        image->GetBounds(bounds);
        if (c[0] >= bounds[0] && c[0] <= bounds[1] && c[1] >= bounds[2] && c[1] <= bounds[3] &&
          c[2] >= bounds[4] && c[2] <= bounds[5])
        {
          this->GetResliceCursor()->SetCenter(c);

          this->InvokeEvent(svtkResliceImageViewer::SliceChangedEvent, nullptr);
          this->InvokeEvent(svtkCommand::InteractionEvent, nullptr);
        }
      }
    }
  }
}

//----------------------------------------------------------------------------
void svtkResliceImageViewer::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "ResliceCursorWidget:\n";
  this->ResliceCursorWidget->PrintSelf(os, indent.GetNextIndent());
  os << indent << "ResliceMode: " << this->ResliceMode << endl;
  os << indent << "SliceScrollOnMouseWheel: " << this->SliceScrollOnMouseWheel << endl;
  os << indent << "Point Placer: ";
  this->PointPlacer->PrintSelf(os, indent.GetNextIndent());
  os << indent << "Measurements: ";
  this->Measurements->PrintSelf(os, indent.GetNextIndent());
  os << indent << "Interactor: " << this->Interactor << "\n";
  if (this->Interactor)
  {
    this->Interactor->PrintSelf(os, indent.GetNextIndent());
  }
}
