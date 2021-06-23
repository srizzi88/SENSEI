/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkGraphMapper.cxx

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
#include "svtkGraphMapper.h"

#include "svtkActor.h"
#include "svtkActor2D.h"
#include "svtkCamera.h"
#include "svtkCellArray.h"
#include "svtkCellData.h"
#include "svtkDirectedGraph.h"
#include "svtkExecutive.h"
#include "svtkFollower.h"
#include "svtkGarbageCollector.h"
#include "svtkGlyph3D.h"
#include "svtkGraphToPolyData.h"
#include "svtkIconGlyphFilter.h"
#include "svtkImageData.h"
#include "svtkInformation.h"
#include "svtkLookupTableWithEnabling.h"
#include "svtkMapArrayValues.h"
#include "svtkMath.h"
#include "svtkObjectFactory.h"
#include "svtkPlaneSource.h"
#include "svtkPointData.h"
#include "svtkPolyData.h"
#include "svtkPolyDataMapper.h"
#include "svtkPolyDataMapper2D.h"
#include "svtkProperty.h"
#include "svtkRenderer.h"
#include "svtkTexture.h"
#include "svtkTexturedActor2D.h"
#include "svtkTransformCoordinateSystems.h"
#include "svtkUndirectedGraph.h"
#include "svtkVertexGlyphFilter.h"

svtkStandardNewMacro(svtkGraphMapper);

#define SVTK_CREATE(type, name) svtkSmartPointer<type> name = svtkSmartPointer<type>::New()

