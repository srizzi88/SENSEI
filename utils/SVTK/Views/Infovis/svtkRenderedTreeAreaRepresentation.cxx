/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkRenderedTreeAreaRepresentation.cxx

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

#include "svtkRenderedTreeAreaRepresentation.h"

#include "svtkActor.h"
#include "svtkActor2D.h"
#include "svtkAlgorithmOutput.h"
#include "svtkAppendPolyData.h"
#include "svtkApplyColors.h"
#include "svtkAreaLayout.h"
#include "svtkCellArray.h"
#include "svtkConvertSelection.h"
#include "svtkDataSetAttributes.h"
#include "svtkDynamic2DLabelMapper.h"
#include "svtkEdgeCenters.h"
#include "svtkExtractEdges.h"
#include "svtkExtractSelectedGraph.h"
#include "svtkExtractSelectedPolyDataIds.h"
#include "svtkGraphHierarchicalBundleEdges.h"
#include "svtkGraphLayout.h"
#include "svtkGraphMapper.h"
#include "svtkHierarchicalGraphPipeline.h"
#include "svtkIdTypeArray.h"
#include "svtkInformation.h"
#include "svtkInteractorStyleAreaSelectHover.h"
#include "svtkLookupTable.h"
#include "svtkMath.h"
#include "svtkObjectFactory.h"
#include "svtkOutEdgeIterator.h"
#include "svtkPointSetToLabelHierarchy.h"
#include "svtkPolyData.h"
#include "svtkPolyDataMapper.h"
#include "svtkProp.h"
#include "svtkProperty.h"
#include "svtkRenderView.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkScalarBarActor.h"
#include "svtkScalarBarWidget.h"
#include "svtkSectorSource.h"
#include "svtkSelection.h"
#include "svtkSelectionNode.h"
#include "svtkSplineGraphEdges.h"
#include "svtkStackedTreeLayoutStrategy.h"
#include "svtkStringArray.h"
#include "svtkTextProperty.h"
#include "svtkTexturedActor2D.h"
#include "svtkTreeFieldAggregator.h"
#include "svtkTreeLevelsFilter.h"
#include "svtkTreeRingToPolyData.h"
#include "svtkVertexDegree.h"
#include "svtkViewTheme.h"
#include "svtkWorldPointPicker.h"

#ifdef SVTK_USE_QT
#include "svtkQtTreeRingLabelMapper.h"
#endif

#include <vector>

class svtkRenderedTreeAreaRepresentation::Internals
{
public:
  std::vector<svtkSmartPointer<svtkHierarchicalGraphPipeline> > Graphs;
};

svtkStandardNewMacro(svtkRenderedTreeAreaRepresentation);

