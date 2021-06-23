/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkOrientedGlyphContourRepresentation.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkOrientedGlyphContourRepresentation.h"
#include "svtkActor.h"
#include "svtkAssemblyPath.h"
#include "svtkBezierContourLineInterpolator.h"
#include "svtkCamera.h"
#include "svtkCellArray.h"
#include "svtkCleanPolyData.h"
#include "svtkCoordinate.h"
#include "svtkCursor2D.h"
#include "svtkCylinderSource.h"
#include "svtkDoubleArray.h"
#include "svtkFocalPlanePointPlacer.h"
#include "svtkGlyph3D.h"
#include "svtkInteractorObserver.h"
#include "svtkLine.h"
#include "svtkMath.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkPoints.h"
#include "svtkPolyData.h"
#include "svtkPolyDataMapper.h"
#include "svtkProperty.h"
#include "svtkRenderWindow.h"
#include "svtkRenderer.h"
#include "svtkSphereSource.h"
#include "svtkTransform.h"
#include "svtkTransformPolyDataFilter.h"

svtkStandardNewMacro(svtkOrientedGlyphContourRepresentation);

//----------------------------------------------------------------------
svtkOrientedGlyphContourRepresentation::svtkOrientedGlyphContourRepresentation()
{
  // Initialize state
  this->InteractionState = svtkContourRepresentation::Outside;

  this->CursorShape = nullptr;
  this->ActiveCursorShape = nullptr;

  this->HandleSize = 0.01;

  this->PointPlacer = svtkFocalPlanePointPlacer::New();
  this->LineInterpolator = svtkBezierContourLineInterpolator::New();

  // Represent the position of the cursor
  this->FocalPoint = svtkPoints::New();
  this->FocalPoint->SetNumberOfPoints(100);
  this->FocalPoint->SetNumberOfPoints(1);
  this->FocalPoint->SetPoint(0, 0.0, 0.0, 0.0);

  svtkDoubleArray* normals = svtkDoubleArray::New();
  normals->SetNumberOfComponents(3);
  normals->SetNumberOfTuples(100);
  normals->SetNumberOfTuples(1);
  double n[3] = { 0, 0, 0 };
  normals->SetTuple(0, n);

  // Represent the position of the cursor
  this->ActiveFocalPoint = svtkPoints::New();
  this->ActiveFocalPoint->SetNumberOfPoints(100);
  this->ActiveFocalPoint->SetNumberOfPoints(1);
  this->ActiveFocalPoint->SetPoint(0, 0.0, 0.0, 0.0);

  svtkDoubleArray* activeNormals = svtkDoubleArray::New();
  activeNormals->SetNumberOfComponents(3);
  activeNormals->SetNumberOfTuples(100);
  activeNormals->SetNumberOfTuples(1);
  activeNormals->SetTuple(0, n);

  this->FocalData = svtkPolyData::New();
  this->FocalData->SetPoints(this->FocalPoint);
  this->FocalData->GetPointData()->SetNormals(normals);
  normals->Delete();

  this->ActiveFocalData = svtkPolyData::New();
  this->ActiveFocalData->SetPoints(this->ActiveFocalPoint);
  this->ActiveFocalData->GetPointData()->SetNormals(activeNormals);
  activeNormals->Delete();

  this->Glypher = svtkGlyph3D::New();
  this->Glypher->SetInputData(this->FocalData);
  this->Glypher->SetVectorModeToUseNormal();
  this->Glypher->OrientOn();
  this->Glypher->ScalingOn();
  this->Glypher->SetScaleModeToDataScalingOff();
  this->Glypher->SetScaleFactor(1.0);

  this->ActiveGlypher = svtkGlyph3D::New();
  this->ActiveGlypher->SetInputData(this->ActiveFocalData);
  this->ActiveGlypher->SetVectorModeToUseNormal();
  this->ActiveGlypher->OrientOn();
  this->ActiveGlypher->ScalingOn();
  this->ActiveGlypher->SetScaleModeToDataScalingOff();
  this->ActiveGlypher->SetScaleFactor(1.0);

  // The transformation of the cursor will be done via svtkGlyph3D
  // By default a svtkCursor2D will be used to define the cursor shape
  svtkCursor2D* cursor2D = svtkCursor2D::New();
  cursor2D->AllOff();
  cursor2D->PointOn();
  cursor2D->Update();
  this->SetCursorShape(cursor2D->GetOutput());
  cursor2D->Delete();

  svtkCylinderSource* cylinder = svtkCylinderSource::New();
  cylinder->SetResolution(64);
  cylinder->SetRadius(0.5);
  cylinder->SetHeight(0.0);
  cylinder->CappingOff();
  cylinder->SetCenter(0, 0, 0);

  svtkCleanPolyData* clean = svtkCleanPolyData::New();
  clean->PointMergingOn();
  clean->CreateDefaultLocator();
  clean->SetInputConnection(cylinder->GetOutputPort());

  svtkTransform* t = svtkTransform::New();
  t->RotateZ(90.0);

  svtkTransformPolyDataFilter* tpd = svtkTransformPolyDataFilter::New();
  tpd->SetInputConnection(clean->GetOutputPort());
  tpd->SetTransform(t);
  clean->Delete();
  cylinder->Delete();

  tpd->Update();
  this->SetActiveCursorShape(tpd->GetOutput());
  tpd->Delete();
  t->Delete();

  this->Glypher->SetSourceData(this->CursorShape);
  this->ActiveGlypher->SetSourceData(this->ActiveCursorShape);

  this->Mapper = svtkPolyDataMapper::New();
  this->Mapper->SetInputConnection(this->Glypher->GetOutputPort());

  // This turns on resolve coincident topology for everything
  // as it is a class static on the mapper
  this->Mapper->SetResolveCoincidentTopologyToPolygonOffset();
  this->Mapper->ScalarVisibilityOff();
  // Put this on top of other objects
  this->Mapper->SetRelativeCoincidentTopologyLineOffsetParameters(-1, -1);
  this->Mapper->SetRelativeCoincidentTopologyPolygonOffsetParameters(-1, -1);
  this->Mapper->SetRelativeCoincidentTopologyPointOffsetParameter(-1);

  this->ActiveMapper = svtkPolyDataMapper::New();
  this->ActiveMapper->SetInputConnection(this->ActiveGlypher->GetOutputPort());
  this->ActiveMapper->ScalarVisibilityOff();
  this->ActiveMapper->SetRelativeCoincidentTopologyLineOffsetParameters(-1, -1);
  this->ActiveMapper->SetRelativeCoincidentTopologyPolygonOffsetParameters(-1, -1);
  this->ActiveMapper->SetRelativeCoincidentTopologyPointOffsetParameter(-1);

  // Set up the initial properties
  this->CreateDefaultProperties();

  this->Actor = svtkActor::New();
  this->Actor->SetMapper(this->Mapper);
  this->Actor->SetProperty(this->Property);

  this->ActiveActor = svtkActor::New();
  this->ActiveActor->SetMapper(this->ActiveMapper);
  this->ActiveActor->SetProperty(this->ActiveProperty);

  this->Lines = svtkPolyData::New();
  this->LinesMapper = svtkPolyDataMapper::New();
  this->LinesMapper->SetInputData(this->Lines);
  this->LinesMapper->SetResolveCoincidentTopologyToPolygonOffset();
  this->LinesMapper->SetRelativeCoincidentTopologyLineOffsetParameters(-1, -1);
  this->LinesMapper->SetRelativeCoincidentTopologyPolygonOffsetParameters(-1, -1);
  this->LinesMapper->SetRelativeCoincidentTopologyPointOffsetParameter(-1);

  this->LinesActor = svtkActor::New();
  this->LinesActor->SetMapper(this->LinesMapper);
  this->LinesActor->SetProperty(this->LinesProperty);

  this->InteractionOffset[0] = 0.0;
  this->InteractionOffset[1] = 0.0;

  this->AlwaysOnTop = 0;

  this->SelectedNodesPoints = nullptr;
  this->SelectedNodesData = nullptr;
  this->SelectedNodesCursorShape = nullptr;
  this->SelectedNodesGlypher = nullptr;
  this->SelectedNodesMapper = nullptr;
  this->SelectedNodesActor = nullptr;
}

