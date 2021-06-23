/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkRenderedGraphRepresentation.cxx

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

#include "svtkRenderedGraphRepresentation.h"

#include "svtkAbstractTransform.h"
#include "svtkActor.h"
#include "svtkAlgorithmOutput.h"
#include "svtkAnnotationLink.h"
#include "svtkApplyColors.h"
#include "svtkApplyIcons.h"
#include "svtkArcParallelEdgeStrategy.h"
#include "svtkAssignCoordinatesLayoutStrategy.h"
#include "svtkCellData.h"
#include "svtkCircularLayoutStrategy.h"
#include "svtkClustering2DLayoutStrategy.h"
#include "svtkCommunity2DLayoutStrategy.h"
#include "svtkConeLayoutStrategy.h"
#include "svtkConvertSelection.h"
#include "svtkCosmicTreeLayoutStrategy.h"
#include "svtkDirectedGraph.h"
#include "svtkEdgeCenters.h"
#include "svtkEdgeLayout.h"
#include "svtkFast2DLayoutStrategy.h"
#include "svtkForceDirectedLayoutStrategy.h"
#include "svtkGeoEdgeStrategy.h"
#include "svtkGraphLayout.h"
#include "svtkGraphToGlyphs.h"
#include "svtkGraphToPoints.h"
#include "svtkGraphToPolyData.h"
#include "svtkIconGlyphFilter.h"
#include "svtkIdTypeArray.h"
#include "svtkImageData.h"
#include "svtkInformation.h"
#include "svtkLookupTable.h"
#include "svtkObjectFactory.h"
#include "svtkPassThroughEdgeStrategy.h"
#include "svtkPassThroughLayoutStrategy.h"
#include "svtkPerturbCoincidentVertices.h"
#include "svtkPointSetToLabelHierarchy.h"
#include "svtkPolyData.h"
#include "svtkPolyDataMapper.h"
#include "svtkPolyDataMapper2D.h"
#include "svtkProperty.h"
#include "svtkRandomLayoutStrategy.h"
#include "svtkRemoveHiddenData.h"
#include "svtkRenderView.h"
#include "svtkRenderWindow.h"
#include "svtkRenderer.h"
#include "svtkScalarBarActor.h"
#include "svtkScalarBarWidget.h"
#include "svtkSelection.h"
#include "svtkSelectionNode.h"
#include "svtkSimple2DLayoutStrategy.h"
#include "svtkSmartPointer.h"
#include "svtkSpanTreeLayoutStrategy.h"
#include "svtkSphereSource.h"
#include "svtkTable.h"
#include "svtkTextProperty.h"
#include "svtkTexture.h"
#include "svtkTexturedActor2D.h"
#include "svtkTransformCoordinateSystems.h"
#include "svtkTreeLayoutStrategy.h"
#include "svtkVertexDegree.h"
#include "svtkViewTheme.h"

#include <algorithm>
#include <cctype>

svtkStandardNewMacro(svtkRenderedGraphRepresentation);

