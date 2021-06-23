/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkRenderView.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/

#include "svtkRenderView.h"

#include "svtkActor2D.h"
#include "svtkAlgorithmOutput.h"
#include "svtkAppendPoints.h"
#include "svtkBalloonRepresentation.h"
#include "svtkCamera.h"
#include "svtkCommand.h"
#include "svtkDataRepresentation.h"
#include "svtkDoubleArray.h"
#include "svtkDynamic2DLabelMapper.h"
#include "svtkFreeTypeLabelRenderStrategy.h"
#include "svtkHardwareSelector.h"
#include "svtkHoverWidget.h"
#include "svtkImageData.h"
#include "svtkInteractorStyleRubberBand2D.h"
#include "svtkInteractorStyleRubberBand3D.h"
#include "svtkLabelPlacementMapper.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkPolyDataMapper2D.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderedRepresentation.h"
#include "svtkRenderer.h"
#include "svtkRendererCollection.h"
#include "svtkSelection.h"
#include "svtkSelectionNode.h"
#include "svtkSmartPointer.h"
#include "svtkStringArray.h"
#include "svtkTexture.h"
#include "svtkTexturedActor2D.h"
#include "svtkTransform.h"
#include "svtkTransformCoordinateSystems.h"
#include "svtkUnicodeString.h"
#include "svtkViewTheme.h"

#ifdef SVTK_USE_QT
#include "svtkQtLabelRenderStrategy.h"
#endif

#include <sstream>

svtkStandardNewMacro(svtkRenderView);
svtkCxxSetObjectMacro(svtkRenderView, Transform, svtkAbstractTransform);
svtkCxxSetObjectMacro(svtkRenderView, IconTexture, svtkTexture);

svtkRenderView::svtkRenderView()
{
  this->RenderOnMouseMove = false;
  this->InteractionMode = -1;
  this->LabelRenderer = svtkSmartPointer<svtkRenderer>::New();
  this->Transform = svtkTransform::New();
  this->DisplayHoverText = false;
  this->IconTexture = nullptr;
  this->Interacting = false;
  this->LabelRenderMode = FREETYPE;
  this->SelectionMode = SURFACE;
  this->Selector = svtkSmartPointer<svtkHardwareSelector>::New();
  this->Balloon = svtkSmartPointer<svtkBalloonRepresentation>::New();
  this->LabelPlacementMapper = svtkSmartPointer<svtkLabelPlacementMapper>::New();
  this->LabelActor = svtkSmartPointer<svtkTexturedActor2D>::New();
  this->HoverWidget = svtkSmartPointer<svtkHoverWidget>::New();
  this->InHoverTextRender = false;
  this->IconSize[0] = 16;
  this->IconSize[1] = 16;
  this->DisplaySize[0] = 0;
  this->DisplaySize[1] = 0;
  this->PickRenderNeedsUpdate = true;
  this->InPickRender = false;

  svtkTransform::SafeDownCast(this->Transform)->Identity();

  this->LabelRenderer->EraseOff();
  this->LabelRenderer->InteractiveOff();

  this->LabelRenderer->SetActiveCamera(this->Renderer->GetActiveCamera());
  this->RenderWindow->AddRenderer(this->LabelRenderer);

  // Initialize the selector and listen to render events to help Selector know
  // when to update the full-screen hardware pick.
  this->Selector->SetRenderer(this->Renderer);
  this->Selector->SetFieldAssociation(svtkDataObject::FIELD_ASSOCIATION_CELLS);
  this->RenderWindow->AddObserver(svtkCommand::EndEvent, this->GetObserver());

  svtkRenderWindowInteractor* iren = this->RenderWindow->GetInteractor();
  // this ensure that the observer is added to the interactor correctly.
  this->SetInteractor(iren);

  // The interaction mode is -1 before calling SetInteractionMode,
  // this will force an initialization of the interaction mode/style.
  this->SetInteractionModeTo3D();

  this->HoverWidget->AddObserver(svtkCommand::TimerEvent, this->GetObserver());

  this->LabelActor->SetMapper(this->LabelPlacementMapper);
  this->LabelActor->PickableOff();
  this->LabelRenderer->AddActor(this->LabelActor);

  this->Balloon->SetBalloonText("");
  this->Balloon->SetOffset(1, 1);
  this->LabelRenderer->AddViewProp(this->Balloon);
  this->Balloon->SetRenderer(this->LabelRenderer);
  this->Balloon->PickableOff();
  this->Balloon->VisibilityOn();

  // Apply default theme
  svtkViewTheme* theme = svtkViewTheme::New();
  this->ApplyViewTheme(theme);
  theme->Delete();
}

