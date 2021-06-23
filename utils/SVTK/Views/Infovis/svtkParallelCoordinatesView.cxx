/*=========================================================================
Program:   Visualization Toolkit
Module:    svtkParallelCoordinatesView.cxx

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2009 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
  -------------------------------------------------------------------------*/
#include "svtkParallelCoordinatesView.h"

#include "svtkActor2D.h"
#include "svtkAlgorithmOutput.h"
#include "svtkBalloonRepresentation.h"
#include "svtkCellArray.h"
#include "svtkCellArrayIterator.h"
#include "svtkCommand.h"
#include "svtkDataSetAttributes.h"
#include "svtkDoubleArray.h"
#include "svtkIdList.h"
#include "svtkObjectFactory.h"
#include "svtkOutlineSource.h"
#include "svtkParallelCoordinatesHistogramRepresentation.h"
#include "svtkParallelCoordinatesInteractorStyle.h"
#include "svtkParallelCoordinatesRepresentation.h"
#include "svtkPoints.h"
#include "svtkPolyData.h"
#include "svtkPolyDataMapper2D.h"
#include "svtkProperty2D.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRendererCollection.h"
#include "svtkSCurveSpline.h"
#include "svtkSelection.h"
#include "svtkSelectionNode.h"
#include "svtkTable.h"
#include "svtkTextProperty.h"
#include "svtkViewTheme.h"

#include <cassert>
#include <sstream>

svtkStandardNewMacro(svtkParallelCoordinatesView);

// ----------------------------------------------------------------------

svtkParallelCoordinatesView::svtkParallelCoordinatesView()
{
  svtkParallelCoordinatesInteractorStyle* istyle = svtkParallelCoordinatesInteractorStyle::New();
  this->SetInteractorStyle(istyle);
  istyle->Delete();

  this->ReuseSingleRepresentationOn();

  istyle->AddObserver(svtkCommand::StartInteractionEvent, this->GetObserver());
  istyle->AddObserver(svtkCommand::InteractionEvent, this->GetObserver());
  istyle->AddObserver(svtkCommand::EndInteractionEvent, this->GetObserver());
  istyle->AddObserver(svtkCommand::UpdateEvent, this->GetObserver());

  this->BrushData = svtkSmartPointer<svtkPolyData>::New();
  this->BrushMapper = svtkSmartPointer<svtkPolyDataMapper2D>::New();
  this->BrushActor = svtkSmartPointer<svtkActor2D>::New();

  svtkSmartPointer<svtkCoordinate> dummycoord = svtkSmartPointer<svtkCoordinate>::New();
  dummycoord->SetCoordinateSystemToNormalizedViewport();
  this->BrushMapper->SetInputData(this->BrushData);
  this->BrushMapper->SetTransformCoordinate(dummycoord);
  this->BrushActor->SetMapper(this->BrushMapper);
  this->BrushActor->GetProperty()->SetColor(.1, 1.0, 1.0);

  this->InspectMode = SVTK_INSPECT_MANIPULATE_AXES;
  this->BrushMode = SVTK_BRUSH_LASSO;
  this->BrushOperator = SVTK_BRUSHOPERATOR_ADD;
  this->MaximumNumberOfBrushPoints = -1;
  this->NumberOfBrushPoints = 0;
  this->SetMaximumNumberOfBrushPoints(100);
  this->ClearBrushPoints();
  this->FirstFunctionBrushLineDrawn = 0;
  this->CurrentBrushClass = 0;
  this->AxisHighlightPosition = SVTK_HIGHLIGHT_CENTER;

  this->SelectedAxisPosition = -1;
  this->HighlightSource = svtkSmartPointer<svtkOutlineSource>::New();
  this->HighlightMapper = svtkSmartPointer<svtkPolyDataMapper2D>::New();
  this->HighlightActor = svtkSmartPointer<svtkActor2D>::New();

  this->HighlightSource->SetBounds(-1, -1, -1, -1, -1, -1);
  this->HighlightMapper->SetInputConnection(this->HighlightSource->GetOutputPort());
  this->HighlightMapper->SetTransformCoordinate(dummycoord);
  this->HighlightActor->SetMapper(this->HighlightMapper);
  this->HighlightActor->GetProperty()->SetColor(.1, 1.0, .1);
  this->HighlightActor->VisibilityOff();

  //  svtkSmartPointer<svtkViewTheme> theme = svtkSmartPointer<svtkViewTheme>::New();
  //  theme->SetBackgroundColor(0,0,0);
  //  theme->SetBackgroundColor2(0,0,0);
  //  this->ApplyViewTheme(theme);
}

