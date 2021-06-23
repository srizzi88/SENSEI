/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkResliceImageViewerMeasurements.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkResliceImageViewerMeasurements.h"

#include "svtkAngleRepresentation.h"
#include "svtkAngleWidget.h"
#include "svtkBiDimensionalRepresentation.h"
#include "svtkBiDimensionalWidget.h"
#include "svtkCallbackCommand.h"
#include "svtkCamera.h"
#include "svtkCaptionRepresentation.h"
#include "svtkCaptionWidget.h"
#include "svtkCommand.h"
#include "svtkContourRepresentation.h"
#include "svtkContourWidget.h"
#include "svtkDistanceRepresentation.h"
#include "svtkDistanceWidget.h"
#include "svtkHandleRepresentation.h"
#include "svtkHandleWidget.h"
#include "svtkImageActor.h"
#include "svtkImageData.h"
#include "svtkObjectFactory.h"
#include "svtkPlane.h"
#include "svtkPointHandleRepresentation3D.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkResliceCursor.h"
#include "svtkResliceCursorActor.h"
#include "svtkResliceCursorLineRepresentation.h"
#include "svtkResliceCursorPolyDataAlgorithm.h"
#include "svtkResliceCursorWidget.h"
#include "svtkResliceImageViewer.h"
#include "svtkSeedRepresentation.h"
#include "svtkSeedWidget.h"
#include "svtkSmartPointer.h"

svtkStandardNewMacro(svtkResliceImageViewerMeasurements);

//----------------------------------------------------------------------------
svtkResliceImageViewerMeasurements::svtkResliceImageViewerMeasurements()
{
  this->ResliceImageViewer = nullptr;
  this->WidgetCollection = svtkCollection::New();

  // Setup event processing
  this->EventCallbackCommand = svtkCallbackCommand::New();
  this->EventCallbackCommand->SetClientData(this);
  this->EventCallbackCommand->SetCallback(svtkResliceImageViewerMeasurements::ProcessEventsHandler);

  this->ProcessEvents = 1;
  this->Tolerance = 6;
}

//----------------------------------------------------------------------------
svtkResliceImageViewerMeasurements::~svtkResliceImageViewerMeasurements()
{
  // Remove any added observers
  if (this->ResliceImageViewer)
  {
    this->ResliceImageViewer->GetResliceCursor()->RemoveObservers(
      svtkResliceCursorWidget::ResliceAxesChangedEvent, this->EventCallbackCommand);
  }

  this->WidgetCollection->Delete();
  this->EventCallbackCommand->Delete();
}

//----------------------------------------------------------------------------
void svtkResliceImageViewerMeasurements ::SetResliceImageViewer(svtkResliceImageViewer* i)
{
  // Weak reference. No need to delete
  this->ResliceImageViewer = i;

  if (i)
  {
    // Add the observer
    i->GetResliceCursor()->AddObserver(
      svtkResliceCursorWidget::ResliceAxesChangedEvent, this->EventCallbackCommand);
    i->GetResliceCursor()->AddObserver(
      svtkResliceCursorWidget::ResliceAxesChangedEvent, this->EventCallbackCommand);
  }
}

//----------------------------------------------------------------------------
void svtkResliceImageViewerMeasurements::Render()
{
  this->ResliceImageViewer->Render();
}

//-------------------------------------------------------------------------
void svtkResliceImageViewerMeasurements ::ProcessEventsHandler(
  svtkObject*, unsigned long, void* clientdata, void*)
{
  svtkResliceImageViewerMeasurements* self =
    reinterpret_cast<svtkResliceImageViewerMeasurements*>(clientdata);

  // if ProcessEvents is Off, we ignore all interaction events.
  if (!self->GetProcessEvents())
  {
    return;
  }

  self->Update();
}