svtkRenderedGraphRepresentation::svtkRenderedGraphRepresentation()
{
  this->ApplyColors = svtkSmartPointer<svtkApplyColors>::New();
  this->VertexDegree = svtkSmartPointer<svtkVertexDegree>::New();
  this->EmptyPolyData = svtkSmartPointer<svtkPolyData>::New();
  this->EdgeCenters = svtkSmartPointer<svtkEdgeCenters>::New();
  this->GraphToPoints = svtkSmartPointer<svtkGraphToPoints>::New();
  this->VertexLabelHierarchy = svtkSmartPointer<svtkPointSetToLabelHierarchy>::New();
  this->EdgeLabelHierarchy = svtkSmartPointer<svtkPointSetToLabelHierarchy>::New();
  this->Layout = svtkSmartPointer<svtkGraphLayout>::New();
  this->Coincident = svtkSmartPointer<svtkPerturbCoincidentVertices>::New();
  this->EdgeLayout = svtkSmartPointer<svtkEdgeLayout>::New();
  this->GraphToPoly = svtkSmartPointer<svtkGraphToPolyData>::New();
  this->EdgeMapper = svtkSmartPointer<svtkPolyDataMapper>::New();
  this->EdgeActor = svtkSmartPointer<svtkActor>::New();
  this->VertexGlyph = svtkSmartPointer<svtkGraphToGlyphs>::New();
  this->VertexMapper = svtkSmartPointer<svtkPolyDataMapper>::New();
  this->VertexActor = svtkSmartPointer<svtkActor>::New();
  this->OutlineGlyph = svtkSmartPointer<svtkGraphToGlyphs>::New();
  this->OutlineMapper = svtkSmartPointer<svtkPolyDataMapper>::New();
  this->OutlineActor = svtkSmartPointer<svtkActor>::New();
  this->VertexScalarBar = svtkSmartPointer<svtkScalarBarWidget>::New();
  this->EdgeScalarBar = svtkSmartPointer<svtkScalarBarWidget>::New();
  this->RemoveHiddenGraph = svtkSmartPointer<svtkRemoveHiddenData>::New();
  this->ApplyVertexIcons = svtkSmartPointer<svtkApplyIcons>::New();
  this->VertexIconPoints = svtkSmartPointer<svtkGraphToPoints>::New();
  this->VertexIconTransform = svtkSmartPointer<svtkTransformCoordinateSystems>::New();
  this->VertexIconGlyph = svtkSmartPointer<svtkIconGlyphFilter>::New();
  this->VertexIconMapper = svtkSmartPointer<svtkPolyDataMapper2D>::New();
  this->VertexIconActor = svtkSmartPointer<svtkTexturedActor2D>::New();

  this->VertexHoverArrayName = nullptr;
  this->EdgeHoverArrayName = nullptr;
  this->VertexColorArrayNameInternal = nullptr;
  this->EdgeColorArrayNameInternal = nullptr;
  this->ScalingArrayNameInternal = nullptr;
  this->LayoutStrategyName = nullptr;
  this->EdgeLayoutStrategyName = nullptr;

  this->HideVertexLabelsOnInteraction = false;
  this->HideEdgeLabelsOnInteraction = false;

  this->EdgeSelection = true;

  /*
   <graphviz>
   digraph {
     Layout -> Coincident -> EdgeLayout -> VertexDegree -> ApplyColors
     ApplyColors -> VertexGlyph -> VertexMapper -> VertexActor
     ApplyColors -> GraphToPoly -> EdgeMapper -> EdgeActor
     ApplyColors -> ApplyVertexIcons
     Coincident -> OutlineGlyph -> OutlineMapper -> OutlineActor

     VertexDegree -> GraphToPoints
     GraphToPoints -> VertexLabelHierarchy -> "svtkRenderView Labels"
     GraphToPoints -> VertexIcons -> VertexIconPriority -> "svtkRenderView Icons"
     ApplyVertexIcons -> VertexIconPoints -> VertexIconTransform -> VertexIconGlyphFilter ->
   VertexIconMapper -> VertexIconActor VertexDegree -> EdgeCenters EdgeCenters -> EdgeLabelHierarchy
   -> "svtkRenderView Labels" EdgeCenters -> EdgeIcons -> EdgeIconPriority -> "svtkRenderView Icons"
   }
   </graphviz>
  */

  this->Coincident->SetInputConnection(this->Layout->GetOutputPort());
  this->RemoveHiddenGraph->SetInputConnection(this->Coincident->GetOutputPort());
  this->EdgeLayout->SetInputConnection(this->RemoveHiddenGraph->GetOutputPort());
  this->VertexDegree->SetInputConnection(this->EdgeLayout->GetOutputPort());
  this->ApplyColors->SetInputConnection(this->VertexDegree->GetOutputPort());
  this->ApplyVertexIcons->SetInputConnection(this->ApplyColors->GetOutputPort());

  // Vertex actor
  this->VertexGlyph->SetInputConnection(this->ApplyColors->GetOutputPort());
  this->VertexMapper->SetInputConnection(this->VertexGlyph->GetOutputPort());
  this->VertexActor->SetMapper(this->VertexMapper);

  // Outline actor
  this->OutlineGlyph->SetInputConnection(this->RemoveHiddenGraph->GetOutputPort());
  this->OutlineMapper->SetInputConnection(this->OutlineGlyph->GetOutputPort());
  this->OutlineActor->SetMapper(this->OutlineMapper);

  // Edge actor
  this->GraphToPoly->SetInputConnection(this->ApplyColors->GetOutputPort());
  this->EdgeMapper->SetInputConnection(this->GraphToPoly->GetOutputPort());
  this->EdgeActor->SetMapper(this->EdgeMapper);

  // Experimental icons
  this->VertexIconPoints->SetInputConnection(this->ApplyVertexIcons->GetOutputPort());
  this->VertexIconTransform->SetInputConnection(this->VertexIconPoints->GetOutputPort());
  this->VertexIconGlyph->SetInputConnection(this->VertexIconTransform->GetOutputPort());
  this->VertexIconMapper->SetInputConnection(this->VertexIconGlyph->GetOutputPort());
  this->VertexIconActor->SetMapper(this->VertexIconMapper);
  this->VertexIconTransform->SetInputCoordinateSystemToWorld();
  this->VertexIconTransform->SetOutputCoordinateSystemToDisplay();
  this->VertexIconGlyph->SetInputArrayToProcess(
    0, 0, 0, svtkDataObject::FIELD_ASSOCIATION_POINTS, "svtkApplyIcons icon");
  this->ApplyVertexIcons->SetInputArrayToProcess(
    0, 0, 0, svtkDataObject::FIELD_ASSOCIATION_VERTICES, "icon");
  this->VertexIconActor->VisibilityOff();

  this->GraphToPoints->SetInputConnection(this->VertexDegree->GetOutputPort());
  this->EdgeCenters->SetInputConnection(this->VertexDegree->GetOutputPort());
  this->EdgeLabelHierarchy->SetInputData(this->EmptyPolyData);
  this->VertexLabelHierarchy->SetInputData(this->EmptyPolyData);

  // Set default parameters
  svtkSmartPointer<svtkDirectedGraph> g = svtkSmartPointer<svtkDirectedGraph>::New();
  this->Layout->SetInputData(g);
  svtkSmartPointer<svtkFast2DLayoutStrategy> strategy =
    svtkSmartPointer<svtkFast2DLayoutStrategy>::New();
  this->Layout->SetLayoutStrategy(strategy);
  // this->Layout->SetZRange(0.001);
  this->Layout->SetZRange(0.0);
  svtkSmartPointer<svtkArcParallelEdgeStrategy> edgeStrategy =
    svtkSmartPointer<svtkArcParallelEdgeStrategy>::New();
  this->Layout->UseTransformOn();
  this->SetVertexColorArrayName("VertexDegree");
  this->SetVertexLabelArrayName("VertexDegree");
  this->SetVertexLabelPriorityArrayName("VertexDegree");
  this->SetVertexIconArrayName("IconIndex");
  this->SetVertexIconPriorityArrayName("VertexDegree");
  this->EdgeLayout->SetLayoutStrategy(edgeStrategy);

  this->VertexGlyph->FilledOn();
  this->VertexGlyph->SetGlyphType(svtkGraphToGlyphs::VERTEX);
  this->VertexMapper->SetScalarModeToUseCellFieldData();
  this->VertexMapper->SelectColorArray("svtkApplyColors color");
  this->VertexMapper->SetScalarVisibility(true);

  this->OutlineGlyph->FilledOff();
  this->OutlineGlyph->SetGlyphType(svtkGraphToGlyphs::VERTEX);
  this->OutlineMapper->SetScalarVisibility(false);
  this->OutlineActor->PickableOff();
  this->OutlineActor->GetProperty()->FrontfaceCullingOn();

  this->EdgeMapper->SetScalarModeToUseCellFieldData();
  this->EdgeMapper->SelectColorArray("svtkApplyColors color");
  this->EdgeMapper->SetScalarVisibility(true);
  this->EdgeActor->SetPosition(0, 0, -0.003);

  this->VertexScalarBar->GetScalarBarActor()->VisibilityOff();
  this->EdgeScalarBar->GetScalarBarActor()->VisibilityOff();

  svtkSmartPointer<svtkViewTheme> theme = svtkSmartPointer<svtkViewTheme>::New();
  this->ApplyViewTheme(theme);
}

svtkRenderedGraphRepresentation::~svtkRenderedGraphRepresentation()
{
  this->SetScalingArrayNameInternal(nullptr);
  this->SetVertexColorArrayNameInternal(nullptr);
  this->SetEdgeColorArrayNameInternal(nullptr);
  this->SetLayoutStrategyName(nullptr);
  this->SetEdgeLayoutStrategyName(nullptr);
  this->SetVertexHoverArrayName(nullptr);
  this->SetEdgeHoverArrayName(nullptr);
}

void svtkRenderedGraphRepresentation::SetVertexLabelArrayName(const char* name)
{
  this->VertexLabelHierarchy->SetLabelArrayName(name);
}

void svtkRenderedGraphRepresentation::SetEdgeLabelArrayName(const char* name)
{
  this->EdgeLabelHierarchy->SetLabelArrayName(name);
}

const char* svtkRenderedGraphRepresentation::GetVertexLabelArrayName()
{
  return this->VertexLabelHierarchy->GetLabelArrayName();
}

const char* svtkRenderedGraphRepresentation::GetEdgeLabelArrayName()
{
  return this->EdgeLabelHierarchy->GetLabelArrayName();
}

void svtkRenderedGraphRepresentation::SetVertexLabelPriorityArrayName(const char* name)
{
  this->VertexLabelHierarchy->SetPriorityArrayName(name);
}

void svtkRenderedGraphRepresentation::SetEdgeLabelPriorityArrayName(const char* name)
{
  this->EdgeLabelHierarchy->SetPriorityArrayName(name);
}

const char* svtkRenderedGraphRepresentation::GetVertexLabelPriorityArrayName()
{
  return this->VertexLabelHierarchy->GetPriorityArrayName();
}

const char* svtkRenderedGraphRepresentation::GetEdgeLabelPriorityArrayName()
{
  return this->EdgeLabelHierarchy->GetPriorityArrayName();
}

void svtkRenderedGraphRepresentation::SetVertexLabelVisibility(bool b)
{
  if (b)
  {
    this->VertexLabelHierarchy->SetInputConnection(this->GraphToPoints->GetOutputPort());
  }
  else
  {
    this->VertexLabelHierarchy->SetInputData(this->EmptyPolyData);
  }
}