//----------------------------------------------------------------------
svtkOrientedGlyphContourRepresentation::~svtkOrientedGlyphContourRepresentation()
{
  this->FocalPoint->Delete();
  this->FocalData->Delete();

  this->ActiveFocalPoint->Delete();
  this->ActiveFocalData->Delete();

  this->SetCursorShape(nullptr);
  this->SetActiveCursorShape(nullptr);

  this->Glypher->Delete();
  this->Mapper->Delete();
  this->Actor->Delete();

  this->ActiveGlypher->Delete();
  this->ActiveMapper->Delete();
  this->ActiveActor->Delete();

  this->Lines->Delete();
  this->LinesMapper->Delete();
  this->LinesActor->Delete();

  this->Property->Delete();
  this->ActiveProperty->Delete();
  this->LinesProperty->Delete();

  // Clear the selected nodes representation
  if (this->SelectedNodesPoints)
  {
    this->SelectedNodesPoints->Delete();
  }
  if (this->SelectedNodesData)
  {
    this->SelectedNodesData->Delete();
  }
  if (this->SelectedNodesCursorShape)
  {
    this->SelectedNodesCursorShape->Delete();
  }
  if (this->SelectedNodesGlypher)
  {
    this->SelectedNodesGlypher->Delete();
  }
  if (this->SelectedNodesMapper)
  {
    this->SelectedNodesMapper->Delete();
  }
  if (this->SelectedNodesActor)
  {
    this->SelectedNodesActor->Delete();
  }
}