//----------------------------------------------------------------------------
svtkGraphMapper::svtkGraphMapper()
{
  this->GraphToPoly = svtkSmartPointer<svtkGraphToPolyData>::New();
  this->VertexGlyph = svtkSmartPointer<svtkVertexGlyphFilter>::New();
  this->IconTypeToIndex = svtkSmartPointer<svtkMapArrayValues>::New();
  this->CircleGlyph = svtkSmartPointer<svtkGlyph3D>::New();
  this->CircleOutlineGlyph = svtkSmartPointer<svtkGlyph3D>::New();
  this->IconGlyph = svtkSmartPointer<svtkIconGlyphFilter>::New();
  this->IconTransform = svtkSmartPointer<svtkTransformCoordinateSystems>::New();
  this->EdgeMapper = svtkSmartPointer<svtkPolyDataMapper>::New();
  this->VertexMapper = svtkSmartPointer<svtkPolyDataMapper>::New();
  this->OutlineMapper = svtkSmartPointer<svtkPolyDataMapper>::New();
  this->IconMapper = svtkSmartPointer<svtkPolyDataMapper2D>::New();
  this->EdgeActor = svtkSmartPointer<svtkActor>::New();
  this->VertexActor = svtkSmartPointer<svtkActor>::New();
  this->OutlineActor = svtkSmartPointer<svtkActor>::New();
  this->IconActor = svtkSmartPointer<svtkTexturedActor2D>::New();
  this->VertexLookupTable = svtkLookupTableWithEnabling::New();
  this->EdgeLookupTable = svtkLookupTableWithEnabling::New();
  this->VertexColorArrayNameInternal = nullptr;
  this->EdgeColorArrayNameInternal = nullptr;
  this->EnabledEdgesArrayName = nullptr;
  this->EnabledVerticesArrayName = nullptr;
  this->VertexPointSize = 5;
  this->EdgeLineWidth = 1;
  this->ScaledGlyphs = false;
  this->ScalingArrayName = nullptr;

  this->VertexMapper->SetScalarModeToUsePointData();
  this->VertexMapper->SetLookupTable(this->VertexLookupTable);
  this->VertexMapper->SetScalarVisibility(false);
  this->VertexActor->PickableOff();
  this->VertexActor->GetProperty()->SetPointSize(this->GetVertexPointSize());
  this->OutlineActor->PickableOff();
  this->OutlineActor->GetProperty()->SetPointSize(this->GetVertexPointSize() + 2);
  this->OutlineActor->SetPosition(0, 0, -0.001);
  this->OutlineActor->GetProperty()->SetRepresentationToWireframe();
  this->OutlineMapper->SetScalarVisibility(false);
  this->EdgeMapper->SetScalarModeToUseCellData();
  this->EdgeMapper->SetLookupTable(this->EdgeLookupTable);
  this->EdgeMapper->SetScalarVisibility(false);
  this->EdgeActor->SetPosition(0, 0, -0.003);
  this->EdgeActor->GetProperty()->SetLineWidth(this->GetEdgeLineWidth());

  this->IconTransform->SetInputCoordinateSystemToWorld();
  this->IconTransform->SetOutputCoordinateSystemToDisplay();
  this->IconTransform->SetInputConnection(this->VertexGlyph->GetOutputPort());

  this->IconTypeToIndex->SetInputConnection(this->IconTransform->GetOutputPort());
  this->IconTypeToIndex->SetFieldType(svtkMapArrayValues::POINT_DATA);
  this->IconTypeToIndex->SetOutputArrayType(SVTK_INT);
  this->IconTypeToIndex->SetPassArray(0);
  this->IconTypeToIndex->SetFillValue(-1);

  this->IconGlyph->SetInputConnection(this->IconTypeToIndex->GetOutputPort());
  this->IconGlyph->SetUseIconSize(true);
  this->IconMapper->SetInputConnection(this->IconGlyph->GetOutputPort());
  this->IconMapper->ScalarVisibilityOff();

  this->IconActor->SetMapper(this->IconMapper);
  this->IconArrayNameInternal = nullptr;

  this->VertexMapper->SetInputConnection(this->VertexGlyph->GetOutputPort());
  this->OutlineMapper->SetInputConnection(this->VertexGlyph->GetOutputPort());

  this->VertexActor->SetMapper(this->VertexMapper);
  this->OutlineActor->SetMapper(this->OutlineMapper);
  this->EdgeMapper->SetInputConnection(this->GraphToPoly->GetOutputPort());
  this->EdgeActor->SetMapper(this->EdgeMapper);

  // Set default parameters
  this->SetVertexColorArrayName("VertexDegree");
  this->ColorVerticesOff();
  this->SetEdgeColorArrayName("weight");
  this->ColorEdgesOff();
  this->SetEnabledEdgesArrayName("weight");
  this->SetEnabledVerticesArrayName("VertexDegree");
  this->EnableEdgesByArray = 0;
  this->EnableVerticesByArray = 0;
  this->IconVisibilityOff();
}

//----------------------------------------------------------------------------
svtkGraphMapper::~svtkGraphMapper()
{
  // Delete internally created objects.
  // Note: All of the smartpointer objects
  //       will be deleted for us

  this->SetVertexColorArrayNameInternal(nullptr);
  this->SetEdgeColorArrayNameInternal(nullptr);
  this->SetEnabledEdgesArrayName(nullptr);
  this->SetEnabledVerticesArrayName(nullptr);
  this->SetIconArrayNameInternal(nullptr);
  this->VertexLookupTable->Delete();
  this->VertexLookupTable = nullptr;
  this->EdgeLookupTable->Delete();
  this->EdgeLookupTable = nullptr;
  delete[] this->ScalingArrayName;
}

//----------------------------------------------------------------------------
void svtkGraphMapper::SetIconArrayName(const char* name)
{
  this->SetIconArrayNameInternal(name);
  this->IconGlyph->SetInputArrayToProcess(0, 0, 0, svtkDataObject::FIELD_ASSOCIATION_POINTS, name);
  this->IconTypeToIndex->SetInputArrayName(name);
}

//----------------------------------------------------------------------------
const char* svtkGraphMapper::GetIconArrayName()
{
  return this->GetIconArrayNameInternal();
}

