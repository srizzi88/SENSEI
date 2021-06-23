/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestDiagram.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkGraphItem.h"

#include "svtkBrush.h"
#include "svtkCallbackCommand.h"
#include "svtkContext2D.h"
#include "svtkContextMouseEvent.h"
#include "svtkContextScene.h"
#include "svtkGraph.h"
#include "svtkImageData.h"
#include "svtkIncrementalForceLayout.h"
#include "svtkMarkerUtilities.h"
#include "svtkObjectFactory.h"
#include "svtkPen.h"
#include "svtkPoints.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkTooltipItem.h"
#include "svtkTransform2D.h"
#include "svtkVectorOperators.h"

#include <vector>

svtkStandardNewMacro(svtkGraphItem);
svtkCxxSetObjectMacro(svtkGraphItem, Graph, svtkGraph);

struct svtkGraphItem::Internals
{
  std::vector<float> VertexSizes;
  std::vector<svtkVector2f> VertexPositions;
  std::vector<svtkColor4ub> VertexColors;
  std::vector<int> VertexMarkers;

  std::vector<std::vector<svtkVector2f> > EdgePositions;
  std::vector<std::vector<svtkColor4ub> > EdgeColors;
  std::vector<float> EdgeWidths;

  bool Animating;
  bool AnimationCallbackInitialized;
  svtkRenderWindowInteractor* Interactor;
  svtkNew<svtkCallbackCommand> AnimationCallback;
  int TimerId;
  bool GravityPointSet;

  float CurrentScale[2];
  svtkVector2f LastMousePos;

  float LayoutAlphaStart;
  float LayoutAlphaCoolDown;
  float LayoutAlphaStop;
};

svtkGraphItem::svtkGraphItem()
{
  this->Graph = nullptr;
  this->GraphBuildTime = 0;
  this->Internal = new Internals();
  this->Internal->Interactor = nullptr;
  this->Internal->Animating = false;
  this->Internal->AnimationCallbackInitialized = false;
  this->Internal->TimerId = 0;
  this->Internal->CurrentScale[0] = 1.0f;
  this->Internal->CurrentScale[1] = 1.0f;
  this->Internal->LastMousePos = svtkVector2f(0, 0);
  this->Internal->LayoutAlphaStart = 0.1f;
  this->Internal->LayoutAlphaCoolDown = 0.99f;
  this->Internal->LayoutAlphaStop = 0.005f;
  this->Internal->GravityPointSet = false;
  this->Tooltip->SetVisible(false);
  this->AddItem(this->Tooltip);
}

svtkGraphItem::~svtkGraphItem()
{
  if (this->Internal->Animating)
  {
    this->StopLayoutAnimation();
  }
  if (this->Internal->AnimationCallbackInitialized)
  {
    this->Internal->Interactor->RemoveObserver(this->Internal->AnimationCallback);
  }
  delete this->Internal;
  if (this->Graph)
  {
    this->Graph->Delete();
  }
}

svtkIncrementalForceLayout* svtkGraphItem::GetLayout()
{
  return this->Layout;
}

svtkColor4ub svtkGraphItem::VertexColor(svtkIdType svtkNotUsed(item))
{
  return svtkColor4ub(128, 128, 128, 255);
}

svtkVector2f svtkGraphItem::VertexPosition(svtkIdType item)
{
  double* p = this->Graph->GetPoints()->GetPoint(item);
  return svtkVector2f(static_cast<float>(p[0]), static_cast<float>(p[1]));
}

float svtkGraphItem::VertexSize(svtkIdType svtkNotUsed(item))
{
  return 10.0f;
}

int svtkGraphItem::VertexMarker(svtkIdType svtkNotUsed(item))
{
  return svtkMarkerUtilities::CIRCLE;
}

svtkStdString svtkGraphItem::VertexTooltip(svtkIdType svtkNotUsed(item))
{
  return "";
}

svtkColor4ub svtkGraphItem::EdgeColor(svtkIdType svtkNotUsed(edgeIdx), svtkIdType svtkNotUsed(point))
{
  return svtkColor4ub(0, 0, 0, 255);
}