//----------------------------------------------------------------------
void svtkOrientedGlyphContourRepresentation::SetCursorShape(svtkPolyData* shape)
{
  if (shape != this->CursorShape)
  {
    if (this->CursorShape)
    {
      this->CursorShape->Delete();
    }
    this->CursorShape = shape;
    if (this->CursorShape)
    {
      this->CursorShape->Register(this);
    }
    if (this->CursorShape)
    {
      this->Glypher->SetSourceData(this->CursorShape);
    }
    this->Modified();
  }
}

//----------------------------------------------------------------------
svtkPolyData* svtkOrientedGlyphContourRepresentation::GetCursorShape()
{
  return this->CursorShape;
}

//----------------------------------------------------------------------
void svtkOrientedGlyphContourRepresentation::SetActiveCursorShape(svtkPolyData* shape)
{
  if (shape != this->ActiveCursorShape)
  {
    if (this->ActiveCursorShape)
    {
      this->ActiveCursorShape->Delete();
    }
    this->ActiveCursorShape = shape;
    if (this->ActiveCursorShape)
    {
      this->ActiveCursorShape->Register(this);
    }
    if (this->ActiveCursorShape)
    {
      this->ActiveGlypher->SetSourceData(this->ActiveCursorShape);
    }
    this->Modified();
  }
}

//----------------------------------------------------------------------
svtkPolyData* svtkOrientedGlyphContourRepresentation::GetActiveCursorShape()
{
  return this->ActiveCursorShape;
}

//----------------------------------------------------------------------
void svtkOrientedGlyphContourRepresentation::SetRenderer(svtkRenderer* ren)
{
  //  this->WorldPosition->SetViewport(ren);
  this->Superclass::SetRenderer(ren);
}

//-------------------------------------------------------------------------
int svtkOrientedGlyphContourRepresentation::ComputeInteractionState(
  int X, int Y, int svtkNotUsed(modified))
{

  double pos[4], xyz[3];
  this->FocalPoint->GetPoint(0, pos);
  pos[3] = 1.0;
  this->Renderer->SetWorldPoint(pos);
  this->Renderer->WorldToDisplay();
  this->Renderer->GetDisplayPoint(pos);

  xyz[0] = static_cast<double>(X);
  xyz[1] = static_cast<double>(Y);
  xyz[2] = pos[2];

  this->VisibilityOn();
  double tol2 = this->PixelTolerance * this->PixelTolerance;
  if (svtkMath::Distance2BetweenPoints(xyz, pos) <= tol2)
  {
    this->InteractionState = svtkContourRepresentation::Nearby;
    if (!this->ActiveCursorShape)
    {
      this->VisibilityOff();
    }
  }
  else
  {
    this->InteractionState = svtkContourRepresentation::Outside;
    if (!this->CursorShape)
    {
      this->VisibilityOff();
    }
  }

  return this->InteractionState;
}

//----------------------------------------------------------------------
// Record the current event position, and the rectilinear wipe position.
void svtkOrientedGlyphContourRepresentation::StartWidgetInteraction(double startEventPos[2])
{
  this->StartEventPosition[0] = startEventPos[0];
  this->StartEventPosition[1] = startEventPos[1];
  this->StartEventPosition[2] = 0.0;

  this->LastEventPosition[0] = startEventPos[0];
  this->LastEventPosition[1] = startEventPos[1];

  // How far is this in pixels from the position of this widget?
  // Maintain this during interaction such as translating (don't
  // force center of widget to snap to mouse position)

  // convert position to display coordinates
  double pos[2];
  this->GetNthNodeDisplayPosition(this->ActiveNode, pos);

  this->InteractionOffset[0] = pos[0] - startEventPos[0];
  this->InteractionOffset[1] = pos[1] - startEventPos[1];
}

//----------------------------------------------------------------------
// Based on the displacement vector (computed in display coordinates) and
// the cursor state (which corresponds to which part of the widget has been
// selected), the widget points are modified.
// First construct a local coordinate system based on the display coordinates
// of the widget.
void svtkOrientedGlyphContourRepresentation::WidgetInteraction(double eventPos[2])
{
  // Process the motion
  if (this->CurrentOperation == svtkContourRepresentation::Translate)
  {
    this->Translate(eventPos);
  }
  if (this->CurrentOperation == svtkContourRepresentation::Shift)
  {
    this->ShiftContour(eventPos);
  }
  if (this->CurrentOperation == svtkContourRepresentation::Scale)
  {
    this->ScaleContour(eventPos);
  }

  // Book keeping
  this->LastEventPosition[0] = eventPos[0];
  this->LastEventPosition[1] = eventPos[1];
}