void svtkRenderedGraphRepresentation::SetEdgeLabelVisibility(bool b)
{
  if (b)
  {
    this->EdgeLabelHierarchy->SetInputConnection(this->EdgeCenters->GetOutputPort());
  }
  else
  {
    this->EdgeLabelHierarchy->SetInputData(this->EmptyPolyData);
  }
}

bool svtkRenderedGraphRepresentation::GetVertexLabelVisibility()
{
  return this->VertexLabelHierarchy->GetInputConnection(0, 0) ==
    this->GraphToPoints->GetOutputPort();
}

bool svtkRenderedGraphRepresentation::GetEdgeLabelVisibility()
{
  return this->EdgeLabelHierarchy->GetInputConnection(0, 0) == this->EdgeCenters->GetOutputPort();
}

void svtkRenderedGraphRepresentation::SetEdgeVisibility(bool b)
{
  this->EdgeActor->SetVisibility(b);
}

bool svtkRenderedGraphRepresentation::GetEdgeVisibility()
{
  return this->EdgeActor->GetVisibility() ? true : false;
}

void svtkRenderedGraphRepresentation::SetEdgeSelection(bool b)
{
  this->EdgeSelection = b;
}

bool svtkRenderedGraphRepresentation::GetEdgeSelection()
{
  return this->EdgeSelection;
}

void svtkRenderedGraphRepresentation::SetVertexLabelTextProperty(svtkTextProperty* p)
{
  this->VertexLabelHierarchy->SetTextProperty(p);
}

void svtkRenderedGraphRepresentation::SetEdgeLabelTextProperty(svtkTextProperty* p)
{
  this->EdgeLabelHierarchy->SetTextProperty(p);
}

svtkTextProperty* svtkRenderedGraphRepresentation::GetVertexLabelTextProperty()
{
  return this->VertexLabelHierarchy->GetTextProperty();
}

svtkTextProperty* svtkRenderedGraphRepresentation::GetEdgeLabelTextProperty()
{
  return this->EdgeLabelHierarchy->GetTextProperty();
}

void svtkRenderedGraphRepresentation::SetVertexIconArrayName(const char* name)
{
  this->ApplyVertexIcons->SetInputArrayToProcess(
    0, 0, 0, svtkDataObject::FIELD_ASSOCIATION_VERTICES, name);
}

void svtkRenderedGraphRepresentation::SetEdgeIconArrayName(const char*)
{
  // TODO: Implement.
}

const char* svtkRenderedGraphRepresentation::GetVertexIconArrayName()
{
  // TODO: Implement.
  return nullptr;
}

const char* svtkRenderedGraphRepresentation::GetEdgeIconArrayName()
{
  // TODO: Implement.
  return nullptr;
}

void svtkRenderedGraphRepresentation::SetVertexIconPriorityArrayName(const char*)
{
  // TODO: Implement.
}

void svtkRenderedGraphRepresentation::SetEdgeIconPriorityArrayName(const char*)
{
  // TODO: Implement.
}

const char* svtkRenderedGraphRepresentation::GetVertexIconPriorityArrayName()
{
  // TODO: Implement.
  return nullptr;
}

const char* svtkRenderedGraphRepresentation::GetEdgeIconPriorityArrayName()
{
  // TODO: Implement.
  return nullptr;
}

void svtkRenderedGraphRepresentation::SetVertexIconVisibility(bool b)
{
  this->VertexIconActor->SetVisibility(b);
}

void svtkRenderedGraphRepresentation::SetEdgeIconVisibility(bool)
{
  // TODO: Implement.
}

bool svtkRenderedGraphRepresentation::GetVertexIconVisibility()
{
  return (this->VertexIconActor->GetVisibility() ? true : false);
}

bool svtkRenderedGraphRepresentation::GetEdgeIconVisibility()
{
  // TODO: Implement.
  return false;
}

void svtkRenderedGraphRepresentation::AddVertexIconType(const char* name, int type)
{
  this->ApplyVertexIcons->SetIconType(name, type);
  this->ApplyVertexIcons->UseLookupTableOn();
}

void svtkRenderedGraphRepresentation::AddEdgeIconType(const char*, int)
{
  // TODO: Implement.
}

void svtkRenderedGraphRepresentation::ClearVertexIconTypes()
{
  this->ApplyVertexIcons->ClearAllIconTypes();
  this->ApplyVertexIcons->UseLookupTableOff();
}

void svtkRenderedGraphRepresentation::ClearEdgeIconTypes()
{
  // TODO: Implement.
}

void svtkRenderedGraphRepresentation::SetUseVertexIconTypeMap(bool b)
{
  this->ApplyVertexIcons->SetUseLookupTable(b);
}

void svtkRenderedGraphRepresentation::SetUseEdgeIconTypeMap(bool)
{
  // TODO: Implement.
}

bool svtkRenderedGraphRepresentation::GetUseVertexIconTypeMap()
{
  return this->ApplyVertexIcons->GetUseLookupTable();
}

bool svtkRenderedGraphRepresentation::GetUseEdgeIconTypeMap()
{
  // TODO: Implement.
  return false;
}

// TODO: Icon alignment
void svtkRenderedGraphRepresentation::SetVertexIconAlignment(int align)
{
  (void)align;
}

int svtkRenderedGraphRepresentation::GetVertexIconAlignment()
{
  return 0;
}

void svtkRenderedGraphRepresentation::SetEdgeIconAlignment(int align)
{
  (void)align;
}

int svtkRenderedGraphRepresentation::GetEdgeIconAlignment()
{
  return 0;
}

void svtkRenderedGraphRepresentation::SetVertexSelectedIcon(int icon)
{
  this->ApplyVertexIcons->SetSelectedIcon(icon);
}

int svtkRenderedGraphRepresentation::GetVertexSelectedIcon()
{
  return this->ApplyVertexIcons->GetSelectedIcon();
}

void svtkRenderedGraphRepresentation::SetVertexDefaultIcon(int icon)
{
  this->ApplyVertexIcons->SetDefaultIcon(icon);
}

int svtkRenderedGraphRepresentation::GetVertexDefaultIcon()
{
  return this->ApplyVertexIcons->GetDefaultIcon();
}

void svtkRenderedGraphRepresentation::SetVertexIconSelectionMode(int mode)
{
  this->ApplyVertexIcons->SetSelectionMode(mode);
}

int svtkRenderedGraphRepresentation::GetVertexIconSelectionMode()
{
  return this->ApplyVertexIcons->GetSelectionMode();
}

void svtkRenderedGraphRepresentation::SetColorVerticesByArray(bool b)
{
  this->ApplyColors->SetUsePointLookupTable(b);
}

bool svtkRenderedGraphRepresentation::GetColorVerticesByArray()
{
  return this->ApplyColors->GetUsePointLookupTable();
}

void svtkRenderedGraphRepresentation::SetVertexColorArrayName(const char* name)
{
  this->SetVertexColorArrayNameInternal(name);
  this->ApplyColors->SetInputArrayToProcess(
    0, 0, 0, svtkDataObject::FIELD_ASSOCIATION_VERTICES, name);
  this->VertexScalarBar->GetScalarBarActor()->SetTitle(name);
}

const char* svtkRenderedGraphRepresentation::GetVertexColorArrayName()
{
  return this->GetVertexColorArrayNameInternal();
}