svtkVector2f svtkGraphItem::EdgePosition(svtkIdType edgeIdx, svtkIdType point)
{
  double* p;
  if (point == 0)
  {
    svtkPoints* points = this->Graph->GetPoints();
    p = points->GetPoint(this->Graph->GetSourceVertex(edgeIdx));
  }
  else if (point == this->NumberOfEdgePoints(edgeIdx) - 1)
  {
    svtkPoints* points = this->Graph->GetPoints();
    p = points->GetPoint(this->Graph->GetTargetVertex(edgeIdx));
  }
  else
  {
    p = this->Graph->GetEdgePoint(edgeIdx, point - 1);
  }
  return svtkVector2f(static_cast<float>(p[0]), static_cast<float>(p[1]));
}

float svtkGraphItem::EdgeWidth(svtkIdType svtkNotUsed(line), svtkIdType svtkNotUsed(point))
{
  return 0.0f;
}

void svtkGraphItem::RebuildBuffers()
{
  svtkIdType numEdges = this->NumberOfEdges();
  this->Internal->EdgePositions = std::vector<std::vector<svtkVector2f> >(numEdges);
  this->Internal->EdgeColors = std::vector<std::vector<svtkColor4ub> >(numEdges);
  this->Internal->EdgeWidths = std::vector<float>(numEdges);
  for (svtkIdType edgeIdx = 0; edgeIdx < numEdges; ++edgeIdx)
  {
    svtkIdType numPoints = this->NumberOfEdgePoints(edgeIdx);
    this->Internal->EdgePositions[edgeIdx] = std::vector<svtkVector2f>(numPoints);
    this->Internal->EdgeColors[edgeIdx] = std::vector<svtkColor4ub>(numPoints);
    this->Internal->EdgeWidths[edgeIdx] = this->EdgeWidth(edgeIdx, 0);
    for (svtkIdType pointIdx = 0; pointIdx < numPoints; ++pointIdx)
    {
      this->Internal->EdgePositions[edgeIdx][pointIdx] = this->EdgePosition(edgeIdx, pointIdx);
      this->Internal->EdgeColors[edgeIdx][pointIdx] = this->EdgeColor(edgeIdx, pointIdx);
    }
  }

  svtkIdType numVertices = this->NumberOfVertices();
  this->Internal->VertexPositions = std::vector<svtkVector2f>(numVertices);
  this->Internal->VertexColors = std::vector<svtkColor4ub>(numVertices);
  this->Internal->VertexSizes = std::vector<float>(numVertices);
  this->Internal->VertexMarkers = std::vector<int>(numVertices);
  svtkMarkerUtilities::GenerateMarker(
    this->Sprite, this->VertexMarker(0), static_cast<int>(this->VertexSize(0)));
  for (svtkIdType vertexIdx = 0; vertexIdx < numVertices; ++vertexIdx)
  {
    this->Internal->VertexPositions[vertexIdx] = this->VertexPosition(vertexIdx);
    this->Internal->VertexColors[vertexIdx] = this->VertexColor(vertexIdx);
    this->Internal->VertexSizes[vertexIdx] = this->VertexSize(vertexIdx);
    this->Internal->VertexMarkers[vertexIdx] = this->VertexMarker(vertexIdx);
  }
}

void svtkGraphItem::PaintBuffers(svtkContext2D* painter)
{
  if (this->Internal->EdgePositions.empty())
  {
    return;
  }
  svtkIdType numEdges = static_cast<svtkIdType>(this->Internal->EdgePositions.size());
  for (svtkIdType edgeIdx = 0; edgeIdx < numEdges; ++edgeIdx)
  {
    if (this->Internal->EdgePositions[edgeIdx].empty())
    {
      continue;
    }
    painter->GetPen()->SetWidth(this->Internal->EdgeWidths[edgeIdx]);
    painter->DrawPoly(this->Internal->EdgePositions[edgeIdx][0].GetData(),
      static_cast<int>(this->Internal->EdgePositions[edgeIdx].size()),
      this->Internal->EdgeColors[edgeIdx][0].GetData(), 4);
  }

  if (this->Internal->VertexPositions.empty())
  {
    return;
  }
  painter->GetPen()->SetWidth(this->Internal->VertexSizes[0]);
  painter->GetBrush()->SetTextureProperties(svtkBrush::Linear);
  painter->DrawPointSprites(this->Sprite, this->Internal->VertexPositions[0].GetData(),
    static_cast<int>(this->Internal->VertexPositions.size()),
    this->Internal->VertexColors[0].GetData(), 4);
}

