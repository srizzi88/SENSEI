/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkGraphLayoutView.cxx

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

#include "svtkGraphLayoutView.h"

#include "svtkAlgorithmOutput.h"
#include "svtkCamera.h"
#include "svtkCommand.h"
#include "svtkDirectedGraph.h"
#include "svtkFast2DLayoutStrategy.h"
#include "svtkInteractorStyle.h"
#include "svtkObjectFactory.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderedGraphRepresentation.h"
#include "svtkRenderer.h"
#include "svtkSelection.h"
#include "svtkSimple2DLayoutStrategy.h"
#include "svtkTextProperty.h"

svtkStandardNewMacro(svtkGraphLayoutView);
//----------------------------------------------------------------------------
svtkGraphLayoutView::svtkGraphLayoutView()
{
  this->SetInteractionModeTo2D();
  this->SetSelectionModeToFrustum();
  this->ReuseSingleRepresentationOn();
  this->VertexLabelsRequested = false;
  this->EdgeLabelsRequested = false;
  this->Interacting = false;
}

//----------------------------------------------------------------------------
svtkGraphLayoutView::~svtkGraphLayoutView() = default;

//----------------------------------------------------------------------------
svtkRenderedGraphRepresentation* svtkGraphLayoutView::GetGraphRepresentation()
{
  svtkRenderedGraphRepresentation* graphRep = nullptr;
  for (int i = 0; i < this->GetNumberOfRepresentations(); ++i)
  {
    svtkDataRepresentation* rep = this->GetRepresentation(i);
    graphRep = svtkRenderedGraphRepresentation::SafeDownCast(rep);
    if (graphRep)
    {
      break;
    }
  }
  if (!graphRep)
  {
    svtkSmartPointer<svtkDirectedGraph> g = svtkSmartPointer<svtkDirectedGraph>::New();
    graphRep = svtkRenderedGraphRepresentation::SafeDownCast(this->AddRepresentationFromInput(g));
  }
  return graphRep;
}

//----------------------------------------------------------------------------
svtkDataRepresentation* svtkGraphLayoutView::CreateDefaultRepresentation(svtkAlgorithmOutput* port)
{
  svtkRenderedGraphRepresentation* rep = svtkRenderedGraphRepresentation::New();
  rep->SetInputConnection(port);
  return rep;
}

//----------------------------------------------------------------------------
void svtkGraphLayoutView::ProcessEvents(svtkObject* caller, unsigned long eventId, void* callData)
{
  if (eventId == svtkCommand::StartInteractionEvent)
  {
    if (GetHideVertexLabelsOnInteraction() && this->VertexLabelsRequested)
    {
      this->Interacting = true;
      this->GetGraphRepresentation()->SetVertexLabelVisibility(false);
    }
    if (GetHideEdgeLabelsOnInteraction() && this->EdgeLabelsRequested)
    {
      this->Interacting = true;
      this->GetGraphRepresentation()->SetEdgeLabelVisibility(false);
    }
  }
  if (eventId == svtkCommand::EndInteractionEvent)
  {
    bool forceRender = false;
    if (GetHideVertexLabelsOnInteraction() && this->VertexLabelsRequested)
    {
      this->Interacting = false;
      this->GetGraphRepresentation()->SetVertexLabelVisibility(true);
      // Force the labels to reappear
      forceRender = true;
    }
    if (GetHideEdgeLabelsOnInteraction() && this->EdgeLabelsRequested)
    {
      this->Interacting = false;
      this->GetGraphRepresentation()->SetEdgeLabelVisibility(true);
      // Force the labels to reappear
      forceRender = true;
    }
    if (forceRender)
      // Force the labels to reappear
      this->Render();
  }
  if (eventId != svtkCommand::ComputeVisiblePropBoundsEvent)
  {
    this->Superclass::ProcessEvents(caller, eventId, callData);
  }
}

//----------------------------------------------------------------------------
void svtkGraphLayoutView::SetVertexLabelArrayName(const char* name)
{
  this->GetGraphRepresentation()->SetVertexLabelArrayName(name);
}

//----------------------------------------------------------------------------
const char* svtkGraphLayoutView::GetVertexLabelArrayName()
{
  return this->GetGraphRepresentation()->GetVertexLabelArrayName();
}