svtkRenderView::~svtkRenderView()
{
  if (this->Transform)
  {
    this->Transform->Delete();
  }
  if (this->IconTexture)
  {
    this->IconTexture->Delete();
  }
}

void svtkRenderView::SetInteractor(svtkRenderWindowInteractor* interactor)
{
  if (!interactor)
  {
    svtkErrorMacro(<< "SetInteractor called with a null interactor pointer."
                  << " That can't be right.");
    return;
  }

  if (this->GetInteractor())
  {
    this->GetInteractor()->RemoveObserver(this->GetObserver());
  }

  this->Superclass::SetInteractor(interactor);
  this->HoverWidget->SetInteractor(interactor);

  interactor->EnableRenderOff();
  interactor->AddObserver(svtkCommand::RenderEvent, this->GetObserver());
  interactor->AddObserver(svtkCommand::StartInteractionEvent, this->GetObserver());
  interactor->AddObserver(svtkCommand::EndInteractionEvent, this->GetObserver());
}

void svtkRenderView::SetInteractorStyle(svtkInteractorObserver* style)
{
  if (!style)
  {
    svtkErrorMacro("Interactor style must not be null.");
    return;
  }
  svtkInteractorObserver* oldStyle = this->GetInteractorStyle();
  if (style != oldStyle)
  {
    if (oldStyle)
    {
      oldStyle->RemoveObserver(this->GetObserver());
    }
    this->RenderWindow->GetInteractor()->SetInteractorStyle(style);
    style->AddObserver(svtkCommand::SelectionChangedEvent, this->GetObserver());
    svtkInteractorStyleRubberBand2D* style2D = svtkInteractorStyleRubberBand2D::SafeDownCast(style);
    svtkInteractorStyleRubberBand3D* style3D = svtkInteractorStyleRubberBand3D::SafeDownCast(style);
    if (style2D)
    {
      style2D->SetRenderOnMouseMove(this->GetRenderOnMouseMove());
      this->InteractionMode = INTERACTION_MODE_2D;
    }
    else if (style3D)
    {
      style3D->SetRenderOnMouseMove(this->GetRenderOnMouseMove());
      this->InteractionMode = INTERACTION_MODE_3D;
    }
    else
    {
      this->InteractionMode = INTERACTION_MODE_UNKNOWN;
    }
  }
}

svtkInteractorObserver* svtkRenderView::GetInteractorStyle()
{
  if (this->GetInteractor())
  {
    return this->GetInteractor()->GetInteractorStyle();
  }
  return nullptr;
}

void svtkRenderView::SetRenderOnMouseMove(bool b)
{
  if (b == this->RenderOnMouseMove)
  {
    return;
  }

  svtkInteractorObserver* style = this->GetInteractor()->GetInteractorStyle();
  svtkInteractorStyleRubberBand2D* style2D = svtkInteractorStyleRubberBand2D::SafeDownCast(style);
  if (style2D)
  {
    style2D->SetRenderOnMouseMove(b);
  }
  svtkInteractorStyleRubberBand3D* style3D = svtkInteractorStyleRubberBand3D::SafeDownCast(style);
  if (style3D)
  {
    style3D->SetRenderOnMouseMove(b);
  }
  this->RenderOnMouseMove = b;
}

