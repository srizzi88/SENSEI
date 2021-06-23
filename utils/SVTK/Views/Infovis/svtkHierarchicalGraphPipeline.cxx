/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkHierarchicalGraphPipeline.cxx

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

#include "svtkHierarchicalGraphPipeline.h"

#include "svtkActor.h"
#include "svtkActor2D.h"
#include "svtkApplyColors.h"
#include "svtkConvertSelection.h"
#include "svtkDataRepresentation.h"
#include "svtkDynamic2DLabelMapper.h"
#include "svtkEdgeCenters.h"
#include "svtkGraphHierarchicalBundleEdges.h"
#include "svtkGraphToPolyData.h"
#include "svtkInformation.h"
#include "svtkLookupTable.h"
#include "svtkObjectFactory.h"
#include "svtkPolyDataMapper.h"
#include "svtkProperty.h"
#include "svtkRenderView.h"
#include "svtkSelection.h"
#include "svtkSelectionNode.h"
#include "svtkSmartPointer.h"
#include "svtkSplineFilter.h"
#include "svtkSplineGraphEdges.h"
#include "svtkTextProperty.h"
#include "svtkViewTheme.h"

svtkStandardNewMacro(svtkHierarchicalGraphPipeline);

svtkHierarchicalGraphPipeline::svtkHierarchicalGraphPipeline()
{
  this->ApplyColors = svtkApplyColors::New();
  this->Bundle = svtkGraphHierarchicalBundleEdges::New();
  this->GraphToPoly = svtkGraphToPolyData::New();
  this->Spline = svtkSplineGraphEdges::New();
  this->Mapper = svtkPolyDataMapper::New();
  this->Actor = svtkActor::New();
  this->TextProperty = svtkTextProperty::New();
  this->EdgeCenters = svtkEdgeCenters::New();
  this->LabelMapper = svtkDynamic2DLabelMapper::New();
  this->LabelActor = svtkActor2D::New();

  this->ColorArrayNameInternal = nullptr;
  this->LabelArrayNameInternal = nullptr;
  this->HoverArrayName = nullptr;

  /*
  <graphviz>
  digraph {
    "Graph input" -> Bundle
    "Tree input" -> Bundle
    Bundle -> Spline -> ApplyColors -> GraphToPoly -> Mapper -> Actor
    Spline -> EdgeCenters -> LabelMapper -> LabelActor
  }
  </graphviz>
  */

  this->Spline->SetInputConnection(this->Bundle->GetOutputPort());
  this->ApplyColors->SetInputConnection(this->Spline->GetOutputPort());
  this->GraphToPoly->SetInputConnection(this->ApplyColors->GetOutputPort());
  this->Mapper->SetInputConnection(this->GraphToPoly->GetOutputPort());
  this->Actor->SetMapper(this->Mapper);

  this->EdgeCenters->SetInputConnection(this->Spline->GetOutputPort());
  this->LabelMapper->SetInputConnection(this->EdgeCenters->GetOutputPort());
  this->LabelMapper->SetLabelTextProperty(this->TextProperty);
  this->LabelMapper->SetLabelModeToLabelFieldData();
  this->LabelActor->SetMapper(this->LabelMapper);
  this->LabelActor->VisibilityOff();

  this->Mapper->SetScalarModeToUseCellFieldData();
  this->Mapper->SelectColorArray("svtkApplyColors color");
  this->Mapper->ScalarVisibilityOn();
  this->Actor->PickableOn();

  // Make sure this gets rendered on top
  this->Actor->SetPosition(0.0, 0.0, 1.0);

  this->Bundle->SetBundlingStrength(0.5);
  this->Spline->SetSplineType(svtkSplineGraphEdges::BSPLINE);
  // this->Spline->SetNumberOfSubdivisions(4);
}

svtkHierarchicalGraphPipeline::~svtkHierarchicalGraphPipeline()
{
  this->SetColorArrayNameInternal(nullptr);
  this->SetLabelArrayNameInternal(nullptr);
  this->SetHoverArrayName(nullptr);
  this->ApplyColors->Delete();
  this->Bundle->Delete();
  this->GraphToPoly->Delete();
  this->Spline->Delete();
  this->Mapper->Delete();
  this->Actor->Delete();
  this->TextProperty->Delete();
  this->EdgeCenters->Delete();
  this->LabelMapper->Delete();
  this->LabelActor->Delete();
}

void svtkHierarchicalGraphPipeline::RegisterProgress(svtkRenderView* rv)
{
  rv->RegisterProgress(this->ApplyColors);
  rv->RegisterProgress(this->Bundle);
  rv->RegisterProgress(this->ApplyColors);
  rv->RegisterProgress(this->GraphToPoly);
  rv->RegisterProgress(this->Spline);
  rv->RegisterProgress(this->Mapper);
}

void svtkHierarchicalGraphPipeline::SetBundlingStrength(double strength)
{
  this->Bundle->SetBundlingStrength(strength);
}

double svtkHierarchicalGraphPipeline::GetBundlingStrength()
{
  return this->Bundle->GetBundlingStrength();
}

void svtkHierarchicalGraphPipeline::SetLabelArrayName(const char* name)
{
  this->LabelMapper->SetFieldDataName(name);
  this->SetLabelArrayNameInternal(name);
}

const char* svtkHierarchicalGraphPipeline::GetLabelArrayName()
{
  return this->GetLabelArrayNameInternal();
}