//----------------------------------------------------------------------------
void svtkGraphMapper::SetScaledGlyphs(bool arg)
{
  if (arg)
  {
    if (this->ScalingArrayName)
    {
      svtkPolyData* circle = this->CreateCircle(true);
      this->CircleGlyph->SetSourceData(circle);
      circle->Delete();
      this->CircleGlyph->SetInputConnection(this->VertexGlyph->GetOutputPort());
      this->CircleGlyph->SetScaling(1);
      // this->CircleGlyph->SetScaleFactor(.1); // Total hack
      this->CircleGlyph->SetInputArrayToProcess(
        0, 0, 0, svtkDataObject::FIELD_ASSOCIATION_POINTS, this->ScalingArrayName);
      this->VertexMapper->SetInputConnection(this->CircleGlyph->GetOutputPort());

      svtkPolyData* outline = this->CreateCircle(false);
      this->CircleOutlineGlyph->SetSourceData(outline);
      outline->Delete();
      this->CircleOutlineGlyph->SetInputConnection(this->VertexGlyph->GetOutputPort());
      this->CircleOutlineGlyph->SetScaling(1);
      // this->CircleOutlineGlyph->SetScaleFactor(.1); // Total hack
      this->CircleOutlineGlyph->SetInputArrayToProcess(
        0, 0, 0, svtkDataObject::FIELD_ASSOCIATION_POINTS, this->ScalingArrayName);
      this->OutlineMapper->SetInputConnection(this->CircleOutlineGlyph->GetOutputPort());
      this->OutlineActor->SetPosition(0, 0, 0.001);
      this->OutlineActor->GetProperty()->SetLineWidth(2);
    }
    else
    {
      svtkErrorMacro("No scaling array name set");
    }
  }
  else
  {
    this->VertexMapper->SetInputConnection(this->VertexGlyph->GetOutputPort());
    this->OutlineActor->SetPosition(0, 0, -0.001);
    this->OutlineMapper->SetInputConnection(this->VertexGlyph->GetOutputPort());
  }
}

//----------------------------------------------------------------------------
// Helper method
svtkPolyData* svtkGraphMapper::CreateCircle(bool filled)
{
  int circleRes = 16;
  svtkIdType ptIds[17];
  double x[3], theta;

  // Allocate storage
  svtkPolyData* poly = svtkPolyData::New();
  SVTK_CREATE(svtkPoints, pts);
  SVTK_CREATE(svtkCellArray, circle);
  SVTK_CREATE(svtkCellArray, outline);

  // generate points around the circle
  x[2] = 0.0;
  theta = 2.0 * svtkMath::Pi() / circleRes;
  for (int i = 0; i < circleRes; i++)
  {
    x[0] = 0.5 * cos(i * theta);
    x[1] = 0.5 * sin(i * theta);
    ptIds[i] = pts->InsertNextPoint(x);
  }
  circle->InsertNextCell(circleRes, ptIds);

  // Outline
  ptIds[circleRes] = ptIds[0];
  outline->InsertNextCell(circleRes + 1, ptIds);

  // Set up polydata
  poly->SetPoints(pts);
  if (filled)
  {
    poly->SetPolys(circle);
  }
  else
  {
    poly->SetLines(outline);
  }

  return poly;
}

//----------------------------------------------------------------------------
void svtkGraphMapper::SetVertexColorArrayName(const char* name)
{
  this->SetVertexColorArrayNameInternal(name);
  this->VertexMapper->SetScalarModeToUsePointFieldData();
  this->VertexMapper->SelectColorArray(name);
}

//----------------------------------------------------------------------------
const char* svtkGraphMapper::GetVertexColorArrayName()
{
  return this->GetVertexColorArrayNameInternal();
}

//----------------------------------------------------------------------------
void svtkGraphMapper::SetColorVertices(bool vis)
{
  this->VertexMapper->SetScalarVisibility(vis);
}