svtkRenderedTreeAreaRepresentation::svtkRenderedTreeAreaRepresentation()
{
  this->Implementation = new Internals;
  this->SetNumberOfInputPorts(2);
  // Processing objects
  this->ApplyColors = svtkSmartPointer<svtkApplyColors>::New();
  this->VertexDegree = svtkSmartPointer<svtkVertexDegree>::New();
  this->TreeAggregation = svtkSmartPointer<svtkTreeFieldAggregator>::New();
  this->TreeLevels = svtkSmartPointer<svtkTreeLevelsFilter>::New();
  this->Picker = svtkSmartPointer<svtkWorldPointPicker>::New();
  this->EdgeScalarBar = svtkSmartPointer<svtkScalarBarWidget>::New();

  // Area objects
  this->AreaLayout = svtkSmartPointer<svtkAreaLayout>::New();
  this->AreaToPolyData = svtkTreeRingToPolyData::New();
  this->AreaMapper = svtkSmartPointer<svtkPolyDataMapper>::New();
  this->AreaActor = svtkSmartPointer<svtkActor>::New();
  this->AreaLabelMapper = svtkDynamic2DLabelMapper::New();
  this->AreaLabelActor = svtkSmartPointer<svtkActor2D>::New();
  this->HighlightData = svtkSmartPointer<svtkPolyData>::New();
  this->HighlightMapper = svtkSmartPointer<svtkPolyDataMapper>::New();
  this->HighlightActor = svtkSmartPointer<svtkActor>::New();
  this->AreaLabelHierarchy = svtkSmartPointer<svtkPointSetToLabelHierarchy>::New();
  this->EmptyPolyData = svtkSmartPointer<svtkPolyData>::New();

  this->AreaSizeArrayNameInternal = nullptr;
  this->AreaColorArrayNameInternal = nullptr;
  this->AreaLabelArrayNameInternal = nullptr;
  this->AreaLabelPriorityArrayNameInternal = nullptr;
  this->AreaHoverTextInternal = nullptr;
  this->AreaHoverArrayName = nullptr;
  this->UseRectangularCoordinates = false;

  this->SetAreaColorArrayName("level");
  this->ColorAreasByArrayOn();
  this->SetAreaSizeArrayName("size");
  this->SetGraphEdgeColorArrayName("fraction");
  this->ColorGraphEdgesByArrayOn();
  svtkDynamic2DLabelMapper* areaMapper = svtkDynamic2DLabelMapper::New();
  this->SetAreaLabelMapper(areaMapper);
  areaMapper->Delete();
  this->AreaLabelActor->PickableOff();

  svtkSmartPointer<svtkStackedTreeLayoutStrategy> strategy =
    svtkSmartPointer<svtkStackedTreeLayoutStrategy>::New();
  strategy->SetReverse(true);
  this->AreaLayout->SetLayoutStrategy(strategy);
  this->AreaLayout->SetAreaArrayName("area");
  this->SetShrinkPercentage(0.1);
  this->AreaToPolyData->SetInputArrayToProcess(
    0, 0, 0, svtkDataObject::FIELD_ASSOCIATION_VERTICES, "area");

  // Set default parameters
  this->SetAreaLabelArrayName("id");
  this->AreaLabelVisibilityOff();
  this->EdgeScalarBar->GetScalarBarActor()->VisibilityOff();
  this->EdgeScalarBar->SetRepositionable(true);

  // Apply default theme
  svtkViewTheme* theme = svtkViewTheme::New();
  this->ApplyViewTheme(theme);
  theme->Delete();

  // Set filter attributes
  this->TreeAggregation->LeafVertexUnitSizeOn();

  // Highlight
  this->HighlightMapper->SetInputData(this->HighlightData);
  this->HighlightActor->SetMapper(this->HighlightMapper);
  this->HighlightActor->VisibilityOff();
  this->HighlightActor->PickableOff();
  this->HighlightActor->GetProperty()->SetLineWidth(4.0);

  /*
  <graphviz>
  digraph {
    "Tree input" -> TreeLevels -> VertexDegree -> TreeAggregation -> AreaLayout
    AreaLayout -> ApplyColors -> AreaToPolyData -> AreaMapper -> AreaActor
    AreaLayout -> AreaLabelMapper -> AreaLabelActor
    AreaLayout -> svtkHierarchicalGraphPipeline
    "Graph input" -> svtkHierarchicalGraphPipeline
  }
  </graphviz>
  */

  this->VertexDegree->SetInputConnection(this->TreeLevels->GetOutputPort());
  this->TreeAggregation->SetInputConnection(this->VertexDegree->GetOutputPort());
  this->AreaLayout->SetInputConnection(this->TreeAggregation->GetOutputPort());

  this->ApplyColors->SetInputConnection(this->AreaLayout->GetOutputPort());
  this->AreaToPolyData->SetInputConnection(this->ApplyColors->GetOutputPort());
  this->AreaMapper->SetInputConnection(this->AreaToPolyData->GetOutputPort());
  this->AreaMapper->SetScalarModeToUseCellFieldData();
  this->AreaMapper->SelectColorArray("svtkApplyColors color");
  this->AreaActor->SetMapper(this->AreaMapper);

  this->AreaLabelHierarchy->SetInputData(this->EmptyPolyData);

  // Set the orientation array to be the text rotation array produced by
  // svtkStackedTreeLayoutStrategy.
  this->AreaLabelHierarchy->SetInputArrayToProcess(4, 0, 0, svtkDataObject::VERTEX, "TextRotation");
  this->AreaLabelHierarchy->SetInputArrayToProcess(
    5, 0, 0, svtkDataObject::VERTEX, "TextBoundedSize");

  // this->AreaLabelMapper->SetInputConnection(this->AreaLayout->GetOutputPort());
  // this->AreaLabelActor->SetMapper(this->AreaLabelMapper);
}

svtkRenderedTreeAreaRepresentation::~svtkRenderedTreeAreaRepresentation()
{
  this->SetAreaSizeArrayNameInternal(nullptr);
  this->SetAreaColorArrayNameInternal(nullptr);
  this->SetAreaLabelArrayNameInternal(nullptr);
  this->SetAreaLabelPriorityArrayNameInternal(nullptr);
  this->SetAreaHoverTextInternal(nullptr);
  this->SetAreaHoverArrayName(nullptr);
  delete this->Implementation;
  if (this->AreaLabelMapper)
  {
    this->AreaLabelMapper->Delete();
  }
  if (this->AreaToPolyData)
  {
    this->AreaToPolyData->Delete();
  }
}

const char* svtkRenderedTreeAreaRepresentation::GetAreaSizeArrayName()
{
  return this->GetAreaSizeArrayNameInternal();
}

void svtkRenderedTreeAreaRepresentation::SetAreaSizeArrayName(const char* name)
{
  this->AreaLayout->SetSizeArrayName(name);
  this->SetAreaSizeArrayNameInternal(name);
}