void svtkRenderView::SetInteractionMode(int mode)
{
  if (this->InteractionMode != mode)
  {
    this->InteractionMode = mode;
    svtkInteractorObserver* oldStyle = this->GetInteractor()->GetInteractorStyle();
    if (mode == INTERACTION_MODE_2D)
    {
      if (oldStyle)
      {
        oldStyle->RemoveObserver(this->GetObserver());
      }
      svtkInteractorStyleRubberBand2D* style = svtkInteractorStyleRubberBand2D::New();
      this->GetInteractor()->SetInteractorStyle(style);
      style->SetRenderOnMouseMove(this->GetRenderOnMouseMove());
      style->AddObserver(svtkCommand::SelectionChangedEvent, this->GetObserver());
      this->Renderer->GetActiveCamera()->ParallelProjectionOn();
      style->Delete();
    }
    else if (mode == INTERACTION_MODE_3D)
    {
      if (oldStyle)
      {
        oldStyle->RemoveObserver(this->GetObserver());
      }
      svtkInteractorStyleRubberBand3D* style = svtkInteractorStyleRubberBand3D::New();
      this->GetInteractor()->SetInteractorStyle(style);
      style->SetRenderOnMouseMove(this->GetRenderOnMouseMove());
      style->AddObserver(svtkCommand::SelectionChangedEvent, this->GetObserver());
      this->Renderer->GetActiveCamera()->ParallelProjectionOff();
      style->Delete();
    }
    else
    {
      svtkErrorMacro("Unknown interaction mode.");
    }
  }
}

void svtkRenderView::SetRenderWindow(svtkRenderWindow* win)
{
  svtkSmartPointer<svtkRenderWindowInteractor> irenOld = this->GetInteractor();
  this->Superclass::SetRenderWindow(win);
  svtkRenderWindowInteractor* irenNew = this->GetInteractor();
  if (irenOld != irenNew)
  {
    if (irenOld)
    {
      irenOld->RemoveObserver(this->GetObserver());
    }
    if (irenNew)
    {
      this->SetInteractor(irenNew);
    }
  }
}

void svtkRenderView::Render()
{
  // Why should we have to initialize in here at all?
  if (!this->RenderWindow->GetInteractor()->GetInitialized())
  {
    this->RenderWindow->GetInteractor()->Initialize();
  }
  this->PrepareForRendering();
  this->Renderer->ResetCameraClippingRange();
  this->RenderWindow->Render();
}

void svtkRenderView::AddLabels(svtkAlgorithmOutput* conn)
{
  this->LabelPlacementMapper->AddInputConnection(0, conn);
}

void svtkRenderView::RemoveLabels(svtkAlgorithmOutput* conn)
{
  this->LabelPlacementMapper->RemoveInputConnection(0, conn);
}