// ----------------------------------------------------------------------
svtkParallelCoordinatesView::~svtkParallelCoordinatesView()
{
  // nothing to do
}

void svtkParallelCoordinatesView::PrepareForRendering()
{
  svtkDebugMacro(<< "*** PrepareForRendering called");

  svtkParallelCoordinatesRepresentation* rep =
    svtkParallelCoordinatesRepresentation::SafeDownCast(this->GetRepresentation());

  if (rep)
  {
    svtkRenderer* ren = this->GetRenderer();

    this->Superclass::PrepareForRendering();

    if (!ren->HasViewProp(this->HighlightActor))
    {
      ren->AddActor(this->HighlightActor);
    }
    if (!ren->HasViewProp(this->BrushActor))
    {
      ren->AddActor(this->BrushActor);
    }

    // this is a hack to make sure that the balloon hover text is sitting on top
    if (ren->HasViewProp(this->Balloon))
    {
      this->Renderer->RemoveViewProp(this->Balloon);
      this->Renderer->AddViewProp(this->Balloon);
    }
  }
}

// ----------------------------------------------------------------------
void svtkParallelCoordinatesView::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << "MaximumNumberOfBrushPoints: " << this->MaximumNumberOfBrushPoints << endl;
  os << "BrushOperator: " << this->BrushOperator << endl;
  os << "BrushMode: " << this->BrushMode << endl;
  os << "InspectMode: " << this->InspectMode << endl;
  os << "CurrentBrushClass: " << this->CurrentBrushClass << endl;
}

// ----------------------------------------------------------------------
// The frustum selection code is borrowed from svtkRenderView.
void svtkParallelCoordinatesView::ProcessEvents(
  svtkObject* caller, unsigned long eventId, void* callData)
{

  //  if (caller == this->GetInteractor() || caller == this->GetInteractorStyle())
  if (caller == this->GetInteractorStyle())
  {
    svtkParallelCoordinatesInteractorStyle* style =
      svtkParallelCoordinatesInteractorStyle::SafeDownCast(this->GetInteractorStyle());
    svtkParallelCoordinatesRepresentation* rep =
      svtkParallelCoordinatesRepresentation::SafeDownCast(this->GetRepresentation());

    if (style && rep)
    {
      int state = style->GetState();

      if (eventId == svtkCommand::UpdateEvent)
      {
        rep->ResetAxes();
      }
      else
      {
        switch (state)
        {
          case svtkParallelCoordinatesInteractorStyle::INTERACT_HOVER:
            this->Hover(eventId);
            break;
          case svtkParallelCoordinatesInteractorStyle::INTERACT_INSPECT:
            if (this->InspectMode == SVTK_INSPECT_MANIPULATE_AXES)
            {
              this->ManipulateAxes(eventId);
            }
            else if (this->InspectMode == SVTK_INSPECT_SELECT_DATA)
            {
              this->SelectData(eventId);
            }
            break;
          case svtkParallelCoordinatesInteractorStyle::INTERACT_ZOOM:
            this->Zoom(eventId);
            break;
          case svtkParallelCoordinatesInteractorStyle::INTERACT_PAN:
            this->Pan(eventId);
            break;
        }
      }

      this->Render();
    }
  }

  this->Superclass::ProcessEvents(caller, eventId, callData);
}

svtkDataRepresentation* svtkParallelCoordinatesView::CreateDefaultRepresentation(
  svtkAlgorithmOutput* conn)
{
  svtkParallelCoordinatesHistogramRepresentation* rep =
    svtkParallelCoordinatesHistogramRepresentation::New();
  rep->SetInputConnection(conn);
  svtkDataObject* data = conn->GetProducer()->GetOutputDataObject(0);
  svtkTable* td = svtkTable::SafeDownCast(data);

  if (!td)
  {
    rep->SetInputArrayToProcess(
      0, 0, 0, svtkDataObject::FIELD_ASSOCIATION_POINTS_THEN_CELLS, svtkDataSetAttributes::SCALARS);
  }
  else
  {
    int ncols = td->GetNumberOfColumns();
    for (int i = 0; i < ncols; i++)
    {
      svtkAbstractArray* a = td->GetColumn(i);
      rep->SetInputArrayToProcess(i, 0, 0, svtkDataObject::FIELD_ASSOCIATION_ROWS, a->GetName());
    }
  }

  return rep;
}

