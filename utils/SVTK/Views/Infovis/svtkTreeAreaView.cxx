/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkTreeAreaView.cxx

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

#include "svtkTreeAreaView.h"

#include "svtkActor2D.h"
#include "svtkAlgorithmOutput.h"
#include "svtkAreaLayoutStrategy.h"
#include "svtkCamera.h"
#include "svtkDirectedGraph.h"
#include "svtkInteractorStyle.h"
#include "svtkObjectFactory.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderedTreeAreaRepresentation.h"
#include "svtkRenderer.h"
#include "svtkSelection.h"
#include "svtkSmartPointer.h"
#include "svtkTextProperty.h"
#include "svtkTree.h"

svtkStandardNewMacro(svtkTreeAreaView);
//----------------------------------------------------------------------------
svtkTreeAreaView::svtkTreeAreaView()
{
  this->SetInteractionModeTo2D();
  this->ReuseSingleRepresentationOn();
}

//----------------------------------------------------------------------------
svtkTreeAreaView::~svtkTreeAreaView() = default;

//----------------------------------------------------------------------------
svtkRenderedTreeAreaRepresentation* svtkTreeAreaView::GetTreeAreaRepresentation()
{
  svtkRenderedTreeAreaRepresentation* treeAreaRep = nullptr;
  for (int i = 0; i < this->GetNumberOfRepresentations(); ++i)
  {
    svtkDataRepresentation* rep = this->GetRepresentation(i);
    treeAreaRep = svtkRenderedTreeAreaRepresentation::SafeDownCast(rep);
    if (treeAreaRep)
    {
      break;
    }
  }
  if (!treeAreaRep)
  {
    svtkSmartPointer<svtkTree> g = svtkSmartPointer<svtkTree>::New();
    treeAreaRep =
      svtkRenderedTreeAreaRepresentation::SafeDownCast(this->AddRepresentationFromInput(g));
  }
  return treeAreaRep;
}

//----------------------------------------------------------------------------
svtkDataRepresentation* svtkTreeAreaView::CreateDefaultRepresentation(svtkAlgorithmOutput* port)
{
  svtkRenderedTreeAreaRepresentation* rep = svtkRenderedTreeAreaRepresentation::New();
  rep->SetInputConnection(port);
  return rep;
}

//----------------------------------------------------------------------------
svtkDataRepresentation* svtkTreeAreaView::SetTreeFromInputConnection(svtkAlgorithmOutput* conn)
{
  this->GetTreeAreaRepresentation()->SetInputConnection(conn);
  return this->GetTreeAreaRepresentation();
}

//----------------------------------------------------------------------------
svtkDataRepresentation* svtkTreeAreaView::SetTreeFromInput(svtkTree* input)
{
  this->GetTreeAreaRepresentation()->SetInputData(input);
  return this->GetTreeAreaRepresentation();
}

//----------------------------------------------------------------------------
svtkDataRepresentation* svtkTreeAreaView::SetGraphFromInputConnection(svtkAlgorithmOutput* conn)
{
  this->GetTreeAreaRepresentation()->SetInputConnection(1, conn);
  return this->GetTreeAreaRepresentation();
}

//----------------------------------------------------------------------------
svtkDataRepresentation* svtkTreeAreaView::SetGraphFromInput(svtkGraph* input)
{
  this->GetTreeAreaRepresentation()->SetInputData(1, input);
  return this->GetTreeAreaRepresentation();
}

//----------------------------------------------------------------------------
void svtkTreeAreaView::SetAreaLabelArrayName(const char* name)
{
  this->GetTreeAreaRepresentation()->SetAreaLabelArrayName(name);
}

//----------------------------------------------------------------------------
const char* svtkTreeAreaView::GetLabelPriorityArrayName()
{
  return this->GetTreeAreaRepresentation()->GetAreaLabelPriorityArrayName();
}

//----------------------------------------------------------------------------
void svtkTreeAreaView::SetLabelPriorityArrayName(const char* name)
{
  this->GetTreeAreaRepresentation()->SetAreaLabelPriorityArrayName(name);
}