//----------------------------------------------------------------------
// Translate everything
void svtkOrientedGlyphContourRepresentation::Translate(double eventPos[2])
{
  double ref[3];

  if (!this->GetActiveNodeWorldPosition(ref))
  {
    return;
  }

  double displayPos[2];
  displayPos[0] = eventPos[0] + this->InteractionOffset[0];
  displayPos[1] = eventPos[1] + this->InteractionOffset[1];

  double worldPos[3];
  double worldOrient[9] = { 1.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 1.0 };
  if (this->PointPlacer->ComputeWorldPosition(
        this->Renderer, displayPos, ref, worldPos, worldOrient))
  {
    this->SetActiveNodeToWorldPosition(worldPos, worldOrient);
  }
  else
  {
    // I really want to track the closest point here,
    // but I am postponing this at the moment....
  }
}

//----------------------------------------------------------------------
void svtkOrientedGlyphContourRepresentation::ShiftContour(double eventPos[2])
{
  double ref[3];

  if (!this->GetActiveNodeWorldPosition(ref))
  {
    return;
  }

  double displayPos[2];
  displayPos[0] = eventPos[0] + this->InteractionOffset[0];
  displayPos[1] = eventPos[1] + this->InteractionOffset[1];

  double worldPos[3];
  double worldOrient[9] = { 1.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 1.0 };
  if (this->PointPlacer->ComputeWorldPosition(
        this->Renderer, displayPos, ref, worldPos, worldOrient))
  {

    this->SetActiveNodeToWorldPosition(worldPos, worldOrient);

    double vector[3];
    vector[0] = worldPos[0] - ref[0];
    vector[1] = worldPos[1] - ref[1];
    vector[2] = worldPos[2] - ref[2];

    for (int i = 0; i < this->GetNumberOfNodes(); i++)
    {
      if (i != this->ActiveNode)
      {
        this->GetNthNodeWorldPosition(i, ref);
        worldPos[0] = ref[0] + vector[0];
        worldPos[1] = ref[1] + vector[1];
        worldPos[2] = ref[2] + vector[2];
        this->SetNthNodeWorldPosition(i, worldPos, worldOrient);
      }
    }
  }
}

//----------------------------------------------------------------------
void svtkOrientedGlyphContourRepresentation::ScaleContour(double eventPos[2])
{
  double ref[3];

  if (!this->GetActiveNodeWorldPosition(ref))
  {
    return;
  }

  double centroid[3];
  ComputeCentroid(centroid);

  double r2 = svtkMath::Distance2BetweenPoints(ref, centroid);

  double displayPos[2];
  displayPos[0] = eventPos[0] + this->InteractionOffset[0];
  displayPos[1] = eventPos[1] + this->InteractionOffset[1];

  double worldPos[3];
  double worldOrient[9] = { 1.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 1.0 };
  if (this->PointPlacer->ComputeWorldPosition(
        this->Renderer, displayPos, ref, worldPos, worldOrient))
  {
    double d2 = svtkMath::Distance2BetweenPoints(worldPos, centroid);
    if (d2 != 0.0)
    {
      double ratio = sqrt(d2 / r2);
      // this->SetActiveNodeToWorldPosition(worldPos, worldOrient);

      for (int i = 0; i < this->GetNumberOfNodes(); i++)
      {
        // if (i != this->ActiveNode)
        {
          this->GetNthNodeWorldPosition(i, ref);
          worldPos[0] = centroid[0] + ratio * (ref[0] - centroid[0]);
          worldPos[1] = centroid[1] + ratio * (ref[1] - centroid[1]);
          worldPos[2] = centroid[2] + ratio * (ref[2] - centroid[2]);
          this->SetNthNodeWorldPosition(i, worldPos, worldOrient);
        }
      }
    }
  }
}

//----------------------------------------------------------------------
void svtkOrientedGlyphContourRepresentation::ComputeCentroid(double* ioCentroid)
{
  double p[3];
  ioCentroid[0] = 0.;
  ioCentroid[1] = 0.;
  ioCentroid[2] = 0.;

  for (int i = 0; i < this->GetNumberOfNodes(); i++)
  {
    this->GetNthNodeWorldPosition(i, p);
    ioCentroid[0] += p[0];
    ioCentroid[1] += p[1];
    ioCentroid[2] += p[2];
  }
  double inv_N = 1. / static_cast<double>(this->GetNumberOfNodes());
  ioCentroid[0] *= inv_N;
  ioCentroid[1] *= inv_N;
  ioCentroid[2] *= inv_N;
}