//----------------------------------------------------------------------------
void svtkGraphLayoutView::SetEdgeLabelArrayName(const char* name)
{
  this->GetGraphRepresentation()->SetEdgeLabelArrayName(name);
}

//----------------------------------------------------------------------------
const char* svtkGraphLayoutView::GetEdgeLabelArrayName()
{
  return this->GetGraphRepresentation()->GetEdgeLabelArrayName();
}

//----------------------------------------------------------------------------
void svtkGraphLayoutView::SetVertexLabelVisibility(bool vis)
{
  this->VertexLabelsRequested = vis;
  // Don't update the visibility of the vertex label actor while an interaction
  // is in progress
  if (!this->Interacting)
    this->GetGraphRepresentation()->SetVertexLabelVisibility(vis);
}

//----------------------------------------------------------------------------
bool svtkGraphLayoutView::GetVertexLabelVisibility()
{
  return this->GetGraphRepresentation()->GetVertexLabelVisibility();
}

//----------------------------------------------------------------------------
void svtkGraphLayoutView::SetHideVertexLabelsOnInteraction(bool vis)
{
  this->GetGraphRepresentation()->SetHideVertexLabelsOnInteraction(vis);
}

//----------------------------------------------------------------------------
bool svtkGraphLayoutView::GetHideVertexLabelsOnInteraction()
{
  return this->GetGraphRepresentation()->GetHideVertexLabelsOnInteraction();
}

//----------------------------------------------------------------------------
void svtkGraphLayoutView::SetEdgeVisibility(bool vis)
{
  this->GetGraphRepresentation()->SetEdgeVisibility(vis);
}

//----------------------------------------------------------------------------
bool svtkGraphLayoutView::GetEdgeVisibility()
{
  return this->GetGraphRepresentation()->GetEdgeVisibility();
}

//----------------------------------------------------------------------------
void svtkGraphLayoutView::SetEdgeLabelVisibility(bool vis)
{
  this->EdgeLabelsRequested = vis;
  // Don't update the visibility of the edge label actor while an interaction
  // is in progress
  if (!this->Interacting)
    this->GetGraphRepresentation()->SetEdgeLabelVisibility(vis);
}

//----------------------------------------------------------------------------
bool svtkGraphLayoutView::GetEdgeLabelVisibility()
{
  return this->GetGraphRepresentation()->GetEdgeLabelVisibility();
}

//----------------------------------------------------------------------------
void svtkGraphLayoutView::SetHideEdgeLabelsOnInteraction(bool vis)
{
  this->GetGraphRepresentation()->SetHideEdgeLabelsOnInteraction(vis);
}

//----------------------------------------------------------------------------
bool svtkGraphLayoutView::GetHideEdgeLabelsOnInteraction()
{
  return this->GetGraphRepresentation()->GetHideEdgeLabelsOnInteraction();
}

//----------------------------------------------------------------------------
void svtkGraphLayoutView::SetVertexColorArrayName(const char* name)
{
  this->GetGraphRepresentation()->SetVertexColorArrayName(name);
}

//----------------------------------------------------------------------------
const char* svtkGraphLayoutView::GetVertexColorArrayName()
{
  return this->GetGraphRepresentation()->GetVertexColorArrayName();
}

//----------------------------------------------------------------------------
void svtkGraphLayoutView::SetColorVertices(bool vis)
{
  this->GetGraphRepresentation()->SetColorVerticesByArray(vis);
}

//----------------------------------------------------------------------------
bool svtkGraphLayoutView::GetColorVertices()
{
  return this->GetGraphRepresentation()->GetColorVerticesByArray();
}

//----------------------------------------------------------------------------
void svtkGraphLayoutView::SetVertexScalarBarVisibility(bool vis)
{
  this->GetGraphRepresentation()->SetVertexScalarBarVisibility(vis);
}

//----------------------------------------------------------------------------
bool svtkGraphLayoutView::GetVertexScalarBarVisibility()
{
  return this->GetGraphRepresentation()->GetVertexScalarBarVisibility();
}

//----------------------------------------------------------------------------
void svtkGraphLayoutView::SetEdgeColorArrayName(const char* name)
{
  this->GetGraphRepresentation()->SetEdgeColorArrayName(name);
}

//----------------------------------------------------------------------------
const char* svtkGraphLayoutView::GetEdgeColorArrayName()
{
  return this->GetGraphRepresentation()->GetEdgeColorArrayName();
}