//----------------------------------------------------------------------------
const char* svtkTreeAreaView::GetAreaLabelArrayName()
{
  return this->GetTreeAreaRepresentation()->GetAreaLabelArrayName();
}

//----------------------------------------------------------------------------
void svtkTreeAreaView::SetEdgeLabelArrayName(const char* name)
{
  this->GetTreeAreaRepresentation()->SetGraphEdgeLabelArrayName(name);
}

//----------------------------------------------------------------------------
const char* svtkTreeAreaView::GetEdgeLabelArrayName()
{
  return this->GetTreeAreaRepresentation()->GetGraphEdgeLabelArrayName();
}

//----------------------------------------------------------------------------
void svtkTreeAreaView::SetEdgeLabelVisibility(bool vis)
{
  this->GetTreeAreaRepresentation()->SetGraphEdgeLabelVisibility(vis);
}

//----------------------------------------------------------------------------
bool svtkTreeAreaView::GetEdgeLabelVisibility()
{
  return this->GetTreeAreaRepresentation()->GetGraphEdgeLabelVisibility();
}

//----------------------------------------------------------------------------
void svtkTreeAreaView::SetAreaHoverArrayName(const char* name)
{
  this->GetTreeAreaRepresentation()->SetAreaHoverArrayName(name);
}

//----------------------------------------------------------------------------
const char* svtkTreeAreaView::GetAreaHoverArrayName()
{
  return this->GetTreeAreaRepresentation()->GetAreaHoverArrayName();
}

//----------------------------------------------------------------------------
void svtkTreeAreaView::SetAreaLabelVisibility(bool vis)
{
  this->GetTreeAreaRepresentation()->SetAreaLabelVisibility(vis);
}

//----------------------------------------------------------------------------
bool svtkTreeAreaView::GetAreaLabelVisibility()
{
  return this->GetTreeAreaRepresentation()->GetAreaLabelVisibility();
}

//----------------------------------------------------------------------------
void svtkTreeAreaView::SetAreaColorArrayName(const char* name)
{
  this->GetTreeAreaRepresentation()->SetAreaColorArrayName(name);
}

//----------------------------------------------------------------------------
const char* svtkTreeAreaView::GetAreaColorArrayName()
{
  return this->GetTreeAreaRepresentation()->GetAreaColorArrayName();
}

//----------------------------------------------------------------------------
void svtkTreeAreaView::SetColorAreas(bool vis)
{
  this->GetTreeAreaRepresentation()->SetColorAreasByArray(vis);
}

//----------------------------------------------------------------------------
bool svtkTreeAreaView::GetColorAreas()
{
  return this->GetTreeAreaRepresentation()->GetColorAreasByArray();
}

//----------------------------------------------------------------------------
void svtkTreeAreaView::SetEdgeColorArrayName(const char* name)
{
  this->GetTreeAreaRepresentation()->SetGraphEdgeColorArrayName(name);
}

//----------------------------------------------------------------------------
const char* svtkTreeAreaView::GetEdgeColorArrayName()
{
  return this->GetTreeAreaRepresentation()->GetGraphEdgeColorArrayName();
}

//----------------------------------------------------------------------------
void svtkTreeAreaView::SetEdgeColorToSplineFraction()
{
  this->GetTreeAreaRepresentation()->SetGraphEdgeColorToSplineFraction();
}

//----------------------------------------------------------------------------
void svtkTreeAreaView::SetColorEdges(bool vis)
{
  this->GetTreeAreaRepresentation()->SetColorGraphEdgesByArray(vis);
}

//----------------------------------------------------------------------------
bool svtkTreeAreaView::GetColorEdges()
{
  return this->GetTreeAreaRepresentation()->GetColorGraphEdgesByArray();
}

//----------------------------------------------------------------------------
void svtkTreeAreaView::SetAreaSizeArrayName(const char* name)
{
  this->GetTreeAreaRepresentation()->SetAreaSizeArrayName(name);
}

//----------------------------------------------------------------------------
const char* svtkTreeAreaView::GetAreaSizeArrayName()
{
  return this->GetTreeAreaRepresentation()->GetAreaSizeArrayName();
}