void svtkRenderedGraphRepresentation::SetColorEdgesByArray(bool b)
{
  this->ApplyColors->SetUseCellLookupTable(b);
}

bool svtkRenderedGraphRepresentation::GetColorEdgesByArray()
{
  return this->ApplyColors->GetUseCellLookupTable();
}

void svtkRenderedGraphRepresentation::SetEdgeColorArrayName(const char* name)
{
  this->SetEdgeColorArrayNameInternal(name);
  this->ApplyColors->SetInputArrayToProcess(1, 0, 0, svtkDataObject::FIELD_ASSOCIATION_EDGES, name);
  this->EdgeScalarBar->GetScalarBarActor()->SetTitle(name);
}

const char* svtkRenderedGraphRepresentation::GetEdgeColorArrayName()
{
  return this->GetEdgeColorArrayNameInternal();
}

// TODO: Implement enable stuff.
void svtkRenderedGraphRepresentation::SetEnableVerticesByArray(bool b)
{
  (void)b;
}

bool svtkRenderedGraphRepresentation::GetEnableVerticesByArray()
{
  return false;
}

void svtkRenderedGraphRepresentation::SetEnabledVerticesArrayName(const char* name)
{
  (void)name;
}

const char* svtkRenderedGraphRepresentation::GetEnabledVerticesArrayName()
{
  return nullptr;
}

void svtkRenderedGraphRepresentation::SetEnableEdgesByArray(bool b)
{
  (void)b;
}

bool svtkRenderedGraphRepresentation::GetEnableEdgesByArray()
{
  return false;
}

void svtkRenderedGraphRepresentation::SetEnabledEdgesArrayName(const char* name)
{
  (void)name;
}

const char* svtkRenderedGraphRepresentation::GetEnabledEdgesArrayName()
{
  return nullptr;
}

void svtkRenderedGraphRepresentation::SetGlyphType(int type)
{
  if (type != this->VertexGlyph->GetGlyphType())
  {
    this->VertexGlyph->SetGlyphType(type);
    this->OutlineGlyph->SetGlyphType(type);
    if (type == svtkGraphToGlyphs::SPHERE)
    {
      this->OutlineActor->GetProperty()->FrontfaceCullingOn();
    }
    else
    {
      this->OutlineActor->GetProperty()->FrontfaceCullingOff();
    }
  }
}

int svtkRenderedGraphRepresentation::GetGlyphType()
{
  return this->VertexGlyph->GetGlyphType();
}

void svtkRenderedGraphRepresentation::SetScaling(bool b)
{
  this->VertexGlyph->SetScaling(b);
  this->OutlineGlyph->SetScaling(b);
}

bool svtkRenderedGraphRepresentation::GetScaling()
{
  return this->VertexGlyph->GetScaling();
}

void svtkRenderedGraphRepresentation::SetScalingArrayName(const char* name)
{
  this->VertexGlyph->SetInputArrayToProcess(
    0, 0, 0, svtkDataObject::FIELD_ASSOCIATION_VERTICES, name);
  this->OutlineGlyph->SetInputArrayToProcess(
    0, 0, 0, svtkDataObject::FIELD_ASSOCIATION_VERTICES, name);
  this->SetScalingArrayNameInternal(name);
}

const char* svtkRenderedGraphRepresentation::GetScalingArrayName()
{
  return this->GetScalingArrayNameInternal();
}

void svtkRenderedGraphRepresentation::SetVertexScalarBarVisibility(bool b)
{
  this->VertexScalarBar->GetScalarBarActor()->SetVisibility(b);
}

bool svtkRenderedGraphRepresentation::GetVertexScalarBarVisibility()
{
  return this->VertexScalarBar->GetScalarBarActor()->GetVisibility() ? true : false;
}

void svtkRenderedGraphRepresentation::SetEdgeScalarBarVisibility(bool b)
{
  this->EdgeScalarBar->GetScalarBarActor()->SetVisibility(b);
}

bool svtkRenderedGraphRepresentation::GetEdgeScalarBarVisibility()
{
  return this->EdgeScalarBar->GetScalarBarActor()->GetVisibility() ? true : false;
}

svtkScalarBarWidget* svtkRenderedGraphRepresentation::GetVertexScalarBar()
{
  return this->VertexScalarBar;
}

svtkScalarBarWidget* svtkRenderedGraphRepresentation::GetEdgeScalarBar()
{
  return this->EdgeScalarBar;
}

bool svtkRenderedGraphRepresentation::IsLayoutComplete()
{
  return this->Layout->IsLayoutComplete() ? true : false;
}

void svtkRenderedGraphRepresentation::UpdateLayout()
{
  if (!this->IsLayoutComplete())
  {
    this->Layout->Modified();
    // TODO : Should render here??
  }
}

void svtkRenderedGraphRepresentation::SetLayoutStrategy(svtkGraphLayoutStrategy* s)
{
  if (!s)
  {
    svtkErrorMacro("Layout strategy must not be nullptr.");
    return;
  }
  if (svtkRandomLayoutStrategy::SafeDownCast(s))
  {
    this->SetLayoutStrategyName("Random");
  }
  else if (svtkForceDirectedLayoutStrategy::SafeDownCast(s))
  {
    this->SetLayoutStrategyName("Force Directed");
  }
  else if (svtkSimple2DLayoutStrategy::SafeDownCast(s))
  {
    this->SetLayoutStrategyName("Simple 2D");
  }
  else if (svtkClustering2DLayoutStrategy::SafeDownCast(s))
  {
    this->SetLayoutStrategyName("Clustering 2D");
  }
  else if (svtkCommunity2DLayoutStrategy::SafeDownCast(s))
  {
    this->SetLayoutStrategyName("Community 2D");
  }
  else if (svtkFast2DLayoutStrategy::SafeDownCast(s))
  {
    this->SetLayoutStrategyName("Fast 2D");
  }
  else if (svtkCircularLayoutStrategy::SafeDownCast(s))
  {
    this->SetLayoutStrategyName("Circular");
  }
  else if (svtkTreeLayoutStrategy::SafeDownCast(s))
  {
    this->SetLayoutStrategyName("Tree");
  }
  else if (svtkCosmicTreeLayoutStrategy::SafeDownCast(s))
  {
    this->SetLayoutStrategyName("Cosmic Tree");
  }
  else if (svtkPassThroughLayoutStrategy::SafeDownCast(s))
  {
    this->SetLayoutStrategyName("Pass Through");
  }
  else if (svtkConeLayoutStrategy::SafeDownCast(s))
  {
    this->SetLayoutStrategyName("Cone");
  }
  else if (svtkSpanTreeLayoutStrategy::SafeDownCast(s))
  {
    this->SetLayoutStrategyName("Span Tree");
  }
  else
  {
    this->SetLayoutStrategyName("Unknown");
  }
  this->Layout->SetLayoutStrategy(s);
}

svtkGraphLayoutStrategy* svtkRenderedGraphRepresentation::GetLayoutStrategy()
{
  return this->Layout->GetLayoutStrategy();
}