//----------------------------------------------------------------------------
bool svtkGraphMapper::GetColorVertices()
{
  return this->VertexMapper->GetScalarVisibility() ? true : false;
}

//----------------------------------------------------------------------------
void svtkGraphMapper::ColorVerticesOn()
{
  this->VertexMapper->SetScalarVisibility(true);
}

//----------------------------------------------------------------------------
void svtkGraphMapper::ColorVerticesOff()
{
  this->VertexMapper->SetScalarVisibility(false);
}

//----------------------------------------------------------------------------
void svtkGraphMapper::SetIconVisibility(bool vis)
{
  this->IconActor->SetVisibility(vis);
}

//----------------------------------------------------------------------------
bool svtkGraphMapper::GetIconVisibility()
{
  return this->IconActor->GetVisibility() ? true : false;
}

//----------------------------------------------------------------------------
void svtkGraphMapper::SetEdgeColorArrayName(const char* name)
{
  this->SetEdgeColorArrayNameInternal(name);
  this->EdgeMapper->SetScalarModeToUseCellFieldData();
  this->EdgeMapper->SelectColorArray(name);
}

//----------------------------------------------------------------------------
const char* svtkGraphMapper::GetEdgeColorArrayName()
{
  return this->GetEdgeColorArrayNameInternal();
}

//----------------------------------------------------------------------------
void svtkGraphMapper::SetColorEdges(bool vis)
{
  this->EdgeMapper->SetScalarVisibility(vis);
}

//----------------------------------------------------------------------------
bool svtkGraphMapper::GetColorEdges()
{
  return this->EdgeMapper->GetScalarVisibility() ? true : false;
}

//----------------------------------------------------------------------------
void svtkGraphMapper::ColorEdgesOn()
{
  this->EdgeMapper->SetScalarVisibility(true);
}

//----------------------------------------------------------------------------
void svtkGraphMapper::ColorEdgesOff()
{
  this->EdgeMapper->SetScalarVisibility(false);
}

//----------------------------------------------------------------------------
void svtkGraphMapper::SetVertexPointSize(float size)
{
  this->VertexPointSize = size;
  this->VertexActor->GetProperty()->SetPointSize(this->GetVertexPointSize());
  this->OutlineActor->GetProperty()->SetPointSize(this->GetVertexPointSize() + 2);
}

//----------------------------------------------------------------------------
void svtkGraphMapper::SetEdgeLineWidth(float width)
{
  this->EdgeLineWidth = width;
  this->EdgeActor->GetProperty()->SetLineWidth(this->GetEdgeLineWidth());
}

//----------------------------------------------------------------------------
void svtkGraphMapper::AddIconType(const char* type, int index)
{
  this->IconTypeToIndex->AddToMap(type, index);
}

//----------------------------------------------------------------------------
void svtkGraphMapper::ClearIconTypes()
{
  this->IconTypeToIndex->ClearMap();
}

//----------------------------------------------------------------------------
void svtkGraphMapper::SetEdgeVisibility(bool vis)
{
  this->EdgeActor->SetVisibility(vis);
}

//----------------------------------------------------------------------------
bool svtkGraphMapper::GetEdgeVisibility()
{
  return this->EdgeActor->GetVisibility() ? true : false;
}

//----------------------------------------------------------------------------
void svtkGraphMapper::SetIconSize(int* size)
{
  this->IconGlyph->SetIconSize(size);
}

//----------------------------------------------------------------------------
void svtkGraphMapper::SetIconAlignment(int alignment)
{
  this->IconGlyph->SetGravity(alignment);
}

//----------------------------------------------------------------------------
int* svtkGraphMapper::GetIconSize()
{
  return this->IconGlyph->GetIconSize();
}

//----------------------------------------------------------------------------
void svtkGraphMapper::SetIconTexture(svtkTexture* texture)
{
  this->IconActor->SetTexture(texture);
}