void svtkHierarchicalGraphPipeline::SetLabelVisibility(bool vis)
{
  this->LabelActor->SetVisibility(vis);
}

bool svtkHierarchicalGraphPipeline::GetLabelVisibility()
{
  return (this->LabelActor->GetVisibility() ? true : false);
}

void svtkHierarchicalGraphPipeline::SetLabelTextProperty(svtkTextProperty* prop)
{
  this->TextProperty->ShallowCopy(prop);
}

svtkTextProperty* svtkHierarchicalGraphPipeline::GetLabelTextProperty()
{
  return this->TextProperty;
}

void svtkHierarchicalGraphPipeline::SetColorArrayName(const char* name)
{
  this->SetColorArrayNameInternal(name);
  this->ApplyColors->SetInputArrayToProcess(1, 0, 0, svtkDataObject::FIELD_ASSOCIATION_EDGES, name);
}

const char* svtkHierarchicalGraphPipeline::GetColorArrayName()
{
  return this->GetColorArrayNameInternal();
}

void svtkHierarchicalGraphPipeline::SetColorEdgesByArray(bool vis)
{
  this->ApplyColors->SetUseCellLookupTable(vis);
}

bool svtkHierarchicalGraphPipeline::GetColorEdgesByArray()
{
  return this->ApplyColors->GetUseCellLookupTable();
}

void svtkHierarchicalGraphPipeline::SetVisibility(bool vis)
{
  this->Actor->SetVisibility(vis);
}

bool svtkHierarchicalGraphPipeline::GetVisibility()
{
  return this->Actor->GetVisibility() ? true : false;
}

void svtkHierarchicalGraphPipeline::SetSplineType(int type)
{
  this->Spline->SetSplineType(type);
}

int svtkHierarchicalGraphPipeline::GetSplineType()
{
  return this->Spline->GetSplineType();
}

void svtkHierarchicalGraphPipeline::PrepareInputConnections(
  svtkAlgorithmOutput* graphConn, svtkAlgorithmOutput* treeConn, svtkAlgorithmOutput* annConn)
{
  this->Bundle->SetInputConnection(0, graphConn);
  this->Bundle->SetInputConnection(1, treeConn);
  this->ApplyColors->SetInputConnection(1, annConn);
}

svtkSelection* svtkHierarchicalGraphPipeline::ConvertSelection(
  svtkDataRepresentation* rep, svtkSelection* sel)
{
  svtkSelection* converted = svtkSelection::New();
  for (unsigned int j = 0; j < sel->GetNumberOfNodes(); ++j)
  {
    svtkSelectionNode* node = sel->GetNode(j);
    svtkProp* prop = svtkProp::SafeDownCast(node->GetProperties()->Get(svtkSelectionNode::PROP()));
    if (prop == this->Actor)
    {
      svtkDataObject* input = this->Bundle->GetInputDataObject(0, 0);
      svtkDataObject* poly = this->GraphToPoly->GetOutput();
      svtkSmartPointer<svtkSelection> edgeSel = svtkSmartPointer<svtkSelection>::New();
      svtkSmartPointer<svtkSelectionNode> nodeCopy = svtkSmartPointer<svtkSelectionNode>::New();
      nodeCopy->ShallowCopy(node);
      nodeCopy->GetProperties()->Remove(svtkSelectionNode::PROP());
      edgeSel->AddNode(nodeCopy);
      svtkSelection* polyConverted =
        svtkConvertSelection::ToSelectionType(edgeSel, poly, svtkSelectionNode::PEDIGREEIDS);
      for (unsigned int i = 0; i < polyConverted->GetNumberOfNodes(); ++i)
      {
        polyConverted->GetNode(i)->SetFieldType(svtkSelectionNode::EDGE);
      }
      svtkSelection* edgeConverted = svtkConvertSelection::ToSelectionType(
        polyConverted, input, rep->GetSelectionType(), rep->GetSelectionArrayNames());
      for (unsigned int i = 0; i < edgeConverted->GetNumberOfNodes(); ++i)
      {
        converted->AddNode(edgeConverted->GetNode(i));
      }
      polyConverted->Delete();
      edgeConverted->Delete();
    }
  }
  return converted;
}

void svtkHierarchicalGraphPipeline::ApplyViewTheme(svtkViewTheme* theme)
{
  this->ApplyColors->SetDefaultCellColor(theme->GetCellColor());
  this->ApplyColors->SetDefaultCellOpacity(theme->GetCellOpacity());
  this->ApplyColors->SetSelectedCellColor(theme->GetSelectedCellColor());
  this->ApplyColors->SetSelectedCellOpacity(theme->GetSelectedCellOpacity());

  this->ApplyColors->SetCellLookupTable(theme->GetCellLookupTable());

  this->TextProperty->ShallowCopy(theme->GetCellTextProperty());
  this->Actor->GetProperty()->SetLineWidth(theme->GetLineWidth());
}

void svtkHierarchicalGraphPipeline::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Actor: ";
  if (this->Actor && this->Bundle->GetNumberOfInputConnections(0) > 0)
  {
    os << "\n";
    this->Actor->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << "(none)\n";
  }
  os << indent << "LabelActor: ";
  if (this->LabelActor && this->Bundle->GetNumberOfInputConnections(0) > 0)
  {
    os << "\n";
    this->LabelActor->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << "(none)\n";
  }
  os << indent << "HoverArrayName: " << (this->HoverArrayName ? this->HoverArrayName : "(none)")
     << "\n";
}
