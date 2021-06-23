/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkHierarchicalGraphView.cxx

-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkHierarchicalGraphView.h"

#include "svtkDataObject.h"
#include "svtkDirectedGraph.h"
#include "svtkObjectFactory.h"
#include "svtkRenderedHierarchyRepresentation.h"
#include "svtkTree.h"
#include "svtkTrivialProducer.h"

svtkStandardNewMacro(svtkHierarchicalGraphView);
//----------------------------------------------------------------------------
svtkHierarchicalGraphView::svtkHierarchicalGraphView() = default;

//----------------------------------------------------------------------------
svtkHierarchicalGraphView::~svtkHierarchicalGraphView() = default;

//----------------------------------------------------------------------------
svtkRenderedGraphRepresentation* svtkHierarchicalGraphView::GetGraphRepresentation()
{
  svtkRenderedHierarchyRepresentation* graphRep = nullptr;
  for (int i = 0; i < this->GetNumberOfRepresentations(); ++i)
  {
    svtkDataRepresentation* rep = this->GetRepresentation(i);
    graphRep = svtkRenderedHierarchyRepresentation::SafeDownCast(rep);
    if (graphRep)
    {
      break;
    }
  }
  if (!graphRep)
  {
    svtkSmartPointer<svtkTree> t = svtkSmartPointer<svtkTree>::New();
    graphRep =
      svtkRenderedHierarchyRepresentation::SafeDownCast(this->AddRepresentationFromInput(t));
    svtkSmartPointer<svtkDirectedGraph> g = svtkSmartPointer<svtkDirectedGraph>::New();
    graphRep->SetInputData(1, g);
  }
  return graphRep;
}

//----------------------------------------------------------------------------
svtkRenderedHierarchyRepresentation* svtkHierarchicalGraphView::GetHierarchyRepresentation()
{
  return svtkRenderedHierarchyRepresentation::SafeDownCast(this->GetGraphRepresentation());
}

//----------------------------------------------------------------------------
svtkDataRepresentation* svtkHierarchicalGraphView::CreateDefaultRepresentation(
  svtkAlgorithmOutput* port)
{
  svtkRenderedHierarchyRepresentation* rep = svtkRenderedHierarchyRepresentation::New();
  rep->SetInputConnection(port);
  return rep;
}

//----------------------------------------------------------------------------
svtkDataRepresentation* svtkHierarchicalGraphView::SetHierarchyFromInputConnection(
  svtkAlgorithmOutput* conn)
{
  this->GetHierarchyRepresentation()->SetInputConnection(0, conn);
  return this->GetHierarchyRepresentation();
}

//----------------------------------------------------------------------------
svtkDataRepresentation* svtkHierarchicalGraphView::SetHierarchyFromInput(svtkDataObject* input)
{
  svtkSmartPointer<svtkTrivialProducer> tp = svtkSmartPointer<svtkTrivialProducer>::New();
  tp->SetOutput(input);
  return this->SetHierarchyFromInputConnection(tp->GetOutputPort());
}

//----------------------------------------------------------------------------
svtkDataRepresentation* svtkHierarchicalGraphView::SetGraphFromInputConnection(
  svtkAlgorithmOutput* conn)
{
  this->GetHierarchyRepresentation()->SetInputConnection(1, conn);
  return this->GetHierarchyRepresentation();
}

//----------------------------------------------------------------------------
svtkDataRepresentation* svtkHierarchicalGraphView::SetGraphFromInput(svtkDataObject* input)
{
  svtkSmartPointer<svtkTrivialProducer> tp = svtkSmartPointer<svtkTrivialProducer>::New();
  tp->SetOutput(input);
  return this->SetGraphFromInputConnection(tp->GetOutputPort());
}

//----------------------------------------------------------------------------
void svtkHierarchicalGraphView::SetGraphEdgeLabelArrayName(const char* name)
{
  this->GetHierarchyRepresentation()->SetGraphEdgeLabelArrayName(name);
}

//----------------------------------------------------------------------------
const char* svtkHierarchicalGraphView::GetGraphEdgeLabelArrayName()
{
  return this->GetHierarchyRepresentation()->GetGraphEdgeLabelArrayName();
}

//----------------------------------------------------------------------------
void svtkHierarchicalGraphView::SetGraphEdgeLabelVisibility(bool vis)
{
  this->GetHierarchyRepresentation()->SetGraphEdgeLabelVisibility(vis);
}

//----------------------------------------------------------------------------
bool svtkHierarchicalGraphView::GetGraphEdgeLabelVisibility()
{
  return this->GetHierarchyRepresentation()->GetGraphEdgeLabelVisibility();
}

//----------------------------------------------------------------------------
void svtkHierarchicalGraphView::SetGraphEdgeColorArrayName(const char* name)
{
  this->GetHierarchyRepresentation()->SetGraphEdgeColorArrayName(name);
}

//----------------------------------------------------------------------------
const char* svtkHierarchicalGraphView::GetGraphEdgeColorArrayName()
{
  return this->GetHierarchyRepresentation()->GetGraphEdgeColorArrayName();
}

//----------------------------------------------------------------------------
void svtkHierarchicalGraphView::SetGraphEdgeColorToSplineFraction()
{
  this->GetHierarchyRepresentation()->SetGraphEdgeColorToSplineFraction();
}

//----------------------------------------------------------------------------
void svtkHierarchicalGraphView::SetColorGraphEdgesByArray(bool vis)
{
  this->GetHierarchyRepresentation()->SetColorGraphEdgesByArray(vis);
}

//----------------------------------------------------------------------------
bool svtkHierarchicalGraphView::GetColorGraphEdgesByArray()
{
  return this->GetHierarchyRepresentation()->GetColorGraphEdgesByArray();
}

//----------------------------------------------------------------------------
void svtkHierarchicalGraphView::SetGraphVisibility(bool vis)
{
  this->GetHierarchyRepresentation()->SetGraphVisibility(vis);
}

//----------------------------------------------------------------------------
bool svtkHierarchicalGraphView::GetGraphVisibility()
{
  return this->GetHierarchyRepresentation()->GetGraphVisibility();
}

// ----------------------------------------------------------------------
void svtkHierarchicalGraphView::SetBundlingStrength(double strength)
{
  this->GetHierarchyRepresentation()->SetBundlingStrength(strength);
}

// ----------------------------------------------------------------------
double svtkHierarchicalGraphView::GetBundlingStrength()
{
  return this->GetHierarchyRepresentation()->GetBundlingStrength();
}

//----------------------------------------------------------------------------
void svtkHierarchicalGraphView::SetGraphEdgeLabelFontSize(const int size)
{
  this->GetHierarchyRepresentation()->SetGraphEdgeLabelFontSize(size);
}

//----------------------------------------------------------------------------
int svtkHierarchicalGraphView::GetGraphEdgeLabelFontSize()
{
  return this->GetHierarchyRepresentation()->GetGraphEdgeLabelFontSize();
}

//----------------------------------------------------------------------------
void svtkHierarchicalGraphView::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