void svtkRenderView::ProcessEvents(svtkObject* caller, unsigned long eventId, void* callData)
{
  if (caller == this->GetInteractor() && eventId == svtkCommand::RenderEvent)
  {
    svtkDebugMacro(<< "interactor causing a render event.");
    this->Render();
  }
  if (caller == this->HoverWidget.GetPointer() && eventId == svtkCommand::TimerEvent)
  {
    svtkDebugMacro(<< "hover widget timer causing a render event.");
    this->UpdateHoverText();
    this->InHoverTextRender = true;
    this->Render();
    this->InHoverTextRender = false;
  }
  if (caller == this->GetInteractor() && eventId == svtkCommand::StartInteractionEvent)
  {
    this->Interacting = true;
    this->UpdateHoverWidgetState();
  }
  if (caller == this->GetInteractor() && eventId == svtkCommand::EndInteractionEvent)
  {
    this->Interacting = false;
    this->UpdateHoverWidgetState();
    this->PickRenderNeedsUpdate = true;
  }
  if (caller == this->RenderWindow.GetPointer() && eventId == svtkCommand::EndEvent)
  {
    svtkDebugMacro(<< "did a render, interacting: " << this->Interacting << " in pick render: "
                  << this->InPickRender << " in hover text render: " << this->InHoverTextRender);
    if (!this->Interacting && !this->InPickRender && !this->InHoverTextRender)
    {
      // This will cause UpdatePickRender to create a new snapshot of the view
      // for picking with the next drag selection or hover event.
      this->PickRenderNeedsUpdate = true;
    }
  }
  if (svtkDataRepresentation::SafeDownCast(caller) && eventId == svtkCommand::SelectionChangedEvent)
  {
    svtkDebugMacro("selection changed causing a render event");
    this->Render();
  }
  else if (svtkDataRepresentation::SafeDownCast(caller) && eventId == svtkCommand::UpdateEvent)
  {
    // UpdateEvent is called from push pipeline executions from
    // svtkExecutionScheduler. We want to automatically render the view
    // when one of our representations is updated.
    svtkDebugMacro("push pipeline causing a render event");
    this->Render();
  }
  else if (caller == this->GetInteractorStyle() && eventId == svtkCommand::SelectionChangedEvent)
  {
    svtkDebugMacro("interactor style made a selection changed event");
    svtkSmartPointer<svtkSelection> selection = svtkSmartPointer<svtkSelection>::New();
    this->GenerateSelection(callData, selection);

    // This enum value is the same for 2D and 3D interactor styles
    unsigned int* data = reinterpret_cast<unsigned int*>(callData);
    bool extend = (data[4] == svtkInteractorStyleRubberBand2D::SELECT_UNION);

    // Call select on the representation(s)
    for (int i = 0; i < this->GetNumberOfRepresentations(); ++i)
    {
      this->GetRepresentation(i)->Select(this, selection, extend);
    }
  }
  this->Superclass::ProcessEvents(caller, eventId, callData);
}

void svtkRenderView::UpdatePickRender()
{
  if (this->PickRenderNeedsUpdate)
  {
    this->InPickRender = true;
    unsigned int area[4] = { 0, 0, 0, 0 };
    area[2] = static_cast<unsigned int>(this->Renderer->GetSize()[0] - 1);
    area[3] = static_cast<unsigned int>(this->Renderer->GetSize()[1] - 1);
    this->Selector->SetArea(area);
    this->LabelRenderer->DrawOff();
    this->Selector->CaptureBuffers();
    this->LabelRenderer->DrawOn();
    this->InPickRender = false;
    this->PickRenderNeedsUpdate = false;
  }
}