void svtkRenderedGraphRepresentation::SetLayoutStrategy(const char* name)
{
  std::string str = name;
  std::transform(str.begin(), str.end(), str.begin(), tolower);
  str.erase(std::remove(str.begin(), str.end(), ' '), str.end());
  svtkSmartPointer<svtkGraphLayoutStrategy> strategy =
    svtkSmartPointer<svtkPassThroughLayoutStrategy>::New();
  if (str == "random")
  {
    strategy = svtkSmartPointer<svtkRandomLayoutStrategy>::New();
  }
  else if (str == "forcedirected")
  {
    strategy = svtkSmartPointer<svtkForceDirectedLayoutStrategy>::New();
  }
  else if (str == "simple2d")
  {
    strategy = svtkSmartPointer<svtkSimple2DLayoutStrategy>::New();
  }
  else if (str == "clustering2d")
  {
    strategy = svtkSmartPointer<svtkClustering2DLayoutStrategy>::New();
  }
  else if (str == "community2d")
  {
    strategy = svtkSmartPointer<svtkCommunity2DLayoutStrategy>::New();
  }
  else if (str == "fast2d")
  {
    strategy = svtkSmartPointer<svtkFast2DLayoutStrategy>::New();
  }
  else if (str == "circular")
  {
    strategy = svtkSmartPointer<svtkCircularLayoutStrategy>::New();
  }
  else if (str == "tree")
  {
    strategy = svtkSmartPointer<svtkTreeLayoutStrategy>::New();
  }
  else if (str == "cosmictree")
  {
    strategy = svtkSmartPointer<svtkCosmicTreeLayoutStrategy>::New();
  }
  else if (str == "cone")
  {
    strategy = svtkSmartPointer<svtkConeLayoutStrategy>::New();
  }
  else if (str == "spantree")
  {
    strategy = svtkSmartPointer<svtkSpanTreeLayoutStrategy>::New();
  }
  else if (str != "passthrough")
  {
    svtkErrorMacro("Unknown layout strategy: \"" << name << "\"");
  }
  std::string type1 = strategy->GetClassName();
  std::string type2 = this->GetLayoutStrategy()->GetClassName();
  if (type1 != type2)
  {
    this->SetLayoutStrategy(strategy);
  }
}

void svtkRenderedGraphRepresentation::SetLayoutStrategyToAssignCoordinates(
  const char* xarr, const char* yarr, const char* zarr)
{
  svtkAssignCoordinatesLayoutStrategy* s =
    svtkAssignCoordinatesLayoutStrategy::SafeDownCast(this->GetLayoutStrategy());
  if (!s)
  {
    s = svtkAssignCoordinatesLayoutStrategy::New();
    this->SetLayoutStrategy(s);
    s->Delete();
  }
  s->SetXCoordArrayName(xarr);
  s->SetYCoordArrayName(yarr);
  s->SetZCoordArrayName(zarr);
}

void svtkRenderedGraphRepresentation::SetLayoutStrategyToTree(
  bool radial, double angle, double leafSpacing, double logSpacing)
{
  svtkTreeLayoutStrategy* s = svtkTreeLayoutStrategy::SafeDownCast(this->GetLayoutStrategy());
  if (!s)
  {
    s = svtkTreeLayoutStrategy::New();
    this->SetLayoutStrategy(s);
    s->Delete();
  }
  s->SetRadial(radial);
  s->SetAngle(angle);
  s->SetLeafSpacing(leafSpacing);
  s->SetLogSpacingValue(logSpacing);
}

void svtkRenderedGraphRepresentation::SetLayoutStrategyToCosmicTree(
  const char* nodeSizeArrayName, bool sizeLeafNodesOnly, int layoutDepth, svtkIdType layoutRoot)
{
  svtkCosmicTreeLayoutStrategy* s =
    svtkCosmicTreeLayoutStrategy::SafeDownCast(this->GetLayoutStrategy());
  if (!s)
  {
    s = svtkCosmicTreeLayoutStrategy::New();
    this->SetLayoutStrategy(s);
    s->Delete();
  }
  s->SetNodeSizeArrayName(nodeSizeArrayName);
  s->SetSizeLeafNodesOnly(sizeLeafNodesOnly);
  s->SetLayoutDepth(layoutDepth);
  s->SetLayoutRoot(layoutRoot);
}

void svtkRenderedGraphRepresentation::SetEdgeLayoutStrategy(svtkEdgeLayoutStrategy* s)
{
  if (!s)
  {
    svtkErrorMacro("Layout strategy must not be nullptr.");
    return;
  }
  if (svtkArcParallelEdgeStrategy::SafeDownCast(s))
  {
    this->SetEdgeLayoutStrategyName("Arc Parallel");
  }
  else if (svtkGeoEdgeStrategy::SafeDownCast(s))
  {
    this->SetEdgeLayoutStrategyName("Geo");
  }
  else if (svtkPassThroughEdgeStrategy::SafeDownCast(s))
  {
    this->SetEdgeLayoutStrategyName("Pass Through");
  }
  else
  {
    this->SetEdgeLayoutStrategyName("Unknown");
  }
  this->EdgeLayout->SetLayoutStrategy(s);
}

svtkEdgeLayoutStrategy* svtkRenderedGraphRepresentation::GetEdgeLayoutStrategy()
{
  return this->EdgeLayout->GetLayoutStrategy();
}

void svtkRenderedGraphRepresentation::SetEdgeLayoutStrategy(const char* name)
{
  std::string str = name;
  std::transform(str.begin(), str.end(), str.begin(), tolower);
  str.erase(std::remove(str.begin(), str.end(), ' '), str.end());
  svtkSmartPointer<svtkEdgeLayoutStrategy> strategy =
    svtkSmartPointer<svtkPassThroughEdgeStrategy>::New();
  if (str == "arcparallel")
  {
    strategy = svtkSmartPointer<svtkArcParallelEdgeStrategy>::New();
  }
  else if (str == "geo")
  {
    strategy = svtkSmartPointer<svtkGeoEdgeStrategy>::New();
  }
  else if (str != "passthrough")
  {
    svtkErrorMacro("Unknown layout strategy: \"" << name << "\"");
  }
  std::string type1 = strategy->GetClassName();
  std::string type2 = this->GetEdgeLayoutStrategy()->GetClassName();
  if (type1 != type2)
  {
    this->SetEdgeLayoutStrategy(strategy);
  }
}

void svtkRenderedGraphRepresentation::SetEdgeLayoutStrategyToGeo(double explodeFactor)
{
  svtkGeoEdgeStrategy* s = svtkGeoEdgeStrategy::SafeDownCast(this->GetLayoutStrategy());
  if (!s)
  {
    s = svtkGeoEdgeStrategy::New();
    this->SetEdgeLayoutStrategy(s);
    s->Delete();
  }
  s->SetExplodeFactor(explodeFactor);
}