//----------------------------------------------------------------------------
void svtkGraphLayoutView::SetColorEdges(bool vis)
{
  this->GetGraphRepresentation()->SetColorEdgesByArray(vis);
}

//----------------------------------------------------------------------------
bool svtkGraphLayoutView::GetColorEdges()
{
  return this->GetGraphRepresentation()->GetColorEdgesByArray();
}
//----------------------------------------------------------------------------
void svtkGraphLayoutView::SetEdgeSelection(bool vis)
{
  this->GetGraphRepresentation()->SetEdgeSelection(vis);
}

//----------------------------------------------------------------------------
bool svtkGraphLayoutView::GetEdgeSelection()
{
  return this->GetGraphRepresentation()->GetEdgeSelection();
}
//----------------------------------------------------------------------------
void svtkGraphLayoutView::SetEdgeScalarBarVisibility(bool vis)
{
  this->GetGraphRepresentation()->SetEdgeScalarBarVisibility(vis);
}

//----------------------------------------------------------------------------
bool svtkGraphLayoutView::GetEdgeScalarBarVisibility()
{
  return this->GetGraphRepresentation()->GetEdgeScalarBarVisibility();
}

//----------------------------------------------------------------------------
void svtkGraphLayoutView::SetEnabledEdgesArrayName(const char* name)
{
  this->GetGraphRepresentation()->SetEnabledEdgesArrayName(name);
}

//----------------------------------------------------------------------------
const char* svtkGraphLayoutView::GetEnabledEdgesArrayName()
{
  return this->GetGraphRepresentation()->GetEnabledEdgesArrayName();
}

//----------------------------------------------------------------------------
void svtkGraphLayoutView::SetEnableEdgesByArray(bool vis)
{
  this->GetGraphRepresentation()->SetEnableEdgesByArray(vis);
}

//----------------------------------------------------------------------------
int svtkGraphLayoutView::GetEnableEdgesByArray()
{
  return this->GetGraphRepresentation()->GetEnableEdgesByArray();
}

//----------------------------------------------------------------------------
void svtkGraphLayoutView::SetEnabledVerticesArrayName(const char* name)
{
  this->GetGraphRepresentation()->SetEnabledVerticesArrayName(name);
}

//----------------------------------------------------------------------------
const char* svtkGraphLayoutView::GetEnabledVerticesArrayName()
{
  return this->GetGraphRepresentation()->GetEnabledVerticesArrayName();
}

//----------------------------------------------------------------------------
void svtkGraphLayoutView::SetEnableVerticesByArray(bool vis)
{
  this->GetGraphRepresentation()->SetEnableVerticesByArray(vis);
}

//----------------------------------------------------------------------------
int svtkGraphLayoutView::GetEnableVerticesByArray()
{
  return this->GetGraphRepresentation()->GetEnableVerticesByArray();
}

//----------------------------------------------------------------------------
void svtkGraphLayoutView::SetGlyphType(int type)
{
  this->GetGraphRepresentation()->SetGlyphType(type);
}

//----------------------------------------------------------------------------
int svtkGraphLayoutView::GetGlyphType()
{
  return this->GetGraphRepresentation()->GetGlyphType();
}

//----------------------------------------------------------------------------
void svtkGraphLayoutView::SetScaledGlyphs(bool arg)
{
  this->GetGraphRepresentation()->SetScaling(arg);
}

//----------------------------------------------------------------------------
bool svtkGraphLayoutView::GetScaledGlyphs()
{
  return this->GetGraphRepresentation()->GetScaling();
}

//----------------------------------------------------------------------------
void svtkGraphLayoutView::SetScalingArrayName(const char* name)
{
  this->GetGraphRepresentation()->SetScalingArrayName(name);
}

//----------------------------------------------------------------------------
const char* svtkGraphLayoutView::GetScalingArrayName()
{
  return this->GetGraphRepresentation()->GetScalingArrayName();
}

//----------------------------------------------------------------------------
void svtkGraphLayoutView::SetIconArrayName(const char* name)
{
  this->GetGraphRepresentation()->SetVertexIconArrayName(name);
}

//----------------------------------------------------------------------------
const char* svtkGraphLayoutView::GetIconArrayName()
{
  return this->GetGraphRepresentation()->GetVertexIconArrayName();
}