int svtkParallelCoordinatesView::SetAxisHighlightPosition(
  svtkParallelCoordinatesRepresentation* rep, int position)
{
  int numAxes = rep->GetNumberOfAxes();

  if (numAxes <= 0)
    return -1;

  double p1[2], p2[2];
  rep->GetPositionAndSize(p1, p2);
  double xpos = rep->GetXCoordinateOfPosition(position);

  if (xpos < 0 || position < 0 || position >= numAxes)
  {
    this->HighlightSource->SetBounds(-1, -1, -1, -1, -1, -1);
    this->HighlightActor->VisibilityOff();
    return -1;
  }
  else
  {
    double xmargin = .3 * p2[0] / static_cast<double>(numAxes);
    double ymargin = .05 * p2[1];
    if (this->AxisHighlightPosition == SVTK_HIGHLIGHT_CENTER)
    {
      this->HighlightSource->SetBounds(
        xpos - xmargin, xpos + xmargin, p1[1] + ymargin, p1[1] + p2[1] - ymargin, 0, 0);
    }
    else if (this->AxisHighlightPosition == SVTK_HIGHLIGHT_MIN)
    {
      this->HighlightSource->SetBounds(
        xpos - xmargin, xpos + xmargin, p1[1] - ymargin, p1[1] + ymargin, 0, 0);
    }
    else if (this->AxisHighlightPosition == SVTK_HIGHLIGHT_MAX)
    {
      this->HighlightSource->SetBounds(
        xpos - xmargin, xpos + xmargin, p1[1] + p2[1] - ymargin, p1[1] + p2[1] + ymargin, 0, 0);
    }
    this->HighlightSource->Update();
    this->HighlightActor->VisibilityOn();
  }

  return position;
}

int svtkParallelCoordinatesView::SetAxisHighlightPosition(
  svtkParallelCoordinatesRepresentation* rep, double xpos)
{
  int nearestPosition = rep->GetPositionNearXCoordinate(xpos);

  return this->SetAxisHighlightPosition(rep, nearestPosition);
}

void svtkParallelCoordinatesView::SetBrushMode(int mode)
{
  if (mode < 0 || mode >= SVTK_BRUSH_MODECOUNT)
  {
    return;
  }
  else
  {
    this->BrushMode = mode;

    // if we made it into function mode but left early, clear the lines
    if (this->FirstFunctionBrushLineDrawn && this->BrushMode != SVTK_BRUSH_FUNCTION)
    {
      this->FirstFunctionBrushLineDrawn = 0;
      this->ClearBrushPoints();
      this->Render();
    }
  }
}

void svtkParallelCoordinatesView::SetBrushOperator(int op)
{
  if (op < 0 || op >= SVTK_BRUSHOPERATOR_MODECOUNT)
  {
    return;
  }
  else
  {
    this->BrushOperator = op;
  }
}

void svtkParallelCoordinatesView::SetInspectMode(int mode)
{
  if (mode < 0 || mode >= SVTK_INSPECT_MODECOUNT)
  {
    return;
  }
  else
  {
    this->InspectMode = mode;

    if (this->InspectMode != SVTK_INSPECT_MANIPULATE_AXES)
    {
      this->HighlightActor->VisibilityOff();
    }
  }
}

void svtkParallelCoordinatesView::SetMaximumNumberOfBrushPoints(int num)
{
  if (num >= 2 && num != this->MaximumNumberOfBrushPoints)
  {
    this->MaximumNumberOfBrushPoints = num;

    svtkSmartPointer<svtkPoints> pts = svtkSmartPointer<svtkPoints>::New();
    pts->SetNumberOfPoints(4 * this->MaximumNumberOfBrushPoints);

    for (int i = 0; i < 4 * this->MaximumNumberOfBrushPoints; i++)
      pts->InsertPoint(i, -1, -1, 0);

    svtkSmartPointer<svtkCellArray> lines = svtkSmartPointer<svtkCellArray>::New();
    lines->AllocateEstimate(4, this->MaximumNumberOfBrushPoints);

    // first line is for a manually drawn curve, for selecting lines
    // second line is for the spline used for angular brushing
    // third and fourth lines are for the splines used for function brushing
    for (int i = 0; i < 4; i++)
    {
      lines->InsertNextCell(this->MaximumNumberOfBrushPoints);
      for (int j = 0; j < this->MaximumNumberOfBrushPoints; j++)
        lines->InsertCellPoint(0);
    }

    this->BrushData->SetPoints(pts);
    this->BrushData->SetLines(lines);
  }
}