void svtkRenderView::GenerateSelection(void* callData, svtkSelection* sel)
{
  unsigned int* rect = reinterpret_cast<unsigned int*>(callData);
  unsigned int pos1X = rect[0];
  unsigned int pos1Y = rect[1];
  unsigned int pos2X = rect[2];
  unsigned int pos2Y = rect[3];
  int stretch = 2;
  if (pos1X == pos2X && pos1Y == pos2Y)
  {
    pos1X = pos1X - stretch > 0 ? pos1X - stretch : 0;
    pos1Y = pos1Y - stretch > 0 ? pos1Y - stretch : 0;
    pos2X = pos2X + stretch;
    pos2Y = pos2Y + stretch;
  }
  unsigned int screenMinX = pos1X < pos2X ? pos1X : pos2X;
  unsigned int screenMaxX = pos1X < pos2X ? pos2X : pos1X;
  unsigned int screenMinY = pos1Y < pos2Y ? pos1Y : pos2Y;
  unsigned int screenMaxY = pos1Y < pos2Y ? pos2Y : pos1Y;

  if (this->SelectionMode == FRUSTUM)
  {
    // Do a frustum selection.
    double displayRectangle[4] = { static_cast<double>(screenMinX), static_cast<double>(screenMinY),
      static_cast<double>(screenMaxX), static_cast<double>(screenMaxY) };
    svtkSmartPointer<svtkDoubleArray> frustcorners = svtkSmartPointer<svtkDoubleArray>::New();
    frustcorners->SetNumberOfComponents(4);
    frustcorners->SetNumberOfTuples(8);
    // convert screen rectangle to world frustum
    svtkRenderer* renderer = this->GetRenderer();
    double worldP[32];
    int index = 0;
    renderer->SetDisplayPoint(displayRectangle[0], displayRectangle[1], 0.0);
    renderer->DisplayToWorld();
    renderer->GetWorldPoint(&worldP[index * 4]);
    frustcorners->SetTuple4(index, worldP[index * 4], worldP[index * 4 + 1], worldP[index * 4 + 2],
      worldP[index * 4 + 3]);
    index++;
    renderer->SetDisplayPoint(displayRectangle[0], displayRectangle[1], 1.0);
    renderer->DisplayToWorld();
    renderer->GetWorldPoint(&worldP[index * 4]);
    frustcorners->SetTuple4(index, worldP[index * 4], worldP[index * 4 + 1], worldP[index * 4 + 2],
      worldP[index * 4 + 3]);
    index++;
    renderer->SetDisplayPoint(displayRectangle[0], displayRectangle[3], 0.0);
    renderer->DisplayToWorld();
    renderer->GetWorldPoint(&worldP[index * 4]);
    frustcorners->SetTuple4(index, worldP[index * 4], worldP[index * 4 + 1], worldP[index * 4 + 2],
      worldP[index * 4 + 3]);
    index++;
    renderer->SetDisplayPoint(displayRectangle[0], displayRectangle[3], 1.0);
    renderer->DisplayToWorld();
    renderer->GetWorldPoint(&worldP[index * 4]);
    frustcorners->SetTuple4(index, worldP[index * 4], worldP[index * 4 + 1], worldP[index * 4 + 2],
      worldP[index * 4 + 3]);
    index++;
    renderer->SetDisplayPoint(displayRectangle[2], displayRectangle[1], 0.0);
    renderer->DisplayToWorld();
    renderer->GetWorldPoint(&worldP[index * 4]);
    frustcorners->SetTuple4(index, worldP[index * 4], worldP[index * 4 + 1], worldP[index * 4 + 2],
      worldP[index * 4 + 3]);
    index++;
    renderer->SetDisplayPoint(displayRectangle[2], displayRectangle[1], 1.0);
    renderer->DisplayToWorld();
    renderer->GetWorldPoint(&worldP[index * 4]);
    frustcorners->SetTuple4(index, worldP[index * 4], worldP[index * 4 + 1], worldP[index * 4 + 2],
      worldP[index * 4 + 3]);
    index++;
    renderer->SetDisplayPoint(displayRectangle[2], displayRectangle[3], 0.0);
    renderer->DisplayToWorld();
    renderer->GetWorldPoint(&worldP[index * 4]);
    frustcorners->SetTuple4(index, worldP[index * 4], worldP[index * 4 + 1], worldP[index * 4 + 2],
      worldP[index * 4 + 3]);
    index++;
    renderer->SetDisplayPoint(displayRectangle[2], displayRectangle[3], 1.0);
    renderer->DisplayToWorld();
    renderer->GetWorldPoint(&worldP[index * 4]);
    frustcorners->SetTuple4(index, worldP[index * 4], worldP[index * 4 + 1], worldP[index * 4 + 2],
      worldP[index * 4 + 3]);

    svtkSmartPointer<svtkSelectionNode> node = svtkSmartPointer<svtkSelectionNode>::New();
    node->SetContentType(svtkSelectionNode::FRUSTUM);
    node->SetFieldType(svtkSelectionNode::CELL);
    node->SetSelectionList(frustcorners);
    sel->AddNode(node);
  }
  else
  {
    this->UpdatePickRender();
    svtkSelection* vsel =
      this->Selector->GenerateSelection(screenMinX, screenMinY, screenMaxX, screenMaxY);
    sel->ShallowCopy(vsel);
    vsel->Delete();
  }
}