//----------------------------------------------------------------------
void svtkOrientedGlyphContourRepresentation::Scale(double eventPos[2])
{
  // Get the current scale factor
  double sf = this->Glypher->GetScaleFactor();

  // Compute the scale factor
  const int* size = this->Renderer->GetSize();
  double dPos = static_cast<double>(eventPos[1] - this->LastEventPosition[1]);
  sf *= (1.0 + 2.0 * (dPos / size[1])); // scale factor of 2.0 is arbitrary

  // Scale the handle
  this->Glypher->SetScaleFactor(sf);
  if (this->ShowSelectedNodes && this->SelectedNodesGlypher)
  {
    this->SelectedNodesGlypher->SetScaleFactor(sf);
  }
}

//----------------------------------------------------------------------
void svtkOrientedGlyphContourRepresentation::CreateDefaultProperties()
{
  this->Property = svtkProperty::New();
  this->Property->SetColor(1.0, 1.0, 1.0);
  this->Property->SetLineWidth(0.5);
  this->Property->SetPointSize(3);

  this->ActiveProperty = svtkProperty::New();
  this->ActiveProperty->SetColor(0.0, 1.0, 0.0);
  this->ActiveProperty->SetRepresentationToWireframe();
  this->ActiveProperty->SetAmbient(1.0);
  this->ActiveProperty->SetDiffuse(0.0);
  this->ActiveProperty->SetSpecular(0.0);
  this->ActiveProperty->SetLineWidth(1.0);

  this->LinesProperty = svtkProperty::New();
  this->LinesProperty->SetAmbient(1.0);
  this->LinesProperty->SetDiffuse(0.0);
  this->LinesProperty->SetSpecular(0.0);
  this->LinesProperty->SetColor(1, 1, 1);
  this->LinesProperty->SetLineWidth(1);
}

//----------------------------------------------------------------------
void svtkOrientedGlyphContourRepresentation::BuildLines()
{
  svtkPoints* points = svtkPoints::New();
  svtkCellArray* lines = svtkCellArray::New();

  int i, j;
  svtkIdType index = 0;

  int count = this->GetNumberOfNodes();
  for (i = 0; i < this->GetNumberOfNodes(); i++)
  {
    count += this->GetNumberOfIntermediatePoints(i);
  }

  points->SetNumberOfPoints(count);
  svtkIdType numLines;

  if (this->ClosedLoop && count > 0)
  {
    numLines = count + 1;
  }
  else
  {
    numLines = count;
  }

  if (numLines > 0)
  {
    svtkIdType* lineIndices = new svtkIdType[numLines];

    double pos[3];
    for (i = 0; i < this->GetNumberOfNodes(); i++)
    {
      // Add the node
      this->GetNthNodeWorldPosition(i, pos);
      points->InsertPoint(index, pos);
      lineIndices[index] = index;
      index++;

      int numIntermediatePoints = this->GetNumberOfIntermediatePoints(i);

      for (j = 0; j < numIntermediatePoints; j++)
      {
        this->GetIntermediatePointWorldPosition(i, j, pos);
        points->InsertPoint(index, pos);
        lineIndices[index] = index;
        index++;
      }
    }

    if (this->ClosedLoop)
    {
      lineIndices[index] = 0;
    }

    lines->InsertNextCell(numLines, lineIndices);
    delete[] lineIndices;
  }

  this->Lines->SetPoints(points);
  this->Lines->SetLines(lines);

  points->Delete();
  lines->Delete();
}

//----------------------------------------------------------------------
svtkPolyData* svtkOrientedGlyphContourRepresentation::GetContourRepresentationAsPolyData()
{
  // Get the points in this contour as a svtkPolyData.
  return this->Lines;
}