void svtkParallelCoordinatesView::ClearBrushPoints()
{
  this->NumberOfBrushPoints = 0;

  svtkIdType npts = this->BrushData->GetPoints()->GetNumberOfPoints();
  for (svtkIdType i = 0; i < npts; i++)
    this->BrushData->GetPoints()->SetPoint(i, -1, -1, 0);

  // clear them for all of the lines
  svtkNew<svtkIdList> cell;
  auto cellIter = svtk::TakeSmartPointer(this->BrushData->GetLines()->NewIterator());
  for (cellIter->GoToFirstCell(); !cellIter->IsDoneWithTraversal(); cellIter->GoToNextCell())
  {
    const svtkIdType cellNum = cellIter->GetCurrentCellId();
    cellIter->GetCurrentCell(cell);

    for (svtkIdType j = 0; j < cell->GetNumberOfIds(); j++)
    {
      cell->SetId(j, cellNum * this->MaximumNumberOfBrushPoints);
    }

    cellIter->ReplaceCurrentCell(cell);
  }

  this->BrushData->Modified();
}

//----------------------------------------------------------------------------
int svtkParallelCoordinatesView::AddLassoBrushPoint(double* p)
{
  if (this->NumberOfBrushPoints >= this->MaximumNumberOfBrushPoints)
    return 0;

  svtkIdType ptid = this->NumberOfBrushPoints;
  this->BrushData->GetPoints()->SetPoint(ptid, p[0], p[1], 0);

  const svtkIdType npts = this->BrushData->GetLines()->GetCellSize(0);
  std::vector<svtkIdType> ptids(static_cast<svtkIdType>(npts));

  for (svtkIdType i = ptid; i < npts; i++)
  {
    ptids[static_cast<size_t>(i)] = ptid;
  }

  this->BrushData->GetLines()->ReplaceCellAtId(0, npts, ptids.data());

  this->NumberOfBrushPoints++;
  this->BrushData->Modified();

  return 1;
}

//------------------------------------------------------------------------------
int svtkParallelCoordinatesView::SetAngleBrushLine(double* p1, double* p2)
{
  this->SetBrushLine(1, p1, p2);
  return 1;
}

//----------------------------------------------------------------------------
int svtkParallelCoordinatesView::SetFunctionBrushLine1(double* p1, double* p2)
{
  this->SetBrushLine(2, p1, p2);
  return 1;
}
//----------------------------------------------------------------------------
int svtkParallelCoordinatesView::SetFunctionBrushLine2(double* p1, double* p2)
{
  this->SetBrushLine(3, p1, p2);
  return 1;
}

//------------------------------------------------------------------------------
int svtkParallelCoordinatesView::SetBrushLine(int line, double* p1, double* p2)
{
  double p1x = p1[0], p1y = p1[1];
  double p2x = p2[0], p2y = p2[1];
  svtkParallelCoordinatesRepresentation* rep =
    svtkParallelCoordinatesRepresentation::SafeDownCast(this->GetRepresentation());
  int numAxes = rep->GetNumberOfAxes();
  double* xs = new double[numAxes];
  rep->GetXCoordinatesOfPositions(xs);

  if (p1x == p2x)
  {
    delete[] xs;
    return 0;
  }

  // swap, if necessary...I don't think the splines like being out of order
  if (p1x > p2x)
  {
    double tmp;
    tmp = p1x;
    p1x = p2x;
    p2x = tmp;
    tmp = p1y;
    p1y = p2y;
    p2y = tmp;
  }

  // find the axis to the left of p1
  int left = numAxes - 1;
  for (int i = 0; i < numAxes; i++)
  {
    if (p1x > xs[i])
    {
      left = i;
    }
    else
    {
      break;
    }
  }

  int right = left + 1;

  // sanity check
  if (right >= numAxes || left < 0)
  {
    delete[] xs;
    return 0;
  }

  // find the points that line (p1-p2) intersects on the left/right axes
  double m = (double)(p2y - p1y) / (double)(p2x - p1x);
  double lefty = p1y - m * (p1x - xs[left]);
  double righty = p1y - m * (p1x - xs[right]);

  p1x = xs[left];
  p2x = xs[right];
  p1y = lefty;
  p2y = righty;
  // p1y = std::max<int>(std::min<int>(lefty,this->YMax),this->YMin);
  // p2y = std::max<int>(std::min<int>(righty,this->YMax),this->YMin);

  // sanity check
  if (p1x >= p2x)
  {
    delete[] xs;
    return 0;
  }

  int pointOffset = line * this->MaximumNumberOfBrushPoints;

  double dx = (double)(p2x - p1x) / (this->MaximumNumberOfBrushPoints - 1);

  if (!rep->GetUseCurves())
  {
    double dy = (double)(p2y - p1y) / (this->MaximumNumberOfBrushPoints - 1);

    for (int i = 0; i < this->MaximumNumberOfBrushPoints; i++)
    {
      this->BrushData->GetPoints()->SetPoint(pointOffset + i, p1x + i * dx, p1y + i * dy, 0);
    }
  }
  else
  {
    svtkSmartPointer<svtkSCurveSpline> spline = svtkSmartPointer<svtkSCurveSpline>::New();
    spline->SetParametricRange(p1x, p2x);
    spline->AddPoint(p1x, p1y);
    spline->AddPoint(p2x, p2y);

    for (int i = 0; i < this->MaximumNumberOfBrushPoints; i++)
    {
      this->BrushData->GetPoints()->SetPoint(
        pointOffset + i, p1x + (double)i * dx, spline->Evaluate(p1x + (double)i * dx), 0);
    }
  }

  svtkNew<svtkIdList> cell;
  this->BrushData->GetLines()->GetCellAtId(line, cell);

  for (svtkIdType j = 0; j < cell->GetNumberOfIds(); j++)
  {
    cell->SetId(j, pointOffset + j);
  }

  this->BrushData->GetLines()->ReplaceCellAtId(line, cell);
  this->BrushData->Modified();

  delete[] xs;
  return 1;
}