//----------------------------------------------------------------------------
svtkTexture* svtkGraphMapper::GetIconTexture()
{
  return this->IconActor->GetTexture();
}

//----------------------------------------------------------------------------
void svtkGraphMapper::SetInputData(svtkGraph* input)
{
  this->SetInputDataInternal(0, input);
}

//----------------------------------------------------------------------------
svtkGraph* svtkGraphMapper::GetInput()
{
  svtkGraph* inputGraph = svtkGraph::SafeDownCast(this->Superclass::GetInputAsDataSet());
  return inputGraph;
}

//----------------------------------------------------------------------------
void svtkGraphMapper::ReleaseGraphicsResources(svtkWindow* renWin)
{
  if (this->EdgeActor)
  {
    this->EdgeActor->ReleaseGraphicsResources(renWin);
  }
  if (this->VertexActor)
  {
    this->VertexActor->ReleaseGraphicsResources(renWin);
  }
  if (this->OutlineActor)
  {
    this->OutlineActor->ReleaseGraphicsResources(renWin);
  }
  if (this->IconActor)
  {
    this->IconActor->ReleaseGraphicsResources(renWin);
  }
}

//----------------------------------------------------------------------------
// Receives from Actor -> maps data to primitives
//
void svtkGraphMapper::Render(svtkRenderer* ren, svtkActor* svtkNotUsed(act))
{
  // make sure that we've been properly initialized
  if (!this->GetExecutive()->GetInputData(0, 0))
  {
    svtkErrorMacro(<< "No input!\n");
    return;
  }

  // Update the pipeline up until the graph to poly data
  svtkGraph* input = svtkGraph::SafeDownCast(this->GetExecutive()->GetInputData(0, 0));
  if (!input)
  {
    svtkErrorMacro(<< "Input is not a graph!\n");
    return;
  }
  svtkGraph* graph = nullptr;
  if (svtkDirectedGraph::SafeDownCast(input))
  {
    graph = svtkDirectedGraph::New();
  }
  else
  {
    graph = svtkUndirectedGraph::New();
  }
  graph->ShallowCopy(input);

  this->GraphToPoly->SetInputData(graph);
  this->VertexGlyph->SetInputData(graph);
  graph->Delete();
  this->GraphToPoly->Update();
  this->VertexGlyph->Update();
  svtkPolyData* edgePd = this->GraphToPoly->GetOutput();
  svtkPolyData* vertPd = this->VertexGlyph->GetOutput();

  // Try to find the range the user-specified color array.
  // If we cannot find that array, use the scalar range.
  double range[2];
  svtkDataArray* arr = nullptr;
  if (this->GetColorEdges())
  {
    if (this->GetEdgeColorArrayName())
    {
      arr = edgePd->GetCellData()->GetArray(this->GetEdgeColorArrayName());
    }
    if (!arr)
    {
      arr = edgePd->GetCellData()->GetScalars();
    }
    if (arr)
    {
      arr->GetRange(range);
      this->EdgeMapper->SetScalarRange(range[0], range[1]);
    }
  }

  arr = nullptr;
  if (this->EnableEdgesByArray && this->EnabledEdgesArrayName)
  {
    svtkLookupTableWithEnabling::SafeDownCast(this->EdgeLookupTable)
      ->SetEnabledArray(edgePd->GetCellData()->GetArray(this->GetEnabledEdgesArrayName()));
  }
  else
  {
    svtkLookupTableWithEnabling::SafeDownCast(this->EdgeLookupTable)->SetEnabledArray(nullptr);
  }

  // Do the same thing for the vertex array.
  arr = nullptr;
  if (this->GetColorVertices())
  {
    if (this->GetVertexColorArrayName())
    {
      arr = vertPd->GetPointData()->GetArray(this->GetVertexColorArrayName());
    }
    if (!arr)
    {
      arr = vertPd->GetPointData()->GetScalars();
    }
    if (arr)
    {
      arr->GetRange(range);
      this->VertexMapper->SetScalarRange(range[0], range[1]);
    }
  }

  arr = nullptr;
  if (this->EnableVerticesByArray && this->EnabledVerticesArrayName)
  {
    svtkLookupTableWithEnabling::SafeDownCast(this->VertexLookupTable)
      ->SetEnabledArray(vertPd->GetPointData()->GetArray(this->GetEnabledVerticesArrayName()));
  }
  else
  {
    svtkLookupTableWithEnabling::SafeDownCast(this->VertexLookupTable)->SetEnabledArray(nullptr);
  }

  if (this->IconActor->GetTexture() && this->IconActor->GetTexture()->GetInput() &&
    this->IconActor->GetVisibility())
  {
    this->IconTransform->SetViewport(ren);
    this->IconActor->GetTexture()->SetColorMode(SVTK_COLOR_MODE_DEFAULT);
    this->IconActor->GetTexture()->GetInputAlgorithm()->Update();
    int* dim = this->IconActor->GetTexture()->GetInput()->GetDimensions();
    this->IconGlyph->SetIconSheetSize(dim);
    // Override the array for svtkIconGlyphFilter to process if we have
    // a map of icon types.
    if (this->IconTypeToIndex->GetMapSize())
    {
      this->IconGlyph->SetInputArrayToProcess(0, 0, 0, svtkDataObject::FIELD_ASSOCIATION_POINTS,
        this->IconTypeToIndex->GetOutputArrayName());
    }
  }
  if (this->EdgeActor->GetVisibility())
  {
    this->EdgeActor->RenderOpaqueGeometry(ren);
  }
  if (this->OutlineActor->GetVisibility())
  {
    this->OutlineActor->RenderOpaqueGeometry(ren);
  }
  this->VertexActor->RenderOpaqueGeometry(ren);
  if (this->IconActor->GetVisibility())
  {
    this->IconActor->RenderOpaqueGeometry(ren);
  }

  if (this->EdgeActor->GetVisibility())
  {
    this->EdgeActor->RenderTranslucentPolygonalGeometry(ren);
  }
  this->VertexActor->RenderTranslucentPolygonalGeometry(ren);
  if (this->OutlineActor->GetVisibility())
  {
    this->OutlineActor->RenderTranslucentPolygonalGeometry(ren);
  }
  if (this->IconActor->GetVisibility())
  {
    this->IconActor->RenderTranslucentPolygonalGeometry(ren);
  }
  if (this->IconActor->GetVisibility())
  {
    this->IconActor->RenderOverlay(ren);
  }
  this->TimeToDraw = this->EdgeMapper->GetTimeToDraw() + this->VertexMapper->GetTimeToDraw() +
    this->OutlineMapper->GetTimeToDraw() + this->IconMapper->GetTimeToDraw();
}