svtkIdType svtkGraphItem::NumberOfVertices()
{
  if (!this->Graph)
  {
    return 0;
  }
  return this->Graph->GetNumberOfVertices();
}

svtkIdType svtkGraphItem::NumberOfEdges()
{
  if (!this->Graph)
  {
    return 0;
  }
  return this->Graph->GetNumberOfEdges();
}

svtkIdType svtkGraphItem::NumberOfEdgePoints(svtkIdType edgeIdx)
{
  if (!this->Graph)
  {
    return 0;
  }
  return this->Graph->GetNumberOfEdgePoints(edgeIdx) + 2;
}

bool svtkGraphItem::IsDirty()
{
  if (!this->Graph)
  {
    return false;
  }
  if (this->Graph->GetMTime() > this->GraphBuildTime)
  {
    this->GraphBuildTime = this->Graph->GetMTime();
    return true;
  }
  return false;
}

bool svtkGraphItem::Paint(svtkContext2D* painter)
{
  if (this->IsDirty())
  {
    this->RebuildBuffers();
  }
  this->PaintBuffers(painter);
  this->PaintChildren(painter);

  // Keep the current scale so we can use it in event handlers.
  painter->GetTransform()->GetScale(this->Internal->CurrentScale);

  return true;
}

void svtkGraphItem::ProcessEvents(
  svtkObject* svtkNotUsed(caller), unsigned long event, void* clientData, void* callerData)
{
  svtkGraphItem* self = reinterpret_cast<svtkGraphItem*>(clientData);
  switch (event)
  {
    case svtkCommand::TimerEvent:
    {
      // We must filter the events to ensure we actually get the timer event we
      // created. I would love signals and slots...
      int timerId = *static_cast<int*>(callerData); // Seems to work.
      if (self->Internal->Animating && timerId == static_cast<int>(self->Internal->TimerId))
      {
        self->UpdateLayout();
        svtkIdType v = self->HitVertex(self->Internal->LastMousePos);
        self->PlaceTooltip(v);
        self->GetScene()->SetDirty(true);
      }
      break;
    }
    default:
      break;
  }
}

void svtkGraphItem::StartLayoutAnimation(svtkRenderWindowInteractor* interactor)
{
  // Start a simple repeating timer
  if (!this->Internal->Animating && interactor)
  {
    if (!this->Internal->AnimationCallbackInitialized)
    {
      this->Internal->AnimationCallback->SetClientData(this);
      this->Internal->AnimationCallback->SetCallback(svtkGraphItem::ProcessEvents);
      interactor->AddObserver(svtkCommand::TimerEvent, this->Internal->AnimationCallback, 0);
      this->Internal->Interactor = interactor;
      this->Internal->AnimationCallbackInitialized = true;
    }
    this->Internal->Animating = true;
    // This defines the interval at which the animation will proceed. 60Hz?
    this->Internal->TimerId = interactor->CreateRepeatingTimer(1000 / 60);
    if (!this->Internal->GravityPointSet)
    {
      svtkVector2f screenPos(
        this->Scene->GetSceneWidth() / 2.0f, this->Scene->GetSceneHeight() / 2.0f);
      svtkVector2f pos = this->MapFromScene(screenPos);
      this->Layout->SetGravityPoint(pos);
      this->Internal->GravityPointSet = true;
    }
    this->Layout->SetAlpha(this->Internal->LayoutAlphaStart);
  }
}

void svtkGraphItem::StopLayoutAnimation()
{
  this->Internal->Interactor->DestroyTimer(this->Internal->TimerId);
  this->Internal->TimerId = 0;
  this->Internal->Animating = false;
}

void svtkGraphItem::UpdateLayout()
{
  if (this->Graph)
  {
    this->Layout->SetGraph(this->Graph);
    this->Layout->SetAlpha(this->Layout->GetAlpha() * this->Internal->LayoutAlphaCoolDown);
    this->Layout->UpdatePositions();
    this->Graph->Modified();
    if (this->Internal->Animating && this->Layout->GetAlpha() < this->Internal->LayoutAlphaStop)
    {
      this->StopLayoutAnimation();
    }
  }
}