//----------------------------------------------------------------------------
void svtkTreeAreaView::SetLayoutStrategy(svtkAreaLayoutStrategy* s)
{
  this->GetTreeAreaRepresentation()->SetAreaLayoutStrategy(s);
}

//----------------------------------------------------------------------------
svtkAreaLayoutStrategy* svtkTreeAreaView::GetLayoutStrategy()
{
  return this->GetTreeAreaRepresentation()->GetAreaLayoutStrategy();
}

//----------------------------------------------------------------------------
void svtkTreeAreaView::SetAreaLabelFontSize(const int size)
{
  this->GetTreeAreaRepresentation()->GetAreaLabelTextProperty()->SetFontSize(size);
}

//----------------------------------------------------------------------------
int svtkTreeAreaView::GetAreaLabelFontSize()
{
  return this->GetTreeAreaRepresentation()->GetAreaLabelTextProperty()->GetFontSize();
}

//----------------------------------------------------------------------------
void svtkTreeAreaView::SetEdgeLabelFontSize(const int size)
{
  svtkTextProperty* prop = this->GetTreeAreaRepresentation()->GetGraphEdgeLabelTextProperty();
  if (prop)
  {
    prop->SetFontSize(size);
  }
}

//----------------------------------------------------------------------------
int svtkTreeAreaView::GetEdgeLabelFontSize()
{
  svtkTextProperty* prop = this->GetTreeAreaRepresentation()->GetGraphEdgeLabelTextProperty();
  if (prop)
  {
    return prop->GetFontSize();
  }
  return 0;
}

//----------------------------------------------------------------------------
void svtkTreeAreaView::SetUseRectangularCoordinates(bool b)
{
  this->GetTreeAreaRepresentation()->SetUseRectangularCoordinates(b);
}

//----------------------------------------------------------------------------
bool svtkTreeAreaView::GetUseRectangularCoordinates()
{
  return this->GetTreeAreaRepresentation()->GetUseRectangularCoordinates();
}

//----------------------------------------------------------------------------
void svtkTreeAreaView::SetAreaToPolyData(svtkPolyDataAlgorithm* alg)
{
  this->GetTreeAreaRepresentation()->SetAreaToPolyData(alg);
}

//----------------------------------------------------------------------------
svtkPolyDataAlgorithm* svtkTreeAreaView::GetAreaToPolyData()
{
  return this->GetTreeAreaRepresentation()->GetAreaToPolyData();
}

//----------------------------------------------------------------------------
void svtkTreeAreaView::SetAreaLabelMapper(svtkLabeledDataMapper* alg)
{
  this->GetTreeAreaRepresentation()->SetAreaLabelMapper(alg);
}

//----------------------------------------------------------------------------
svtkLabeledDataMapper* svtkTreeAreaView::GetAreaLabelMapper()
{
  return this->GetTreeAreaRepresentation()->GetAreaLabelMapper();
}

//----------------------------------------------------------------------------
void svtkTreeAreaView::SetShrinkPercentage(double p)
{
  this->GetTreeAreaRepresentation()->SetShrinkPercentage(p);
}

//----------------------------------------------------------------------------
double svtkTreeAreaView::GetShrinkPercentage()
{
  return this->GetTreeAreaRepresentation()->GetShrinkPercentage();
}

//----------------------------------------------------------------------------
void svtkTreeAreaView::SetBundlingStrength(double p)
{
  this->GetTreeAreaRepresentation()->SetGraphBundlingStrength(p);
}

//----------------------------------------------------------------------------
double svtkTreeAreaView::GetBundlingStrength()
{
  return this->GetTreeAreaRepresentation()->GetGraphBundlingStrength();
}

//----------------------------------------------------------------------------
void svtkTreeAreaView::SetEdgeScalarBarVisibility(bool b)
{
  this->GetTreeAreaRepresentation()->SetEdgeScalarBarVisibility(b);
}

//----------------------------------------------------------------------------
bool svtkTreeAreaView::GetEdgeScalarBarVisibility()
{
  return this->GetTreeAreaRepresentation()->GetEdgeScalarBarVisibility();
}

//----------------------------------------------------------------------------
void svtkTreeAreaView::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