//----------------------------------------------------------------------
void svtkOrientedGlyphContourRepresentation::BuildRepresentation()
{
  // Make sure we are up to date with any changes made in the placer
  this->UpdateContour();

  if (this->AlwaysOnTop)
  {
    // max value 65536 so we subtract 66000 to make sure we are
    // zero or negative
    this->LinesMapper->SetRelativeCoincidentTopologyLineOffsetParameters(0, -66000);
    this->LinesMapper->SetRelativeCoincidentTopologyPolygonOffsetParameters(0, -66000);
    this->LinesMapper->SetRelativeCoincidentTopologyPointOffsetParameter(-66000);
    this->Mapper->SetRelativeCoincidentTopologyLineOffsetParameters(0, -66000);
    this->Mapper->SetRelativeCoincidentTopologyPolygonOffsetParameters(0, -66000);
    this->Mapper->SetRelativeCoincidentTopologyPointOffsetParameter(-66000);
    this->ActiveMapper->SetRelativeCoincidentTopologyLineOffsetParameters(0, -66000);
    this->ActiveMapper->SetRelativeCoincidentTopologyPolygonOffsetParameters(0, -66000);
    this->ActiveMapper->SetRelativeCoincidentTopologyPointOffsetParameter(-66000);
  }
  else
  {
    this->LinesMapper->SetRelativeCoincidentTopologyLineOffsetParameters(-1, -1);
    this->LinesMapper->SetRelativeCoincidentTopologyPolygonOffsetParameters(-1, -1);
    this->LinesMapper->SetRelativeCoincidentTopologyPointOffsetParameter(-1);
    this->Mapper->SetRelativeCoincidentTopologyLineOffsetParameters(-1, -1);
    this->Mapper->SetRelativeCoincidentTopologyPolygonOffsetParameters(-1, -1);
    this->Mapper->SetRelativeCoincidentTopologyPointOffsetParameter(-1);
    this->ActiveMapper->SetRelativeCoincidentTopologyLineOffsetParameters(-1, -1);
    this->ActiveMapper->SetRelativeCoincidentTopologyPolygonOffsetParameters(-1, -1);
    this->ActiveMapper->SetRelativeCoincidentTopologyPointOffsetParameter(-1);
  }

  double p1[4], p2[4];
  this->Renderer->GetActiveCamera()->GetFocalPoint(p1);
  p1[3] = 1.0;
  this->Renderer->SetWorldPoint(p1);
  this->Renderer->WorldToView();
  this->Renderer->GetViewPoint(p1);

  double depth = p1[2];
  double aspect[2];
  this->Renderer->ComputeAspect();
  this->Renderer->GetAspect(aspect);

  p1[0] = -aspect[0];
  p1[1] = -aspect[1];
  this->Renderer->SetViewPoint(p1);
  this->Renderer->ViewToWorld();
  this->Renderer->GetWorldPoint(p1);

  p2[0] = aspect[0];
  p2[1] = aspect[1];
  p2[2] = depth;
  p2[3] = 1.0;
  this->Renderer->SetViewPoint(p2);
  this->Renderer->ViewToWorld();
  this->Renderer->GetWorldPoint(p2);

  double distance = sqrt(svtkMath::Distance2BetweenPoints(p1, p2));

  const int* size = this->Renderer->GetRenderWindow()->GetSize();
  double viewport[4];
  this->Renderer->GetViewport(viewport);

  double x, y, scale;

  x = size[0] * (viewport[2] - viewport[0]);
  y = size[1] * (viewport[3] - viewport[1]);

  scale = sqrt(x * x + y * y);

  distance = 1000 * distance / scale;

  this->Glypher->SetScaleFactor(distance * this->HandleSize);
  this->ActiveGlypher->SetScaleFactor(distance * this->HandleSize);
  int numPoints = this->GetNumberOfNodes();
  int i;
  if (this->ShowSelectedNodes && this->SelectedNodesGlypher)
  {
    this->SelectedNodesGlypher->SetScaleFactor(distance * this->HandleSize);
    this->FocalPoint->Reset();
    this->FocalPoint->SetNumberOfPoints(0);
    this->FocalData->GetPointData()->GetNormals()->SetNumberOfTuples(0);
    this->SelectedNodesPoints->Reset();
    this->SelectedNodesPoints->SetNumberOfPoints(0);
    this->SelectedNodesData->GetPointData()->GetNormals()->SetNumberOfTuples(0);
    for (i = 0; i < numPoints; i++)
    {
      if (i != this->ActiveNode)
      {
        double worldPos[3];
        double worldOrient[9];
        this->GetNthNodeWorldPosition(i, worldPos);
        this->GetNthNodeWorldOrientation(i, worldOrient);
        if (this->GetNthNodeSelected(i))
        {
          this->SelectedNodesPoints->InsertNextPoint(worldPos);
          this->SelectedNodesData->GetPointData()->GetNormals()->InsertNextTuple(worldOrient + 6);
        }
        else
        {
          this->FocalPoint->InsertNextPoint(worldPos);
          this->FocalData->GetPointData()->GetNormals()->InsertNextTuple(worldOrient + 6);
        }
      }
    }
    this->SelectedNodesPoints->Modified();
    this->SelectedNodesData->GetPointData()->GetNormals()->Modified();
    this->SelectedNodesData->Modified();
  }
  else
  {
    if (this->ActiveNode >= 0 && this->ActiveNode < this->GetNumberOfNodes())
    {
      this->FocalPoint->SetNumberOfPoints(numPoints - 1);
      this->FocalData->GetPointData()->GetNormals()->SetNumberOfTuples(numPoints - 1);
    }
    else
    {
      this->FocalPoint->SetNumberOfPoints(numPoints);
      this->FocalData->GetPointData()->GetNormals()->SetNumberOfTuples(numPoints);
    }
    int idx = 0;
    for (i = 0; i < numPoints; i++)
    {
      if (i != this->ActiveNode)
      {
        double worldPos[3];
        double worldOrient[9];
        this->GetNthNodeWorldPosition(i, worldPos);
        this->GetNthNodeWorldOrientation(i, worldOrient);
        this->FocalPoint->SetPoint(idx, worldPos);
        this->FocalData->GetPointData()->GetNormals()->SetTuple(idx, worldOrient + 6);
        idx++;
      }
    }
  }

  this->FocalPoint->Modified();
  this->FocalData->GetPointData()->GetNormals()->Modified();
  this->FocalData->Modified();

  if (this->ActiveNode >= 0 && this->ActiveNode < this->GetNumberOfNodes())
  {
    double worldPos[3];
    double worldOrient[9];
    this->GetNthNodeWorldPosition(this->ActiveNode, worldPos);
    this->GetNthNodeWorldOrientation(this->ActiveNode, worldOrient);
    this->ActiveFocalPoint->SetPoint(0, worldPos);
    this->ActiveFocalData->GetPointData()->GetNormals()->SetTuple(0, worldOrient + 6);

    this->ActiveFocalPoint->Modified();
    this->ActiveFocalData->GetPointData()->GetNormals()->Modified();
    this->ActiveFocalData->Modified();
    this->ActiveActor->VisibilityOn();
  }
  else
  {
    this->ActiveActor->VisibilityOff();
  }
}