const char* svtkRenderedTreeAreaRepresentation::GetAreaLabelArrayName()
{
  return this->AreaLabelHierarchy->GetLabelArrayName();
}

void svtkRenderedTreeAreaRepresentation::SetAreaLabelArrayName(const char* name)
{
  this->AreaLabelHierarchy->SetInputArrayToProcess(2, 0, 0, svtkDataObject::VERTEX, name);
}

svtkTextProperty* svtkRenderedTreeAreaRepresentation::GetAreaLabelTextProperty()
{
  return this->AreaLabelHierarchy->GetTextProperty();
}

void svtkRenderedTreeAreaRepresentation::SetAreaLabelTextProperty(svtkTextProperty* prop)
{
  this->AreaLabelHierarchy->SetTextProperty(prop);
}

const char* svtkRenderedTreeAreaRepresentation::GetAreaColorArrayName()
{
  return this->GetAreaColorArrayNameInternal();
}

void svtkRenderedTreeAreaRepresentation::SetAreaColorArrayName(const char* name)
{
  this->ApplyColors->SetInputArrayToProcess(
    0, 0, 0, svtkDataObject::FIELD_ASSOCIATION_VERTICES, name);
  this->SetAreaColorArrayNameInternal(name);
}

bool svtkRenderedTreeAreaRepresentation::ValidIndex(int idx)
{
  return (idx >= 0 && idx < static_cast<int>(this->Implementation->Graphs.size()));
}

const char* svtkRenderedTreeAreaRepresentation::GetGraphEdgeColorArrayName(int idx)
{
  if (this->ValidIndex(idx))
  {
    return this->Implementation->Graphs[idx]->GetColorArrayName();
  }
  return nullptr;
}

void svtkRenderedTreeAreaRepresentation::SetGraphEdgeColorArrayName(const char* name, int idx)
{
  if (this->ValidIndex(idx))
  {
    this->Implementation->Graphs[idx]->SetColorArrayName(name);
    this->EdgeScalarBar->GetScalarBarActor()->SetTitle(name);
  }
}

void svtkRenderedTreeAreaRepresentation::SetGraphEdgeColorToSplineFraction(int idx)
{
  this->SetGraphEdgeColorArrayName("fraction", idx);
}

void svtkRenderedTreeAreaRepresentation::SetAreaLabelPriorityArrayName(const char* name)
{
  this->AreaLabelHierarchy->SetInputArrayToProcess(0, 0, 0, svtkDataObject::VERTEX, name);
}

const char* svtkRenderedTreeAreaRepresentation::GetAreaLabelPriorityArrayName()
{
  return this->AreaLabelHierarchy->GetPriorityArrayName();
}

void svtkRenderedTreeAreaRepresentation::SetGraphBundlingStrength(double strength, int idx)
{
  if (this->ValidIndex(idx))
  {
    this->Implementation->Graphs[idx]->SetBundlingStrength(strength);
  }
}

double svtkRenderedTreeAreaRepresentation::GetGraphBundlingStrength(int idx)
{
  if (this->ValidIndex(idx))
  {
    return this->Implementation->Graphs[idx]->GetBundlingStrength();
  }
  return 0.0;
}

void svtkRenderedTreeAreaRepresentation::SetGraphSplineType(int type, int idx)
{
  if (this->ValidIndex(idx))
  {
    this->Implementation->Graphs[idx]->SetSplineType(type);
  }
}

int svtkRenderedTreeAreaRepresentation::GetGraphSplineType(int idx)
{
  if (this->ValidIndex(idx))
  {
    return this->Implementation->Graphs[idx]->GetSplineType();
  }
  return 0;
}

void svtkRenderedTreeAreaRepresentation::SetEdgeScalarBarVisibility(bool b)
{
  this->EdgeScalarBar->GetScalarBarActor()->SetVisibility(b);
}

bool svtkRenderedTreeAreaRepresentation::GetEdgeScalarBarVisibility()
{
  return this->EdgeScalarBar->GetScalarBarActor()->GetVisibility() ? true : false;
}

void svtkRenderedTreeAreaRepresentation::SetGraphHoverArrayName(const char* name, int idx)
{
  if (this->ValidIndex(idx))
  {
    this->Implementation->Graphs[idx]->SetHoverArrayName(name);
  }
}

const char* svtkRenderedTreeAreaRepresentation::GetGraphHoverArrayName(int idx)
{
  if (this->ValidIndex(idx))
  {
    return this->Implementation->Graphs[idx]->GetHoverArrayName();
  }
  return nullptr;
}