//----------------------------------------------------------------------------
void svtkParallelCoordinatesView::GetBrushLine(int line, svtkIdType& npts, svtkIdType const*& ptids)
{
  int cellNum = 0;
  for (this->BrushData->GetLines()->InitTraversal();
       this->BrushData->GetLines()->GetNextCell(npts, ptids); cellNum++)
  {
    if (cellNum == line)
    {
      return;
    }
  }
}

void svtkParallelCoordinatesView::Hover(unsigned long eventId)
{
  svtkParallelCoordinatesInteractorStyle* style =
    svtkParallelCoordinatesInteractorStyle::SafeDownCast(this->GetInteractorStyle());
  svtkParallelCoordinatesRepresentation* rep =
    svtkParallelCoordinatesRepresentation::SafeDownCast(this->GetRepresentation());

  double p1[2], p2[2];
  if (!rep->GetPositionAndSize(p1, p2))
    return;

  double cursorPosition[2] = { 0, 0 };
  style->GetCursorCurrentPosition(this->GetRenderer(), cursorPosition);

  // deal with hovering
  if (this->InspectMode == SVTK_INSPECT_MANIPULATE_AXES && eventId == svtkCommand::InteractionEvent)
  {
    // if we're close to the representation
    if (cursorPosition[0] >= 0.0 && cursorPosition[0] <= 1.0 && cursorPosition[1] >= 0.0 &&
      cursorPosition[1] <= 1.0 && cursorPosition[0] > p1[0] - .05 * p2[0] &&
      cursorPosition[0] < p1[0] + 1.05 * p2[0])
    {

      this->SelectedAxisPosition = rep->GetPositionNearXCoordinate(cursorPosition[0]);
      double xpos = rep->GetXCoordinateOfPosition(this->SelectedAxisPosition);

      if (fabs(xpos - cursorPosition[0]) > .05)
      {
        this->SelectedAxisPosition = -1;
      }
      else if (cursorPosition[1] < p1[1] + .05 * p2[1])
      {
        this->AxisHighlightPosition = SVTK_HIGHLIGHT_MIN;
      }
      else if (cursorPosition[1] > p1[1] + .95 * p2[1])
      {
        this->AxisHighlightPosition = SVTK_HIGHLIGHT_MAX;
      }
      else
      {
        this->AxisHighlightPosition = SVTK_HIGHLIGHT_CENTER;
      }
      this->SetAxisHighlightPosition(rep, this->SelectedAxisPosition);
    }
    else
    {
      this->SelectedAxisPosition = -1;
      this->SetAxisHighlightPosition(rep, this->SelectedAxisPosition);
    }
  }
}