//----------------------------------------------------------------------
void svtkOrientedGlyphContourRepresentation::GetActors(svtkPropCollection* pc)
{
  this->Actor->GetActors(pc);
  this->ActiveActor->GetActors(pc);
  this->LinesActor->GetActors(pc);
  if (this->ShowSelectedNodes && this->SelectedNodesActor)
  {
    this->SelectedNodesActor->GetActors(pc);
  }
}

//----------------------------------------------------------------------
void svtkOrientedGlyphContourRepresentation::ReleaseGraphicsResources(svtkWindow* win)
{
  this->Actor->ReleaseGraphicsResources(win);
  this->ActiveActor->ReleaseGraphicsResources(win);
  this->LinesActor->ReleaseGraphicsResources(win);
}

//----------------------------------------------------------------------
int svtkOrientedGlyphContourRepresentation::RenderOverlay(svtkViewport* viewport)
{
  int count = 0;
  count += this->LinesActor->RenderOverlay(viewport);
  if (this->Actor->GetVisibility())
  {
    count += this->Actor->RenderOverlay(viewport);
  }
  if (this->ActiveActor->GetVisibility())
  {
    count += this->ActiveActor->RenderOverlay(viewport);
  }
  return count;
}

//-----------------------------------------------------------------------------
int svtkOrientedGlyphContourRepresentation::RenderOpaqueGeometry(svtkViewport* viewport)
{
  // Since we know RenderOpaqueGeometry gets called first, will do the
  // build here
  this->BuildRepresentation();

  int count = 0;
  count += this->LinesActor->RenderOpaqueGeometry(viewport);
  if (this->Actor->GetVisibility())
  {
    count += this->Actor->RenderOpaqueGeometry(viewport);
  }
  if (this->ActiveActor->GetVisibility())
  {
    count += this->ActiveActor->RenderOpaqueGeometry(viewport);
  }
  if (this->ShowSelectedNodes && this->SelectedNodesActor &&
    this->SelectedNodesActor->GetVisibility())
  {
    count += this->SelectedNodesActor->RenderOpaqueGeometry(viewport);
  }

  return count;
}

//-----------------------------------------------------------------------------
int svtkOrientedGlyphContourRepresentation::RenderTranslucentPolygonalGeometry(svtkViewport* viewport)
{
  int count = 0;
  count += this->LinesActor->RenderTranslucentPolygonalGeometry(viewport);
  if (this->Actor->GetVisibility())
  {
    count += this->Actor->RenderTranslucentPolygonalGeometry(viewport);
  }
  if (this->ActiveActor->GetVisibility())
  {
    count += this->ActiveActor->RenderTranslucentPolygonalGeometry(viewport);
  }
  return count;
}

//-----------------------------------------------------------------------------
svtkTypeBool svtkOrientedGlyphContourRepresentation::HasTranslucentPolygonalGeometry()
{
  int result = 0;
  result |= this->LinesActor->HasTranslucentPolygonalGeometry();
  if (this->Actor->GetVisibility())
  {
    result |= this->Actor->HasTranslucentPolygonalGeometry();
  }
  if (this->ActiveActor->GetVisibility())
  {
    result |= this->ActiveActor->HasTranslucentPolygonalGeometry();
  }
  return result;
}

//----------------------------------------------------------------------------
void svtkOrientedGlyphContourRepresentation::SetLineColor(double r, double g, double b)
{
  if (this->GetLinesProperty())
  {
    this->GetLinesProperty()->SetColor(r, g, b);
  }
}