void svtkRenderedTreeAreaRepresentation::SetAreaLabelMapper(svtkLabeledDataMapper* mapper)
{
  // AreaLayout -> AreaLabelMapper -> AreaLabelActor
  if (this->AreaLabelMapper != mapper)
  {
    svtkLabeledDataMapper* oldMapper = this->AreaLabelMapper;
    this->AreaLabelMapper = mapper;
    if (this->AreaLabelMapper)
    {
      this->AreaLabelMapper->Register(this);
      this->AreaLabelMapper->SetLabelModeToLabelFieldData();
      if (oldMapper)
      {
        this->AreaLabelMapper->SetFieldDataName(oldMapper->GetFieldDataName());
        this->SetAreaLabelTextProperty(oldMapper->GetLabelTextProperty());
      }
      this->AreaLabelMapper->SetInputConnection(this->AreaLayout->GetOutputPort());
      this->AreaLabelActor->SetMapper(this->AreaLabelMapper);
    }
    if (oldMapper)
    {
      oldMapper->Delete();
    }
  }
}

void svtkRenderedTreeAreaRepresentation::SetAreaToPolyData(svtkPolyDataAlgorithm* alg)
{
  // AreaLayout -> ApplyColors -> AreaToPolyData -> AreaMapper -> AreaActor
  if (this->AreaToPolyData != alg)
  {
    svtkPolyDataAlgorithm* oldAlg = this->AreaToPolyData;
    this->AreaToPolyData = alg;
    if (this->AreaToPolyData)
    {
      this->AreaToPolyData->Register(this);
      this->AreaToPolyData->SetInputConnection(this->ApplyColors->GetOutputPort());
      this->AreaMapper->SetInputConnection(this->AreaToPolyData->GetOutputPort());
    }
    if (oldAlg)
    {
      oldAlg->Delete();
    }
  }
}