//----------------------------------------------------------------------------
void svtkGraphLayoutView::AddIconType(const char* type, int index)
{
  this->GetGraphRepresentation()->AddVertexIconType(type, index);
}

//----------------------------------------------------------------------------
void svtkGraphLayoutView::ClearIconTypes()
{
  this->GetGraphRepresentation()->ClearVertexIconTypes();
}

//----------------------------------------------------------------------------
int svtkGraphLayoutView::IsLayoutComplete()
{
  return this->GetGraphRepresentation()->IsLayoutComplete();
}

//----------------------------------------------------------------------------
void svtkGraphLayoutView::UpdateLayout()
{
  this->GetGraphRepresentation()->UpdateLayout();
}

//----------------------------------------------------------------------------
void svtkGraphLayoutView::SetLayoutStrategy(svtkGraphLayoutStrategy* s)
{
  this->GetGraphRepresentation()->SetLayoutStrategy(s);
}

//----------------------------------------------------------------------------
void svtkGraphLayoutView::SetLayoutStrategy(const char* name)
{
  this->GetGraphRepresentation()->SetLayoutStrategy(name);
}

//----------------------------------------------------------------------------
svtkGraphLayoutStrategy* svtkGraphLayoutView::GetLayoutStrategy()
{
  return this->GetGraphRepresentation()->GetLayoutStrategy();
}

//----------------------------------------------------------------------------
const char* svtkGraphLayoutView::GetLayoutStrategyName()
{
  return this->GetGraphRepresentation()->GetLayoutStrategyName();
}

//----------------------------------------------------------------------------
void svtkGraphLayoutView::SetEdgeLayoutStrategy(svtkEdgeLayoutStrategy* s)
{
  this->GetGraphRepresentation()->SetEdgeLayoutStrategy(s);
}

//----------------------------------------------------------------------------
svtkEdgeLayoutStrategy* svtkGraphLayoutView::GetEdgeLayoutStrategy()
{
  return this->GetGraphRepresentation()->GetEdgeLayoutStrategy();
}

//----------------------------------------------------------------------------
void svtkGraphLayoutView::SetEdgeLayoutStrategy(const char* name)
{
  this->GetGraphRepresentation()->SetEdgeLayoutStrategy(name);
}

//----------------------------------------------------------------------------
const char* svtkGraphLayoutView::GetEdgeLayoutStrategyName()
{
  return this->GetGraphRepresentation()->GetEdgeLayoutStrategyName();
}

//----------------------------------------------------------------------------
void svtkGraphLayoutView::SetIconAlignment(int alignment)
{
  this->GetGraphRepresentation()->SetVertexIconAlignment(alignment);
}

//----------------------------------------------------------------------------
void svtkGraphLayoutView::SetIconVisibility(bool b)
{
  this->GetGraphRepresentation()->SetVertexIconVisibility(b);
}

//----------------------------------------------------------------------------
bool svtkGraphLayoutView::GetIconVisibility()
{
  return this->GetGraphRepresentation()->GetVertexIconVisibility();
}

//----------------------------------------------------------------------------
void svtkGraphLayoutView::SetVertexLabelFontSize(const int size)
{
  this->GetGraphRepresentation()->GetVertexLabelTextProperty()->SetFontSize(size);
}

//----------------------------------------------------------------------------
int svtkGraphLayoutView::GetVertexLabelFontSize()
{
  return this->GetGraphRepresentation()->GetVertexLabelTextProperty()->GetFontSize();
}

//----------------------------------------------------------------------------
void svtkGraphLayoutView::SetEdgeLabelFontSize(const int size)
{
  this->GetGraphRepresentation()->GetEdgeLabelTextProperty()->SetFontSize(size);
}

//----------------------------------------------------------------------------
int svtkGraphLayoutView::GetEdgeLabelFontSize()
{
  return this->GetGraphRepresentation()->GetEdgeLabelTextProperty()->GetFontSize();
}

//----------------------------------------------------------------------------
void svtkGraphLayoutView::ZoomToSelection()
{
  double bounds[6];
  this->GetGraphRepresentation()->ComputeSelectedGraphBounds(bounds);
  this->Renderer->ResetCamera(bounds);
}

//----------------------------------------------------------------------------
void svtkGraphLayoutView::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