void svtkRenderView::SetDisplayHoverText(bool b)
{
  this->Balloon->SetVisibility(b);
  this->DisplayHoverText = b;
}

void svtkRenderView::UpdateHoverWidgetState()
{
  // Make sure we have a context, then ensure hover widget is
  // enabled if we are displaying hover text.
  this->RenderWindow->MakeCurrent();
  if (this->RenderWindow->IsCurrent())
  {
    if (!this->Interacting &&
      (this->HoverWidget->GetEnabled() ? true : false) != this->DisplayHoverText)
    {
      svtkDebugMacro(<< "turning " << (this->DisplayHoverText ? "on" : "off") << " hover widget");
      this->HoverWidget->SetEnabled(this->DisplayHoverText);
    }
    // Disable hover text when interacting.
    else if (this->Interacting && this->HoverWidget->GetEnabled())
    {
      svtkDebugMacro(<< "turning off hover widget");
      this->HoverWidget->SetEnabled(false);
    }
  }
  if (!this->HoverWidget->GetEnabled())
  {
    this->Balloon->SetBalloonText("");
  }
}

void svtkRenderView::PrepareForRendering()
{
  this->Update();
  this->UpdateHoverWidgetState();

  for (int i = 0; i < this->GetNumberOfRepresentations(); ++i)
  {
    svtkRenderedRepresentation* rep =
      svtkRenderedRepresentation::SafeDownCast(this->GetRepresentation(i));
    if (rep)
    {
      rep->PrepareForRendering(this);
    }
  }
}

void svtkRenderView::UpdateHoverText()
{
  this->UpdatePickRender();

  int pos[2] = { 0, 0 };
  unsigned int upos[2] = { 0, 0 };
  double loc[2] = { 0.0, 0.0 };
  if (this->RenderWindow->GetInteractor())
  {
    this->RenderWindow->GetInteractor()->GetEventPosition(pos);
    loc[0] = pos[0];
    loc[1] = pos[1];
    upos[0] = static_cast<unsigned int>(pos[0]);
    upos[1] = static_cast<unsigned int>(pos[1]);
  }
  this->Balloon->EndWidgetInteraction(loc);

  // The number of pixels away from the pointer to search for hovered objects.
  int hoverTol = 3;

  // Retrieve the hovered cell from the saved buffer.
  svtkHardwareSelector::PixelInformation info = this->Selector->GetPixelInformation(upos, hoverTol);
  svtkIdType cell = info.AttributeID;
  svtkProp* prop = info.Prop;
  if (prop == nullptr || cell == -1)
  {
    this->Balloon->SetBalloonText("");
    return;
  }

  // For debugging
  // std::ostringstream oss;
  // oss << "prop: " << prop << " cell: " << cell;
  // this->Balloon->SetBalloonText(oss.str().c_str());
  // this->Balloon->StartWidgetInteraction(loc);

  svtkUnicodeString hoverText;
  for (int i = 0; i < this->GetNumberOfRepresentations(); ++i)
  {
    svtkRenderedRepresentation* rep =
      svtkRenderedRepresentation::SafeDownCast(this->GetRepresentation(i));
    if (rep && this->RenderWindow->GetInteractor())
    {
      hoverText = rep->GetHoverText(this, prop, cell);
      if (!hoverText.empty())
      {
        break;
      }
    }
  }
  this->Balloon->SetBalloonText(hoverText.utf8_str());
  this->Balloon->StartWidgetInteraction(loc);
  this->InvokeEvent(svtkCommand::HoverEvent, &hoverText);
}

void svtkRenderView::ApplyViewTheme(svtkViewTheme* theme)
{
  this->Renderer->SetBackground(theme->GetBackgroundColor());
  this->Renderer->SetBackground2(theme->GetBackgroundColor2());
  this->Renderer->SetGradientBackground(true);
  for (int i = 0; i < this->GetNumberOfRepresentations(); ++i)
  {
    this->GetRepresentation(i)->ApplyViewTheme(theme);
  }
}