//----------------------------------------------------------------------
void svtkOrientedGlyphContourRepresentation::SetShowSelectedNodes(svtkTypeBool flag)
{
  svtkDebugMacro(<< this->GetClassName() << " (" << this << "): setting ShowSelectedNodes to "
                << flag);
  if (this->ShowSelectedNodes != flag)
  {
    this->ShowSelectedNodes = flag;
    this->Modified();

    if (this->ShowSelectedNodes)
    {
      if (!this->SelectedNodesActor)
      {
        this->CreateSelectedNodesRepresentation();
      }
      else
      {
        this->SelectedNodesActor->SetVisibility(1);
      }
    }
    else
    {
      if (this->SelectedNodesActor)
      {
        this->SelectedNodesActor->SetVisibility(0);
      }
    }
  }
}

//----------------------------------------------------------------------
double* svtkOrientedGlyphContourRepresentation::GetBounds()
{
  return this->Lines->GetPoints() ? this->Lines->GetPoints()->GetBounds() : nullptr;
}

//-----------------------------------------------------------------------------
void svtkOrientedGlyphContourRepresentation::CreateSelectedNodesRepresentation()
{
  svtkSphereSource* sphere = svtkSphereSource::New();
  sphere->SetThetaResolution(12);
  sphere->SetRadius(0.3);
  this->SelectedNodesCursorShape = sphere->GetOutput();
  this->SelectedNodesCursorShape->Register(this);
  sphere->Delete();

  // Represent the position of the cursor
  this->SelectedNodesPoints = svtkPoints::New();
  this->SelectedNodesPoints->SetNumberOfPoints(100);
  // this->SelectedNodesPoints->SetNumberOfPoints(1);
  // this->SelectedNodesPoints->SetPoint(0, 0.0, 0.0, 0.0);

  svtkDoubleArray* normals = svtkDoubleArray::New();
  normals->SetNumberOfComponents(3);
  normals->SetNumberOfTuples(100);
  normals->SetNumberOfTuples(1);
  double n[3] = { 0, 0, 0 };
  normals->SetTuple(0, n);

  this->SelectedNodesData = svtkPolyData::New();
  this->SelectedNodesData->SetPoints(this->SelectedNodesPoints);
  this->SelectedNodesData->GetPointData()->SetNormals(normals);
  normals->Delete();

  this->SelectedNodesGlypher = svtkGlyph3D::New();
  this->SelectedNodesGlypher->SetInputData(this->SelectedNodesData);
  this->SelectedNodesGlypher->SetVectorModeToUseNormal();
  this->SelectedNodesGlypher->OrientOn();
  this->SelectedNodesGlypher->ScalingOn();
  this->SelectedNodesGlypher->SetScaleModeToDataScalingOff();
  this->SelectedNodesGlypher->SetScaleFactor(1.0);

  this->SelectedNodesGlypher->SetSourceData(this->SelectedNodesCursorShape);

  this->SelectedNodesMapper = svtkPolyDataMapper::New();
  this->SelectedNodesMapper->SetInputData(this->SelectedNodesGlypher->GetOutput());
  this->SelectedNodesMapper->SetResolveCoincidentTopologyToPolygonOffset();
  this->SelectedNodesMapper->ScalarVisibilityOff();

  svtkProperty* selProperty = svtkProperty::New();
  selProperty->SetColor(0.0, 1.0, 0.0);
  selProperty->SetLineWidth(0.5);
  selProperty->SetPointSize(3);

  this->SelectedNodesActor = svtkActor::New();
  this->SelectedNodesActor->SetMapper(this->SelectedNodesMapper);
  this->SelectedNodesActor->SetProperty(selProperty);
  selProperty->Delete();
}

//-----------------------------------------------------------------------------
void svtkOrientedGlyphContourRepresentation::PrintSelf(ostream& os, svtkIndent indent)
{
  // Superclass typedef defined in svtkTypeMacro() found in svtkSetGet.h
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Always On Top: " << (this->AlwaysOnTop ? "On\n" : "Off\n");
  os << indent << "ShowSelectedNodes: " << this->ShowSelectedNodes << endl;

  if (this->Property)
  {
    os << indent << "Property: " << this->Property << "\n";
  }
  else
  {
    os << indent << "Property: (none)\n";
  }

  if (this->ActiveProperty)
  {
    os << indent << "Active Property: " << this->ActiveProperty << "\n";
  }
  else
  {
    os << indent << "Active Property: (none)\n";
  }

  if (this->LinesProperty)
  {
    os << indent << "Lines Property: " << this->LinesProperty << "\n";
  }
  else
  {
    os << indent << "Lines Property: (none)\n";
  }
}