//-------------------------------------------------------------------------
void svtkResliceImageViewerMeasurements::Update()
{
  if (this->ResliceImageViewer->GetResliceMode() != svtkResliceImageViewer::RESLICE_OBLIQUE)
  {
    return; // nothing to do.
  }

  const int nItems = this->WidgetCollection->GetNumberOfItems();
  for (int i = 0; i < nItems; i++)
  {
    svtkAbstractWidget* a =
      svtkAbstractWidget::SafeDownCast(this->WidgetCollection->GetItemAsObject(i));

    svtkSeedWidget* s = svtkSeedWidget::SafeDownCast(a);

    // seed is handled differently since its really a collection of several
    // markers which may exist on different planes.
    if (!s)
    {
      a->SetEnabled(this->IsItemOnReslicedPlane(a));
    }
  }
}

//-------------------------------------------------------------------------
bool svtkResliceImageViewerMeasurements ::IsItemOnReslicedPlane(svtkAbstractWidget* w)
{

  if (svtkDistanceWidget* dw = svtkDistanceWidget::SafeDownCast(w))
  {
    return this->IsWidgetOnReslicedPlane(dw);
  }
  if (svtkAngleWidget* aw = svtkAngleWidget::SafeDownCast(w))
  {
    return this->IsWidgetOnReslicedPlane(aw);
  }
  if (svtkBiDimensionalWidget* aw = svtkBiDimensionalWidget::SafeDownCast(w))
  {
    return this->IsWidgetOnReslicedPlane(aw);
  }
  if (svtkCaptionWidget* capw = svtkCaptionWidget::SafeDownCast(w))
  {
    return this->IsWidgetOnReslicedPlane(capw);
  }
  if (svtkContourWidget* capw = svtkContourWidget::SafeDownCast(w))
  {
    return this->IsWidgetOnReslicedPlane(capw);
  }
  if (svtkSeedWidget* s = svtkSeedWidget::SafeDownCast(w))
  {
    return this->IsWidgetOnReslicedPlane(s);
  }
  if (svtkHandleWidget* s = svtkHandleWidget::SafeDownCast(w))
  {
    return this->IsWidgetOnReslicedPlane(s);
  }

  return true;
}

//-------------------------------------------------------------------------
bool svtkResliceImageViewerMeasurements ::IsWidgetOnReslicedPlane(svtkDistanceWidget* w)
{
  if (w->GetWidgetState() != svtkDistanceWidget::Manipulate)
  {
    return true; // widget is not yet defined.
  }

  if (svtkDistanceRepresentation* rep =
        svtkDistanceRepresentation::SafeDownCast(w->GetRepresentation()))
  {
    return this->IsPointOnReslicedPlane(rep->GetPoint1Representation()) &&
      this->IsPointOnReslicedPlane(rep->GetPoint2Representation());
  }

  return true;
}

//-------------------------------------------------------------------------
bool svtkResliceImageViewerMeasurements ::IsWidgetOnReslicedPlane(svtkAngleWidget* w)
{
  if (w->GetWidgetState() != svtkAngleWidget::Manipulate)
  {
    return true; // widget is not yet defined.
  }

  if (svtkAngleRepresentation* rep = svtkAngleRepresentation::SafeDownCast(w->GetRepresentation()))
  {
    return this->IsPointOnReslicedPlane(rep->GetPoint1Representation()) &&
      this->IsPointOnReslicedPlane(rep->GetPoint2Representation()) &&
      this->IsPointOnReslicedPlane(rep->GetCenterRepresentation());
  }

  return true;
}

//-------------------------------------------------------------------------
bool svtkResliceImageViewerMeasurements ::IsWidgetOnReslicedPlane(svtkBiDimensionalWidget* w)
{
  if (w->GetWidgetState() != svtkBiDimensionalWidget::Manipulate)
  {
    return true; // widget is not yet defined.
  }

  if (svtkBiDimensionalRepresentation* rep =
        svtkBiDimensionalRepresentation::SafeDownCast(w->GetRepresentation()))
  {
    return this->IsPointOnReslicedPlane(rep->GetPoint1Representation()) &&
      this->IsPointOnReslicedPlane(rep->GetPoint2Representation()) &&
      this->IsPointOnReslicedPlane(rep->GetPoint3Representation()) &&
      this->IsPointOnReslicedPlane(rep->GetPoint4Representation());
  }

  return true;
}