void svtkParallelCoordinatesView::ManipulateAxes(unsigned long eventId)
{
  svtkParallelCoordinatesInteractorStyle* style =
    svtkParallelCoordinatesInteractorStyle::SafeDownCast(this->GetInteractorStyle());
  svtkParallelCoordinatesRepresentation* rep =
    svtkParallelCoordinatesRepresentation::SafeDownCast(this->GetRepresentation());

  double cursorPosition[2], cursorLastPosition[2], cursorStartPosition[2];
  style->GetCursorCurrentPosition(this->GetRenderer(), cursorPosition);
  style->GetCursorLastPosition(this->GetRenderer(), cursorLastPosition);
  style->GetCursorStartPosition(this->GetRenderer(), cursorStartPosition);

  double dx = cursorPosition[0] - cursorLastPosition[0];
  double dy = cursorPosition[1] - cursorLastPosition[1];

  if (eventId == svtkCommand::StartInteractionEvent)
  {
  }
  else if (eventId == svtkCommand::InteractionEvent)
  {
    if (this->SelectedAxisPosition >= 0)
    {
      if (AxisHighlightPosition == SVTK_HIGHLIGHT_CENTER)
      {
        double xpos = rep->GetXCoordinateOfPosition(this->SelectedAxisPosition);
        this->SelectedAxisPosition =
          rep->SetXCoordinateOfPosition(this->SelectedAxisPosition, xpos + dx);
        this->SetAxisHighlightPosition(rep, this->SelectedAxisPosition);
      }
      else
      {
        double range[] = { 0.0, 0.0 };
        rep->GetRangeAtPosition(this->SelectedAxisPosition, range);

        if (AxisHighlightPosition == SVTK_HIGHLIGHT_MAX)
        {
          range[1] += dy * (range[1] - range[0]);
        }
        else if (AxisHighlightPosition == SVTK_HIGHLIGHT_MIN)
        {
          range[0] += dy * (range[1] - range[0]);
        }
        rep->SetRangeAtPosition(this->SelectedAxisPosition, range);
      }
    }
  }
  else if (eventId == svtkCommand::EndInteractionEvent)
  {
    this->SelectedAxisPosition = -1;
  }
}