svtkUnicodeString svtkRenderedTreeAreaRepresentation::GetHoverTextInternal(svtkSelection* sel)
{
  svtkGraph* input = svtkGraph::SafeDownCast(this->GetInput());
  svtkSmartPointer<svtkIdTypeArray> selectedItems = svtkSmartPointer<svtkIdTypeArray>::New();
  svtkConvertSelection::GetSelectedVertices(sel, input, selectedItems);
  svtkDataSetAttributes* data = input->GetVertexData();
  const char* hoverArrName = this->GetAreaHoverArrayName();
  if (selectedItems->GetNumberOfTuples() == 0)
  {
    for (int i = 0; i < this->GetNumberOfInputConnections(i); ++i)
    {
      svtkGraph* g = svtkGraph::SafeDownCast(this->GetInputDataObject(1, i));
      svtkConvertSelection::GetSelectedEdges(sel, g, selectedItems);
      if (selectedItems->GetNumberOfTuples() > 0)
      {
        hoverArrName = this->GetGraphHoverArrayName(i);
        data = g->GetEdgeData();
        break;
      }
    }
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

void svtkRenderedTreeAreaRepresentation::UpdateHoverHighlight(svtkView* view, int x, int y)
{
  // Make sure we have a context.
  svtkRenderer* r = svtkRenderView::SafeDownCast(view)->GetRenderer();
  svtkRenderWindow* win = r->GetRenderWindow();
  if (!win)
  {
    return;
  }
  win->MakeCurrent();
  if (!win->IsCurrent())
  {
    return;
  }

  // Use the hardware picker to find a point in world coordinates.
  this->Picker->Pick(x, y, 0, r);
  double pos[3];
  this->Picker->GetPickPosition(pos);
  float posFloat[3] = { static_cast<float>(pos[0]), static_cast<float>(pos[1]),
    static_cast<float>(pos[2]) };
  this->AreaLayout->Update();
  svtkIdType id = this->AreaLayout->FindVertex(posFloat);
  if (id >= 0)
  {
    float sinfo[4] = { 0.0, 1.0, 0.0, 1.0 };
    double z = 0.02;
    this->AreaLayout->GetBoundingArea(id, sinfo);
    if (this->UseRectangularCoordinates)
    {
      svtkSmartPointer<svtkPoints> highlightPoints = svtkSmartPointer<svtkPoints>::New();
      highlightPoints->SetNumberOfPoints(5);

      svtkSmartPointer<svtkCellArray> highA = svtkSmartPointer<svtkCellArray>::New();
      highA->InsertNextCell(5);
      for (int i = 0; i < 5; ++i)
      {
        highA->InsertCellPoint(i);
      }
      highlightPoints->SetPoint(0, sinfo[0], sinfo[2], z);
      highlightPoints->SetPoint(1, sinfo[1], sinfo[2], z);
      highlightPoints->SetPoint(2, sinfo[1], sinfo[3], z);
      highlightPoints->SetPoint(3, sinfo[0], sinfo[3], z);
      highlightPoints->SetPoint(4, sinfo[0], sinfo[2], z);
      this->HighlightData->SetPoints(highlightPoints);
      this->HighlightData->SetLines(highA);
    }
    else
    {
      if (sinfo[1] - sinfo[0] != 360.)
      {
        svtkSmartPointer<svtkSectorSource> sector = svtkSmartPointer<svtkSectorSource>::New();
        sector->SetInnerRadius(sinfo[2]);
        sector->SetOuterRadius(sinfo[3]);
        sector->SetZCoord(z);
        sector->SetStartAngle(sinfo[0]);
        sector->SetEndAngle(sinfo[1]);

        int resolution = (int)((sinfo[1] - sinfo[0]) / 1);
        if (resolution < 1)
          resolution = 1;
        sector->SetCircumferentialResolution(resolution);
        sector->Update();

        svtkSmartPointer<svtkExtractEdges> extract = svtkSmartPointer<svtkExtractEdges>::New();
        extract->SetInputConnection(sector->GetOutputPort());

        svtkSmartPointer<svtkAppendPolyData> append = svtkSmartPointer<svtkAppendPolyData>::New();
        append->AddInputConnection(extract->GetOutputPort());
        append->Update();

        this->HighlightData->ShallowCopy(append->GetOutput());
      }
      else
      {
        svtkSmartPointer<svtkPoints> highlightPoints = svtkSmartPointer<svtkPoints>::New();
        highlightPoints->SetNumberOfPoints(240);

        double conversion = svtkMath::Pi() / 180.;
        double current_angle = 0.;

        svtkSmartPointer<svtkCellArray> highA = svtkSmartPointer<svtkCellArray>::New();
        for (int i = 0; i < 120; ++i)
        {
          highA->InsertNextCell(2);
          double current_x = sinfo[2] * cos(conversion * current_angle);
          double current_y = sinfo[2] * sin(conversion * current_angle);
          highlightPoints->SetPoint(i, current_x, current_y, z);

          current_angle += 3.;

          highA->InsertCellPoint(i);
          highA->InsertCellPoint((i + 1) % 120);
        }

        current_angle = 0.;
        for (int i = 0; i < 120; ++i)
        {
          highA->InsertNextCell(2);
          double current_x = sinfo[3] * cos(conversion * current_angle);
          double current_y = sinfo[3] * sin(conversion * current_angle);
          highlightPoints->SetPoint(120 + i, current_x, current_y, z);

          current_angle += 3.;

          highA->InsertCellPoint(120 + i);
          highA->InsertCellPoint(120 + ((i + 1) % 120));
        }
        this->HighlightData->SetPoints(highlightPoints);
        this->HighlightData->SetLines(highA);
      }
    }
    this->HighlightActor->VisibilityOn();
  }
  else
  {
    this->HighlightActor->VisibilityOff();
  }
}

double svtkRenderedTreeAreaRepresentation::GetShrinkPercentage()
{
  return this->AreaLayout->GetLayoutStrategy()->GetShrinkPercentage();
}

void svtkRenderedTreeAreaRepresentation::SetShrinkPercentage(double pcent)
{
  this->AreaLayout->GetLayoutStrategy()->SetShrinkPercentage(pcent);
}

const char* svtkRenderedTreeAreaRepresentation::GetGraphEdgeLabelArrayName(int idx)
{
  if (this->ValidIndex(idx))
  {
    return this->Implementation->Graphs[idx]->GetLabelArrayName();
  }
  return nullptr;
}

void svtkRenderedTreeAreaRepresentation::SetGraphEdgeLabelArrayName(const char* name, int idx)
{
  if (this->ValidIndex(idx))
  {
    this->Implementation->Graphs[idx]->SetLabelArrayName(name);
  }
}

svtkTextProperty* svtkRenderedTreeAreaRepresentation::GetGraphEdgeLabelTextProperty(int idx)
{
  if (this->ValidIndex(idx))
  {
    return this->Implementation->Graphs[idx]->GetLabelTextProperty();
  }
  return nullptr;
}

void svtkRenderedTreeAreaRepresentation::SetGraphEdgeLabelTextProperty(
  svtkTextProperty* prop, int idx)
{
  if (this->ValidIndex(idx))
  {
    this->Implementation->Graphs[idx]->SetLabelTextProperty(prop);
  }
}

svtkAreaLayoutStrategy* svtkRenderedTreeAreaRepresentation::GetAreaLayoutStrategy()
{
  return this->AreaLayout->GetLayoutStrategy();
}

void svtkRenderedTreeAreaRepresentation::SetAreaLayoutStrategy(svtkAreaLayoutStrategy* s)
{
  this->AreaLayout->SetLayoutStrategy(s);
}

bool svtkRenderedTreeAreaRepresentation::GetAreaLabelVisibility()
{
  return this->AreaLabelHierarchy->GetInputConnection(0, 0) == this->AreaLayout->GetOutputPort();
}

void svtkRenderedTreeAreaRepresentation::SetAreaLabelVisibility(bool b)
{
  if (b)
  {
    this->AreaLabelHierarchy->SetInputConnection(this->AreaLayout->GetOutputPort());
  }
  else
  {
    this->AreaLabelHierarchy->SetInputData(this->EmptyPolyData);
  }
}

bool svtkRenderedTreeAreaRepresentation::GetGraphEdgeLabelVisibility(int idx)
{
  if (this->ValidIndex(idx))
  {
    return this->Implementation->Graphs[idx]->GetLabelVisibility();
  }
  return false;
}

void svtkRenderedTreeAreaRepresentation::SetGraphEdgeLabelVisibility(bool b, int idx)
{
  if (this->ValidIndex(idx))
  {
    this->Implementation->Graphs[idx]->SetLabelVisibility(b);
  }
}

bool svtkRenderedTreeAreaRepresentation::GetColorGraphEdgesByArray(int idx)
{
  if (this->ValidIndex(idx))
  {
    return this->Implementation->Graphs[idx]->GetColorEdgesByArray();
  }
  return false;
}

void svtkRenderedTreeAreaRepresentation::SetColorGraphEdgesByArray(bool b, int idx)
{
  if (this->ValidIndex(idx))
  {
    this->Implementation->Graphs[idx]->SetColorEdgesByArray(b);
  }
}

bool svtkRenderedTreeAreaRepresentation::GetColorAreasByArray()
{
  return this->ApplyColors->GetUsePointLookupTable();
}

void svtkRenderedTreeAreaRepresentation::SetColorAreasByArray(bool b)
{
  this->ApplyColors->SetUsePointLookupTable(b);
}

void svtkRenderedTreeAreaRepresentation::SetLabelRenderMode(int mode)
{
  if (mode != this->GetLabelRenderMode())
  {
    this->Superclass::SetLabelRenderMode(mode);
    if (mode == svtkRenderView::FREETYPE)
    {
      this->AreaLabelActor = svtkSmartPointer<svtkActor2D>::New();
      this->AreaLabelActor->PickableOff();

      svtkSmartPointer<svtkDynamic2DLabelMapper> mapper =
        svtkSmartPointer<svtkDynamic2DLabelMapper>::New();
      this->SetAreaLabelMapper(mapper);
    }
    else if (mode == svtkRenderView::QT)
    {
#ifdef SVTK_USE_QT
      this->AreaLabelActor = svtkSmartPointer<svtkTexturedActor2D>::New();
      this->AreaLabelActor->PickableOff();

      svtkSmartPointer<svtkQtTreeRingLabelMapper> mapper =
        svtkSmartPointer<svtkQtTreeRingLabelMapper>::New();
      this->SetAreaLabelMapper(mapper);
#else
      svtkErrorMacro("Qt label rendering not supported.");
#endif
    }
    else
    {
      svtkErrorMacro("Unknown label render mode.");
    }
  }
}

bool svtkRenderedTreeAreaRepresentation::AddToView(svtkView* view)
{
  this->Superclass::AddToView(view);
  svtkRenderView* rv = svtkRenderView::SafeDownCast(view);
  if (rv)
  {
    this->EdgeScalarBar->SetInteractor(rv->GetInteractor());
    rv->GetRenderer()->AddActor(this->AreaActor);
    // rv->GetRenderer()->AddActor(this->AreaLabelActor);
    rv->GetRenderer()->AddActor(this->HighlightActor);
    rv->GetRenderer()->AddActor(this->EdgeScalarBar->GetScalarBarActor());
    rv->AddLabels(this->AreaLabelHierarchy->GetOutputPort());

#if 0
    // Debug code: display underlying tree
    svtkSmartPointer<svtkGraphMapper> mapper = svtkSmartPointer<svtkGraphMapper>::New();
    svtkSmartPointer<svtkActor> actor = svtkSmartPointer<svtkActor>::New();
    mapper->SetInputConnection(this->AreaLayout->GetOutputPort(1));
    actor->SetMapper(mapper);
    rv->GetRenderer()->AddActor(actor);
#endif

    rv->RegisterProgress(this->TreeAggregation);
    rv->RegisterProgress(this->VertexDegree);
    rv->RegisterProgress(this->AreaLayout);
    rv->RegisterProgress(this->AreaToPolyData);
    return true;
  }
  return false;
}

bool svtkRenderedTreeAreaRepresentation::RemoveFromView(svtkView* view)
{
  this->Superclass::RemoveFromView(view);
  svtkRenderView* rv = svtkRenderView::SafeDownCast(view);
  if (rv)
  {
    rv->GetRenderer()->RemoveActor(this->AreaActor);
    rv->GetRenderer()->RemoveActor(this->AreaLabelActor);
    rv->GetRenderer()->RemoveActor(this->HighlightActor);
    rv->GetRenderer()->RemoveActor(this->EdgeScalarBar->GetScalarBarActor());
    rv->UnRegisterProgress(this->TreeAggregation);
    rv->UnRegisterProgress(this->VertexDegree);
    rv->UnRegisterProgress(this->AreaLayout);
    rv->UnRegisterProgress(this->AreaToPolyData);
    return true;
  }
  return false;
}

svtkSelection* svtkRenderedTreeAreaRepresentation::ConvertSelection(svtkView* view, svtkSelection* sel)
{
  svtkSelection* converted = svtkSelection::New();

  // TODO: Somehow to figure out single select mode.
  unsigned int rect[4];
  rect[0] = 0;
  rect[1] = 0;
  rect[2] = 0;
  rect[3] = 0;
  bool singleSelectMode = false;
  if (rect[0] == rect[2] && rect[1] == rect[3])
  {
    singleSelectMode = true;
  }

  for (unsigned int i = 0; i < sel->GetNumberOfNodes(); ++i)
  {
    svtkSelectionNode* node = sel->GetNode(i);
    svtkProp* prop = svtkProp::SafeDownCast(node->GetProperties()->Get(svtkSelectionNode::PROP()));
    if (prop == this->AreaActor.GetPointer())
    {
      svtkSmartPointer<svtkIdTypeArray> vertexIds;
      vertexIds = svtkArrayDownCast<svtkIdTypeArray>(node->GetSelectionList());

      // If we are in single select mode, make sure to select only the vertex
      // that is being hovered over.
      svtkRenderView* rv = svtkRenderView::SafeDownCast(view);
      if (rv && singleSelectMode)
      {
        svtkInteractorStyleAreaSelectHover* style =
          svtkInteractorStyleAreaSelectHover::SafeDownCast(rv->GetInteractorStyle());
        if (style)
        {
          svtkIdType v = style->GetIdAtPos(rect[0], rect[1]);
          vertexIds = svtkSmartPointer<svtkIdTypeArray>::New();
          if (v >= 0)
          {
            vertexIds->InsertNextValue(v);
          }
        }
      }

      // Create a vertex selection.
      svtkSmartPointer<svtkSelection> vertexIndexSelection = svtkSmartPointer<svtkSelection>::New();
      svtkSmartPointer<svtkSelectionNode> vertexIndexNode = svtkSmartPointer<svtkSelectionNode>::New();
      vertexIndexNode->SetContentType(svtkSelectionNode::INDICES);
      vertexIndexNode->SetFieldType(svtkSelectionNode::CELL);
      vertexIndexNode->SetSelectionList(vertexIds);
      vertexIndexSelection->AddNode(vertexIndexNode);

      // Convert to pedigree ids.
      // Make it a vertex selection.
      this->AreaToPolyData->Update();
      svtkSmartPointer<svtkSelection> vertexSelection;
      vertexSelection.TakeReference(svtkConvertSelection::ToSelectionType(
        vertexIndexSelection, this->AreaToPolyData->GetOutput(), svtkSelectionNode::PEDIGREEIDS));
      svtkSelectionNode* vnode = vertexSelection->GetNode(0);
      if (vnode && vnode->GetSelectionList()->GetNumberOfTuples() > 0)
      {
        vnode->SetFieldType(svtkSelectionNode::VERTEX);
        converted->AddNode(vnode);

        // Find matching vertex pedigree ids in all input graphs
        // and add outgoing edges to selection

        svtkAbstractArray* arr = vnode->GetSelectionList();
        size_t numGraphs = static_cast<size_t>(this->GetNumberOfInputConnections(1));
        svtkSmartPointer<svtkOutEdgeIterator> iter = svtkSmartPointer<svtkOutEdgeIterator>::New();
        for (size_t k = 0; k < numGraphs; ++k)
        {
          svtkSmartPointer<svtkSelection> edgeIndexSelection = svtkSmartPointer<svtkSelection>::New();
          svtkSmartPointer<svtkSelectionNode> edgeIndexNode =
            svtkSmartPointer<svtkSelectionNode>::New();
          edgeIndexNode->SetContentType(svtkSelectionNode::INDICES);
          edgeIndexNode->SetFieldType(svtkSelectionNode::EDGE);
          svtkSmartPointer<svtkIdTypeArray> edgeIds = svtkSmartPointer<svtkIdTypeArray>::New();
          edgeIndexNode->SetSelectionList(edgeIds);
          edgeIndexSelection->AddNode(edgeIndexNode);

          svtkGraph* g = svtkGraph::SafeDownCast(this->GetInternalOutputPort(1, static_cast<int>(k))
                                                 ->GetProducer()
                                                 ->GetOutputDataObject(0));
          svtkAbstractArray* arr2 = g->GetVertexData()->GetPedigreeIds();
          svtkStringArray* domainArr =
            svtkArrayDownCast<svtkStringArray>(g->GetVertexData()->GetAbstractArray("domain"));
          for (svtkIdType j = 0; j < arr->GetNumberOfTuples(); ++j)
          {
            svtkIdType id = arr2->LookupValue(arr->GetVariantValue(j));
            if (id == -1)
            {
              continue;
            }

            // Before adding vertex's edges, make sure its in the same domain as selected vertex
            svtkStdString domain;
            if (domainArr)
            {
              domain = domainArr->GetValue(id);
            }
            else
            {
              domain = arr2->GetName();
            }
            if (domain != arr->GetName())
            {
              continue;
            }

            g->GetOutEdges(id, iter);
            while (iter->HasNext())
            {
              edgeIds->InsertNextValue(iter->Next().Id);
            }
          }

          svtkSmartPointer<svtkSelection> edgeSelection;
          edgeSelection.TakeReference(svtkConvertSelection::ToSelectionType(
            edgeIndexSelection, g, svtkSelectionNode::PEDIGREEIDS));
          converted->AddNode(edgeSelection->GetNode(0));
        }
      }
    }
  }
  // Graph edge selections.
  for (size_t i = 0; i < this->Implementation->Graphs.size(); ++i)
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

int svtkRenderedTreeAreaRepresentation::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** svtkNotUsed(inputVector), svtkInformationVector* svtkNotUsed(outputVector))
{
  // Tree area connections
  this->TreeLevels->SetInputConnection(this->GetInternalOutputPort());
  this->ApplyColors->SetInputConnection(1, this->GetInternalAnnotationOutputPort());

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
    this->RemovePropOnNextRender(this->Implementation->Graphs[i]->GetLabelActor());
  }
  this->Implementation->Graphs.resize(numGraphs);

  // Make sure all hierarchical graph edges inputs are up to date.
  for (size_t i = 0; i < numGraphs; ++i)
  {
    this->AddPropOnNextRender(this->Implementation->Graphs[i]->GetActor());
    this->AddPropOnNextRender(this->Implementation->Graphs[i]->GetLabelActor());
    svtkHierarchicalGraphPipeline* p = this->Implementation->Graphs[i];
    p->PrepareInputConnections(this->GetInternalOutputPort(1, static_cast<int>(i)),
      this->AreaLayout->GetOutputPort(1),
      this->GetInternalAnnotationOutputPort(1, static_cast<int>(i)));
  }
  return 1;
}

void svtkRenderedTreeAreaRepresentation::PrepareForRendering(svtkRenderView* view)
{
#if 0
  // Make hover highlight up to date
  int pos[2] = {0, 0};
  if (view->GetInteractorStyle() && view->GetInteractorStyle()->GetInteractor())
  {
    view->GetInteractorStyle()->GetInteractor()->GetEventPosition(pos);
    this->UpdateHoverHighlight(view, pos[0], pos[1]);
  }
#ifdef SVTK_USE_QT
  svtkQtTreeRingLabelMapper* mapper =
    svtkQtTreeRingLabelMapper::SafeDownCast(this->AreaLabelMapper);
  if (mapper)
  {
    mapper->SetRenderer(view->GetRenderer());
  }
  //view->GetRenderer()->AddActor(this->AreaLabelActor);
#endif
#endif

  // Make sure all the graphs are registered.
  for (size_t i = 0; i < this->Implementation->Graphs.size(); ++i)
  {
    this->Implementation->Graphs[i]->RegisterProgress(view);
  }

  this->Superclass::PrepareForRendering(view);
}

void svtkRenderedTreeAreaRepresentation::ApplyViewTheme(svtkViewTheme* theme)
{
  this->Superclass::ApplyViewTheme(theme);

  this->ApplyColors->SetPointLookupTable(theme->GetPointLookupTable());
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

  this->GetAreaLabelTextProperty()->ShallowCopy(theme->GetPointTextProperty());

  // Make sure we have the right number of graphs
  if (this->GetNumberOfInputConnections(1) != static_cast<int>(this->Implementation->Graphs.size()))
  {
    this->Update();
  }

  for (size_t i = 0; i < this->Implementation->Graphs.size(); ++i)
  {
    svtkHierarchicalGraphPipeline* p = this->Implementation->Graphs[i];
    p->ApplyViewTheme(theme);
  }
}

int svtkRenderedTreeAreaRepresentation::FillInputPortInformation(int port, svtkInformation* info)
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

void svtkRenderedTreeAreaRepresentation::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "UseRectangularCoordinates: " << this->UseRectangularCoordinates << endl;
  os << indent
     << "AreaHoverArrayName: " << (this->AreaHoverArrayName ? this->AreaHoverArrayName : "(none)")
     << endl;
  os << indent << "AreaToPolyData: ";
  if (this->AreaToPolyData)
  {
    os << "\n";
    this->AreaToPolyData->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << "(none)\n";
  }
  os << indent << "AreaLabelMapper: ";
  if (this->AreaLabelMapper)
  {
    os << "\n";
    this->AreaLabelMapper->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << "(none)\n";
  }
}