//-------------------------------------------------------------------------
bool svtkResliceImageViewerMeasurements ::IsWidgetOnReslicedPlane(svtkHandleWidget* w)
{
  return this->IsPointOnReslicedPlane(w->GetHandleRepresentation());
}

//-------------------------------------------------------------------------
bool svtkResliceImageViewerMeasurements ::IsWidgetOnReslicedPlane(svtkCaptionWidget* w)
{
  if (svtkCaptionRepresentation* rep =
        svtkCaptionRepresentation::SafeDownCast(w->GetRepresentation()))
  {
    return this->IsPointOnReslicedPlane(rep->GetAnchorRepresentation());
  }

  return true;
}

//-------------------------------------------------------------------------
bool svtkResliceImageViewerMeasurements ::IsWidgetOnReslicedPlane(svtkContourWidget* w)
{
  if (w->GetWidgetState() != svtkContourWidget::Manipulate)
  {
    return true; // widget is not yet defined.
  }

  if (svtkContourRepresentation* rep =
        svtkContourRepresentation::SafeDownCast(w->GetRepresentation()))
  {
    const int nNodes = rep->GetNumberOfNodes();
    for (int i = 0; i < nNodes; i++)
    {
      double p[3];
      rep->GetNthNodeWorldPosition(i, p);
      if (this->IsPositionOnReslicedPlane(p) == false)
      {
        return false;
      }
    }
  }

  return true;
}

//-------------------------------------------------------------------------
bool svtkResliceImageViewerMeasurements ::IsWidgetOnReslicedPlane(svtkSeedWidget* w)
{
  if (svtkSeedRepresentation* rep = svtkSeedRepresentation::SafeDownCast(w->GetRepresentation()))
  {
    const int nNodes = rep->GetNumberOfSeeds();
    for (int i = 0; i < nNodes; i++)
    {
      w->GetSeed(i)->GetHandleRepresentation()->SetVisibility(
        w->GetEnabled() && this->IsPointOnReslicedPlane(w->GetSeed(i)->GetHandleRepresentation()));
    }
  }

  return true;
}

//-------------------------------------------------------------------------
bool svtkResliceImageViewerMeasurements ::IsPointOnReslicedPlane(svtkHandleRepresentation* h)
{
  double pos[3];
  h->GetWorldPosition(pos);
  return this->IsPositionOnReslicedPlane(pos);
}

//-------------------------------------------------------------------------
bool svtkResliceImageViewerMeasurements ::IsPositionOnReslicedPlane(double p[3])
{
  if (svtkResliceCursorRepresentation* rep = svtkResliceCursorRepresentation::SafeDownCast(
        this->ResliceImageViewer->GetResliceCursorWidget()->GetRepresentation()))
  {
    const int planeOrientation = rep->GetCursorAlgorithm()->GetReslicePlaneNormal();
    svtkPlane* plane = this->ResliceImageViewer->GetResliceCursor()->GetPlane(planeOrientation);
    const double d = plane->DistanceToPlane(p);
    return (d < this->Tolerance);
  }

  return true;
}

//-------------------------------------------------------------------------
void svtkResliceImageViewerMeasurements::AddItem(svtkAbstractWidget* w)
{
  this->WidgetCollection->AddItem(w);
}

//-------------------------------------------------------------------------
void svtkResliceImageViewerMeasurements::RemoveItem(svtkAbstractWidget* w)
{
  this->WidgetCollection->RemoveItem(w);
}

//-------------------------------------------------------------------------
void svtkResliceImageViewerMeasurements::RemoveAllItems()
{
  this->WidgetCollection->RemoveAllItems();
}

//----------------------------------------------------------------------------
void svtkResliceImageViewerMeasurements::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "ResliceImageViewer: " << this->ResliceImageViewer << "\n";
  os << indent << "WidgetCollection: " << this->WidgetCollection << endl;
  this->WidgetCollection->PrintSelf(os, indent.GetNextIndent());

  os << indent << "ProcessEvents: " << (this->ProcessEvents ? "On" : "Off") << "\n";

  os << indent << "Tolerance: " << this->Tolerance << endl;
}