void svtkParallelCoordinatesView::SelectData(unsigned long eventId)
{
  svtkParallelCoordinatesInteractorStyle* style =
    svtkParallelCoordinatesInteractorStyle::SafeDownCast(this->GetInteractorStyle());
  svtkParallelCoordinatesRepresentation* rep =
    svtkParallelCoordinatesRepresentation::SafeDownCast(this->GetRepresentation());

  double cursorPosition[2], cursorStartPosition[2];
  style->GetCursorCurrentPosition(this->GetRenderer(), cursorPosition);
  style->GetCursorStartPosition(this->GetRenderer(), cursorStartPosition);

  // in lasso mode, the user sketches a curve. Lines that are
  // near that curve are selected.
  if (this->BrushMode == SVTK_BRUSH_LASSO)
  {
    if (eventId == svtkCommand::StartInteractionEvent)
    {
      this->AddLassoBrushPoint(cursorPosition);
    }
    else if (eventId == svtkCommand::InteractionEvent)
    {
      this->AddLassoBrushPoint(cursorPosition);
    }
    else if (eventId == svtkCommand::EndInteractionEvent)
    {
      const svtkIdType* ptids;
      svtkIdType npts;
      this->BrushData->GetLines()->GetCellAtId(0, npts, ptids);

      svtkSmartPointer<svtkPoints> pts = svtkSmartPointer<svtkPoints>::New();
      for (int i = 0; i < npts; i++)
      {
        pts->InsertNextPoint(this->BrushData->GetPoints()->GetPoint(ptids[i]));
      }

      rep->LassoSelect(this->CurrentBrushClass, this->BrushOperator, pts);
      /*
        svtkSmartPointer<svtkDoubleArray> locs = svtkSmartPointer<svtkDoubleArray>::New();
        locs->SetNumberOfComponents(2);
        locs->SetNumberOfTuples(this->NumberOfBrushPoints);
        for (int i=0; i<this->NumberOfBrushPoints; i++)
        {
        locs->SetTuple(i,this->BrushData->GetPoints()->GetPoint(ptids[i]));
        }

        // generate a selection based on the lasso points
        svtkSmartPointer<svtkSelection> sel = svtkSmartPointer<svtkSelection>::New();
        svtkSmartPointer<svtkParallelCoordinatesSelectionNode> node =
        svtkSmartPointer<svtkParallelCoordinatesSelectionNode>::New();
        node->SetSelectionOperator(this->BrushOperator);
        node->SetSelectionIndex(this->CurrentBrushClass);
        node->SetContentType(svtkSelectionNode::LOCATIONS);
        node->SetFieldType(svtkSelectionNode::ROW);
        node->SetSelectionList(locs);

        sel->AddNode(node);
        rep->Select(this,sel);
      */
      this->ClearBrushPoints();
    }
  }
  // in angle brush mode, the user clicks one point to start the
  // line.  the cursor position is the second endpoint of the line.
  else if (this->BrushMode == SVTK_BRUSH_ANGLE)
  {
    if (eventId == svtkCommand::StartInteractionEvent || eventId == svtkCommand::InteractionEvent)
    {
      this->SetAngleBrushLine(cursorStartPosition, cursorPosition);
    }
    else if (eventId == svtkCommand::EndInteractionEvent)
    {
      const svtkIdType* ptids = nullptr;
      svtkIdType npts = 0;
      this->GetBrushLine(1, npts, ptids);

      double p1[3] = { 0, 0, 0 };
      double p2[3] = { 0, 0, 0 };
      this->BrushData->GetPoints()->GetPoint(ptids[0], p1);
      this->BrushData->GetPoints()->GetPoint(ptids[npts - 1], p2);

      rep->AngleSelect(this->CurrentBrushClass, this->BrushOperator, p1, p2);
      /*
        svtkSmartPointer<svtkDoubleArray> locs = svtkSmartPointer<svtkDoubleArray>::New();
        locs->SetNumberOfComponents(2);
        locs->SetNumberOfTuples(2);
        locs->SetTuple(0,p1);
        locs->SetTuple(1,p2);

        // generate a selection based on the angle brush points
        svtkSmartPointer<svtkSelection> sel = svtkSmartPointer<svtkSelection>::New();
        svtkSmartPointer<svtkParallelCoordinatesSelectionNode> node =
        svtkSmartPointer<svtkParallelCoordinatesSelectionNode>::New();
        node->SetSelectionOperator(this->BrushOperator);
        node->SetSelectionIndex(this->CurrentBrushClass);
        node->SetContentType(svtkParallelCoordinatesSelectionNode::ANGLE_LOCATIONS);
        node->SetFieldType(svtkSelectionNode::ROW);
        node->SetSelectionList(locs);

        sel->AddNode(node);
        rep->Select(this,sel);
      */
      this->ClearBrushPoints();
    }
  }
  // same as angle mode, but do two of them.
  else if (this->BrushMode == SVTK_BRUSH_FUNCTION)
  {
    if (eventId == svtkCommand::StartInteractionEvent || eventId == svtkCommand::InteractionEvent)
    {
      if (!this->FirstFunctionBrushLineDrawn)
      {
        this->SetFunctionBrushLine1(cursorStartPosition, cursorPosition);
      }
      else
      {
        this->SetFunctionBrushLine2(cursorStartPosition, cursorPosition);
      }
    }
    else if (eventId == svtkCommand::EndInteractionEvent)
    {
      // if the first line isn't finished, keep going with the second.
      if (!this->FirstFunctionBrushLineDrawn)
      {
        this->FirstFunctionBrushLineDrawn = 1;
      }
      // the first line is finished, so do the selection
      else
      {
        const svtkIdType* ptids = nullptr;
        svtkIdType npts = 0;

        double p1[3] = { 0, 0, 0 };
        double p2[3] = { 0, 0, 0 };
        this->GetBrushLine(2, npts, ptids);
        this->BrushData->GetPoints()->GetPoint(ptids[0], p1);
        this->BrushData->GetPoints()->GetPoint(ptids[npts - 1], p2);

        double q1[3] = { 0, 0, 0 };
        double q2[3] = { 0, 0, 0 };
        this->GetBrushLine(3, npts, ptids);
        this->BrushData->GetPoints()->GetPoint(ptids[0], q1);
        this->BrushData->GetPoints()->GetPoint(ptids[npts - 1], q2);

        rep->FunctionSelect(this->CurrentBrushClass, this->BrushOperator, p1, p2, q1, q2);

        /*        svtkSmartPointer<svtkDoubleArray> locs = svtkSmartPointer<svtkDoubleArray>::New();
                  locs->SetNumberOfComponents(2);
                  locs->SetNumberOfTuples(4);
                  locs->SetTuple(0,p1);
                  locs->SetTuple(1,p2);
                  locs->SetTuple(2,q1);
                  locs->SetTuple(3,q2);


                  // generate a selection based on the angle brush points
                  svtkSmartPointer<svtkSelection> sel = svtkSmartPointer<svtkSelection>::New();
                  svtkSmartPointer<svtkParallelCoordinatesSelectionNode> node =
           svtkSmartPointer<svtkParallelCoordinatesSelectionNode>::New();
                  node->SetSelectionOperator(this->BrushOperator);
                  node->SetSelectionIndex(this->CurrentBrushClass);
                  node->SetContentType(svtkParallelCoordinatesSelectionNode::FUNCTION_LOCATIONS);
                  node->SetFieldType(svtkSelectionNode::ROW);
                  node->SetSelectionList(locs);
                  sel->AddNode(node);
                  rep->Select(this,sel);
        */

        this->FirstFunctionBrushLineDrawn = 0;
        this->ClearBrushPoints();
      }
    }
  }
  else if (this->BrushMode == SVTK_BRUSH_AXISTHRESHOLD)
  {
  }
}