bool svtkRenderedGraphRepresentation::AddToView(svtkView* view)
{
  this->Superclass::AddToView(view);
  svtkRenderView* rv = svtkRenderView::SafeDownCast(view);
  if (rv)
  {
    this->VertexScalarBar->SetInteractor(rv->GetRenderWindow()->GetInteractor());
    this->EdgeScalarBar->SetInteractor(rv->GetRenderWindow()->GetInteractor());
    this->VertexGlyph->SetRenderer(rv->GetRenderer());
    this->OutlineGlyph->SetRenderer(rv->GetRenderer());
    this->VertexIconTransform->SetViewport(rv->GetRenderer());
    rv->GetRenderer()->AddActor(this->OutlineActor);
    rv->GetRenderer()->AddActor(this->VertexActor);
    rv->GetRenderer()->AddActor(this->EdgeActor);
    rv->GetRenderer()->AddActor(this->VertexScalarBar->GetScalarBarActor());
    rv->GetRenderer()->AddActor(this->EdgeScalarBar->GetScalarBarActor());
    rv->GetRenderer()->AddActor(this->VertexIconActor);
    rv->AddLabels(this->VertexLabelHierarchy->GetOutputPort());
    rv->AddLabels(this->EdgeLabelHierarchy->GetOutputPort());
    // rv->AddIcons(this->VertexIconPriority->GetOutputPort());
    // rv->AddIcons(this->EdgeIconPriority->GetOutputPort());
    rv->RegisterProgress(this->Layout);
    rv->RegisterProgress(this->EdgeCenters);
    rv->RegisterProgress(this->GraphToPoints);
    rv->RegisterProgress(this->VertexLabelHierarchy);
    rv->RegisterProgress(this->EdgeLabelHierarchy);
    rv->RegisterProgress(this->Layout);
    rv->RegisterProgress(this->EdgeLayout);
    rv->RegisterProgress(this->GraphToPoly);
    rv->RegisterProgress(this->EdgeMapper);
    rv->RegisterProgress(this->VertexGlyph);
    rv->RegisterProgress(this->VertexMapper);
    rv->RegisterProgress(this->OutlineGlyph);
    rv->RegisterProgress(this->OutlineMapper);
    return true;
  }
  return false;
}

bool svtkRenderedGraphRepresentation::RemoveFromView(svtkView* view)
{
  this->Superclass::RemoveFromView(view);
  svtkRenderView* rv = svtkRenderView::SafeDownCast(view);
  if (rv)
  {
    this->VertexGlyph->SetRenderer(nullptr);
    this->OutlineGlyph->SetRenderer(nullptr);
    rv->GetRenderer()->RemoveActor(this->VertexActor);
    rv->GetRenderer()->RemoveActor(this->OutlineActor);
    rv->GetRenderer()->RemoveActor(this->EdgeActor);
    rv->GetRenderer()->RemoveActor(this->VertexScalarBar->GetScalarBarActor());
    rv->GetRenderer()->RemoveActor(this->EdgeScalarBar->GetScalarBarActor());
    rv->GetRenderer()->RemoveActor(this->VertexIconActor);
    rv->RemoveLabels(this->VertexLabelHierarchy->GetOutputPort());
    rv->RemoveLabels(this->EdgeLabelHierarchy->GetOutputPort());
    // rv->RemoveIcons(this->VertexIcons->GetOutputPort());
    // rv->RemoveIcons(this->EdgeIcons->GetOutputPort());
    rv->UnRegisterProgress(this->Layout);
    rv->UnRegisterProgress(this->EdgeCenters);
    rv->UnRegisterProgress(this->GraphToPoints);
    rv->UnRegisterProgress(this->VertexLabelHierarchy);
    rv->UnRegisterProgress(this->EdgeLabelHierarchy);
    rv->UnRegisterProgress(this->Layout);
    rv->UnRegisterProgress(this->EdgeLayout);
    rv->UnRegisterProgress(this->GraphToPoly);
    rv->UnRegisterProgress(this->EdgeMapper);
    rv->UnRegisterProgress(this->VertexGlyph);
    rv->UnRegisterProgress(this->VertexMapper);
    rv->UnRegisterProgress(this->OutlineGlyph);
    rv->UnRegisterProgress(this->OutlineMapper);
    return true;
  }
  return false;
}

void svtkRenderedGraphRepresentation::PrepareForRendering(svtkRenderView* view)
{
  this->Superclass::PrepareForRendering(view);

  this->VertexIconActor->SetTexture(view->GetIconTexture());
  if (this->VertexIconActor->GetTexture() && this->VertexIconActor->GetTexture()->GetInput())
  {
    this->VertexIconGlyph->SetIconSize(view->GetIconSize());
    this->VertexIconGlyph->SetDisplaySize(view->GetDisplaySize());
    this->VertexIconGlyph->SetUseIconSize(false);
    this->VertexIconActor->GetTexture()->SetColorMode(SVTK_COLOR_MODE_DEFAULT);
    this->VertexIconActor->GetTexture()->GetInputAlgorithm()->Update();
    int* dim = this->VertexIconActor->GetTexture()->GetInput()->GetDimensions();
    this->VertexIconGlyph->SetIconSheetSize(dim);
  }

  // Make sure the transform is synchronized between rep and view
  this->Layout->SetTransform(view->GetTransform());
}