svtkIdType svtkGraphItem::HitVertex(const svtkVector2f& pos)
{
  svtkIdType numVert = static_cast<svtkIdType>(this->Internal->VertexPositions.size());
  for (svtkIdType v = 0; v < numVert; ++v)
  {
    if ((pos - this->Internal->VertexPositions[v]).Norm() <
      this->Internal->VertexSizes[v] / this->Internal->CurrentScale[0] / 2.0)
    {
      return v;
    }
  }
  return -1;
}

bool svtkGraphItem::MouseMoveEvent(const svtkContextMouseEvent& event)
{
  this->Internal->LastMousePos = event.GetPos();
  if (event.GetButton() == svtkContextMouseEvent::NO_BUTTON)
  {
    svtkIdType v = this->HitVertex(event.GetPos());
    this->Scene->SetDirty(true);
    if (v < 0)
    {
      this->Tooltip->SetVisible(false);
      return true;
    }
    svtkStdString text = this->VertexTooltip(v);
    if (text.empty())
    {
      this->Tooltip->SetVisible(false);
      return true;
    }
    this->PlaceTooltip(v);
    this->Tooltip->SetText(text);
    this->Tooltip->SetVisible(true);
    return true;
  }
  if (event.GetButton() == svtkContextMouseEvent::LEFT_BUTTON)
  {
    if (this->Layout->GetFixed() >= 0)
    {
      this->Layout->SetAlpha(this->Internal->LayoutAlphaStart);
      this->Graph->GetPoints()->SetPoint(
        this->Layout->GetFixed(), event.GetPos()[0], event.GetPos()[1], 0.0);
    }
    return true;
  }

  if (this->Tooltip->GetVisible())
  {
    svtkIdType v = this->HitVertex(event.GetPos());
    this->PlaceTooltip(v);
    this->Scene->SetDirty(true);
  }

  return false;
}

bool svtkGraphItem::MouseEnterEvent(const svtkContextMouseEvent& svtkNotUsed(event))
{
  return true;
}

bool svtkGraphItem::MouseLeaveEvent(const svtkContextMouseEvent& svtkNotUsed(event))
{
  this->Tooltip->SetVisible(false);
  return true;
}

bool svtkGraphItem::MouseButtonPressEvent(const svtkContextMouseEvent& event)
{
  this->Tooltip->SetVisible(false);
  if (event.GetButton() == svtkContextMouseEvent::LEFT_BUTTON)
  {
    svtkIdType hitVertex = this->HitVertex(event.GetPos());
    this->Layout->SetFixed(hitVertex);
    if (hitVertex >= 0 && this->Internal->Interactor)
    {
      this->Layout->SetAlpha(this->Internal->LayoutAlphaStart);
      if (!this->Internal->Animating && this->Internal->Interactor)
      {
        this->StartLayoutAnimation(this->Internal->Interactor);
      }
    }
    return true;
  }
  return false;
}

bool svtkGraphItem::MouseButtonReleaseEvent(const svtkContextMouseEvent& event)
{
  if (event.GetButton() == svtkContextMouseEvent::LEFT_BUTTON)
  {
    this->Layout->SetFixed(-1);
    return true;
  }
  return false;
}

bool svtkGraphItem::MouseWheelEvent(const svtkContextMouseEvent& event, int svtkNotUsed(delta))
{
  if (this->Tooltip->GetVisible())
  {
    svtkIdType v = this->HitVertex(event.GetPos());
    this->PlaceTooltip(v);
    this->Scene->SetDirty(true);
  }

  return false;
}

bool svtkGraphItem::Hit(const svtkContextMouseEvent& event)
{
  svtkIdType v = this->HitVertex(event.GetPos());
  return (v >= 0);
}

void svtkGraphItem::PlaceTooltip(svtkIdType v)
{
  if (v >= 0)
  {
    svtkVector2f pos = this->Internal->VertexPositions[v];
    this->Tooltip->SetPosition(
      pos[0] + 5 / this->Internal->CurrentScale[0], pos[1] + 5 / this->Internal->CurrentScale[1]);
  }
  else
  {
    this->Tooltip->SetVisible(false);
  }
}

void svtkGraphItem::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << "Graph: " << (this->Graph ? "" : "(null)") << std::endl;
  if (this->Graph)
  {
    this->Graph->PrintSelf(os, indent.GetNextIndent());
  }
  os << "GraphBuildTime: " << this->GraphBuildTime << std::endl;
}