//----------------------------------------------------------------------------
void svtkGraphMapper::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  if (this->CircleGlyph)
  {
    os << indent << "CircleGlyph: (" << this->CircleGlyph << ")\n";
  }
  else
  {
    os << indent << "CircleGlyph: (none)\n";
  }
  if (this->CircleOutlineGlyph)
  {
    os << indent << "CircleOutlineGlyph: (" << this->CircleOutlineGlyph << ")\n";
  }
  else
  {
    os << indent << "CircleOutlineGlyph: (none)\n";
  }
  if (this->EdgeMapper)
  {
    os << indent << "EdgeMapper: (" << this->EdgeMapper << ")\n";
  }
  else
  {
    os << indent << "EdgeMapper: (none)\n";
  }
  if (this->VertexMapper)
  {
    os << indent << "VertexMapper: (" << this->VertexMapper << ")\n";
  }
  else
  {
    os << indent << "VertexMapper: (none)\n";
  }
  if (this->OutlineMapper)
  {
    os << indent << "OutlineMapper: (" << this->OutlineMapper << ")\n";
  }
  else
  {
    os << indent << "OutlineMapper: (none)\n";
  }
  if (this->EdgeActor)
  {
    os << indent << "EdgeActor: (" << this->EdgeActor << ")\n";
  }
  else
  {
    os << indent << "EdgeActor: (none)\n";
  }
  if (this->VertexActor)
  {
    os << indent << "VertexActor: (" << this->VertexActor << ")\n";
  }
  else
  {
    os << indent << "VertexActor: (none)\n";
  }
  if (this->OutlineActor)
  {
    os << indent << "OutlineActor: (" << this->OutlineActor << ")\n";
  }
  else
  {
    os << indent << "OutlineActor: (none)\n";
  }

  if (this->GraphToPoly)
  {
    os << indent << "GraphToPoly: (" << this->GraphToPoly << ")\n";
  }
  else
  {
    os << indent << "GraphToPoly: (none)\n";
  }

  if (this->VertexLookupTable)
  {
    os << indent << "VertexLookupTable: (" << this->VertexLookupTable << ")\n";
  }
  else
  {
    os << indent << "VertexLookupTable: (none)\n";
  }

  if (this->EdgeLookupTable)
  {
    os << indent << "EdgeLookupTable: (" << this->EdgeLookupTable << ")\n";
  }
  else
  {
    os << indent << "EdgeLookupTable: (none)\n";
  }

  os << indent << "VertexPointSize: " << this->VertexPointSize << endl;
  os << indent << "EdgeLineWidth: " << this->EdgeLineWidth << endl;
  os << indent << "ScaledGlyphs: " << this->ScaledGlyphs << endl;
  os << indent << "ScalingArrayName: " << (this->ScalingArrayName ? "" : "(null)") << endl;
  os << indent << "EnableEdgesByArray: " << this->EnableEdgesByArray << endl;
  os << indent << "EnableVerticesByArray: " << this->EnableVerticesByArray << endl;
  os << indent << "EnabledEdgesArrayName: " << (this->EnabledEdgesArrayName ? "" : "(null)")
     << endl;
  os << indent << "EnabledVerticesArrayName: " << (this->EnabledVerticesArrayName ? "" : "(null)")
     << endl;
}