svtkSelection* svtkRenderedGraphRepresentation::ConvertSelection(
  svtkView* svtkNotUsed(view), svtkSelection* sel)
{
  // Search for selection nodes relating to the vertex and edges
  // of the graph.
  svtkSmartPointer<svtkSelectionNode> vertexNode = svtkSmartPointer<svtkSelectionNode>::New();
  svtkSmartPointer<svtkSelectionNode> edgeNode = svtkSmartPointer<svtkSelectionNode>::New();
  bool foundEdgeNode = false;

  if (sel->GetNumberOfNodes() > 0)
  {
    for (unsigned int i = 0; i < sel->GetNumberOfNodes(); ++i)
    {
      svtkSelectionNode* node = sel->GetNode(i);
      svtkProp* prop = svtkProp::SafeDownCast(node->GetProperties()->Get(svtkSelectionNode::PROP()));
      if (node->GetContentType() == svtkSelectionNode::FRUSTUM)
      {
        // A frustum selection can be used to select vertices and edges.
        vertexNode->ShallowCopy(node);
        edgeNode->ShallowCopy(node);
        foundEdgeNode = true;
      }
      else if (prop == this->VertexActor.GetPointer())
      {
        // The prop on the selection matches the vertex actor, so
        // this must have been a visible cell selection.
        vertexNode->ShallowCopy(node);
      }
      else if (prop == this->EdgeActor.GetPointer())
      {
        // The prop on the selection matches the edge actor, so
        // this must have been a visible cell selection.
        edgeNode->ShallowCopy(node);
        foundEdgeNode = true;
      }
    }
  }

  // Remove the prop to avoid reference loops.
  vertexNode->GetProperties()->Remove(svtkSelectionNode::PROP());
  edgeNode->GetProperties()->Remove(svtkSelectionNode::PROP());

  svtkSelection* converted = svtkSelection::New();
  svtkGraph* input = svtkGraph::SafeDownCast(this->GetInput());
  if (!input)
  {
    return converted;
  }

  bool selectedVerticesFound = false;
  if (vertexNode)
  {
    // Convert a cell selection on the glyphed vertices into a
    // vertex selection on the graph of the appropriate type.

    // First, convert the cell selection on the polydata to
    // a pedigree ID selection (or index selection if there are no
    // pedigree IDs).
    svtkSmartPointer<svtkSelection> vertexSel = svtkSmartPointer<svtkSelection>::New();
    vertexSel->AddNode(vertexNode);

    svtkPolyData* poly = svtkPolyData::SafeDownCast(this->VertexGlyph->GetOutput());
    svtkSmartPointer<svtkTable> temp = svtkSmartPointer<svtkTable>::New();
    temp->SetRowData(svtkPolyData::SafeDownCast(poly)->GetCellData());
    svtkSelection* polyConverted = nullptr;
    if (poly->GetCellData()->GetPedigreeIds())
    {
      polyConverted =
        svtkConvertSelection::ToSelectionType(vertexSel, poly, svtkSelectionNode::PEDIGREEIDS);
    }
    else
    {
      polyConverted =
        svtkConvertSelection::ToSelectionType(vertexSel, poly, svtkSelectionNode::INDICES);
    }

    // Now that we have a pedigree or index selection, interpret this
    // as a vertex selection on the graph, and convert it to the
    // appropriate selection type for this representation.
    for (unsigned int i = 0; i < polyConverted->GetNumberOfNodes(); ++i)
    {
      polyConverted->GetNode(i)->SetFieldType(svtkSelectionNode::VERTEX);
    }
    svtkSelection* vertexConverted = svtkConvertSelection::ToSelectionType(
      polyConverted, input, this->SelectionType, this->SelectionArrayNames);

    // For all output selection nodes, select all the edges among selected vertices.
    for (unsigned int i = 0; i < vertexConverted->GetNumberOfNodes(); ++i)
    {
      if ((vertexConverted->GetNode(i)->GetSelectionList()->GetNumberOfTuples() > 0) &&
        (input->GetNumberOfEdges()) > 0)
      {
        // Get the list of selected vertices.
        selectedVerticesFound = true;
        svtkSmartPointer<svtkIdTypeArray> selectedVerts = svtkSmartPointer<svtkIdTypeArray>::New();
        svtkConvertSelection::GetSelectedVertices(vertexConverted, input, selectedVerts);

        if (this->EdgeSelection)
        {
          // Get the list of induced edges on these vertices.
          svtkSmartPointer<svtkIdTypeArray> selectedEdges = svtkSmartPointer<svtkIdTypeArray>::New();
          input->GetInducedEdges(selectedVerts, selectedEdges);

          // Create an edge index selection containing the induced edges.
          svtkSmartPointer<svtkSelection> edgeSelection = svtkSmartPointer<svtkSelection>::New();
          svtkSmartPointer<svtkSelectionNode> edgeSelectionNode =
            svtkSmartPointer<svtkSelectionNode>::New();
          edgeSelectionNode->SetSelectionList(selectedEdges);
          edgeSelectionNode->SetContentType(svtkSelectionNode::INDICES);
          edgeSelectionNode->SetFieldType(svtkSelectionNode::EDGE);
          edgeSelection->AddNode(edgeSelectionNode);

          // Convert the edge selection to the appropriate type for this representation.
          svtkSelection* edgeConverted = svtkConvertSelection::ToSelectionType(
            edgeSelection, input, this->SelectionType, this->SelectionArrayNames);

          // Add the converted induced edge selection to the output selection.
          if (edgeConverted->GetNumberOfNodes() > 0)
          {
            converted->AddNode(edgeConverted->GetNode(0));
          }
          edgeConverted->Delete();
        }
      }

      // Add the vertex selection node to the output selection.
      converted->AddNode(vertexConverted->GetNode(i));
    }
    polyConverted->Delete();
    vertexConverted->Delete();
  }
  if (foundEdgeNode && !selectedVerticesFound && this->EdgeSelection)
  {
    // If no vertices were found (hence no induced edges), look for
    // edges that were within the selection box.

    // First, convert the cell selection on the polydata to
    // a pedigree ID selection (or index selection if there are no
    // pedigree IDs).
    svtkSmartPointer<svtkSelection> edgeSel = svtkSmartPointer<svtkSelection>::New();
    edgeSel->AddNode(edgeNode);
    svtkPolyData* poly = svtkPolyData::SafeDownCast(this->GraphToPoly->GetOutput());
    svtkSelection* polyConverted = nullptr;
    if (poly->GetCellData()->GetPedigreeIds())
    {
      polyConverted =
        svtkConvertSelection::ToSelectionType(edgeSel, poly, svtkSelectionNode::PEDIGREEIDS);
    }
    else
    {
      polyConverted =
        svtkConvertSelection::ToSelectionType(edgeSel, poly, svtkSelectionNode::INDICES);
    }

    // Now that we have a pedigree or index selection, interpret this
    // as an edge selection on the graph, and convert it to the
    // appropriate selection type for this representation.
    for (unsigned int i = 0; i < polyConverted->GetNumberOfNodes(); ++i)
    {
      polyConverted->GetNode(i)->SetFieldType(svtkSelectionNode::EDGE);
    }

    // Convert the edge selection to the appropriate type for this representation.
    svtkSelection* edgeConverted = svtkConvertSelection::ToSelectionType(
      polyConverted, input, this->SelectionType, this->SelectionArrayNames);

    // Add the vertex selection node to the output selection.
    for (unsigned int i = 0; i < edgeConverted->GetNumberOfNodes(); ++i)
    {
      converted->AddNode(edgeConverted->GetNode(i));
    }
    polyConverted->Delete();
    edgeConverted->Delete();
  }
  return converted;
}

int svtkRenderedGraphRepresentation::RequestData(
  svtkInformation*, svtkInformationVector**, svtkInformationVector*)
{
  this->Layout->SetInputConnection(this->GetInternalOutputPort());
  this->ApplyColors->SetInputConnection(1, this->GetInternalAnnotationOutputPort());
  this->ApplyVertexIcons->SetInputConnection(1, this->GetInternalAnnotationOutputPort());
  this->RemoveHiddenGraph->SetInputConnection(1, this->GetInternalAnnotationOutputPort());
  return 1;
}

void svtkRenderedGraphRepresentation::ApplyViewTheme(svtkViewTheme* theme)
{
  this->Superclass::ApplyViewTheme(theme);

  this->ApplyColors->SetPointLookupTable(theme->GetPointLookupTable());
  this->ApplyColors->SetCellLookupTable(theme->GetCellLookupTable());
  this->VertexScalarBar->GetScalarBarActor()->SetLookupTable(theme->GetPointLookupTable());
  this->EdgeScalarBar->GetScalarBarActor()->SetLookupTable(theme->GetCellLookupTable());

  this->ApplyColors->SetDefaultPointColor(theme->GetPointColor());
  this->ApplyColors->SetDefaultPointOpacity(theme->GetPointOpacity());
  this->ApplyColors->SetDefaultCellColor(theme->GetCellColor());
  this->ApplyColors->SetDefaultCellOpacity(theme->GetCellOpacity());
  this->ApplyColors->SetSelectedPointColor(theme->GetSelectedPointColor());
  this->ApplyColors->SetSelectedPointOpacity(theme->GetSelectedPointOpacity());
  this->ApplyColors->SetSelectedCellColor(theme->GetSelectedCellColor());
  this->ApplyColors->SetSelectedCellOpacity(theme->GetSelectedCellOpacity());
  this->ApplyColors->SetScalePointLookupTable(theme->GetScalePointLookupTable());
  this->ApplyColors->SetScaleCellLookupTable(theme->GetScaleCellLookupTable());

  float baseSize = static_cast<float>(theme->GetPointSize());
  float lineWidth = static_cast<float>(theme->GetLineWidth());
  this->VertexGlyph->SetScreenSize(baseSize);
  this->VertexActor->GetProperty()->SetPointSize(baseSize);
  this->OutlineGlyph->SetScreenSize(baseSize + 2);
  this->OutlineActor->GetProperty()->SetPointSize(baseSize + 2);
  this->OutlineActor->GetProperty()->SetLineWidth(1);
  this->EdgeActor->GetProperty()->SetLineWidth(lineWidth);

  this->OutlineActor->GetProperty()->SetColor(theme->GetOutlineColor());

  // FIXME: This is a strange hack to get around some weirdness with
  // the gradient background and multiple transparent actors (assuming
  // related to depth peeling or some junk...)
  if (theme->GetPointOpacity() == 0)
  {
    this->OutlineActor->VisibilityOff();
  }

  this->GetVertexLabelTextProperty()->ShallowCopy(theme->GetPointTextProperty());
  this->GetVertexLabelTextProperty()->SetLineOffset(-2 * baseSize);
  this->GetEdgeLabelTextProperty()->ShallowCopy(theme->GetCellTextProperty());

  // Moronic hack.. the circles seem to be really small so make them bigger
  if (this->VertexGlyph->GetGlyphType() == svtkGraphToGlyphs::CIRCLE)
  {
    this->VertexGlyph->SetScreenSize(baseSize * 2 + 1);
    this->OutlineGlyph->SetScreenSize(baseSize * 2 + 1);
  }
}