void svtkRenderView::SetLabelPlacementMode(int mode)
{
  this->LabelPlacementMapper->SetPlaceAllLabels(mode == ALL);
}

int svtkRenderView::GetLabelPlacementMode()
{
  return this->LabelPlacementMapper->GetPlaceAllLabels() ? ALL : NO_OVERLAP;
}

int svtkRenderView::GetLabelRenderMode()
{
  return svtkFreeTypeLabelRenderStrategy::SafeDownCast(
           this->LabelPlacementMapper->GetRenderStrategy())
    ? FREETYPE
    : QT;
}

void svtkRenderView::SetLabelRenderMode(int render_mode)
{
  // First, make sure the render mode is set on all the representations.
  // TODO: Setup global labeller render mode
  if (render_mode != this->GetLabelRenderMode())
  {
    // Set label render mode of all representations.
    for (int r = 0; r < this->GetNumberOfRepresentations(); ++r)
    {
      svtkRenderedRepresentation* rr =
        svtkRenderedRepresentation::SafeDownCast(this->GetRepresentation(r));
      if (rr)
      {
        rr->SetLabelRenderMode(render_mode);
      }
    }
  }

  switch (render_mode)
  {
    case QT:
    {
#ifdef SVTK_USE_QT
      svtkSmartPointer<svtkQtLabelRenderStrategy> qts =
        svtkSmartPointer<svtkQtLabelRenderStrategy>::New();
      this->LabelPlacementMapper->SetRenderStrategy(qts);
#else
      svtkErrorMacro("Qt label rendering not supported.");
#endif
      break;
    }
    default:
    {
      svtkSmartPointer<svtkFreeTypeLabelRenderStrategy> fts =
        svtkSmartPointer<svtkFreeTypeLabelRenderStrategy>::New();
      this->LabelPlacementMapper->SetRenderStrategy(fts);
    }
  }
}

int* svtkRenderView::GetDisplaySize()
{
  if (this->DisplaySize[0] == 0 || this->DisplaySize[1] == 0)
  {
    return this->IconSize;
  }
  else
  {
    return this->DisplaySize;
  }
}

void svtkRenderView::GetDisplaySize(int& dsx, int& dsy)
{
  if (this->DisplaySize[0] == 0 || this->DisplaySize[1] == 0)
  {
    dsx = this->IconSize[0];
    dsy = this->IconSize[1];
  }
  else
  {
    dsx = this->DisplaySize[0];
    dsy = this->DisplaySize[1];
  }
}

void svtkRenderView::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "RenderWindow: ";
  if (this->RenderWindow)
  {
    os << "\n";
    this->RenderWindow->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << "(none)\n";
  }
  os << indent << "Renderer: ";
  if (this->Renderer)
  {
    os << "\n";
    this->Renderer->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << "(none)\n";
  }
  os << indent << "SelectionMode: " << this->SelectionMode << endl;
  os << indent << "InteractionMode: " << this->InteractionMode << endl;
  os << indent << "DisplayHoverText: " << this->DisplayHoverText << endl;
  os << indent << "Transform: ";
  if (this->Transform)
  {
    os << "\n";
    this->Transform->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << "(none)\n";
  }
  os << indent << "LabelRenderMode: " << this->LabelRenderMode << endl;
  os << indent << "IconTexture: ";
  if (this->IconTexture)
  {
    os << "\n";
    this->IconTexture->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << "(none)\n";
  }
  os << indent << "IconSize: " << this->IconSize[0] << "," << this->IconSize[1] << endl;
  os << indent << "DisplaySize: " << this->DisplaySize[0] << "," << this->DisplaySize[1] << endl;
  os << indent << "InteractionMode: " << this->InteractionMode << endl;
  os << indent << "RenderOnMouseMove: " << this->RenderOnMouseMove << endl;
}
