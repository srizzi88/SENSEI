/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkRenderedHierarchyRepresentation.cxx

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

#include "svtkRenderedHierarchyRepresentation.h"

#include "svtkActor.h"
#include "svtkApplyColors.h"
#include "svtkConvertSelection.h"
#include "svtkExtractSelectedGraph.h"
#include "svtkGraphHierarchicalBundleEdges.h"
#include "svtkGraphLayout.h"
#include "svtkGraphToPolyData.h"
#include "svtkHierarchicalGraphPipeline.h"
#include "svtkInformation.h"
#include "svtkObjectFactory.h"
#include "svtkPolyDataMapper.h"
#include "svtkProp.h"
#include "svtkProperty.h"
#include "svtkRenderView.h"
#include "svtkRenderer.h"
#include "svtkSelection.h"
#include "svtkSelectionNode.h"
#include "svtkSplineGraphEdges.h"
#include "svtkTextProperty.h"
#include "svtkViewTheme.h"

#include <vector>

class svtkRenderedHierarchyRepresentation::Internals
{
public:
  std::vector<svtkSmartPointer<svtkHierarchicalGraphPipeline> > Graphs;
};

svtkStandardNewMacro(svtkRenderedHierarchyRepresentation);

svtkRenderedHierarchyRepresentation::svtkRenderedHierarchyRepresentation()
{
  this->Implementation = new Internals;
  this->SetNumberOfInputPorts(2);
  this->Layout->SetZRange(0);
  this->EdgeVisibilityOff();
}

svtkRenderedHierarchyRepresentation::~svtkRenderedHierarchyRepresentation()
{
  delete this->Implementation;
}

bool svtkRenderedHierarchyRepresentation::ValidIndex(int idx)
{
  return (idx >= 0 && idx < static_cast<int>(this->Implementation->Graphs.size()));
}

void svtkRenderedHierarchyRepresentation::SetGraphEdgeLabelArrayName(const char* name, int idx)
{
  if (this->ValidIndex(idx))
  {
    this->Implementation->Graphs[idx]->SetLabelArrayName(name);
  }
}

const char* svtkRenderedHierarchyRepresentation::GetGraphEdgeLabelArrayName(int idx)
{
  if (this->ValidIndex(idx))
  {
    return this->Implementation->Graphs[idx]->GetLabelArrayName();
  }
  return nullptr;
}

void svtkRenderedHierarchyRepresentation::SetGraphEdgeLabelVisibility(bool vis, int idx)
{
  if (this->ValidIndex(idx))
  {
    this->Implementation->Graphs[idx]->SetLabelVisibility(vis);
  }
}

bool svtkRenderedHierarchyRepresentation::GetGraphEdgeLabelVisibility(int idx)
{
  if (this->ValidIndex(idx))
  {
    return this->Implementation->Graphs[idx]->GetLabelVisibility();
  }
  return false;
}

void svtkRenderedHierarchyRepresentation::SetGraphEdgeColorArrayName(const char* name, int idx)
{
  if (this->ValidIndex(idx))
  {
    this->Implementation->Graphs[idx]->SetColorArrayName(name);
  }
}

const char* svtkRenderedHierarchyRepresentation::GetGraphEdgeColorArrayName(int idx)
{
  if (this->ValidIndex(idx))
  {
    return this->Implementation->Graphs[idx]->GetColorArrayName();
  }
  return nullptr;
}

void svtkRenderedHierarchyRepresentation::SetColorGraphEdgesByArray(bool vis, int idx)
{
  if (this->ValidIndex(idx))
  {
    this->Implementation->Graphs[idx]->SetColorEdgesByArray(vis);
  }
}

bool svtkRenderedHierarchyRepresentation::GetColorGraphEdgesByArray(int idx)
{
  if (this->ValidIndex(idx))
  {
    return this->Implementation->Graphs[idx]->GetColorEdgesByArray();
  }
  return false;
}

void svtkRenderedHierarchyRepresentation::SetGraphVisibility(bool vis, int idx)
{
  if (this->ValidIndex(idx))
  {
    this->Implementation->Graphs[idx]->SetVisibility(vis);
  }
}

bool svtkRenderedHierarchyRepresentation::GetGraphVisibility(int idx)
{
  if (this->ValidIndex(idx))
  {
    return this->Implementation->Graphs[idx]->GetVisibility();
  }
  return false;
}

void svtkRenderedHierarchyRepresentation::SetBundlingStrength(double strength, int idx)
{
  if (this->ValidIndex(idx))
  {
    this->Implementation->Graphs[idx]->SetBundlingStrength(strength);
  }
}

double svtkRenderedHierarchyRepresentation::GetBundlingStrength(int idx)
{
  if (this->ValidIndex(idx))
  {
    return this->Implementation->Graphs[idx]->GetBundlingStrength();
  }
  return 0.0;
}