//----------------------------------------------------------------------------
void svtkRenderedGraphRepresentation::ComputeSelectedGraphBounds(double bounds[6])
{
  // Bring the graph up to date
  this->Layout->Update();

  // Convert to an index selection
  svtkSmartPointer<svtkConvertSelection> cs = svtkSmartPointer<svtkConvertSelection>::New();
  cs->SetInputConnection(0, this->GetInternalSelectionOutputPort());
  cs->SetInputConnection(1, this->Layout->GetOutputPort());
  cs->SetOutputType(svtkSelectionNode::INDICES);
  cs->Update();
  svtkGraph* data = svtkGraph::SafeDownCast(this->Layout->GetOutput());
  svtkSelection* converted = cs->GetOutput();

  // Iterate over the selection's nodes, constructing a list of selected vertices.
  // In the case of an edge selection, we add the edges' vertices to vertex list.

  svtkSmartPointer<svtkIdTypeArray> edgeList = svtkSmartPointer<svtkIdTypeArray>::New();
  bool hasEdges = false;
  svtkSmartPointer<svtkIdTypeArray> vertexList = svtkSmartPointer<svtkIdTypeArray>::New();
  for (unsigned int m = 0; m < converted->GetNumberOfNodes(); ++m)
  {
    svtkSelectionNode* node = converted->GetNode(m);
    svtkIdTypeArray* list = nullptr;
    if (node->GetFieldType() == svtkSelectionNode::VERTEX)
    {
      list = vertexList;
    }
    else if (node->GetFieldType() == svtkSelectionNode::EDGE)
    {
      list = edgeList;
      hasEdges = true;
    }

    if (list)
    {
      // Append the selection list to the selection
      svtkIdTypeArray* curList = svtkArrayDownCast<svtkIdTypeArray>(node->GetSelectionList());
      if (curList)
      {
        int inverse = node->GetProperties()->Get(svtkSelectionNode::INVERSE());
        if (inverse)
        {
          svtkIdType num = (node->GetFieldType() == svtkSelectionNode::VERTEX)
            ? data->GetNumberOfVertices()
            : data->GetNumberOfEdges();
          for (svtkIdType j = 0; j < num; ++j)
          {
            if (curList->LookupValue(j) < 0 && list->LookupValue(j) < 0)
            {
              list->InsertNextValue(j);
            }
          }
        }
        else
        {
          svtkIdType numTuples = curList->GetNumberOfTuples();
          for (svtkIdType j = 0; j < numTuples; ++j)
          {
            svtkIdType curValue = curList->GetValue(j);
            if (list->LookupValue(curValue) < 0)
            {
              list->InsertNextValue(curValue);
            }
          }
        }
      } // end if (curList)
    }   // end if (list)
  }     // end for each child

  svtkIdType i;
  if (hasEdges)
  {
    svtkIdType numSelectedEdges = edgeList->GetNumberOfTuples();
    for (i = 0; i < numSelectedEdges; ++i)
    {
      svtkIdType eid = edgeList->GetValue(i);
      vertexList->InsertNextValue(data->GetSourceVertex(eid));
      vertexList->InsertNextValue(data->GetTargetVertex(eid));
    }
  }

  // If there is no selection list, return
  if (vertexList->GetNumberOfTuples() == 0)
  {
    return;
  }

  // Now we use our list of vertices to get the point coordinates
  // of the selection and use that to initialize the bounds that
  // we'll use to reset the camera.

  double position[3];
  data->GetPoint(vertexList->GetValue(0), position);
  bounds[0] = bounds[1] = position[0];
  bounds[2] = bounds[3] = position[1];
  bounds[4] = -0.1;
  bounds[5] = 0.1;
  for (i = 1; i < vertexList->GetNumberOfTuples(); ++i)
  {
    data->GetPoint(vertexList->GetValue(i), position);

    if (position[0] < bounds[0])
    {
      bounds[0] = position[0];
    }
    if (position[0] > bounds[1])
    {
      bounds[1] = position[0];
    }
    if (position[1] < bounds[2])
    {
      bounds[2] = position[1];
    }
    if (position[1] > bounds[3])
    {
      bounds[3] = position[1];
    }
  }
}

svtkUnicodeString svtkRenderedGraphRepresentation::GetHoverTextInternal(svtkSelection* sel)
{
  svtkGraph* input = svtkGraph::SafeDownCast(this->GetInput());
  svtkSmartPointer<svtkIdTypeArray> selectedItems = svtkSmartPointer<svtkIdTypeArray>::New();
  svtkConvertSelection::GetSelectedVertices(sel, input, selectedItems);
  svtkDataSetAttributes* data = input->GetVertexData();
  const char* hoverArrName = this->GetVertexHoverArrayName();
  if (selectedItems->GetNumberOfTuples() == 0)
  {
    svtkConvertSelection::GetSelectedEdges(sel, input, selectedItems);
    data = input->GetEdgeData();
    hoverArrName = this->GetEdgeHoverArrayName();
  }
  if (selectedItems->GetNumberOfTuples() == 0 || !hoverArrName)
  {
    return svtkUnicodeString();
  }
  svtkAbstractArray* arr = data->GetAbstractArray(hoverArrName);
  if (!arr)
  {
    return svtkUnicodeString();
  }
  svtkIdType item = selectedItems->GetValue(0);
  return arr->GetVariantValue(item).ToUnicodeString();
}

void svtkRenderedGraphRepresentation::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent
     << "LayoutStrategyName: " << (this->LayoutStrategyName ? this->LayoutStrategyName : "(none)")
     << endl;
  os << indent << "EdgeLayoutStrategyName: "
     << (this->EdgeLayoutStrategyName ? this->EdgeLayoutStrategyName : "(none)") << endl;
  os << indent << "VertexHoverArrayName: "
     << (this->VertexHoverArrayName ? this->VertexHoverArrayName : "(none)") << endl;
  os << indent
     << "EdgeHoverArrayName: " << (this->EdgeHoverArrayName ? this->EdgeHoverArrayName : "(none)")
     << endl;
  os << indent
     << "HideVertexLabelsOnInteraction: " << (this->HideVertexLabelsOnInteraction ? "On" : "Off")
     << endl;
  os << indent
     << "HideEdgeLabelsOnInteraction: " << (this->HideEdgeLabelsOnInteraction ? "On" : "Off")
     << endl;
}
