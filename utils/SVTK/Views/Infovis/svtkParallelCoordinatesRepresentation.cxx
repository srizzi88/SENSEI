/*=========================================================================

Program:   Visualization Toolkit
Module:    svtkParallelCoordinatesRepresentation.cxx

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

#include "svtkParallelCoordinatesRepresentation.h"

#include "svtkAbstractArray.h"
#include "svtkActor.h"
#include "svtkActor2D.h"
#include "svtkAnnotationLink.h"
#include "svtkArray.h"
#include "svtkArrayData.h"
#include "svtkArrayExtents.h"
#include "svtkArrayIteratorIncludes.h"
#include "svtkArrayToTable.h"
#include "svtkAxisActor2D.h"
#include "svtkBivariateLinearTableThreshold.h"
#include "svtkCellArray.h"
#include "svtkCellData.h"
#include "svtkCollection.h"
#include "svtkConeSource.h"
#include "svtkCoordinate.h"
#include "svtkDataArray.h"
#include "svtkDataObject.h"
#include "svtkDataSetMapper.h"
#include "svtkDoubleArray.h"
#include "svtkExtractSelectedPolyDataIds.h"
#include "svtkFieldData.h"
#include "svtkIdTypeArray.h"
#include "svtkInformation.h"
#include "svtkInformationInformationVectorKey.h"
#include "svtkInformationVector.h"
#include "svtkIntArray.h"
#include "svtkInteractorObserver.h"
#include "svtkLookupTable.h"
#include "svtkMath.h"
#include "svtkObjectFactory.h"
#include "svtkOutlineCornerSource.h"
#include "svtkParallelCoordinatesView.h"
#include "svtkPiecewiseFunction.h"
#include "svtkPointData.h"
#include "svtkPoints.h"
#include "svtkPolyData.h"
#include "svtkPolyDataMapper2D.h"
#include "svtkPolyLine.h"
#include "svtkPropCollection.h"
#include "svtkProperty2D.h"
#include "svtkRenderView.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkSCurveSpline.h"
#include "svtkSelection.h"
#include "svtkSelectionNode.h"
#include "svtkSortDataArray.h"
#include "svtkStdString.h"
#include "svtkStringArray.h"
#include "svtkTable.h"
#include "svtkTextMapper.h"
#include "svtkTextProperty.h"
#include "svtkThreshold.h"
#include "svtkTimeStamp.h"
#include "svtkUnsignedIntArray.h"
#include "svtkViewTheme.h"

#include <algorithm>
#include <sstream>
#include <vector>

svtkStandardNewMacro(svtkParallelCoordinatesRepresentation);

//------------------------------------------------------------------------------
// Esoteric template function that figures out the point positions for a single
// array in the plot.  It would be easier (for me) to loop through row at-a-time
// instead of array at-a-time, but this is more efficient.
template <typename iterT>
void svtkParallelCoordinatesRepresentationBuildLinePoints(iterT* it, svtkIdTypeArray* idsToPlot,
  int positionIdx, double xPosition, int numPositions, double ymin, double ymax, double amin,
  double amax, svtkPoints* points)
{
  svtkIdType numTuples = it->GetNumberOfTuples();
  svtkIdType numComponents = it->GetNumberOfComponents();
  double arange = amax - amin;
  double yrange = ymax - ymin;
  double x[3] = { xPosition, ymin + 0.5 * yrange, 0.0 };

  // if there are no specific ids to plot, plot them all
  if (!idsToPlot)
  {
    if (arange == 0.0)
    {
      for (svtkIdType ptId = positionIdx, i = 0; i < numTuples; i++, ptId += numPositions)
      {
        points->SetPoint(ptId, x);
      }
    }
    else
    {
      // just a little optimization
      double ydiva = yrange / arange;
      svtkIdType ptId = positionIdx;

      for (svtkIdType i = 0, arrayId = 0; i < numTuples;
           i++, ptId += numPositions, arrayId += numComponents)
      {
        // map data value to screen position
        svtkVariant v(it->GetValue(arrayId));
        x[1] = ymin + (v.ToDouble() - amin) * ydiva;
        points->SetPoint(ptId, x);
      }
    }
  }
  // received a list of ids to plot, so only do those.
  else
  {

    int numIdsToPlot = idsToPlot->GetNumberOfTuples();

    if (arange == 0.0)
    {
      for (svtkIdType ptId = positionIdx, i = 0; i < numIdsToPlot; i++, ptId += numPositions)
      {
        points->SetPoint(ptId, x);
      }
    }
    else
    {
      // just a little optimization
      double ydiva = yrange / arange;
      svtkIdType ptId = positionIdx;

      for (svtkIdType i = 0, arrayId = 0; i < numIdsToPlot; i++, ptId += numPositions)
      {
        // map data value to screen position
        arrayId = idsToPlot->GetValue(i) * numComponents;
        svtkVariant v(it->GetValue(arrayId));
        x[1] = ymin + (v.ToDouble() - amin) * ydiva;
        points->SetPoint(ptId, x);
      }
    }
  }
}

//------------------------------------------------------------------------------
// Class that houses the STL ivars.  There can be an arbitrary number of
// selections, so it easier to use STL than to be reallocating arrays.
class svtkParallelCoordinatesRepresentation::Internals
{
public:
  std::vector<svtkSmartPointer<svtkPolyData> > SelectionData;
  std::vector<svtkSmartPointer<svtkPolyDataMapper2D> > SelectionMappers;
  std::vector<svtkSmartPointer<svtkActor2D> > SelectionActors;
  static const double Colors[10][3];
  static const unsigned int NumberOfColors = 10;
  double* GetColor(unsigned int idx)
  {
    idx = (idx >= NumberOfColors) ? NumberOfColors - 1 : idx;
    return const_cast<double*>(Colors[idx]);
  }
};

//------------------------------------------------------------------------------
// The colors used for the selections
const double svtkParallelCoordinatesRepresentation::Internals::Colors[10][3] = { { 1.0, 0.0,
                                                                                  0.0 }, // red
  { 0.0, 1.0, 0.0 },                                                                     // green
  { 0.0, .8, 1.0 },                                                                      // cyan
  { .8, .8, 0.0 },                                                                       // yellow
  { .8, 0.0, .8 },                                                                       // magenta
  { .2, .2, 1.0 },                                                                       // blue
  { 1.0, .65, 0.0 },                                                                     // orange
  { .5, .5, .5 },                                                                        // gray
  { .6, .2, .2 },                                                                        // maroon
  { .3, .3, .3 } }; // dark gray

//------------------------------------------------------------------------------
svtkParallelCoordinatesRepresentation::svtkParallelCoordinatesRepresentation()
{
  this->SetNumberOfInputPorts(svtkParallelCoordinatesRepresentation::NUM_INPUT_PORTS);
  // DBG
  this->SetNumberOfOutputPorts(1);
  // DBG

  this->I = new Internals;
  this->AxisTitles = svtkSmartPointer<svtkStringArray>::New();
  this->PlotData = svtkSmartPointer<svtkPolyData>::New();
  this->PlotActor = svtkSmartPointer<svtkActor2D>::New();

  this->PlotMapper = svtkSmartPointer<svtkPolyDataMapper2D>::New();
  this->PlotMapper.TakeReference(InitializePlotMapper(this->PlotData, this->PlotActor, true));

  this->InverseSelection = svtkSmartPointer<svtkSelection>::New();

  this->InputArrayTable = svtkSmartPointer<svtkTable>::New();
  this->LinearThreshold = svtkSmartPointer<svtkBivariateLinearTableThreshold>::New();
  this->LinearThreshold->SetInputData(this->InputArrayTable);

  this->Axes = nullptr;
  this->NumberOfAxisLabels = 2;

  this->PlotTitleMapper = svtkSmartPointer<svtkTextMapper>::New();
  this->PlotTitleMapper->SetInput("Parallel Coordinates Plot");
  this->PlotTitleMapper->GetTextProperty()->SetJustificationToCentered();

  this->PlotTitleActor = svtkSmartPointer<svtkActor2D>::New();
  this->PlotTitleActor->SetMapper(this->PlotTitleMapper);
  //  this->PlotTitleActor->SetTextScaleModeToViewport();
  this->PlotTitleActor->GetActualPositionCoordinate()->SetCoordinateSystemToNormalizedViewport();
  this->PlotTitleActor->SetPosition(.5, .95);

  this->FunctionTextMapper = svtkSmartPointer<svtkTextMapper>::New();
  this->FunctionTextMapper->SetInput("No function selected.");
  this->FunctionTextMapper->GetTextProperty()->SetJustificationToLeft();
  this->FunctionTextMapper->GetTextProperty()->SetVerticalJustificationToTop();
  //  this->FunctionTextActor->SetInput("No function selected.");
  this->FunctionTextMapper->GetTextProperty()->SetFontSize(
    this->PlotTitleMapper->GetTextProperty()->GetFontSize() / 2);

  this->FunctionTextActor = svtkSmartPointer<svtkActor2D>::New();
  //  this->FunctionTextActor->SetTextScaleModeToViewport();
  this->FunctionTextActor->GetActualPositionCoordinate()->SetCoordinateSystemToNormalizedViewport();
  this->FunctionTextActor->SetPosition(.01, .99);
  this->FunctionTextActor->VisibilityOff();

  this->NumberOfAxes = 0;
  this->NumberOfSamples = 0;
  this->YMin = .1;
  this->YMax = .9;
  this->Xs = nullptr;
  this->Mins = nullptr;
  this->Maxs = nullptr;
  this->MinOffsets = nullptr;
  this->MaxOffsets = nullptr;

  this->CurveResolution = 20;
  this->UseCurves = 0;

  this->AngleBrushThreshold = .03;
  this->FunctionBrushThreshold = .1;
  this->SwapThreshold = 0.0;

  this->FontSize = 1.0;

  // Apply default theme
  this->LineOpacity = 1.0;
  this->LineColor[0] = this->LineColor[1] = this->LineColor[2] = 0.0;
  this->AxisColor[0] = this->AxisColor[1] = this->AxisColor[2] = 0.0;
  this->AxisLabelColor[0] = this->AxisLabelColor[1] = this->AxisLabelColor[2] = 0.0;

  svtkViewTheme* theme = svtkViewTheme::New();
  theme->SetCellOpacity(1.0);
  theme->SetCellColor(1.0, 1.0, 1.0);
  theme->SetEdgeLabelColor(1.0, .8, .3);
  this->ApplyViewTheme(theme);
  theme->Delete();

  this->InternalHoverText = nullptr;
}

//------------------------------------------------------------------------------
svtkParallelCoordinatesRepresentation::~svtkParallelCoordinatesRepresentation()
{
  delete I;
  delete[] this->Maxs;
  delete[] this->Mins;
  delete[] this->MaxOffsets;
  delete[] this->MinOffsets;
  delete[] this->Axes;
  delete[] this->Xs;

  this->SetInternalHoverText(nullptr);
}

//------------------------------------------------------------------------------
// I should fill this out.
const char* svtkParallelCoordinatesRepresentation::GetHoverText(svtkView* view, int x, int y)
{
  svtkRenderView* rv = svtkRenderView::SafeDownCast(view);
  if (rv && this->NumberOfAxes > 0)
  {
    const int* s = rv->GetRenderer()->GetSize();

    double p[2] = { 0.0, 0.0 };
    p[0] = static_cast<double>(x) / s[0];
    p[1] = static_cast<double>(y) / s[1];

    int position = this->GetPositionNearXCoordinate(p[0]);

    if (fabs(p[0] - this->Xs[position]) < .05 && p[1] <= this->YMax && p[1] >= this->YMin)
    {
      double pct = (p[1] - this->YMin) / (this->YMax - this->YMin);

      double r[2] = { 0.0, 0.0 };
      this->GetRangeAtPosition(position, r);

      double v = pct * (r[1] - r[0]) + r[0];
      svtkVariant var(v);

      this->SetInternalHoverText(svtkVariant(v).ToString());
      return this->GetInternalHoverText();
    }
    else if (p[0] > this->Xs[0] && p[1] < this->Xs[this->NumberOfAxes - 1] && p[1] <= this->YMax &&
      p[1] >= this->YMin)
    {
      this->UpdateHoverHighlight(view, x, y);
      return this->GetInternalHoverText();
    }
  }
  return nullptr;
}

//------------------------------------------------------------------------------
// Not sure what this function is for
void svtkParallelCoordinatesRepresentation::UpdateHoverHighlight(svtkView* view, int x, int y)
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

  if (x > 0 && y > 0)
  {
    std::ostringstream str;
    const int* size = win->GetSize();
    int linesFound = 0;
    svtkCellArray* lines = this->PlotData->GetLines();

    int lineNum = 0;
    const svtkIdType* pts = nullptr;
    svtkIdType npts = 0;
    double p[3] = { static_cast<double>(x), static_cast<double>(y), 0.0 };
    p[0] /= size[0];
    p[1] /= size[1];

    if (p[0] < this->Xs[0] || p[0] > this->Xs[this->NumberOfAxes - 1] || p[1] < this->YMin ||
      p[1] > this->YMax)
      return;

    double p1[3];
    double p2[3];
    double dist;

    int position = this->ComputePointPosition(p);

    for (lines->InitTraversal(); lines->GetNextCell(npts, pts); lineNum++)
    {
      if (!pts)
        break;

      this->PlotData->GetPoints()->GetPoint(pts[position], p1);
      this->PlotData->GetPoints()->GetPoint(pts[position + 1], p2);

      dist = fabs((p2[1] - p1[1]) / (p2[0] - p1[0]) * (p[0] - p1[0]) + p1[1] - p[1]);

      if (dist < .01)
      {
        str << lineNum << " ";
        linesFound++;

        if (linesFound > 2)
        {
          str << "...";
          break;
        }
      }
    }

    this->SetInternalHoverText(str.str().c_str());
  }
}

//------------------------------------------------------------------------------
int svtkParallelCoordinatesRepresentation::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* svtkNotUsed(outputVector))
{
  svtkDebugMacro(<< "begin request data.\n");

  // get the info objects and input
  svtkInformation* inDataInfo = inputVector[INPUT_DATA]->GetInformationObject(0);
  svtkInformation* inTitleInfo = inputVector[INPUT_TITLES]->GetInformationObject(0);

  if (!inDataInfo)
    return 0;

  svtkDataObject* inputData = inDataInfo->Get(svtkDataObject::DATA_OBJECT());
  if (!inputData)
    return 0;

  // pull out the title string array
  svtkStringArray* titles = nullptr;
  if (inTitleInfo)
  {
    svtkTable* inputTitles = svtkTable::SafeDownCast(inTitleInfo->Get(svtkDataObject::DATA_OBJECT()));
    if (inputTitles && inputTitles->GetNumberOfColumns() > 0)
    {
      titles = svtkArrayDownCast<svtkStringArray>(inputTitles->GetColumn(0));
    }
  }
  // build the input array table.  This is convenience table that gets used
  // later when building the plots.
  if (this->GetInput()->GetMTime() > this->BuildTime)
  {
    if (inputData->IsA("svtkArrayData"))
    {
      svtkSmartPointer<svtkArrayToTable> att = svtkSmartPointer<svtkArrayToTable>::New();
      att->SetInputData(inputData);
      att->Update();

      this->InputArrayTable->ShallowCopy(att->GetOutput());
    }
    else
    {
      svtkInformationVector* inArrayVec = this->Information->Get(INPUT_ARRAYS_TO_PROCESS());

      if (!inArrayVec)
      {
        svtkErrorMacro(<< "No input arrays specified.  Use SetInputArrayToProcess(i,...).");
        return 0;
      }

      int numberOfInputArrays = inArrayVec->GetNumberOfInformationObjects();

      if (numberOfInputArrays <= 0)
      {
        svtkErrorMacro(<< "No input arrays specified.  Use SetInputArrayToProcess(i,...).");
        return 0;
      }

      this->InputArrayTable->Initialize();

      for (int i = 0; i < numberOfInputArrays; i++)
      {
        svtkDataArray* a = this->GetInputArrayToProcess(i, inputVector);
        if (a)
        {
          this->InputArrayTable->AddColumn(a);
        }
      }
    }
  }

  if (this->InputArrayTable->GetNumberOfColumns() <= 0)
  {
    svtkErrorMacro(<< "No valid input arrays specified.");
    return 0;
  }

  svtkDebugMacro(<< "begin compute data properties.\n");
  if (!this->ComputeDataProperties())
    return 0;

  svtkDebugMacro(<< "begin axis placement.\n");
  if (!this->PlaceAxes())
    return 0;

  svtkDebugMacro(<< "begin line placement.\n");

  this->UpdateSelectionActors();

  svtkIdTypeArray* unselectedRows = nullptr;
  if (this->InverseSelection->GetNode(0))
    unselectedRows =
      svtkArrayDownCast<svtkIdTypeArray>(this->InverseSelection->GetNode(0)->GetSelectionList());

  if (this->UseCurves)
  {
    if (!this->PlaceCurves(this->PlotData, this->InputArrayTable, unselectedRows))
      return 0;
  }
  else
  {
    if (!this->PlaceLines(this->PlotData, this->InputArrayTable, unselectedRows))
      return 0;
  }

  svtkDebugMacro(<< "begin selection line placement.\n");
  svtkSelection* selection = this->GetAnnotationLink()->GetCurrentSelection();
  if (selection)
  {

    for (unsigned int i = 0; i < selection->GetNumberOfNodes(); i++)
    {
      if (!this->PlaceSelection(
            this->I->SelectionData[i], this->InputArrayTable, selection->GetNode(i)))
        return 0;
      if (i > 0)
        continue;
    }
  }

  svtkDebugMacro(<< "begin update plot properties.\n");
  if (!this->UpdatePlotProperties(titles))
    return 0;

  this->BuildTime.Modified();

  return 1;
}

//------------------------------------------------------------------------------
// Add all of the plot actors to the view
bool svtkParallelCoordinatesRepresentation::AddToView(svtkView* view)
{
  this->Superclass::AddToView(view);
  svtkRenderView* rv = svtkRenderView::SafeDownCast(view);
  if (rv)
  {
    rv->GetRenderer()->AddActor(this->PlotTitleActor);
    rv->GetRenderer()->AddActor(this->FunctionTextActor);
    rv->GetRenderer()->AddActor(this->PlotActor);

    for (int i = 0; i < this->NumberOfAxes; i++)
    {
      rv->GetRenderer()->AddActor(this->Axes[i]);
    }
    for (int i = 0; i < (int)this->I->SelectionActors.size(); i++)
    {
      rv->GetRenderer()->AddActor(this->I->SelectionActors[i]);
    }

    // not sure what these are for
    // rv->RegisterProgress(...);
    return true;
  }
  return false;
}

//------------------------------------------------------------------------------
// Remove all of the plot actors from the view
bool svtkParallelCoordinatesRepresentation::RemoveFromView(svtkView* view)
{
  this->Superclass::RemoveFromView(view);
  svtkRenderView* rv = svtkRenderView::SafeDownCast(view);
  if (rv)
  {
    rv->GetRenderer()->RemoveActor(this->PlotTitleActor);
    rv->GetRenderer()->RemoveActor(this->FunctionTextActor);
    rv->GetRenderer()->RemoveActor(this->PlotActor);

    for (int i = 0; i < this->NumberOfAxes; i++)
    {
      rv->GetRenderer()->RemoveActor(this->Axes[i]);
    }

    for (int i = 0; i < (int)this->I->SelectionActors.size(); i++)
    {
      rv->GetRenderer()->RemoveActor(this->I->SelectionActors[i]);
    }

    // not sure what these are for
    // rv->UnRegisterProgress(this->OutlineMapper);
    return true;
  }
  return false;
}

//------------------------------------------------------------------------------
void svtkParallelCoordinatesRepresentation::PrepareForRendering(svtkRenderView* view)
{
  this->Superclass::PrepareForRendering(view);

  // Make hover highlight up to date

  // Add/remove graph actors as necessary as input connections are added/removed
}

//------------------------------------------------------------------------------
void svtkParallelCoordinatesRepresentation::ApplyViewTheme(svtkViewTheme* theme)
{
  this->Superclass::ApplyViewTheme(theme);

  double opacity = std::max(0.0, std::min(1.0, theme->GetCellOpacity()));
  this->SetLineOpacity(opacity);
  this->SetLineColor(theme->GetCellColor());
  this->SetAxisColor(theme->GetEdgeLabelColor());
  this->SetAxisLabelColor(theme->GetCellColor());
  this->SetLineOpacity(theme->GetCellOpacity());
}

//------------------------------------------------------------------------------
int svtkParallelCoordinatesRepresentation::FillInputPortInformation(int port, svtkInformation* info)
{
  if (port == svtkParallelCoordinatesRepresentation::INPUT_DATA)
  {
    info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkDataObject");
    return 1;
  }
  else if (port == svtkParallelCoordinatesRepresentation::INPUT_TITLES)
  {
    info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkTable");
    info->Set(svtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
    return 1;
  }

  return 0;
}
//------------------------------------------------------------------------------
void svtkParallelCoordinatesRepresentation::SetAxisTitles(svtkAlgorithmOutput* ao)
{
  this->SetInputConnection(1, ao);
}
//------------------------------------------------------------------------------
void svtkParallelCoordinatesRepresentation::SetAxisTitles(svtkStringArray* sa)
{
  svtkSmartPointer<svtkTable> t = svtkSmartPointer<svtkTable>::New();
  t->AddColumn(sa);
  this->SetInputData(1, t);
}
//------------------------------------------------------------------------------
void svtkParallelCoordinatesRepresentation::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << "NumberOfAxes: " << this->NumberOfAxes << endl;
  os << "NumberOfSamples: " << this->NumberOfSamples << endl;
  os << "NumberOfAxisLabels: " << this->NumberOfAxisLabels << endl;
  os << "YMin: " << this->YMin << endl;
  os << "YMax: " << this->YMax << endl;
  os << "CurveResolution: " << this->CurveResolution << endl;
  os << "UseCurves: " << this->UseCurves << endl;
  os << "AngleBrushThreshold: " << this->AngleBrushThreshold << endl;
  os << "FunctionBrushThreshold: " << this->FunctionBrushThreshold << endl;
  os << "SwapThreshold: " << this->SwapThreshold << endl;
  os << "LineOpacity: " << this->LineOpacity << endl;
  os << "FontSize: " << this->FontSize << endl;
  os << "LineColor: " << this->LineColor[0] << this->LineColor[1] << this->LineColor[2] << endl;
  os << "AxisColor: " << this->AxisColor[0] << this->AxisColor[1] << this->AxisColor[2] << endl;
  os << "AxisLabelColor: " << this->AxisLabelColor[0] << this->AxisLabelColor[1]
     << this->AxisLabelColor[2] << endl;

  os << "Xs: ";
  for (int i = 0; i < this->NumberOfAxes; i++)
    os << this->Xs[i];
  os << endl;

  os << "Mins: ";
  for (int i = 0; i < this->NumberOfAxes; i++)
    os << this->Mins[i];
  os << endl;

  os << "Maxs: ";
  for (int i = 0; i < this->NumberOfAxes; i++)
    os << this->Maxs[i];
  os << endl;

  os << "MinOffsets: ";
  for (int i = 0; i < this->NumberOfAxes; i++)
    os << this->MinOffsets[i];
  os << endl;

  os << "MaxOffsets: ";
  for (int i = 0; i < this->NumberOfAxes; i++)
    os << this->MaxOffsets[i];
  os << endl;
}

//------------------------------------------------------------------------------
int svtkParallelCoordinatesRepresentation::ComputeDataProperties()
{
  // if the data hasn't changed, there's no reason to recompute
  if (this->BuildTime > this->GetInput()->GetMTime())
  {
    return 1;
  }

  int numberOfInputArrays = this->InputArrayTable->GetNumberOfColumns();
  int newNumberOfAxes = 0;
  int newNumberOfSamples = 0;

  // stores the array names, if there are any
  svtkSmartPointer<svtkStringArray> newtitles = svtkSmartPointer<svtkStringArray>::New();

  for (int i = 0; i < numberOfInputArrays; i++)
  {
    svtkAbstractArray* array = this->InputArrayTable->GetColumn(i);
    int numTuples = array->GetNumberOfTuples();

    if (i > 0 && newNumberOfSamples != numTuples)
    {
      svtkErrorMacro(<< "Error: all arrays must have the same number of values!");
      return 0;
    }
    else
    {
      newNumberOfSamples = numTuples;
    }

    newNumberOfAxes++;

    if (array->GetName())
    {
      newtitles->InsertNextValue(array->GetName());
    }
  }

  if (newNumberOfAxes <= 0 || newNumberOfSamples <= 0)
  {
    return 0;
  }

  // did the number of axes change? reinitialize EVERYTHING.
  if (newNumberOfAxes != this->NumberOfAxes || newNumberOfSamples != this->NumberOfSamples)
  {
    // make sure that the old ones get removed
    for (int i = 0; i < this->NumberOfAxes; i++)
    {
      this->RemovePropOnNextRender(this->Axes[i]);
    }

    this->NumberOfAxes = newNumberOfAxes;
    this->NumberOfSamples = newNumberOfSamples;

    this->ReallocateInternals();
  }

  if (this->AxisTitles->GetNumberOfValues() != this->NumberOfAxes ||
    newtitles->GetNumberOfValues() == this->NumberOfAxes)
  {
    this->AxisTitles->Initialize();
    this->AxisTitles->DeepCopy(newtitles);
  }

  // compute axis ranges
  for (int i = 0; i < numberOfInputArrays; i++)
  {
    svtkDataArray* array = svtkArrayDownCast<svtkDataArray>(this->InputArrayTable->GetColumn(i));
    double* r = array->GetRange(0);
    this->Mins[i] = r[0];
    this->Maxs[i] = r[1];
  }

  return 1;
}

//------------------------------------------------------------------------------
// update colors and such.
int svtkParallelCoordinatesRepresentation::UpdatePlotProperties(svtkStringArray* inputTitles)
{

  this->PlotActor->GetProperty()->SetColor(this->LineColor);
  this->PlotActor->GetProperty()->SetOpacity(this->LineOpacity);
  this->PlotTitleActor->GetProperty()->SetColor(this->AxisLabelColor);

  if (inputTitles)
  {
    this->AxisTitles->DeepCopy(inputTitles);
  }
  // make sure we have sufficient plot titles
  if (this->NumberOfAxes != this->AxisTitles->GetNumberOfValues())
  {
    svtkWarningMacro(<< "Warning: wrong number of axis titles, using default labels.");

    this->AxisTitles->Initialize();
    for (int i = 0; i < this->NumberOfAxes; i++)
    {
      char title[16];
      snprintf(title, sizeof(title), "%c", i + 65);
      this->AxisTitles->InsertNextValue(title);
    }
  }

  // set everything on the axes
  for (int i = 0; i < this->NumberOfAxes; i++)
  {
    this->Axes[i]->SetTitle(this->AxisTitles->GetValue(i));
    this->Axes[i]->SetRange(
      this->Mins[i] + this->MinOffsets[i], this->Maxs[i] + this->MaxOffsets[i]);
    this->Axes[i]->GetProperty()->SetColor(this->AxisColor);
    this->Axes[i]->GetTitleTextProperty()->SetColor(this->AxisLabelColor);
    this->Axes[i]->GetLabelTextProperty()->SetColor(this->AxisLabelColor);
    this->Axes[i]->AdjustLabelsOff();
    this->Axes[i]->GetProperty()->SetLineWidth(2.0);
    this->Axes[i]->SetLabelFactor(0.5);
    this->Axes[i]->TickVisibilityOff();
    this->Axes[i]->SetNumberOfLabels(this->NumberOfAxisLabels);
    this->Axes[i]->SetTitlePosition(-.05);
    this->Axes[i]->GetTitleTextProperty()->SetJustificationToRight();
    this->Axes[i]->GetTitleTextProperty()->ItalicOff();
    this->Axes[i]->GetTitleTextProperty()->BoldOff();
    this->Axes[i]->GetLabelTextProperty()->ItalicOff();
    this->Axes[i]->GetLabelTextProperty()->BoldOff();
    this->Axes[i]->SetFontFactor(this->FontSize);
    this->Axes[i]->GetTitleTextProperty()->Modified();
  }

  for (int i = 0; i < (int)this->I->SelectionActors.size(); i++)
  {
    this->I->SelectionActors[i]->GetProperty()->SetOpacity(this->LineOpacity);
    this->I->SelectionActors[i]->GetProperty()->SetColor(this->I->GetColor(i));
  }

  return 1;
}

//------------------------------------------------------------------------------
// Clear out all of the arrays and initialize them to defaults where appropriate.
int svtkParallelCoordinatesRepresentation::ReallocateInternals()
{
  delete[] this->Maxs;
  delete[] this->Mins;
  delete[] this->MaxOffsets;
  delete[] this->MinOffsets;
  delete[] this->Axes;
  delete[] this->Xs;

  this->Maxs = new double[this->NumberOfAxes];
  this->Mins = new double[this->NumberOfAxes];
  this->MaxOffsets = new double[this->NumberOfAxes];
  this->MinOffsets = new double[this->NumberOfAxes];
  this->Axes = new svtkSmartPointer<svtkAxisActor2D>[this->NumberOfAxes];
  this->Xs = new double[this->NumberOfAxes];

  for (int i = 0; i < this->NumberOfAxes; i++)
  {
    this->Maxs[i] = -SVTK_DOUBLE_MAX;
    this->Mins[i] = SVTK_DOUBLE_MAX;
    this->MaxOffsets[i] = 0.0;
    this->MinOffsets[i] = 0.0;
    this->Axes[i] = svtkSmartPointer<svtkAxisActor2D>::New();
    this->Xs[i] = -1.0;

    this->AddPropOnNextRender(this->Axes[i]);
  }

  // the x positions of axes
  double p1[] = { .1, .1 };
  double p2[] = { .8, .8 };
  double width = (p2[0]) / static_cast<double>(this->NumberOfAxes - 1);
  this->SwapThreshold = width * .1;

  // figure out where each axis should go
  for (int i = 0; i < this->NumberOfAxes; i++)
  {
    this->Xs[i] = p1[0] + i * width;
  }
  return 1;
}

//------------------------------------------------------------------------------
// put the axes where they're supposed to go, which is defined in this->Xs.
int svtkParallelCoordinatesRepresentation::PlaceAxes()
{
  //  int axisI;

  // Get the location of the corners of the box
  double p1[2], p2[2];
  p1[0] = p1[1] = p2[0] = p2[1] = 0.;
  this->GetPositionAndSize(p1, p2);

  // Specify the positions for the axes
  this->YMin = p1[1];
  this->YMax = p1[1] + p2[1];

  // do the placement
  for (int pos = 0; pos < this->NumberOfAxes; pos++)
  {
    // axisI = this->AxisOrder[pos];
    this->Axes[pos]->GetPositionCoordinate()->SetValue(this->Xs[pos], this->YMin);
    this->Axes[pos]->GetPosition2Coordinate()->SetValue(this->Xs[pos], this->YMax);

    this->Axes[pos]->GetPositionCoordinate()->SetCoordinateSystemToNormalizedViewport();
    this->Axes[pos]->GetPosition2Coordinate()->SetCoordinateSystemToNormalizedViewport();
  }

  return 1;
}

//------------------------------------------------------------------------------
int svtkParallelCoordinatesRepresentation::AllocatePolyData(svtkPolyData* polyData, int numLines,
  int numPointsPerLine, int numStrips, int numPointsPerStrip, int numQuads, int numPoints,
  int numCellScalars, int numPointScalars)
{
  // if there are lines requested, make room and fill in some default cells
  if (numLines)
  {
    svtkCellArray* lines = polyData->GetLines();
    if (!lines || lines->GetNumberOfConnectivityIds() != (numLines * numPointsPerLine) ||
      lines->GetNumberOfCells() != numLines)
    {
      lines = svtkCellArray::New();
      lines->AllocateEstimate(numLines, numPointsPerLine);
      polyData->SetLines(lines);
      lines->Delete();

      // prepare the cell array. might as well initialize it now and only
      // recompute it when something actually changes.
      svtkIdType* ptIds = new svtkIdType[numPointsPerLine];

      lines->InitTraversal();
      for (int i = 0; i < numLines; i++)
      {
        for (int j = 0; j < numPointsPerLine; j++)
        {
          ptIds[j] = i * numPointsPerLine + j;
        }
        lines->InsertNextCell(numPointsPerLine, ptIds);
      }
      delete[] ptIds;
    }
  }
  else
  {
    polyData->SetLines(nullptr);
  }

  // if there are strips requested, make room and fill in some default cells
  if (numStrips)
  {
    svtkCellArray* strips = polyData->GetStrips();
    if (!strips || strips->GetNumberOfConnectivityIds() != (numStrips * numPointsPerStrip) ||
      strips->GetNumberOfCells() != numStrips)
    {
      strips = svtkCellArray::New();
      strips->AllocateEstimate(numStrips, numPointsPerStrip);
      polyData->SetStrips(strips);
      strips->Delete();

      // prepare the cell array. might as well initialize it now and only
      // recompute it when something actually changes.
      svtkIdType* ptIds = new svtkIdType[numPointsPerStrip];

      strips->InitTraversal();
      for (int i = 0; i < numStrips; i++)
      {
        for (int j = 0; j < numPointsPerStrip; j++)
        {
          ptIds[j] = i * numPointsPerStrip + j;
        }
        strips->InsertNextCell(numPointsPerStrip, ptIds);
      }
      delete[] ptIds;
    }
  }
  else
  {
    polyData->SetStrips(nullptr);
  }

  // if there are quads requested, make room and fill in some default cells
  if (numQuads)
  {
    svtkCellArray* quads = polyData->GetPolys();
    if (!quads || quads->GetNumberOfConnectivityIds() != (numQuads * 4) ||
      quads->GetNumberOfCells() != numQuads)
    {
      quads = svtkCellArray::New();
      quads->AllocateEstimate(numQuads, 4);
      polyData->SetPolys(quads);
      quads->Delete();

      // prepare the cell array. might as well initialize it now and only
      // recompute it when something actually changes.
      svtkIdType* ptIds = new svtkIdType[4];

      quads->InitTraversal();
      for (int i = 0; i < numQuads; i++)
      {
        for (int j = 0; j < 4; j++)
        {
          ptIds[j] = i * 4 + j;
        }
        quads->InsertNextCell(4, ptIds);
      }
      delete[] ptIds;
    }
  }
  else
  {
    polyData->SetPolys(nullptr);
  }

  // if there are points requested, make room.  don't fill in defaults, as that's
  // what the Place*** functions are for.
  if (numPoints)
  {
    svtkPoints* points = polyData->GetPoints();
    // check if we need to (re)allocate space for the points
    if (!points || points->GetNumberOfPoints() != (numPoints))
    {
      points = svtkPoints::New();
      points->SetNumberOfPoints(numPoints);
      polyData->SetPoints(points);
      points->Delete();
    }
  }
  else
  {
    polyData->SetPoints(nullptr);
  }

  // if there are scalars requested, make room. defaults everything to 0.
  // scalars are all svtkDoubleArrays.
  if (numCellScalars)
  {
    svtkDoubleArray* scalars =
      svtkArrayDownCast<svtkDoubleArray>(polyData->GetCellData()->GetScalars());

    if (!scalars)
    {
      scalars = svtkDoubleArray::New();
      polyData->GetCellData()->SetScalars(scalars);
      scalars->Delete();
    }

    if (scalars->GetNumberOfTuples() != numCellScalars)
    {
      scalars->SetNumberOfTuples(numCellScalars);
      scalars->FillComponent(0, 0);
    }
  }
  else
  {
    polyData->GetCellData()->SetScalars(nullptr);
  }

  // if there are scalars requested, make room. defaults everything to 0.
  // scalars are all svtkDoubleArrays.
  if (numPointScalars)
  {
    svtkDoubleArray* scalars =
      svtkArrayDownCast<svtkDoubleArray>(polyData->GetPointData()->GetScalars());

    if (!scalars)
    {
      scalars = svtkDoubleArray::New();
      polyData->GetPointData()->SetScalars(scalars);
      scalars->Delete();
    }

    if (scalars->GetNumberOfTuples() != numPointScalars)
    {
      scalars->SetNumberOfTuples(numPointScalars);
      scalars->FillComponent(0, 0);
    }
  }
  else
  {
    polyData->GetPointData()->SetScalars(nullptr);
  }

  polyData->BuildCells();
  return 1;
}

//------------------------------------------------------------------------------
int svtkParallelCoordinatesRepresentation::PlaceLines(
  svtkPolyData* polyData, svtkTable* data, svtkIdTypeArray* idsToPlot)
{
  if (!polyData)
    return 0;

  if (!data || data->GetNumberOfColumns() != this->NumberOfAxes)
  {
    polyData->Initialize();
    return 0;
  }

  //  int axisI;
  int position;

  int numPointsPerSample = this->NumberOfAxes;
  int numSamples = (idsToPlot) ? idsToPlot->GetNumberOfTuples()
                               : data->GetNumberOfRows(); // this->NumberOfSamples;

  this->AllocatePolyData(polyData, numSamples, numPointsPerSample, 0, 0, 0,
    numSamples * numPointsPerSample, 0, 0); // no scalars

  svtkPoints* points = polyData->GetPoints();

  for (position = 0; position < this->NumberOfAxes; position++)
  {
    // figure out which axis is at this position
    // axisI = this->AxisOrder[position];

    // get the relevant array information
    //    svtkDataArray* array = this->GetInputArrayAtPosition(position);
    svtkDataArray* array = svtkArrayDownCast<svtkDataArray>(data->GetColumn(position));
    if (!array)
      return 0;

    // start the iterator
    svtkArrayIterator* iter = array->NewIterator();
    switch (array->GetDataType())
    {
      svtkArrayIteratorTemplateMacro(svtkParallelCoordinatesRepresentationBuildLinePoints(
        static_cast<SVTK_TT*>(iter), idsToPlot, position, this->Xs[position], this->NumberOfAxes,
        this->YMin, this->YMax, this->Mins[position] + this->MinOffsets[position],
        this->Maxs[position] + this->MaxOffsets[position], points));
    }
    iter->Delete();
  }

  return 1;
}

//------------------------------------------------------------------------------
int svtkParallelCoordinatesRepresentation::PlaceCurves(
  svtkPolyData* polyData, svtkTable* data, svtkIdTypeArray* idsToPlot)
{
  if (!polyData)
    return 0;

  if (!data || data->GetNumberOfColumns() != this->NumberOfAxes)
  {
    polyData->Initialize();
    return 0;
  }

  //  int axisI;
  int sampleI, position;
  double x[3] = { 0, 0, 0 };

  int numPointsPerSample = (this->NumberOfAxes - 1) * this->CurveResolution + 1;
  int numSamples = (idsToPlot) ? idsToPlot->GetNumberOfTuples()
                               : data->GetNumberOfRows(); // this->NumberOfSamples;

  this->AllocatePolyData(polyData, numSamples, numPointsPerSample, 0, 0, 0,
    numSamples * numPointsPerSample, 0, 0); // numSamples);

  svtkPoints* points = polyData->GetPoints();

  // same as PlaceLines(...), except the number of positions argument has changed.
  for (position = 0; position < this->NumberOfAxes; position++)
  {
    // figure out which axis is at this position
    // axisI = this->AxisOrder[position];

    // get the relevant array information
    //    svtkDataArray* array = this->GetInputArrayAtPosition(position);
    svtkDataArray* array = svtkArrayDownCast<svtkDataArray>(data->GetColumn(position));
    if (!array)
    {
      return 0;
    }

    // start the iterator
    // this fills out a subset of the actual points, namely just the points
    // on the axes.  These get used later to fill in the rest
    svtkArrayIterator* iter = array->NewIterator();
    switch (array->GetDataType())
    {
      svtkArrayIteratorTemplateMacro(
        svtkParallelCoordinatesRepresentationBuildLinePoints(static_cast<SVTK_TT*>(iter), idsToPlot,
          this->CurveResolution * position, this->Xs[position], numPointsPerSample, this->YMin,
          this->YMax, this->Mins[position] + this->MinOffsets[position],
          this->Maxs[position] + this->MaxOffsets[position], points));
    }
    iter->Delete();
  }

  // make a s-curve from (0,0) to (1,1) with the right number of segments.
  // this curve gets transformed based on data values later.
  svtkSmartPointer<svtkDoubleArray> defSplineValues = svtkSmartPointer<svtkDoubleArray>::New();
  this->BuildDefaultSCurve(defSplineValues, this->CurveResolution);

  // now go through what just got filled in and build splines.
  // specifically, the points sitting exactly on the axes are correct,
  // but nothing else is.  Just use that information to build the
  // splines per sample and fill in everything in between.
  svtkIdType ptId = 0;
  double pL[3] = { 0, 0, 0 };
  double pR[3] = { 0, 0, 0 };
  for (sampleI = 0; sampleI < numSamples; sampleI++)
  {
    // build the spline for this sample
    for (position = 0; position < this->NumberOfAxes - 1; position++)
    {
      points->GetPoint(position * this->CurveResolution + sampleI * numPointsPerSample, pL);
      points->GetPoint((position + 1) * this->CurveResolution + sampleI * numPointsPerSample, pR);
      double dy = pR[1] - pL[1];
      double dx =
        (this->Xs[position + 1] - this->Xs[position]) / static_cast<double>(this->CurveResolution);
      for (int curvePosition = 0; curvePosition < this->CurveResolution; curvePosition++)
      {
        x[0] = this->Xs[position] + curvePosition * dx;
        x[1] = defSplineValues->GetValue(curvePosition) * dy + pL[1];
        points->SetPoint(ptId++, x);
      }
    }
    ptId++;
  }

  return 1;
}

//------------------------------------------------------------------------------
void svtkParallelCoordinatesRepresentation::BuildDefaultSCurve(
  svtkDoubleArray* defArray, int numValues)
{
  if (!defArray)
    return;

  // build a default spline, going from (0,0) to (1,1),
  svtkSmartPointer<svtkSCurveSpline> defSpline = svtkSmartPointer<svtkSCurveSpline>::New();
  defSpline->SetParametricRange(0, 1);
  defSpline->AddPoint(0, 0);
  defSpline->AddPoint(1, 1);

  // fill in an array with the interpolated curve values
  defArray->Initialize();
  defArray->SetNumberOfValues(numValues);
  for (int i = 0; i < numValues; i++)
  {
    defArray->SetValue(i, defSpline->Evaluate(static_cast<double>(i) / numValues));
  }
}

//------------------------------------------------------------------------------
int svtkParallelCoordinatesRepresentation::PlaceSelection(
  svtkPolyData* polyData, svtkTable* data, svtkSelectionNode* selectionNode)
{
  svtkIdTypeArray* selectedIds = svtkArrayDownCast<svtkIdTypeArray>(selectionNode->GetSelectionList());

  if (!selectedIds)
    return 0;

  if (this->UseCurves)
  {
    return this->PlaceCurves(polyData, data, selectedIds);
  }
  else
  {
    return this->PlaceLines(polyData, data, selectedIds);
  }
}
//------------------------------------------------------------------------------
void svtkParallelCoordinatesRepresentation::SetPlotTitle(const char* title)
{
  if (title && title[0] != '\0')
  {
    this->PlotTitleActor->VisibilityOn();
    this->PlotTitleMapper->SetInput(title);
  }
  else
  {
    this->PlotTitleActor->VisibilityOff();
  }
}

//------------------------------------------------------------------------------
void svtkParallelCoordinatesRepresentation::SetNumberOfAxisLabels(int num)
{
  if (num > 0)
  {
    this->NumberOfAxisLabels = num;
    for (int i = 0; i < this->NumberOfAxes; i++)
    {
      this->Axes[i]->SetNumberOfLabels(num);
    }
  }
}

//------------------------------------------------------------------------------
int svtkParallelCoordinatesRepresentation::SwapAxisPositions(int position1, int position2)
{
  if (position1 < 0 || position2 < 0 || position1 >= this->NumberOfAxes ||
    position2 >= this->NumberOfAxes)
  {
    return 0;
  }

  // for some reason there's no SetColumn(...)
  if (this->InputArrayTable->GetNumberOfColumns() > 0)
  {
    svtkSmartPointer<svtkTable> oldTable = svtkSmartPointer<svtkTable>::New();
    for (int i = 0; i < this->NumberOfAxes; i++)
    {
      oldTable->AddColumn(this->InputArrayTable->GetColumn(i));
    }

    svtkAbstractArray* a1 = this->InputArrayTable->GetColumn(position1);
    svtkAbstractArray* a2 = this->InputArrayTable->GetColumn(position2);
    this->InputArrayTable->Initialize();
    for (int i = 0; i < this->NumberOfAxes; i++)
    {
      if (i == position1)
        this->InputArrayTable->AddColumn(a2);
      else if (i == position2)
        this->InputArrayTable->AddColumn(a1);
      else
        this->InputArrayTable->AddColumn(oldTable->GetColumn(i));
    }
    this->InputArrayTable->Modified();
  }

  double tmp;
  tmp = this->Mins[position1];
  this->Mins[position1] = this->Mins[position2];
  this->Mins[position2] = tmp;

  tmp = this->Maxs[position1];
  this->Maxs[position1] = this->Maxs[position2];
  this->Maxs[position2] = tmp;

  tmp = this->MinOffsets[position1];
  this->MinOffsets[position1] = this->MinOffsets[position2];
  this->MinOffsets[position2] = tmp;

  tmp = this->MaxOffsets[position1];
  this->MaxOffsets[position1] = this->MaxOffsets[position2];
  this->MaxOffsets[position2] = tmp;

  svtkAxisActor2D* axtmp = this->Axes[position1];
  this->Axes[position1] = this->Axes[position2];
  this->Axes[position2] = axtmp;

  svtkStdString tmpStr = this->AxisTitles->GetValue(position1);
  this->AxisTitles->SetValue(position1, this->AxisTitles->GetValue(position2));
  this->AxisTitles->SetValue(position2, tmpStr);

  // make sure everything's sufficiently far apart
  for (int pos = 1; pos < this->NumberOfAxes; pos++)
  {
    double diff = fabs(this->Xs[pos] - this->Xs[pos - 1]);
    if (diff < this->SwapThreshold)
    {
      this->Xs[pos] += (this->SwapThreshold - diff) + this->SwapThreshold * .1;
    }
  }

  this->Modified();
  return 1;
}

//------------------------------------------------------------------------------
int svtkParallelCoordinatesRepresentation::SetXCoordinateOfPosition(int position, double xcoord)
{
  if (position < 0 || position >= this->NumberOfAxes)
  {
    return -1;
  }

  this->Xs[position] = xcoord;
  this->Modified();

  if (position > 0 && (this->Xs[position] - this->Xs[position - 1]) < this->SwapThreshold)
  {
    this->SwapAxisPositions(position, position - 1);
    return position - 1;
  }
  else if (position < this->NumberOfAxes - 1 &&
    (this->Xs[position + 1] - this->Xs[position]) < this->SwapThreshold)
  {
    this->SwapAxisPositions(position, position + 1);
    return position + 1;
  }

  return position;
}

//------------------------------------------------------------------------------
double svtkParallelCoordinatesRepresentation::GetXCoordinateOfPosition(int position)
{
  if (position >= 0 && position < this->NumberOfAxes)
  {
    return this->Xs[position];
  }
  else
  {
    return -1.0;
  }
}

//------------------------------------------------------------------------------
void svtkParallelCoordinatesRepresentation::GetXCoordinatesOfPositions(double* coords)
{
  for (int i = 0; i < this->NumberOfAxes; i++)
  {
    coords[i] = this->Xs[i];
  }
}

//------------------------------------------------------------------------------
int svtkParallelCoordinatesRepresentation::GetPositionNearXCoordinate(double xcoord)
{
  double minDist = SVTK_DOUBLE_MAX;
  int nearest = -1;
  for (int i = 0; i < this->NumberOfAxes; i++)
  {
    double dist = fabs(this->Xs[i] - xcoord);
    if (dist < minDist)
    {
      nearest = i;
      minDist = dist;
    }
  }

  return nearest;
}

//------------------------------------------------------------------------------
void svtkParallelCoordinatesRepresentation::LassoSelect(
  int brushClass, int brushOperator, svtkPoints* brushPoints)
{
  if (brushPoints->GetNumberOfPoints() < 2)
    return;

  int position = -1, prevPosition = -1;

  svtkSmartPointer<svtkBivariateLinearTableThreshold> threshold =
    nullptr; // svtkSmartPointer<svtkBivariateLinearTableThreshold>::New();
  svtkSmartPointer<svtkIdTypeArray> allIds = svtkSmartPointer<svtkIdTypeArray>::New();

  // for every point in the brush, compute a line in XY space.  A point in XY space satisfies
  // the threshold if it is contained WITHIN all such lines.
  svtkSmartPointer<svtkPoints> posPoints = svtkSmartPointer<svtkPoints>::New();
  for (int i = 0; i < brushPoints->GetNumberOfPoints() - 1; i++)
  {
    double* p = brushPoints->GetPoint(i);
    position = this->ComputePointPosition(p);

    // if we have a valid position
    if (position >= 0 && position < this->NumberOfAxes)
    {
      // position has changed, that means we need to create a new threshold object.
      if (prevPosition != position && i > 0)
      {
        this->LassoSelectInternal(posPoints, allIds);
        posPoints->Initialize();
      }

      posPoints->InsertNextPoint(p);
    }
    prevPosition = position;
  }

  if (posPoints->GetNumberOfPoints() > 0)
  {
    this->LassoSelectInternal(posPoints, allIds);
  }

  this->FunctionTextMapper->SetInput("No function selected.");
  this->FunctionTextActor->VisibilityOff();
  this->SelectRows(brushClass, brushOperator, allIds);
}

//------------------------------------------------------------------------------
void svtkParallelCoordinatesRepresentation::LassoSelectInternal(
  svtkPoints* brushPoints, svtkIdTypeArray* outIds)
{
  if (brushPoints->GetNumberOfPoints() <= 0)
    return;

  double* p = brushPoints->GetPoint(0);
  int position = this->ComputePointPosition(p);

  if (position < 0 || position >= this->NumberOfAxes)
    return;

  double leftAxisRange[2], rightAxisRange[2];
  leftAxisRange[0] = leftAxisRange[1] = rightAxisRange[0] = rightAxisRange[1] = 0;
  this->GetRangeAtPosition(position, leftAxisRange);
  this->GetRangeAtPosition(position + 1, rightAxisRange);

  double dLeft = leftAxisRange[1] - leftAxisRange[0];
  double dRight = rightAxisRange[1] - rightAxisRange[0];
  double dy = this->YMax - this->YMin;

  this->LinearThreshold->Initialize();
  this->LinearThreshold->SetLinearThresholdTypeToBetween();
  this->LinearThreshold->SetDistanceThreshold(this->AngleBrushThreshold);
  this->LinearThreshold->UseNormalizedDistanceOn();
  this->LinearThreshold->SetColumnRanges(dLeft, dRight);
  this->LinearThreshold->AddColumnToThreshold(position, 0);
  this->LinearThreshold->AddColumnToThreshold(position + 1, 0);

  // add a line equation for each brush point
  for (int i = 0; i < brushPoints->GetNumberOfPoints(); i++)
  {
    p = brushPoints->GetPoint(i);

    // normalize p into [0,1]x[0,1]
    double pn[2] = { (p[0] - this->Xs[position]) / (this->Xs[position + 1] - this->Xs[position]),
      (p[1] - this->YMin) / dy };

    // now compute actual data values for two PC lines passing through pn,
    // starting from the endpoints of the left axis
    double q[2] = { leftAxisRange[0], rightAxisRange[0] + pn[1] / pn[0] * dRight };

    double r[2] = { leftAxisRange[1], rightAxisRange[0] + (1.0 + (pn[1] - 1.0) / pn[0]) * dRight };

    this->LinearThreshold->AddLineEquation(q, r);
  }

  this->LinearThreshold->Update();
  svtkIdTypeArray* ids = this->LinearThreshold->GetSelectedRowIds();
  for (int i = 0; i < ids->GetNumberOfTuples(); i++)
  {
    outIds->InsertNextTuple(i, ids);
  }
}
//------------------------------------------------------------------------------
// All lines that have the same slope in PC space represent a set of points
// that define a line in XY space.  PC lines that have similar slope are all
// near the same XY line.
void svtkParallelCoordinatesRepresentation::AngleSelect(
  int brushClass, int brushOperator, double* p1, double* p2)
{
  int position = this->ComputeLinePosition(p1, p2);

  if (position >= 0 && position < this->NumberOfAxes)
  {
    // convert the points into data values
    double leftAxisRange[2], rightAxisRange[2];
    leftAxisRange[0] = leftAxisRange[1] = rightAxisRange[0] = rightAxisRange[1] = 0;
    this->GetRangeAtPosition(position, leftAxisRange);
    this->GetRangeAtPosition(position + 1, rightAxisRange);

    double dLeft = leftAxisRange[1] - leftAxisRange[0];
    double dRight = rightAxisRange[1] - rightAxisRange[0];
    double dy = this->YMax - this->YMin;

    // compute point-slope line definition in XY space
    double xy[2] = { (p1[1] - this->YMin) / dy * dLeft + leftAxisRange[0],
      (p2[1] - this->YMin) / dy * dRight + rightAxisRange[0] };

    // oddly enough, the slope of the XY line is completely
    // independent of the line drawn in PC space.
    double slope = dRight / dLeft;

    this->LinearThreshold->Initialize();
    this->LinearThreshold->SetLinearThresholdTypeToNear();
    this->LinearThreshold->SetDistanceThreshold(this->AngleBrushThreshold);
    this->LinearThreshold->UseNormalizedDistanceOn();
    this->LinearThreshold->SetColumnRanges(dLeft, dRight);
    this->LinearThreshold->AddLineEquation(xy, slope);
    this->LinearThreshold->AddColumnToThreshold(position, 0);
    this->LinearThreshold->AddColumnToThreshold(position + 1, 0);
    this->LinearThreshold->Update();

    char buf[256];
    double b = xy[1] - slope * xy[0];
    snprintf(buf, sizeof(buf), "%s = %f * %s %s %f\n",
      this->AxisTitles->GetValue(position + 1).c_str(), slope,
      this->AxisTitles->GetValue(position).c_str(), (b < 0) ? "-" : "+", fabs(b));

    this->FunctionTextMapper->SetInput(buf);
    this->FunctionTextActor->VisibilityOn();

    this->SelectRows(brushClass, brushOperator, this->LinearThreshold->GetSelectedRowIds());
  }
}

//------------------------------------------------------------------------------
// Line that match a linear function can be found by defining that linear
// function and selecting all points that are near the line.  the linear
// function can be specified by two XY points, equivalent to two PC lines.
void svtkParallelCoordinatesRepresentation::FunctionSelect(
  int brushClass, int brushOperator, double* p1, double* p2, double* q1, double* q2)
{
  int position = this->ComputeLinePosition(p1, p2);
  int position2 = this->ComputeLinePosition(q1, q2);

  if (position != position2)
    return;

  if (position >= 0 && position < this->NumberOfAxes)
  {
    // convert the points into data values
    double leftAxisRange[2], rightAxisRange[2];
    leftAxisRange[0] = leftAxisRange[1] = rightAxisRange[0] = rightAxisRange[1] = 0;
    this->GetRangeAtPosition(position, leftAxisRange);
    this->GetRangeAtPosition(position + 1, rightAxisRange);

    double dLeft = leftAxisRange[1] - leftAxisRange[0];
    double dRight = rightAxisRange[1] - rightAxisRange[0];
    double dy = this->YMax - this->YMin;

    double xy1[2] = { (p1[1] - this->YMin) / dy * dLeft + leftAxisRange[0],
      (p2[1] - this->YMin) / dy * dRight + rightAxisRange[0] };

    double xy2[2] = { (q1[1] - this->YMin) / dy * dLeft + leftAxisRange[0],
      (q2[1] - this->YMin) / dy * dRight + rightAxisRange[0] };

    this->LinearThreshold->Initialize();
    this->LinearThreshold->SetLinearThresholdTypeToNear();
    this->LinearThreshold->SetDistanceThreshold(this->AngleBrushThreshold);
    this->LinearThreshold->UseNormalizedDistanceOn();
    this->LinearThreshold->SetColumnRanges(dLeft, dRight);
    this->LinearThreshold->AddLineEquation(xy1, xy2);
    this->LinearThreshold->AddColumnToThreshold(position, 0);
    this->LinearThreshold->AddColumnToThreshold(position + 1, 0);
    this->LinearThreshold->Update();

    double m = (xy1[1] - xy2[1]) / (xy1[0] - xy2[0]);
    double b = xy1[1] - (xy1[1] - xy2[1]) / (xy1[0] - xy2[0]) * xy1[0];
    char buf[256];
    snprintf(buf, sizeof(buf), "%s = %f * %s %s %f\n",
      this->AxisTitles->GetValue(position + 1).c_str(), m,
      this->AxisTitles->GetValue(position).c_str(), (b < 0) ? "-" : "+", fabs(b));

    this->FunctionTextMapper->SetInput(buf);
    this->FunctionTextActor->VisibilityOn();

    this->SelectRows(brushClass, brushOperator, this->LinearThreshold->GetSelectedRowIds());
  }
}

//------------------------------------------------------------------------------
void svtkParallelCoordinatesRepresentation::RangeSelect(int svtkNotUsed(brushClass),
  int svtkNotUsed(brushOperator), double* svtkNotUsed(p1), double* svtkNotUsed(p2))
{
  // stubbed out for now
}

void svtkParallelCoordinatesRepresentation::UpdateSelectionActors()
{
  svtkSelection* selection = this->GetAnnotationLink()->GetCurrentSelection();
  int numNodes = selection->GetNumberOfNodes();

  for (int i = 0; i < numNodes; i++)
  {
    while (i >= (int)this->I->SelectionData.size())
    {
      // initialize everything for drawing the selection
      svtkSmartPointer<svtkPolyData> polyData = svtkSmartPointer<svtkPolyData>::New();
      svtkSmartPointer<svtkActor2D> actor = svtkSmartPointer<svtkActor2D>::New();
      svtkSmartPointer<svtkPolyDataMapper2D> mapper = svtkSmartPointer<svtkPolyDataMapper2D>::New();
      mapper.TakeReference(this->InitializePlotMapper(polyData, actor));

      this->I->SelectionData.push_back(polyData);
      this->I->SelectionMappers.push_back(mapper);
      this->I->SelectionActors.push_back(actor);

      this->AddPropOnNextRender(actor);
    }
  }

  for (int i = numNodes; i < (int)this->I->SelectionData.size(); i++)
  {
    this->RemovePropOnNextRender(this->I->SelectionActors[i]);
    this->I->SelectionData.pop_back();
    this->I->SelectionMappers.pop_back();
    this->I->SelectionActors.pop_back();
  }

  this->BuildInverseSelection();
}

//------------------------------------------------------------------------------
int svtkParallelCoordinatesRepresentation::ComputePointPosition(double* p)
{
  if (p[0] < this->Xs[0])
    return -1;

  for (int i = 1; i < this->NumberOfAxes; i++)
  {
    if (p[0] < this->Xs[i])
    {
      return i - 1;
    }
  }
  return -1;
}

//------------------------------------------------------------------------------
int svtkParallelCoordinatesRepresentation::ComputeLinePosition(double* p1, double* p2)
{
  double eps = .0001;
  for (int i = 0; i < this->NumberOfAxes - 1; i++)
  {
    if (p1[0] < this->Xs[i] + eps && p2[0] > this->Xs[i + 1] - eps)
    {
      return i;
    }
  }
  return -1;
}

//------------------------------------------------------------------------------
svtkSelection* svtkParallelCoordinatesRepresentation::ConvertSelection(
  svtkView* svtkNotUsed(view), svtkSelection* selection)
{
  return selection;
}

//------------------------------------------------------------------------------
// does the actual selection, including joining the new selection with the
// old selection of the same class with various set operations.
void svtkParallelCoordinatesRepresentation::SelectRows(
  svtkIdType brushClass, svtkIdType brushOperator, svtkIdTypeArray* newSelectedIds)
{
  // keep making new selection nodes (and initializing them) until a node for
  // brushClass actually exists.
  svtkSelection* selection = this->GetAnnotationLink()->GetCurrentSelection();
  svtkSelectionNode* node = selection->GetNode(brushClass);
  while (!node)
  {
    svtkSmartPointer<svtkSelectionNode> newnode = svtkSmartPointer<svtkSelectionNode>::New();
    newnode->GetProperties()->Set(svtkSelectionNode::CONTENT_TYPE(), svtkSelectionNode::PEDIGREEIDS);
    newnode->GetProperties()->Set(svtkSelectionNode::FIELD_TYPE(), svtkSelectionNode::ROW);
    selection->AddNode(newnode);

    // initialize the selection data
    svtkSmartPointer<svtkIdTypeArray> selectedIds = svtkSmartPointer<svtkIdTypeArray>::New();
    newnode->SetSelectionList(selectedIds);

    // initialize everything for drawing the selection
    svtkSmartPointer<svtkPolyData> polyData = svtkSmartPointer<svtkPolyData>::New();
    svtkSmartPointer<svtkActor2D> actor = svtkSmartPointer<svtkActor2D>::New();
    svtkSmartPointer<svtkPolyDataMapper2D> mapper = svtkSmartPointer<svtkPolyDataMapper2D>::New();
    mapper.TakeReference(this->InitializePlotMapper(polyData, actor));

    this->I->SelectionData.push_back(polyData);
    this->I->SelectionMappers.push_back(mapper);
    this->I->SelectionActors.push_back(actor);

    this->AddPropOnNextRender(actor);

    node = selection->GetNode(brushClass);
  }

  svtkIdTypeArray* oldSelectedIds = svtkArrayDownCast<svtkIdTypeArray>(node->GetSelectionList());

  // no selection list yet? that shouldn't be possible...it was allocated above
  if (!oldSelectedIds)
  {
    return;
  }

  svtkSmartPointer<svtkIdTypeArray> outSelectedIds = svtkSmartPointer<svtkIdTypeArray>::New();

  int numOldIds = oldSelectedIds->GetNumberOfTuples();
  int numNewIds = newSelectedIds->GetNumberOfTuples();
  switch (brushOperator)
  {
    case svtkParallelCoordinatesView::SVTK_BRUSHOPERATOR_ADD:
      // add all of the old ones, clobbering the class if it's in the new array
      for (int i = 0; i < numOldIds; i++)
      {
        outSelectedIds->InsertNextValue(oldSelectedIds->GetValue(i));
      }

      // add all of the new ones, as long as they aren't in the old array
      for (int i = 0; i < numNewIds; i++)
      {
        if (oldSelectedIds->LookupValue(newSelectedIds->GetValue(i)) == -1)
        {
          outSelectedIds->InsertNextValue(newSelectedIds->GetValue(i));
        }
      }
      break;

    case svtkParallelCoordinatesView::SVTK_BRUSHOPERATOR_SUBTRACT:
      // if an old id is in the new array and it has the current brush class, skip it
      for (int i = 0; i < numOldIds; i++)
      {
        if (newSelectedIds->LookupValue(oldSelectedIds->GetValue(i)) == -1)
        {
          outSelectedIds->InsertNextValue(oldSelectedIds->GetValue(i));
        }
      }
      break;

    case svtkParallelCoordinatesView::SVTK_BRUSHOPERATOR_INTERSECT:
      // if an old id isn't in the new array and has the current brush class, skip it
      for (int i = 0; i < numOldIds; i++)
      {
        if (newSelectedIds->LookupValue(oldSelectedIds->GetValue(i)) >= 0)
        {
          outSelectedIds->InsertNextValue(oldSelectedIds->GetValue(i));
        }
      }

      break;

    case svtkParallelCoordinatesView::SVTK_BRUSHOPERATOR_REPLACE:
      // add all of the new ones,
      for (int i = 0; i < numNewIds; i++)
      {
        outSelectedIds->InsertNextValue(newSelectedIds->GetValue(i));
      }
      break;
  }

  svtkSortDataArray::Sort(outSelectedIds);
  node->SetSelectionList(outSelectedIds);

  this->BuildInverseSelection();

  this->Modified();
  this->UpdateSelection(selection);
}

//------------------------------------------------------------------------------
//
void svtkParallelCoordinatesRepresentation::BuildInverseSelection()
{
  svtkSelection* selection = this->GetAnnotationLink()->GetCurrentSelection();

  this->InverseSelection->RemoveAllNodes();

  int numNodes = selection->GetNumberOfNodes();
  if (numNodes <= 0)
  {
    return;
  }

  svtkSmartPointer<svtkIdTypeArray> unselected = svtkSmartPointer<svtkIdTypeArray>::New();
  std::vector<int> idxs(numNodes, 0);

  for (int i = 0; i < this->NumberOfSamples; i++)
  {
    bool found = false;
    for (int j = 0; j < numNodes; j++)
    {

      svtkIdTypeArray* a =
        svtkArrayDownCast<svtkIdTypeArray>(selection->GetNode(j)->GetSelectionList());
      if (!a || idxs[j] >= a->GetNumberOfTuples())
      {
        continue;
      }

      int numRows = a->GetNumberOfTuples();
      while (idxs[j] < numRows && a->GetValue(idxs[j]) < i)
      {
        idxs[j]++;
      }

      if (idxs[j] < numRows && a->GetValue(idxs[j]) == i)
      {
        found = true;
        break;
      }
    }

    if (!found)
    {
      unselected->InsertNextValue(i);
    }
  }

  svtkSmartPointer<svtkSelectionNode> totalSelection = svtkSmartPointer<svtkSelectionNode>::New();
  totalSelection->SetSelectionList(unselected);

  if (unselected->GetNumberOfTuples())
    this->InverseSelection->AddNode(totalSelection);
}

//------------------------------------------------------------------------------
// get the value range of an axis
int svtkParallelCoordinatesRepresentation::GetRangeAtPosition(int position, double range[2])
{
  if (position < 0 || position >= this->NumberOfAxes)
  {
    return -1;
  }

  //  int axis = this->AxisOrder[position];
  range[0] = this->Mins[position] + this->MinOffsets[position];
  range[1] = this->Maxs[position] + this->MaxOffsets[position];

  return 1;
}

//------------------------------------------------------------------------------
// set the value range of an axis
int svtkParallelCoordinatesRepresentation::SetRangeAtPosition(int position, double range[2])
{
  if (position < 0 || position >= this->NumberOfAxes)
  {
    return -1;
  }

  //  int axis = this->AxisOrder[position];
  this->MinOffsets[position] = range[0] - this->Mins[position];
  this->MaxOffsets[position] = range[1] - this->Maxs[position];
  this->Modified();
  return 1;
}

//------------------------------------------------------------------------------
void svtkParallelCoordinatesRepresentation::ResetAxes()
{
  this->YMin = .1;
  this->YMax = .9;

  for (int i = 0; i < this->NumberOfAxes; i++)
    this->RemovePropOnNextRender(this->Axes[i]);

  this->ReallocateInternals();

  this->GetInput()->Modified();

  this->Modified();
  this->Update();
}

//------------------------------------------------------------------------------
// get position and size of the entire plot
int svtkParallelCoordinatesRepresentation::GetPositionAndSize(double* position, double* size)
{
  if (!this->Xs)
    return 0;

  position[0] = this->Xs[0];
  position[1] = this->YMin;

  size[0] = this->Xs[this->NumberOfAxes - 1] - this->Xs[0];
  size[1] = this->YMax - this->YMin;
  return 1;
}

//------------------------------------------------------------------------------
// set position and size of the entire plot
int svtkParallelCoordinatesRepresentation::SetPositionAndSize(double* position, double* size)
{
  // rescale the Xs so that they fit into the range prescribed by position and size
  double oldPos[2], oldSize[2];
  oldPos[0] = oldPos[1] = oldSize[0] = oldSize[1] = 0.;
  this->GetPositionAndSize(oldPos, oldSize);

  for (int i = 0; i < this->NumberOfAxes; i++)
  {
    this->Xs[i] = position[0] + size[0] * (this->Xs[i] - oldPos[0]) / oldSize[0];
  }

  this->YMin = position[1];
  this->YMax = position[1] + size[1];

  this->Modified();
  return 1;
}

//------------------------------------------------------------------------------
svtkPolyDataMapper2D* svtkParallelCoordinatesRepresentation::InitializePlotMapper(
  svtkPolyData* input, svtkActor2D* actor, bool svtkNotUsed(forceStandard))
{
  svtkPolyDataMapper2D* mapper = svtkPolyDataMapper2D::New();

  // this tells all the mappers to use the normalized viewport coordinate system
  svtkSmartPointer<svtkCoordinate> dummyCoord = svtkSmartPointer<svtkCoordinate>::New();
  dummyCoord->SetCoordinateSystemToNormalizedViewport();

  mapper->SetInputData(input);
  mapper->SetTransformCoordinate(dummyCoord);
  mapper->ScalarVisibilityOff();
  actor->SetMapper(mapper);

  return mapper;
}
//------------------------------------------------------------------------------
svtkPolyDataMapper2D* svtkParallelCoordinatesRepresentation::GetSelectionMapper(int idx)
{
  if (idx >= 0 && idx < (int)this->I->SelectionMappers.size())
  {
    return this->I->SelectionMappers[idx];
  }
  else
  {
    return nullptr;
  }
}
//------------------------------------------------------------------------------
int svtkParallelCoordinatesRepresentation::GetNumberOfSelections()
{
  return (int)this->I->SelectionActors.size();
}