void svtkRenderedHierarchyRepresentation::SetGraphSplineType(int type, int idx)
{
  if (this->ValidIndex(idx))
  {
    this->Implementation->Graphs[idx]->SetSplineType(type);
  }
}

int svtkRenderedHierarchyRepresentation::GetGraphSplineType(int idx)
{
  if (this->ValidIndex(idx))
  {
    return this->Implementation->Graphs[idx]->GetSplineType();
  }
  return 0;
}

void svtkRenderedHierarchyRepresentation::SetGraphEdgeLabelFontSize(int size, int idx)
{
  if (this->ValidIndex(idx))
  {
    this->Implementation->Graphs[idx]->GetLabelTextProperty()->SetFontSize(size);
  }
}

int svtkRenderedHierarchyRepresentation::GetGraphEdgeLabelFontSize(int idx)
{
  if (this->ValidIndex(idx))
  {
    return this->Implementation->Graphs[idx]->GetLabelTextProperty()->GetFontSize();
  }
  return 0;
}

bool svtkRenderedHierarchyRepresentation::AddToView(svtkView* view)
{
  this->Superclass::AddToView(view);
  svtkRenderView* rv = svtkRenderView::SafeDownCast(view);
  if (rv)
  {
    return true;
  }
  return false;
}

bool svtkRenderedHierarchyRepresentation::RemoveFromView(svtkView* view)
{
  this->Superclass::RemoveFromView(view);
  svtkRenderView* rv = svtkRenderView::SafeDownCast(view);
  if (rv)
  {
    return true;
  }
  return false;
}

svtkSelection* svtkRenderedHierarchyRepresentation::ConvertSelection(svtkView* view, svtkSelection* sel)
{
  svtkSelection* converted = this->Superclass::ConvertSelection(view, sel);

  int numGraphs = static_cast<int>(this->Implementation->Graphs.size());
  for (int i = 0; i < numGraphs; ++i)
  {
    svtkHierarchicalGraphPipeline* p = this->Implementation->Graphs[i];
    svtkSelection* conv = p->ConvertSelection(this, sel);
    if (conv)
    {
      for (unsigned int j = 0; j < conv->GetNumberOfNodes(); ++j)
      {
        converted->AddNode(conv->GetNode(j));
      }
      conv->Delete();
    }
  }
  // cerr << "Tree converted: " << endl;
  // converted->Dump();

  return converted;
}

int svtkRenderedHierarchyRepresentation::RequestData(
  svtkInformation* request, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // Setup superclass connections.
  if (!this->Superclass::RequestData(request, inputVector, outputVector))
  {
    return 0;
  }

  // Add new graph objects if needed.
  size_t numGraphs = static_cast<size_t>(this->GetNumberOfInputConnections(1));
  while (numGraphs > this->Implementation->Graphs.size())
  {
    this->Implementation->Graphs.push_back(svtkSmartPointer<svtkHierarchicalGraphPipeline>::New());
  }

  // Keep track of actors to remove if the number of input connections
  // decreased.
  for (size_t i = numGraphs; i < this->Implementation->Graphs.size(); ++i)
  {
    this->RemovePropOnNextRender(this->Implementation->Graphs[i]->GetActor());
  }
  this->Implementation->Graphs.resize(numGraphs);

  // Setup input connections for bundled graphs.
  for (size_t i = 0; i < numGraphs; ++i)
  {
    this->AddPropOnNextRender(this->Implementation->Graphs[i]->GetActor());
    svtkHierarchicalGraphPipeline* p = this->Implementation->Graphs[i];
    p->PrepareInputConnections(this->GetInternalOutputPort(1, static_cast<int>(i)),
      this->Layout->GetOutputPort(), this->GetInternalAnnotationOutputPort());
  }
  return 1;
}

void svtkRenderedHierarchyRepresentation::ApplyViewTheme(svtkViewTheme* theme)
{
  this->Superclass::ApplyViewTheme(theme);

  // Update all the graphs on the second input port before traversing them.
  this->Update();

  for (size_t i = 0; i < this->Implementation->Graphs.size(); ++i)
  {
    svtkHierarchicalGraphPipeline* p = this->Implementation->Graphs[i];
    p->ApplyViewTheme(theme);
  }
}

int svtkRenderedHierarchyRepresentation::FillInputPortInformation(int port, svtkInformation* info)
{
  if (port == 0)
  {
    info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkTree");
    return 1;
  }
  else if (port == 1)
  {
    info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkGraph");
    info->Set(svtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
    info->Set(svtkAlgorithm::INPUT_IS_REPEATABLE(), 1);
    return 1;
  }
  return 0;
}

void svtkRenderedHierarchyRepresentation::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