void svtkParallelCoordinatesView::Zoom(unsigned long eventId)
{
  svtkParallelCoordinatesInteractorStyle* style =
    svtkParallelCoordinatesInteractorStyle::SafeDownCast(this->GetInteractorStyle());
  svtkParallelCoordinatesRepresentation* rep =
    svtkParallelCoordinatesRepresentation::SafeDownCast(this->GetRepresentation());

  double p1[2], size[2], p2[2];
  rep->GetPositionAndSize(p1, size);
  p2[0] = p1[0] + size[0];
  p2[1] = p1[1] + size[1];

  double cursorPosition[2], cursorLastPosition[2], cursorStartPosition[2];
  style->GetCursorCurrentPosition(this->GetRenderer(), cursorPosition);
  style->GetCursorLastPosition(this->GetRenderer(), cursorLastPosition);
  style->GetCursorStartPosition(this->GetRenderer(), cursorStartPosition);

  double v1[2] = { cursorStartPosition[0] - p1[0], cursorStartPosition[1] - p1[1] };
  double v2[2] = { cursorStartPosition[0] - p2[0], cursorStartPosition[1] - p2[1] };

  double dy = -(cursorPosition[1] - cursorLastPosition[1]);

  if (eventId == svtkCommand::StartInteractionEvent)
  {
  }
  else if (eventId == svtkCommand::InteractionEvent)
  {
    double p1new[2] = { p1[0] + dy * v1[0], p1[1] + dy * v1[1] };
    double p2new[2] = { p2[0] + dy * v2[0], // - p1new[0],
      p2[1] + dy * v2[1] };                 // - p1new[1]};

    double sizenew[] = { p2new[0] - p1new[0], p2new[1] - p1new[1] };

    rep->SetPositionAndSize(p1new, sizenew);
    this->SetAxisHighlightPosition(rep, this->SelectedAxisPosition);
  }
  else if (eventId == svtkCommand::EndInteractionEvent)
  {
  }
}

void svtkParallelCoordinatesView::Pan(unsigned long eventId)
{
  svtkParallelCoordinatesInteractorStyle* style =
    svtkParallelCoordinatesInteractorStyle::SafeDownCast(this->GetInteractorStyle());
  svtkParallelCoordinatesRepresentation* rep =
    svtkParallelCoordinatesRepresentation::SafeDownCast(this->GetRepresentation());

  double p1[2], size[2], p2[2];
  rep->GetPositionAndSize(p1, size);
  p2[0] = p1[0] + size[0];
  p2[1] = p1[1] + size[1];

  double cursorPosition[2], cursorLastPosition[2], cursorStartPosition[2];
  style->GetCursorCurrentPosition(this->GetRenderer(), cursorPosition);
  style->GetCursorLastPosition(this->GetRenderer(), cursorLastPosition);
  style->GetCursorStartPosition(this->GetRenderer(), cursorStartPosition);

  double dx = cursorPosition[0] - cursorLastPosition[0];
  double dy = cursorPosition[1] - cursorLastPosition[1];

  if (eventId == svtkCommand::StartInteractionEvent)
  {
  }
  else if (eventId == svtkCommand::InteractionEvent)
  {
    double p1new[2] = { p1[0] + dx, p1[1] + dy };
    double p2new[2] = { p2[0] + dx, p2[1] + dy };

    double sizenew[] = { p2new[0] - p1new[0], p2new[1] - p1new[1] };

    rep->SetPositionAndSize(p1new, sizenew);
    this->SetAxisHighlightPosition(rep, this->SelectedAxisPosition);
  }
  else if (eventId == svtkCommand::EndInteractionEvent)
  {
  }
}

void svtkParallelCoordinatesView::ApplyViewTheme(svtkViewTheme* theme)
{
  this->Superclass::ApplyViewTheme(theme);
  this->Balloon->GetFrameProperty()->SetColor(theme->GetBackgroundColor()); // CellColor());
  this->Balloon->GetTextProperty()->SetColor(theme->GetCellColor());        // BackgroundColor());
}