//----------------------------------------------------------------------------
svtkMTimeType svtkGraphMapper::GetMTime()
{
  svtkMTimeType mTime = this->svtkMapper::GetMTime();
  svtkMTimeType time;

  if (this->LookupTable != nullptr)
  {
    time = this->LookupTable->GetMTime();
    mTime = (time > mTime ? time : mTime);
  }

  return mTime;
}

//----------------------------------------------------------------------------
int svtkGraphMapper::FillInputPortInformation(int svtkNotUsed(port), svtkInformation* info)
{
  info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkGraph");
  return 1;
}

//----------------------------------------------------------------------------
double* svtkGraphMapper::GetBounds()
{
  svtkGraph* graph = svtkGraph::SafeDownCast(this->GetExecutive()->GetInputData(0, 0));
  if (!graph)
  {
    svtkMath::UninitializeBounds(this->Bounds);
    return this->Bounds;
  }
  if (!this->Static)
  {
    this->Update();
    graph = svtkGraph::SafeDownCast(this->GetExecutive()->GetInputData(0, 0));
  }
  if (!graph)
  {
    svtkMath::UninitializeBounds(this->Bounds);
    return this->Bounds;
  }
  graph->GetBounds(this->Bounds);
  return this->Bounds;
}

#if 1
//----------------------------------------------------------------------------
void svtkGraphMapper::ReportReferences(svtkGarbageCollector* collector)
{
  this->Superclass::ReportReferences(collector);
  // These filters share our input and are therefore involved in a
  // reference loop.
  // svtkGarbageCollectorReport(collector, this->GraphToPoly,
  //                          "GraphToPoly");
}

#endif
