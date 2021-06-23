/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkScatterPlotMatrix.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkScatterPlotMatrix.h"

#include "svtkAnnotationLink.h"
#include "svtkAxis.h"
#include "svtkBrush.h"
#include "svtkCallbackCommand.h"
#include "svtkChartXY.h"
#include "svtkChartXYZ.h"
#include "svtkCommand.h"
#include "svtkContext2D.h"
#include "svtkContextMouseEvent.h"
#include "svtkContextScene.h"
#include "svtkFloatArray.h"
#include "svtkIntArray.h"
#include "svtkMathUtilities.h"
#include "svtkNew.h"
#include "svtkObjectFactory.h"
#include "svtkPen.h"
#include "svtkPlot.h"
#include "svtkPlotPoints.h"
#include "svtkPlotPoints3D.h"
#include "svtkPoints2D.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkStdString.h"
#include "svtkStringArray.h"
#include "svtkTable.h"
#include "svtkTextProperty.h"
#include "svtkTooltipItem.h"
#include "svtkVectorOperators.h"

// STL includes
#include <algorithm>
#include <cassert>
#include <map>
#include <vector>

class svtkScatterPlotMatrix::PIMPL
{
public:
  PIMPL()
    : VisibleColumnsModified(true)
    , BigChart(nullptr)
    , BigChartPos(0, 0)
    , ResizingBigChart(false)
    , AnimationCallbackInitialized(false)
    , TimerId(0)
    , TimerCallbackInitialized(false)
  {
    pimplChartSetting* scatterplotSettings = new pimplChartSetting();
    scatterplotSettings->BackgroundBrush->SetColor(255, 255, 255, 255);
    this->ChartSettings[svtkScatterPlotMatrix::SCATTERPLOT] = scatterplotSettings;
    pimplChartSetting* histogramSettings = new pimplChartSetting();
    histogramSettings->BackgroundBrush->SetColor(127, 127, 127, 102);
    histogramSettings->PlotPen->SetColor(255, 255, 255, 255);
    histogramSettings->ShowAxisLabels = true;
    this->ChartSettings[svtkScatterPlotMatrix::HISTOGRAM] = histogramSettings;
    pimplChartSetting* activeplotSettings = new pimplChartSetting();
    activeplotSettings->BackgroundBrush->SetColor(255, 255, 255, 255);
    activeplotSettings->ShowAxisLabels = true;
    this->ChartSettings[svtkScatterPlotMatrix::ACTIVEPLOT] = activeplotSettings;
    activeplotSettings->MarkerSize = 8.0;
    this->SelectedChartBGBrush->SetColor(0, 204, 0, 102);
    this->SelectedRowColumnBGBrush->SetColor(204, 0, 0, 102);
    this->TooltipItem = svtkSmartPointer<svtkTooltipItem>::New();
  }

  ~PIMPL()
  {
    delete this->ChartSettings[svtkScatterPlotMatrix::SCATTERPLOT];
    delete this->ChartSettings[svtkScatterPlotMatrix::HISTOGRAM];
    delete this->ChartSettings[svtkScatterPlotMatrix::ACTIVEPLOT];
  }

  // Store columns settings such as axis range, title, number of tick marks.
  class ColumnSetting
  {
  public:
    ColumnSetting()
      : min(0)
      , max(0)
      , nTicks(0)
      , title("?!?")
    {
    }

    double min;
    double max;
    int nTicks;
    std::string title;
  };

  class pimplChartSetting
  {
  public:
    pimplChartSetting()
    {
      this->PlotPen->SetColor(0, 0, 0, 255);
      this->MarkerStyle = svtkPlotPoints::CIRCLE;
      this->MarkerSize = 3.0;
      this->AxisColor.Set(0, 0, 0, 255);
      this->GridColor.Set(242, 242, 242, 255);
      this->LabelNotation = svtkAxis::STANDARD_NOTATION;
      this->LabelPrecision = 2;
      this->TooltipNotation = svtkAxis::STANDARD_NOTATION;
      this->TooltipPrecision = 2;
      this->ShowGrid = true;
      this->ShowAxisLabels = false;
      this->LabelFont = svtkSmartPointer<svtkTextProperty>::New();
      this->LabelFont->SetFontFamilyToArial();
      this->LabelFont->SetFontSize(12);
      this->LabelFont->SetColor(0.0, 0.0, 0.0);
      this->LabelFont->SetOpacity(1.0);
    }
    ~pimplChartSetting() = default;

    int MarkerStyle;
    float MarkerSize;
    svtkColor4ub AxisColor;
    svtkColor4ub GridColor;
    int LabelNotation;
    int LabelPrecision;
    int TooltipNotation;
    int TooltipPrecision;
    bool ShowGrid;
    bool ShowAxisLabels;
    svtkSmartPointer<svtkTextProperty> LabelFont;
    svtkNew<svtkBrush> BackgroundBrush;
    svtkNew<svtkPen> PlotPen;
    svtkNew<svtkBrush> PlotBrush;
  };

  void UpdateAxis(svtkAxis* axis, pimplChartSetting* setting, bool updateLabel = true)
  {
    if (axis && setting)
    {
      axis->GetPen()->SetColor(setting->AxisColor);
      axis->GetGridPen()->SetColor(setting->GridColor);
      axis->SetGridVisible(setting->ShowGrid);
      if (updateLabel)
      {
        svtkTextProperty* prop = setting->LabelFont;
        axis->SetNotation(setting->LabelNotation);
        axis->SetPrecision(setting->LabelPrecision);
        axis->SetLabelsVisible(setting->ShowAxisLabels);
        axis->GetLabelProperties()->SetFontSize(prop->GetFontSize());
        axis->GetLabelProperties()->SetColor(prop->GetColor());
        axis->GetLabelProperties()->SetOpacity(prop->GetOpacity());
        axis->GetLabelProperties()->SetFontFamilyAsString(prop->GetFontFamilyAsString());
        axis->GetLabelProperties()->SetBold(prop->GetBold());
        axis->GetLabelProperties()->SetItalic(prop->GetItalic());
      }
    }
  }

  void UpdateChart(svtkChart* chart, pimplChartSetting* setting)
  {
    if (chart && setting)
    {
      svtkPlot* plot = chart->GetPlot(0);
      if (plot)
      {
        plot->SetTooltipNotation(setting->TooltipNotation);
        plot->SetTooltipPrecision(setting->TooltipPrecision);
      }
    }
  }

  svtkNew<svtkTable> Histogram;
  bool VisibleColumnsModified;
  svtkWeakPointer<svtkChart> BigChart;
  svtkVector2i BigChartPos;
  bool ResizingBigChart;
  svtkNew<svtkAnnotationLink> Link;

  // Settings for the charts in the scatter plot matrix.
  std::map<int, pimplChartSetting*> ChartSettings;
  typedef std::map<int, pimplChartSetting*>::iterator chartIterator;

  // Axis ranges for the columns in the scatter plot matrix.
  std::map<std::string, ColumnSetting> ColumnSettings;

  svtkNew<svtkBrush> SelectedRowColumnBGBrush;
  svtkNew<svtkBrush> SelectedChartBGBrush;
  std::vector<svtkVector2i> AnimationPath;
  std::vector<svtkVector2i>::iterator AnimationIter;
  svtkRenderWindowInteractor* Interactor;
  svtkNew<svtkCallbackCommand> AnimationCallback;
  bool AnimationCallbackInitialized;
  unsigned long int TimerId;
  bool TimerCallbackInitialized;
  int AnimationPhase;
  float CurrentAngle;
  float IncAngle;
  float FinalAngle;
  svtkVector2i NextActivePlot;

  svtkNew<svtkChartXYZ> BigChart3D;
  svtkNew<svtkAxis> TestAxis; // Used to get ranges/number of ticks
  svtkSmartPointer<svtkTooltipItem> TooltipItem;
  svtkSmartPointer<svtkStringArray> IndexedLabelsArray;
};

namespace
{

// This is just here for now - quick and dirty historgram calculations...
bool PopulateHistograms(svtkTable* input, svtkTable* output, svtkStringArray* s, int NumberOfBins)
{
  // The output table will have the twice the number of columns, they will be
  // the x and y for input column. This is the bin centers, and the population.
  for (svtkIdType i = 0; i < s->GetNumberOfTuples(); ++i)
  {
    double minmax[2] = { 0.0, 0.0 };
    svtkStdString name(s->GetValue(i));
    svtkDataArray* in = svtkArrayDownCast<svtkDataArray>(input->GetColumnByName(name.c_str()));
    if (in)
    {
      // The bin values are the centers, extending +/- half an inc either side
      in->GetRange(minmax);
      if (minmax[0] == minmax[1])
      {
        minmax[1] = minmax[0] + 1.0;
      }
      double inc = (minmax[1] - minmax[0]) / (NumberOfBins)*1.001;
      double halfInc = inc / 2.0;
      svtkSmartPointer<svtkFloatArray> extents = svtkArrayDownCast<svtkFloatArray>(
        output->GetColumnByName(svtkStdString(name + "_extents").c_str()));
      if (!extents)
      {
        extents = svtkSmartPointer<svtkFloatArray>::New();
        extents->SetName(svtkStdString(name + "_extents").c_str());
      }
      extents->SetNumberOfTuples(NumberOfBins);
      float* centers = static_cast<float*>(extents->GetVoidPointer(0));
      double min = minmax[0] - 0.0005 * inc + halfInc;
      for (int j = 0; j < NumberOfBins; ++j)
      {
        extents->SetValue(j, min + j * inc);
      }
      svtkSmartPointer<svtkIntArray> populations = svtkArrayDownCast<svtkIntArray>(
        output->GetColumnByName(svtkStdString(name + "_pops").c_str()));
      if (!populations)
      {
        populations = svtkSmartPointer<svtkIntArray>::New();
        populations->SetName(svtkStdString(name + "_pops").c_str());
      }
      populations->SetNumberOfTuples(NumberOfBins);
      int* pops = static_cast<int*>(populations->GetVoidPointer(0));
      for (int k = 0; k < NumberOfBins; ++k)
      {
        pops[k] = 0;
      }
      for (svtkIdType j = 0; j < in->GetNumberOfTuples(); ++j)
      {
        double v(0.0);
        in->GetTuple(j, &v);
        for (int k = 0; k < NumberOfBins; ++k)
        {
          if (svtkMathUtilities::FuzzyCompare(v, double(centers[k]), halfInc))
          {
            ++pops[k];
            break;
          }
        }
      }
      output->AddColumn(extents);
      output->AddColumn(populations);
    }
  }
  return true;
}

bool MoveColumn(svtkStringArray* visCols, int fromCol, int toCol)
{
  if (!visCols || visCols->GetNumberOfTuples() == 0 || fromCol == toCol || fromCol == (toCol - 1) ||
    fromCol < 0 || toCol < 0)
  {
    return false;
  }
  int numCols = visCols->GetNumberOfTuples();
  if (fromCol >= numCols || toCol > numCols)
  {
    return false;
  }

  std::vector<svtkStdString> newVisCols;
  svtkIdType c;
  if (toCol == numCols)
  {
    for (c = 0; c < numCols; c++)
    {
      if (c != fromCol)
      {
        newVisCols.push_back(visCols->GetValue(c));
      }
    }
    // move the fromCol to the end
    newVisCols.push_back(visCols->GetValue(fromCol));
  }
  // insert the fromCol before toCol
  else if (fromCol < toCol)
  {
    // move Cols in the middle up
    for (c = 0; c < fromCol; c++)
    {
      newVisCols.push_back(visCols->GetValue(c));
    }
    for (c = fromCol + 1; c < numCols; c++)
    {
      if (c == toCol)
      {
        newVisCols.push_back(visCols->GetValue(fromCol));
      }
      newVisCols.push_back(visCols->GetValue(c));
    }
  }
  else
  {
    for (c = 0; c < toCol; c++)
    {
      newVisCols.push_back(visCols->GetValue(c));
    }
    newVisCols.push_back(visCols->GetValue(fromCol));
    for (c = toCol; c < numCols; c++)
    {
      if (c != fromCol)
      {
        newVisCols.push_back(visCols->GetValue(c));
      }
    }
  }

  // repopulate the visCols
  svtkIdType visId = 0;
  std::vector<svtkStdString>::iterator arrayIt;
  for (arrayIt = newVisCols.begin(); arrayIt != newVisCols.end(); ++arrayIt)
  {
    visCols->SetValue(visId++, *arrayIt);
  }
  return true;
}
} // End of anonymous namespace

svtkObjectFactoryNewMacro(svtkScatterPlotMatrix);

svtkScatterPlotMatrix::svtkScatterPlotMatrix()
  : NumberOfBins(10)
  , NumberOfFrames(25)
  , LayoutUpdatedTime(0)
{
  this->Private = new PIMPL;
  this->TitleProperties = svtkSmartPointer<svtkTextProperty>::New();
  this->TitleProperties->SetFontSize(12);
  this->SelectionMode = svtkContextScene::SELECTION_NONE;
  this->ActivePlot = svtkVector2i(0, -2);
  this->ActivePlotValid = false;
  this->Animating = false;
}

svtkScatterPlotMatrix::~svtkScatterPlotMatrix()
{
  delete this->Private;
}

void svtkScatterPlotMatrix::Update()
{
  if (this->Private->VisibleColumnsModified)
  {
    // We need to handle layout changes due to modified visibility.
    // Build up our histograms data before updating the layout.
    PopulateHistograms(
      this->Input, this->Private->Histogram, this->VisibleColumns, this->NumberOfBins);
    this->UpdateLayout();
    this->Private->VisibleColumnsModified = false;
  }
  else if (this->GetMTime() > this->LayoutUpdatedTime)
  {
    this->UpdateLayout();
  }
}

bool svtkScatterPlotMatrix::Paint(svtkContext2D* painter)
{
  this->CurrentPainter = painter;
  this->Update();
  bool ret = this->Superclass::Paint(painter);
  this->ResizeBigChart();

  if (this->Title)
  {
    // As the BigPlot can take some spaces on the top of the chart
    // we draw the title on the bottom where there is always room for it.
    svtkNew<svtkPoints2D> rect;
    rect->InsertNextPoint(0, 0);
    rect->InsertNextPoint(this->GetScene()->GetSceneWidth(), 10);
    painter->ApplyTextProp(this->TitleProperties);
    painter->DrawStringRect(rect, this->Title);
  }

  return ret;
}

void svtkScatterPlotMatrix::SetScene(svtkContextScene* scene)
{
  // The internal axis shouldn't be a child as it isn't rendered with the
  // chart, but it does need access to the scene.
  this->Private->TestAxis->SetScene(scene);

  this->Superclass::SetScene(scene);
}

bool svtkScatterPlotMatrix::SetActivePlot(const svtkVector2i& pos)
{
  if (pos.GetX() + pos.GetY() + 1 < this->Size.GetX() && pos.GetX() < this->Size.GetX() &&
    pos.GetY() < this->Size.GetY())
  {
    // The supplied index is valid (in the lower quadrant).
    this->ActivePlot = pos;
    this->ActivePlotValid = true;

    // Invoke an interaction event, to let observers know something changed.
    this->InvokeEvent(svtkCommand::AnnotationChangedEvent);

    // set background colors for plots
    if (this->GetChart(this->ActivePlot)->GetPlot(0))
    {
      int plotCount = this->GetSize().GetX();
      for (int i = 0; i < plotCount; ++i)
      {
        for (int j = 0; j < plotCount; ++j)
        {
          if (this->GetPlotType(i, j) == SCATTERPLOT)
          {
            svtkChartXY* chart = svtkChartXY::SafeDownCast(this->GetChart(svtkVector2i(i, j)));

            if (pos[0] == i && pos[1] == j)
            {
              // set the new active chart background color to light green
              chart->SetBackgroundBrush(this->Private->SelectedChartBGBrush);
            }
            else if (pos[0] == i || pos[1] == j)
            {
              // set background color for all other charts in the selected
              // chart's row and column to light red
              chart->SetBackgroundBrush(this->Private->SelectedRowColumnBGBrush);
            }
            else
            {
              // set all else to white
              chart->SetBackgroundBrush(this->Private->ChartSettings[SCATTERPLOT]->BackgroundBrush);
            }
          }
        }
      }
    }
    if (this->Private->BigChart)
    {
      svtkPlot* plot = this->Private->BigChart->GetPlot(0);
      svtkStdString column = this->GetColumnName(pos.GetX());
      svtkStdString row = this->GetRowName(pos.GetY());
      if (!plot)
      {
        plot = this->Private->BigChart->AddPlot(svtkChart::POINTS);
        svtkChart* active = this->GetChart(this->ActivePlot);
        svtkChartXY* xy = svtkChartXY::SafeDownCast(this->Private->BigChart);
        if (xy)
        {
          // Set plot corner, and axis visibility
          xy->SetPlotCorner(plot, 2);
          xy->SetAutoAxes(false);
          xy->GetAxis(svtkAxis::TOP)->SetVisible(true);
          xy->GetAxis(svtkAxis::RIGHT)->SetVisible(true);
          xy->GetAxis(svtkAxis::BOTTOM)->SetLabelsVisible(false);
          xy->GetAxis(svtkAxis::BOTTOM)->SetGridVisible(false);
          xy->GetAxis(svtkAxis::BOTTOM)->SetTicksVisible(false);
          xy->GetAxis(svtkAxis::BOTTOM)->SetVisible(true);
          xy->GetAxis(svtkAxis::LEFT)->SetLabelsVisible(false);
          xy->GetAxis(svtkAxis::LEFT)->SetGridVisible(false);
          xy->GetAxis(svtkAxis::LEFT)->SetTicksVisible(false);
          xy->GetAxis(svtkAxis::LEFT)->SetVisible(true);

          // set labels array
          if (this->Private->IndexedLabelsArray)
          {
            plot->SetIndexedLabels(this->Private->IndexedLabelsArray);
            plot->SetTooltipLabelFormat("%i");
          }
        }
        if (xy && active)
        {
          svtkAxis* a = active->GetAxis(svtkAxis::BOTTOM);
          xy->GetAxis(svtkAxis::TOP)
            ->SetUnscaledRange(a->GetUnscaledMinimum(), a->GetUnscaledMaximum());
          a = active->GetAxis(svtkAxis::LEFT);
          xy->GetAxis(svtkAxis::RIGHT)
            ->SetUnscaledRange(a->GetUnscaledMinimum(), a->GetUnscaledMaximum());
        }
      }
      else
      {
        this->Private->BigChart->ClearPlots();
        plot = this->Private->BigChart->AddPlot(svtkChart::POINTS);
        svtkChartXY* xy = svtkChartXY::SafeDownCast(this->Private->BigChart);
        if (xy)
        {
          xy->SetPlotCorner(plot, 2);
        }

        // set labels array
        if (this->Private->IndexedLabelsArray)
        {
          plot->SetIndexedLabels(this->Private->IndexedLabelsArray);
          plot->SetTooltipLabelFormat("%i");
        }
      }
      plot->SetInputData(this->Input, column, row);
      plot->SetPen(this->Private->ChartSettings[ACTIVEPLOT]->PlotPen);
      this->ApplyAxisSetting(this->Private->BigChart, column, row);

      // Set marker size and style.
      svtkPlotPoints* plotPoints = svtkPlotPoints::SafeDownCast(plot);
      plotPoints->SetMarkerSize(this->Private->ChartSettings[ACTIVEPLOT]->MarkerSize);
      plotPoints->SetMarkerStyle(this->Private->ChartSettings[ACTIVEPLOT]->MarkerStyle);

      // Add supplementary plot if any
      this->AddSupplementaryPlot(this->Private->BigChart, ACTIVEPLOT, row, column, 2);

      // Set background color.
      this->Private->BigChart->SetBackgroundBrush(
        this->Private->ChartSettings[ACTIVEPLOT]->BackgroundBrush);
      this->Private->BigChart->GetAxis(svtkAxis::TOP)
        ->SetTitle(this->VisibleColumns->GetValue(pos.GetX()));
      this->Private->BigChart->GetAxis(svtkAxis::RIGHT)
        ->SetTitle(this->VisibleColumns->GetValue(this->GetSize().GetX() - pos.GetY() - 1));
      // Calculate the ideal range.
      // this->Private->BigChart->RecalculateBounds();
    }
    return true;
  }
  else
  {
    return false;
  }
}

svtkVector2i svtkScatterPlotMatrix::GetActivePlot()
{
  return this->ActivePlot;
}

void svtkScatterPlotMatrix::UpdateAnimationPath(const svtkVector2i& newActivePos)
{
  this->Private->AnimationPath.clear();
  if (newActivePos[0] != this->ActivePlot[0] || newActivePos[1] != this->ActivePlot[1])
  {
    if (newActivePos[1] >= this->ActivePlot[1])
    {
      // x direction first
      if (this->ActivePlot[0] > newActivePos[0])
      {
        for (int r = this->ActivePlot[0] - 1; r >= newActivePos[0]; r--)
        {
          this->Private->AnimationPath.push_back(svtkVector2i(r, this->ActivePlot[1]));
        }
      }
      else
      {
        for (int r = this->ActivePlot[0] + 1; r <= newActivePos[0]; r++)
        {
          this->Private->AnimationPath.push_back(svtkVector2i(r, this->ActivePlot[1]));
        }
      }
      // then y direction
      for (int c = this->ActivePlot[1] + 1; c <= newActivePos[1]; c++)
      {
        this->Private->AnimationPath.push_back(svtkVector2i(newActivePos[0], c));
      }
    }
    else
    {
      // y direction first
      for (int c = this->ActivePlot[1] - 1; c >= newActivePos[1]; c--)
      {
        this->Private->AnimationPath.push_back(svtkVector2i(this->ActivePlot[0], c));
      }
      // then x direction
      if (this->ActivePlot[0] > newActivePos[0])
      {
        for (int r = this->ActivePlot[0] - 1; r >= newActivePos[0]; r--)
        {
          this->Private->AnimationPath.push_back(svtkVector2i(r, newActivePos[1]));
        }
      }
      else
      {
        for (int r = this->ActivePlot[0] + 1; r <= newActivePos[0]; r++)
        {
          this->Private->AnimationPath.push_back(svtkVector2i(r, newActivePos[1]));
        }
      }
    }
  }
}

void svtkScatterPlotMatrix::StartAnimation(svtkRenderWindowInteractor* interactor)
{
  // Start a simple repeating timer to advance along the path until completion.
  if (!this->Private->TimerCallbackInitialized && interactor)
  {
    this->Animating = true;
    if (!this->Private->AnimationCallbackInitialized)
    {
      this->Private->AnimationCallback->SetClientData(this);
      this->Private->AnimationCallback->SetCallback(svtkScatterPlotMatrix::ProcessEvents);
      interactor->AddObserver(svtkCommand::TimerEvent, this->Private->AnimationCallback, 0);
      this->Private->Interactor = interactor;
      this->Private->AnimationCallbackInitialized = true;
    }
    this->Private->TimerCallbackInitialized = true;
    // This defines the interval at which the animation will proceed. 25Hz?
    this->Private->TimerId = interactor->CreateRepeatingTimer(1000 / 50);
    this->Private->AnimationIter = this->Private->AnimationPath.begin();
    this->Private->AnimationPhase = 0;
  }
}

void svtkScatterPlotMatrix::AdvanceAnimation()
{
  // The animation has several phases, and we must track where we are.

  // 1: Remove decoration from the big chart.
  // 2: Set three dimensions to plot in the BigChart3D.
  // 3: Make BigChart invisible, and BigChart3D visible.
  // 4: Rotate between the two dimensions we are transitioning between.
  //    -> Loop from start to end angle to complete the effect.
  // 5: Make the new dimensionality active, update BigChart.
  // 5: Make BigChart3D invisible and BigChart visible.
  // 6: Stop the timer.
  this->InvokeEvent(svtkCommand::AnimationCueTickEvent);
  switch (this->Private->AnimationPhase)
  {
    case 0: // Remove decoration from the big chart, load up the 3D chart
    {
      this->Private->NextActivePlot = *this->Private->AnimationIter;
      svtkChartXYZ* chart = this->Private->BigChart3D;
      chart->SetVisible(false);
      chart->SetAutoRotate(true);
      chart->SetDecorateAxes(false);
      chart->SetFitToScene(false);

      int yColumn = this->GetSize().GetY() - this->ActivePlot.GetY() - 1;
      bool isX = false;
      int zColumn = 0;

      svtkRectf size = this->Private->BigChart->GetSize();
      float zSize;
      this->Private->FinalAngle = 90.0;
      this->Private->IncAngle = this->Private->FinalAngle / this->NumberOfFrames;

      if (this->Private->NextActivePlot.GetY() == this->ActivePlot.GetY())
      {
        // Horizontal move.
        zColumn = this->Private->NextActivePlot.GetX();
        isX = false;
        if (this->ActivePlot.GetX() < zColumn)
        {
          this->Private->IncAngle *= 1.0;
          zSize = size.GetWidth();
        }
        else
        {
          this->Private->IncAngle *= -1.0;
          zSize = -size.GetWidth();
        }
      }
      else
      {
        // Vertical move.
        zColumn = this->GetSize().GetY() - this->Private->NextActivePlot.GetY() - 1;
        isX = true;
        if (this->GetSize().GetY() - this->ActivePlot.GetY() - 1 < zColumn)
        {
          this->Private->IncAngle *= -1.0;
          zSize = size.GetHeight();
        }
        else
        {
          this->Private->IncAngle *= 1.0;
          zSize = -size.GetHeight();
        }
      }
      chart->SetAroundX(isX);
      chart->SetGeometry(size);

      svtkStdString names[3];
      names[0] = this->VisibleColumns->GetValue(this->ActivePlot.GetX());
      names[1] = this->VisibleColumns->GetValue(yColumn);
      names[2] = this->VisibleColumns->GetValue(zColumn);

      // Setup the 3D chart
      this->Private->BigChart3D->ClearPlots();
      svtkNew<svtkPlotPoints3D> scatterPlot3D;
      scatterPlot3D->SetInputData(this->Input, names[0], names[1], names[2]);
      this->Private->BigChart3D->AddPlot(scatterPlot3D);

      // Set the z axis up so that it ends in the right orientation.
      chart->GetAxis(2)->SetPoint2(0, zSize);
      // Now set the ranges for the three axes.
      for (int i = 0; i < 3; ++i)
      {
        PIMPL::ColumnSetting& settings = this->Private->ColumnSettings[names[i]];
        chart->GetAxis(i)->SetUnscaledRange(settings.min, settings.max);
      }
      chart->RecalculateTransform();
      this->GetScene()->SetDirty(true);
      ++this->Private->AnimationPhase;
      return;
    }
    case 1: // Make BigChart invisible, and BigChart3D visible.
      this->Private->BigChart->SetVisible(false);
      this->AddItem(this->Private->BigChart3D);
      this->Private->BigChart3D->SetVisible(true);
      this->GetScene()->SetDirty(true);
      ++this->Private->AnimationPhase;
      this->Private->CurrentAngle = 0.0;
      return;
    case 2: // Rotation of the 3D chart from start to end angle.
      if (fabs(this->Private->CurrentAngle) < (this->Private->FinalAngle - 0.001))
      {
        this->Private->CurrentAngle += this->Private->IncAngle;
        this->Private->BigChart3D->SetAngle(this->Private->CurrentAngle);
      }
      else
      {
        ++this->Private->AnimationPhase;
      }
      this->GetScene()->SetDirty(true);
      return;
    case 3: // Transition to new dimensionality, update the big chart.
      this->SetActivePlot(this->Private->NextActivePlot);
      this->Private->BigChart->Update();
      this->GetScene()->SetDirty(true);
      ++this->Private->AnimationPhase;
      break;
    case 4:
      this->GetScene()->SetDirty(true);
      ++this->Private->AnimationIter;
      // Clean up - we are done.
      this->Private->AnimationPhase = 0;
      if (this->Private->AnimationIter == this->Private->AnimationPath.end())
      {
        this->Private->BigChart->SetVisible(true);
        this->RemoveItem(this->Private->BigChart3D);
        this->Private->BigChart3D->SetVisible(false);
        this->Private->Interactor->DestroyTimer(this->Private->TimerId);
        this->Private->TimerId = 0;
        this->Private->TimerCallbackInitialized = false;
        this->Animating = false;

        // Make sure the active plot is redrawn completely after the animation
        this->Modified();
        this->ActivePlotValid = false;
        this->Update();
      }
  }
}

void svtkScatterPlotMatrix::ProcessEvents(
  svtkObject*, unsigned long event, void* clientData, void* callerData)
{
  svtkScatterPlotMatrix* self = reinterpret_cast<svtkScatterPlotMatrix*>(clientData);
  switch (event)
  {
    case svtkCommand::TimerEvent:
    {
      // We must filter the events to ensure we actually get the timer event we
      // created. I would love signals and slots...
      int timerId = *reinterpret_cast<int*>(callerData); // Seems to work.
      if (self->Private->TimerCallbackInitialized &&
        timerId == static_cast<int>(self->Private->TimerId))
      {
        self->AdvanceAnimation();
      }
      break;
    }
    default:
      break;
  }
}

svtkAnnotationLink* svtkScatterPlotMatrix::GetAnnotationLink()
{
  return this->Private->Link;
}

void svtkScatterPlotMatrix::SetInput(svtkTable* table)
{
  if (table && table->GetNumberOfRows() == 0)
  {
    // do nothing if the table is empty
    return;
  }

  if (this->Input != table)
  {
    // Set the input, then update the size of the scatter plot matrix, set
    // their inputs and all the other stuff needed.
    this->Input = table;
    this->SetSize(svtkVector2i(0, 0));
    this->Modified();

    if (table == nullptr)
    {
      this->SetColumnVisibilityAll(true);
      return;
    }
    int n = static_cast<int>(this->Input->GetNumberOfColumns());
    this->SetColumnVisibilityAll(true);
    this->SetSize(svtkVector2i(n, n));
  }
}

void svtkScatterPlotMatrix::SetColumnVisibility(const svtkStdString& name, bool visible)
{
  if (visible)
  {
    for (svtkIdType i = 0; i < this->VisibleColumns->GetNumberOfTuples(); ++i)
    {
      if (this->VisibleColumns->GetValue(i) == name)
      {
        // Already there, nothing more needs to be done
        return;
      }
    }
    // Add the column to the end of the list if it is a numeric column
    if (this->Input && this->Input->GetColumnByName(name.c_str()) &&
      svtkArrayDownCast<svtkDataArray>(this->Input->GetColumnByName(name.c_str())))
    {
      this->VisibleColumns->InsertNextValue(name);
      this->Private->VisibleColumnsModified = true;
      this->SetSize(svtkVector2i(0, 0));
      this->SetSize(svtkVector2i(
        this->VisibleColumns->GetNumberOfTuples(), this->VisibleColumns->GetNumberOfTuples()));
      this->Modified();
    }
  }
  else
  {
    // Remove the value if present
    for (svtkIdType i = 0; i < this->VisibleColumns->GetNumberOfTuples(); ++i)
    {
      if (this->VisibleColumns->GetValue(i) == name)
      {
        // Move all the later elements down by one, and reduce the size
        while (i < this->VisibleColumns->GetNumberOfTuples() - 1)
        {
          this->VisibleColumns->SetValue(i, this->VisibleColumns->GetValue(i + 1));
          ++i;
        }
        this->VisibleColumns->SetNumberOfTuples(this->VisibleColumns->GetNumberOfTuples() - 1);
        this->SetSize(svtkVector2i(0, 0));
        this->SetSize(svtkVector2i(
          this->VisibleColumns->GetNumberOfTuples(), this->VisibleColumns->GetNumberOfTuples()));
        if (this->ActivePlot.GetX() + this->ActivePlot.GetY() + 1 >=
          this->VisibleColumns->GetNumberOfTuples())
        {
          this->ActivePlot.Set(0, this->VisibleColumns->GetNumberOfTuples() - 1);
        }
        this->Private->VisibleColumnsModified = true;
        this->Modified();
      }
    }
  }
}

void svtkScatterPlotMatrix::InsertVisibleColumn(const svtkStdString& name, int index)
{
  if (!this->Input || !this->Input->GetColumnByName(name.c_str()))
  {
    return;
  }

  // Check if the column is already in the list. If yes,
  // we may need to rearrange the order of the columns.
  svtkIdType currIdx = -1;
  svtkIdType numCols = this->VisibleColumns->GetNumberOfTuples();
  for (svtkIdType i = 0; i < numCols; ++i)
  {
    if (this->VisibleColumns->GetValue(i) == name)
    {
      currIdx = i;
      break;
    }
  }

  if (currIdx > 0 && currIdx == index)
  {
    // This column is already there.
    return;
  }

  if (currIdx < 0)
  {
    this->VisibleColumns->SetNumberOfTuples(numCols + 1);
    if (index >= numCols)
    {
      this->VisibleColumns->SetValue(numCols, name);
    }
    else // move all the values after index down 1
    {
      svtkIdType startidx = numCols;
      svtkIdType idx = (index < 0) ? 0 : index;
      while (startidx > idx)
      {
        this->VisibleColumns->SetValue(startidx, this->VisibleColumns->GetValue(startidx - 1));
        startidx--;
      }
      this->VisibleColumns->SetValue(idx, name);
    }
    this->Private->VisibleColumnsModified = true;
  }
  else // need to rearrange table columns
  {
    svtkIdType toIdx = (index < 0) ? 0 : index;
    toIdx = toIdx > numCols ? numCols : toIdx;
    this->Private->VisibleColumnsModified = MoveColumn(this->VisibleColumns, currIdx, toIdx);
  }
  this->LayoutIsDirty = true;
}

bool svtkScatterPlotMatrix::GetColumnVisibility(const svtkStdString& name)
{
  for (svtkIdType i = 0; i < this->VisibleColumns->GetNumberOfTuples(); ++i)
  {
    if (this->VisibleColumns->GetValue(i) == name)
    {
      return true;
    }
  }
  return false;
}

void svtkScatterPlotMatrix::SetColumnVisibilityAll(bool visible)
{
  if (visible && this->Input)
  {
    svtkIdType n = this->Input->GetNumberOfColumns();
    this->VisibleColumns->SetNumberOfTuples(n);
    for (svtkIdType i = 0; i < n; ++i)
    {
      this->VisibleColumns->SetValue(i, this->Input->GetColumnName(i));
    }
  }
  else
  {
    this->SetSize(svtkVector2i(0, 0));
    this->VisibleColumns->SetNumberOfTuples(0);
  }

  this->Private->VisibleColumnsModified = true;
}

svtkStringArray* svtkScatterPlotMatrix::GetVisibleColumns()
{
  return this->VisibleColumns;
}

void svtkScatterPlotMatrix::SetVisibleColumns(svtkStringArray* visColumns)
{
  if (!visColumns || visColumns->GetNumberOfTuples() == 0)
  {
    this->SetSize(svtkVector2i(0, 0));
    this->VisibleColumns->SetNumberOfTuples(0);
  }
  else
  {
    this->VisibleColumns->SetNumberOfTuples(visColumns->GetNumberOfTuples());
    this->VisibleColumns->DeepCopy(visColumns);
  }
  this->Private->VisibleColumnsModified = true;
  this->LayoutIsDirty = true;
}

void svtkScatterPlotMatrix::SetNumberOfBins(int numberOfBins)
{
  if (this->NumberOfBins != numberOfBins)
  {
    this->NumberOfBins = numberOfBins;
    if (this->Input)
    {
      PopulateHistograms(
        this->Input, this->Private->Histogram, this->VisibleColumns, this->NumberOfBins);
    }
    this->Modified();
  }
}

void svtkScatterPlotMatrix::SetPlotColor(int plotType, const svtkColor4ub& color)
{
  if (plotType >= 0 && plotType < NOPLOT)
  {
    if (plotType == ACTIVEPLOT || plotType == SCATTERPLOT)
    {
      this->Private->ChartSettings[plotType]->PlotPen->SetColor(color);
    }
    else
    {
      this->Private->ChartSettings[HISTOGRAM]->PlotBrush->SetColor(color);
    }
    this->Modified();
  }
}

void svtkScatterPlotMatrix::SetPlotMarkerStyle(int plotType, int style)
{
  if (plotType >= 0 && plotType < NOPLOT &&
    style != this->Private->ChartSettings[plotType]->MarkerStyle)
  {
    this->Private->ChartSettings[plotType]->MarkerStyle = style;

    if (plotType == ACTIVEPLOT)
    {
      svtkChart* chart = this->Private->BigChart;
      if (chart)
      {
        svtkPlotPoints* plot = svtkPlotPoints::SafeDownCast(chart->GetPlot(0));
        if (plot)
        {
          plot->SetMarkerStyle(style);
        }
      }
      this->Modified();
    }
    else if (plotType == SCATTERPLOT)
    {
      int plotCount = this->GetSize().GetX();
      for (int i = 0; i < plotCount - 1; ++i)
      {
        for (int j = 0; j < plotCount - 1; ++j)
        {
          if (this->GetPlotType(i, j) == SCATTERPLOT && this->GetChart(svtkVector2i(i, j)))
          {
            svtkChart* chart = this->GetChart(svtkVector2i(i, j));
            svtkPlotPoints* plot = svtkPlotPoints::SafeDownCast(chart->GetPlot(0));
            if (plot)
            {
              plot->SetMarkerStyle(style);
            }
          }
        }
      }
      this->Modified();
    }
  }
}

void svtkScatterPlotMatrix::SetPlotMarkerSize(int plotType, float size)
{
  if (plotType >= 0 && plotType < NOPLOT &&
    size != this->Private->ChartSettings[plotType]->MarkerSize)
  {
    this->Private->ChartSettings[plotType]->MarkerSize = size;

    if (plotType == ACTIVEPLOT)
    {
      // update marker size on current active plot
      svtkChart* chart = this->Private->BigChart;
      if (chart)
      {
        svtkPlotPoints* plot = svtkPlotPoints::SafeDownCast(chart->GetPlot(0));
        if (plot)
        {
          plot->SetMarkerSize(size);
        }
      }
      this->Modified();
    }
    else if (plotType == SCATTERPLOT)
    {
      int plotCount = this->GetSize().GetX();

      for (int i = 0; i < plotCount - 1; i++)
      {
        for (int j = 0; j < plotCount - 1; j++)
        {
          if (this->GetPlotType(i, j) == SCATTERPLOT && this->GetChart(svtkVector2i(i, j)))
          {
            svtkChart* chart = this->GetChart(svtkVector2i(i, j));
            svtkPlotPoints* plot = svtkPlotPoints::SafeDownCast(chart->GetPlot(0));
            if (plot)
            {
              plot->SetMarkerSize(size);
            }
          }
        }
      }
      this->Modified();
    }
  }
}

bool svtkScatterPlotMatrix::Hit(const svtkContextMouseEvent&)
{
  return true;
}

bool svtkScatterPlotMatrix::MouseMoveEvent(const svtkContextMouseEvent&)
{
  // Eat the event, don't do anything for now...
  return true;
}

bool svtkScatterPlotMatrix::MouseButtonPressEvent(const svtkContextMouseEvent&)
{
  return true;
}

bool svtkScatterPlotMatrix::MouseButtonReleaseEvent(const svtkContextMouseEvent& mouse)
{
  // Check we are not currently already animating
  if (this->Private->TimerCallbackInitialized)
  {
    return true;
  }

  // Work out which scatter plot was clicked - make that one the active plot.
  svtkVector2i pos = this->GetChartIndex(mouse.GetPos());

  if (pos.GetX() == -1 || pos.GetX() + pos.GetY() + 1 >= this->Size.GetX())
  {
    // We didn't click a chart in the bottom-left triangle of the matrix.
    return true;
  }

  // If the left button was used, hyperjump, if the right was used full path.
  if (mouse.GetButton() == svtkContextMouseEvent::LEFT_BUTTON)
  {
    if (this->NumberOfFrames == 0)
    {
      this->SetActivePlot(pos);
      return true;
    }
    this->Private->AnimationPath.clear();
    bool horizontalFirst = pos[0] > this->ActivePlot[0] ? false : true;
    if (horizontalFirst)
    {
      if (pos[0] != this->ActivePlot[0])
      {
        this->Private->AnimationPath.push_back(svtkVector2i(pos[0], this->ActivePlot[1]));
      }
    }
    else
    {
      if (pos[1] != this->ActivePlot[1])
      {
        this->Private->AnimationPath.push_back(svtkVector2i(this->ActivePlot[0], pos[1]));
      }
    }
    if ((this->Private->AnimationPath.size() == 1 && this->Private->AnimationPath.back() != pos) ||
      (this->Private->AnimationPath.empty() && this->ActivePlot != pos))
    {
      this->Private->AnimationPath.push_back(pos);
    }
    if (!this->Private->AnimationPath.empty())
    {
      this->InvokeEvent(svtkCommand::CreateTimerEvent);
      this->StartAnimation(mouse.GetInteractor());
    }
  }
  else if (mouse.GetButton() == svtkContextMouseEvent::RIGHT_BUTTON)
  {
    if (this->NumberOfFrames == 0)
    {
      this->SetActivePlot(pos);
      return true;
    }
    this->UpdateAnimationPath(pos);
    if (!this->Private->AnimationPath.empty())
    {
      this->InvokeEvent(svtkCommand::CreateTimerEvent);
      this->StartAnimation(mouse.GetInteractor());
    }
    else
    {
      this->SetActivePlot(pos);
    }
  }

  return true;
}

void svtkScatterPlotMatrix::SetNumberOfFrames(int frames)
{
  this->NumberOfFrames = frames;
}

int svtkScatterPlotMatrix::GetNumberOfFrames()
{
  return this->NumberOfFrames;
}

void svtkScatterPlotMatrix::ClearAnimationPath()
{
  this->Private->AnimationPath.clear();
}

svtkIdType svtkScatterPlotMatrix::GetNumberOfAnimationPathElements()
{
  return static_cast<svtkIdType>(this->Private->AnimationPath.size());
}

svtkVector2i svtkScatterPlotMatrix::GetAnimationPathElement(svtkIdType i)
{
  return this->Private->AnimationPath.at(i);
}

bool svtkScatterPlotMatrix::AddAnimationPath(const svtkVector2i& move)
{
  svtkVector2i pos = this->ActivePlot;
  if (!this->Private->AnimationPath.empty())
  {
    pos = this->Private->AnimationPath.back();
  }
  if (move.GetX() != pos.GetX() && move.GetY() != pos.GetY())
  {
    // Can only move in x or y, not both. Do not append the element.
    return false;
  }
  else
  {
    this->Private->AnimationPath.push_back(move);
    return true;
  }
}

bool svtkScatterPlotMatrix::BeginAnimationPath(svtkRenderWindowInteractor* interactor)
{
  if (interactor && !this->Private->AnimationPath.empty())
  {
    this->StartAnimation(interactor);
    return true;
  }
  else
  {
    return false;
  }
}

int svtkScatterPlotMatrix::GetPlotType(const svtkVector2i& pos)
{
  int plotCount = this->GetSize().GetX();

  if (pos.GetX() + pos.GetY() + 1 < plotCount)
  {
    return SCATTERPLOT;
  }
  else if (pos.GetX() + pos.GetY() + 1 == plotCount)
  {
    return HISTOGRAM;
  }
  else if (pos.GetX() == pos.GetY() &&
    pos.GetX() == static_cast<int>(plotCount / 2.0) + plotCount % 2)
  {
    return ACTIVEPLOT;
  }
  else
  {
    return NOPLOT;
  }
}

int svtkScatterPlotMatrix::GetPlotType(int row, int column)
{
  return this->GetPlotType(svtkVector2i(row, column));
}

void svtkScatterPlotMatrix::UpdateAxes()
{
  if (!this->Input)
  {
    return;
  }
  // We need to iterate through all visible columns and set up the axis ranges.
  svtkAxis* axis(this->Private->TestAxis);
  axis->SetPoint1(0, 0);
  axis->SetPoint2(0, 200);
  for (svtkIdType i = 0; i < this->VisibleColumns->GetNumberOfTuples(); ++i)
  {
    double range[2] = { 0, 0 };
    std::string name(this->VisibleColumns->GetValue(i));
    svtkDataArray* arr = svtkArrayDownCast<svtkDataArray>(this->Input->GetColumnByName(name.c_str()));
    if (arr)
    {
      PIMPL::ColumnSetting settings;
      arr->GetRange(range);
      // Apply a little padding either side of the ranges.
      range[0] = range[0] - (0.01 * range[0]);
      range[1] = range[1] + (0.01 * range[1]);
      axis->SetUnscaledRange(range);
      axis->AutoScale();
      settings.min = axis->GetUnscaledMinimum();
      settings.max = axis->GetUnscaledMaximum();
      settings.nTicks = axis->GetNumberOfTicks();
      settings.title = name;
      this->Private->ColumnSettings[name] = settings;
    }
    else
    {
      svtkDebugMacro(<< "No valid data array available. " << name);
    }
  }
}

svtkStdString svtkScatterPlotMatrix::GetColumnName(int column)
{
  assert(column < this->VisibleColumns->GetNumberOfTuples());
  return this->VisibleColumns->GetValue(column);
}

svtkStdString svtkScatterPlotMatrix::GetRowName(int row)
{
  assert(row < this->VisibleColumns->GetNumberOfTuples());
  return this->VisibleColumns->GetValue(this->Size.GetY() - row - 1);
}

void svtkScatterPlotMatrix::ApplyAxisSetting(
  svtkChart* chart, const svtkStdString& x, const svtkStdString& y)
{
  PIMPL::ColumnSetting& xSettings = this->Private->ColumnSettings[x];
  PIMPL::ColumnSetting& ySettings = this->Private->ColumnSettings[y];
  svtkAxis* axis = chart->GetAxis(svtkAxis::BOTTOM);
  axis->SetUnscaledRange(xSettings.min, xSettings.max);
  axis->SetBehavior(svtkAxis::FIXED);
  axis = chart->GetAxis(svtkAxis::TOP);
  axis->SetUnscaledRange(xSettings.min, xSettings.max);
  axis->SetBehavior(svtkAxis::FIXED);
  axis = chart->GetAxis(svtkAxis::LEFT);
  axis->SetUnscaledRange(ySettings.min, ySettings.max);
  axis->SetBehavior(svtkAxis::FIXED);
  axis = chart->GetAxis(svtkAxis::RIGHT);
  axis->SetUnscaledRange(ySettings.min, ySettings.max);
  axis->SetBehavior(svtkAxis::FIXED);
}

void svtkScatterPlotMatrix::UpdateLayout()
{
  // We want scatter plots on the lower-left triangle, then histograms along
  // the diagonal and a big plot in the top-right. The basic layout is,
  //
  // 3 H   +++
  // 2 S H +++
  // 1 S S H
  // 0 S S S H
  //   0 1 2 3
  //
  // Where the indices are those of the columns. The indices of the charts
  // originate in the bottom-left. S = scatter plot, H = histogram and + is the
  // big chart.
  this->LayoutUpdatedTime = this->GetMTime();
  int n = this->Size.GetX();
  this->UpdateAxes();
  this->Private->BigChart3D->SetAnnotationLink(this->Private->Link);
  for (int i = 0; i < n; ++i)
  {
    svtkStdString column = this->GetColumnName(i);
    for (int j = 0; j < n; ++j)
    {
      svtkStdString row = this->GetRowName(j);
      svtkVector2i pos(i, j);
      if (this->GetPlotType(pos) == SCATTERPLOT)
      {
        svtkChart* chart = this->GetChart(pos);
        this->ApplyAxisSetting(chart, column, row);
        chart->ClearPlots();
        chart->SetInteractive(false);
        chart->SetAnnotationLink(this->Private->Link);
        // Lower-left triangle - scatter plots.
        chart->SetActionToButton(svtkChart::PAN, -1);
        chart->SetActionToButton(svtkChart::ZOOM, -1);
        chart->SetActionToButton(svtkChart::SELECT, -1);
        svtkPlot* plot = chart->AddPlot(svtkChart::POINTS);
        plot->SetInputData(this->Input, column, row);
        plot->SetPen(this->Private->ChartSettings[SCATTERPLOT]->PlotPen);
        // set plot marker size and style
        svtkPlotPoints* plotPoints = svtkPlotPoints::SafeDownCast(plot);
        plotPoints->SetMarkerSize(this->Private->ChartSettings[SCATTERPLOT]->MarkerSize);
        plotPoints->SetMarkerStyle(this->Private->ChartSettings[SCATTERPLOT]->MarkerStyle);
        this->AddSupplementaryPlot(chart, SCATTERPLOT, row, column);
      }
      else if (this->GetPlotType(pos) == HISTOGRAM)
      {
        // We are on the diagonal - need a histogram plot.
        svtkChart* chart = this->GetChart(pos);
        chart->SetInteractive(false);
        this->ApplyAxisSetting(chart, column, row);
        chart->ClearPlots();
        svtkPlot* plot = chart->AddPlot(svtkChart::BAR);
        plot->SetPen(this->Private->ChartSettings[HISTOGRAM]->PlotPen);
        plot->SetBrush(this->Private->ChartSettings[HISTOGRAM]->PlotBrush);
        svtkStdString name(this->VisibleColumns->GetValue(i));
        plot->SetInputData(this->Private->Histogram, name + "_extents", name + "_pops");
        svtkAxis* axis = chart->GetAxis(svtkAxis::TOP);
        axis->SetTitle(name);
        axis->SetLabelsVisible(false);
        // Show the labels on the right for populations of bins.
        axis = chart->GetAxis(svtkAxis::RIGHT);
        axis->SetLabelsVisible(true);
        axis->SetBehavior(svtkAxis::AUTO);
        axis->AutoScale();
        // Set the plot corner to the top-right
        svtkChartXY* xy = svtkChartXY::SafeDownCast(chart);
        if (xy)
        {
          xy->SetBarWidthFraction(1.0);
          xy->SetPlotCorner(plot, 2);
        }

        // set background color to light gray
        xy->SetBackgroundBrush(this->Private->ChartSettings[HISTOGRAM]->BackgroundBrush);
      }
      else if (this->GetPlotType(pos) == ACTIVEPLOT)
      {
        // This big plot in the top-right
        this->Private->BigChart = this->GetChart(pos);
        this->Private->BigChartPos = pos;
        this->Private->BigChart->SetAnnotationLink(this->Private->Link);
        this->Private->BigChart->AddObserver(svtkCommand::SelectionChangedEvent, this,
          &svtkScatterPlotMatrix::BigChartSelectionCallback);

        // set tooltip item
        svtkChartXY* chartXY = svtkChartXY::SafeDownCast(this->Private->BigChart);
        if (chartXY)
        {
          chartXY->SetTooltip(this->Private->TooltipItem);
        }

        this->SetChartSpan(pos, svtkVector2i(n - i, n - j));
        if (!this->ActivePlotValid)
        {
          if (this->ActivePlot.GetY() < 0)
          {
            this->ActivePlot = svtkVector2i(0, n - 2);
          }
          this->SetActivePlot(this->ActivePlot);
        }
      }
      // Only show bottom axis label for bottom plots
      if (j > 0)
      {
        svtkAxis* axis = this->GetChart(pos)->GetAxis(svtkAxis::BOTTOM);
        axis->SetTitle("");
        axis->SetLabelsVisible(false);
        axis->SetBehavior(svtkAxis::FIXED);
      }
      else
      {
        svtkAxis* axis = this->GetChart(pos)->GetAxis(svtkAxis::BOTTOM);
        axis->SetTitle(this->VisibleColumns->GetValue(i));
        axis->SetLabelsVisible(false);
        this->AttachAxisRangeListener(axis);
      }
      // Only show the left axis labels for left-most plots
      if (i > 0)
      {
        svtkAxis* axis = this->GetChart(pos)->GetAxis(svtkAxis::LEFT);
        axis->SetTitle("");
        axis->SetLabelsVisible(false);
        axis->SetBehavior(svtkAxis::FIXED);
      }
      else
      {
        svtkAxis* axis = this->GetChart(pos)->GetAxis(svtkAxis::LEFT);
        axis->SetTitle(this->VisibleColumns->GetValue(n - j - 1));
        axis->SetLabelsVisible(false);
        this->AttachAxisRangeListener(axis);
      }
    }
  }
}

void svtkScatterPlotMatrix::ResizeBigChart()
{
  if (!this->Private->ResizingBigChart)
  {
    this->ClearSpecificResizes();
    int n = this->Size.GetX();
    // The big chart need to be resized only when it is
    // "between" the histograms, ie. when n is even.
    if (n % 2 == 0)
    {
      // 30*30 is an acceptable default size to resize with
      int resizeX = 30;
      int resizeY = 30;
      if (this->CurrentPainter)
      {
        // Try to use painter to resize the big plot
        int i = this->Private->BigChartPos.GetX();
        int j = this->Private->BigChartPos.GetY();
        svtkVector2i posLeft(i - 1, j);
        svtkVector2i posBottom(i, j - 1);
        svtkChart* leftChart = this->GetChart(posLeft);
        svtkChart* bottomChart = this->GetChart(posLeft);
        if (leftChart)
        {
          svtkAxis* leftAxis = leftChart->GetAxis(svtkAxis::RIGHT);
          if (leftAxis)
          {
            resizeX = std::max(
              leftAxis->GetBoundingRect(this->CurrentPainter).GetWidth() - this->Gutter.GetX(),
              this->Gutter.GetX());
          }
        }
        if (bottomChart)
        {
          svtkAxis* bottomAxis = bottomChart->GetAxis(svtkAxis::TOP);
          if (bottomAxis)
          {
            resizeY = std::max(
              bottomAxis->GetBoundingRect(this->CurrentPainter).GetHeight() - this->Gutter.GetY(),
              this->Gutter.GetY());
          }
        }
      }

      // Move big plot bottom left point to avoid overlap
      svtkVector2f resize(resizeX, resizeY);
      this->SetSpecificResize(this->Private->BigChartPos, resize);
      if (this->LayoutIsDirty)
      {
        this->Private->ResizingBigChart = true;
        this->GetScene()->SetDirty(true);
      }
    }
  }
  else
  {
    this->Private->ResizingBigChart = false;
  }
}

void svtkScatterPlotMatrix::AttachAxisRangeListener(svtkAxis* axis)
{
  axis->AddObserver(svtkChart::UpdateRange, this, &svtkScatterPlotMatrix::AxisRangeForwarderCallback);
}

void svtkScatterPlotMatrix::AxisRangeForwarderCallback(svtkObject*, unsigned long, void*)
{
  // Only set on the end axes, and propagated to all other matching axes.
  double r[2];
  int n = this->GetSize().GetX() - 1;
  for (int i = 0; i < n; ++i)
  {
    this->GetChart(svtkVector2i(i, 0))->GetAxis(svtkAxis::BOTTOM)->GetUnscaledRange(r);
    for (int j = 1; j < n - i; ++j)
    {
      this->GetChart(svtkVector2i(i, j))->GetAxis(svtkAxis::BOTTOM)->SetUnscaledRange(r);
    }
    this->GetChart(svtkVector2i(i, n - i))->GetAxis(svtkAxis::TOP)->SetUnscaledRange(r);
    this->GetChart(svtkVector2i(0, i))->GetAxis(svtkAxis::LEFT)->GetUnscaledRange(r);
    for (int j = 1; j < n - i; ++j)
    {
      this->GetChart(svtkVector2i(j, i))->GetAxis(svtkAxis::LEFT)->SetUnscaledRange(r);
    }
  }
}

void svtkScatterPlotMatrix::BigChartSelectionCallback(svtkObject*, unsigned long event, void*)
{
  // forward the SelectionChangedEvent from the Big Chart plot
  this->InvokeEvent(event);
}

void svtkScatterPlotMatrix::SetTitle(const svtkStdString& title)
{
  if (this->Title != title)
  {
    this->Title = title;
    this->Modified();
  }
}

svtkStdString svtkScatterPlotMatrix::GetTitle()
{
  return this->Title;
}

void svtkScatterPlotMatrix::SetTitleProperties(svtkTextProperty* prop)
{
  if (this->TitleProperties != prop)
  {
    this->TitleProperties = prop;
    this->Modified();
  }
}

svtkTextProperty* svtkScatterPlotMatrix::GetTitleProperties()
{
  return this->TitleProperties;
}

void svtkScatterPlotMatrix::SetAxisLabelProperties(int plotType, svtkTextProperty* prop)
{
  if (plotType >= 0 && plotType < svtkScatterPlotMatrix::NOPLOT &&
    this->Private->ChartSettings[plotType]->LabelFont != prop)
  {
    this->Private->ChartSettings[plotType]->LabelFont = prop;
    this->Modified();
  }
}

svtkTextProperty* svtkScatterPlotMatrix::GetAxisLabelProperties(int plotType)
{
  if (plotType >= 0 && plotType < svtkScatterPlotMatrix::NOPLOT)
  {
    return this->Private->ChartSettings[plotType]->LabelFont;
  }
  return nullptr;
}

//----------------------------------------------------------------------------
void svtkScatterPlotMatrix::SetBackgroundColor(int plotType, const svtkColor4ub& color)
{
  if (plotType >= 0 && plotType < svtkScatterPlotMatrix::NOPLOT)
  {
    this->Private->ChartSettings[plotType]->BackgroundBrush->SetColor(color);
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void svtkScatterPlotMatrix::SetAxisColor(int plotType, const svtkColor4ub& color)
{
  if (plotType >= 0 && plotType < svtkScatterPlotMatrix::NOPLOT)
  {
    this->Private->ChartSettings[plotType]->AxisColor = color;
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void svtkScatterPlotMatrix::SetGridVisibility(int plotType, bool visible)
{
  if (plotType != NOPLOT)
  {
    this->Private->ChartSettings[plotType]->ShowGrid = visible;
    // How to update
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void svtkScatterPlotMatrix::SetGridColor(int plotType, const svtkColor4ub& color)
{
  if (plotType >= 0 && plotType < svtkScatterPlotMatrix::NOPLOT)
  {
    this->Private->ChartSettings[plotType]->GridColor = color;
    // How to update
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void svtkScatterPlotMatrix::SetAxisLabelVisibility(int plotType, bool visible)
{
  if (plotType != NOPLOT)
  {
    this->Private->ChartSettings[plotType]->ShowAxisLabels = visible;
    // How to update
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void svtkScatterPlotMatrix::SetAxisLabelNotation(int plotType, int notation)
{
  if (plotType != NOPLOT)
  {
    this->Private->ChartSettings[plotType]->LabelNotation = notation;
    // How to update
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void svtkScatterPlotMatrix::SetAxisLabelPrecision(int plotType, int precision)
{
  if (plotType != NOPLOT)
  {
    this->Private->ChartSettings[plotType]->LabelPrecision = precision;
    // How to update
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void svtkScatterPlotMatrix::SetTooltipNotation(int plotType, int notation)
{
  if (plotType != NOPLOT)
  {
    this->Private->ChartSettings[plotType]->TooltipNotation = notation;
    // How to update
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void svtkScatterPlotMatrix::SetTooltipPrecision(int plotType, int precision)
{
  if (plotType != NOPLOT)
  {
    this->Private->ChartSettings[plotType]->TooltipPrecision = precision;
    // How to update
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void svtkScatterPlotMatrix::SetScatterPlotSelectedRowColumnColor(const svtkColor4ub& color)
{
  this->Private->SelectedRowColumnBGBrush->SetColor(color);
  this->Modified();
}

//----------------------------------------------------------------------------
void svtkScatterPlotMatrix::SetScatterPlotSelectedActiveColor(const svtkColor4ub& color)
{
  this->Private->SelectedChartBGBrush->SetColor(color);
  this->Modified();
}

//----------------------------------------------------------------------------
void svtkScatterPlotMatrix::UpdateChartSettings(int plotType)
{
  if (plotType == HISTOGRAM)
  {
    int plotCount = this->GetSize().GetX();

    for (int i = 0; i < plotCount; i++)
    {
      svtkChart* chart = this->GetChart(svtkVector2i(i, plotCount - i - 1));
      this->Private->UpdateAxis(
        chart->GetAxis(svtkAxis::TOP), this->Private->ChartSettings[HISTOGRAM]);
      this->Private->UpdateAxis(
        chart->GetAxis(svtkAxis::RIGHT), this->Private->ChartSettings[HISTOGRAM]);
      this->Private->UpdateChart(chart, this->Private->ChartSettings[HISTOGRAM]);
    }
  }
  else if (plotType == SCATTERPLOT)
  {
    int plotCount = this->GetSize().GetX();

    for (int i = 0; i < plotCount - 1; i++)
    {
      for (int j = 0; j < plotCount - 1; j++)
      {
        if (this->GetPlotType(i, j) == SCATTERPLOT)
        {
          svtkChart* chart = this->GetChart(svtkVector2i(i, j));
          bool updateleft = i == 0 ? true : false;
          bool updatebottom = j == 0 ? true : false;
          this->Private->UpdateAxis(
            chart->GetAxis(svtkAxis::LEFT), this->Private->ChartSettings[SCATTERPLOT], updateleft);
          this->Private->UpdateAxis(chart->GetAxis(svtkAxis::BOTTOM),
            this->Private->ChartSettings[SCATTERPLOT], updatebottom);
        }
      }
    }
  }
  else if (plotType == ACTIVEPLOT && this->Private->BigChart)
  {
    this->Private->UpdateAxis(
      this->Private->BigChart->GetAxis(svtkAxis::TOP), this->Private->ChartSettings[ACTIVEPLOT]);
    this->Private->UpdateAxis(
      this->Private->BigChart->GetAxis(svtkAxis::RIGHT), this->Private->ChartSettings[ACTIVEPLOT]);
    this->Private->UpdateChart(this->Private->BigChart, this->Private->ChartSettings[ACTIVEPLOT]);
    this->Private->BigChart->SetSelectionMode(this->SelectionMode);
  }
  this->Modified();
}
//-----------------------------------------------------------------------------
void svtkScatterPlotMatrix::SetSelectionMode(int selMode)
{
  if (this->SelectionMode == selMode || selMode < svtkContextScene::SELECTION_NONE ||
    selMode > svtkContextScene::SELECTION_TOGGLE)
  {
    return;
  }
  this->SelectionMode = selMode;
  if (this->Private->BigChart)
  {
    this->Private->BigChart->SetSelectionMode(selMode);
  }

  this->Modified();
}

//-----------------------------------------------------------------------------
void svtkScatterPlotMatrix::SetSize(const svtkVector2i& size)
{
  if (this->Size.GetX() != size.GetX() || this->Size.GetY() != size.GetY())
  {
    this->ActivePlotValid = false;
    this->ActivePlot = svtkVector2i(0, this->Size.GetX() - 2);
  }
  this->Superclass::SetSize(size);
}

//----------------------------------------------------------------------------
void svtkScatterPlotMatrix::UpdateSettings()
{

  // TODO: Should update the Scatter plot title

  this->UpdateChartSettings(ACTIVEPLOT);
  this->UpdateChartSettings(HISTOGRAM);
  this->UpdateChartSettings(SCATTERPLOT);
}

//----------------------------------------------------------------------------
bool svtkScatterPlotMatrix::GetGridVisibility(int plotType)
{
  assert(plotType != NOPLOT);
  return this->Private->ChartSettings[plotType]->ShowGrid;
}

//----------------------------------------------------------------------------
svtkColor4ub svtkScatterPlotMatrix::GetBackgroundColor(int plotType)
{
  assert(plotType != NOPLOT);
  return this->Private->ChartSettings[plotType]->BackgroundBrush->GetColorObject();
}

//----------------------------------------------------------------------------
svtkColor4ub svtkScatterPlotMatrix::GetAxisColor(int plotType)
{
  assert(plotType != NOPLOT);
  return this->Private->ChartSettings[plotType]->AxisColor;
}

//----------------------------------------------------------------------------
svtkColor4ub svtkScatterPlotMatrix::GetGridColor(int plotType)
{
  assert(plotType != NOPLOT);
  return this->Private->ChartSettings[plotType]->GridColor;
}

//----------------------------------------------------------------------------
bool svtkScatterPlotMatrix::GetAxisLabelVisibility(int plotType)
{
  assert(plotType != NOPLOT);
  return this->Private->ChartSettings[plotType]->ShowAxisLabels;
}

//----------------------------------------------------------------------------
int svtkScatterPlotMatrix::GetAxisLabelNotation(int plotType)
{
  assert(plotType != NOPLOT);
  return this->Private->ChartSettings[plotType]->LabelNotation;
}

//----------------------------------------------------------------------------
int svtkScatterPlotMatrix::GetAxisLabelPrecision(int plotType)
{
  assert(plotType != NOPLOT);
  return this->Private->ChartSettings[plotType]->LabelPrecision;
}

//----------------------------------------------------------------------------
int svtkScatterPlotMatrix::GetTooltipNotation(int plotType)
{
  assert(plotType != NOPLOT);
  return this->Private->ChartSettings[plotType]->TooltipNotation;
}

int svtkScatterPlotMatrix::GetTooltipPrecision(int plotType)
{
  assert(plotType != NOPLOT);
  return this->Private->ChartSettings[plotType]->TooltipPrecision;
}

//----------------------------------------------------------------------------
void svtkScatterPlotMatrix::SetTooltip(svtkTooltipItem* tooltip)
{
  if (tooltip != this->Private->TooltipItem)
  {
    this->Private->TooltipItem = tooltip;
    this->Modified();

    svtkChartXY* chartXY = svtkChartXY::SafeDownCast(this->Private->BigChart);

    if (chartXY)
    {
      chartXY->SetTooltip(tooltip);
    }
  }
}

//----------------------------------------------------------------------------
svtkTooltipItem* svtkScatterPlotMatrix::GetTooltip() const
{
  return this->Private->TooltipItem;
}

//----------------------------------------------------------------------------
void svtkScatterPlotMatrix::SetIndexedLabels(svtkStringArray* labels)
{
  if (labels != this->Private->IndexedLabelsArray)
  {
    this->Private->IndexedLabelsArray = labels;
    this->Modified();

    if (this->Private->BigChart)
    {
      svtkPlot* plot = this->Private->BigChart->GetPlot(0);

      if (plot)
      {
        plot->SetIndexedLabels(labels);
      }
    }
  }
}

//----------------------------------------------------------------------------
svtkStringArray* svtkScatterPlotMatrix::GetIndexedLabels() const
{
  return this->Private->IndexedLabelsArray;
}

//----------------------------------------------------------------------------
svtkColor4ub svtkScatterPlotMatrix::GetScatterPlotSelectedRowColumnColor()
{
  return this->Private->SelectedRowColumnBGBrush->GetColorObject();
}

//----------------------------------------------------------------------------
svtkColor4ub svtkScatterPlotMatrix::GetScatterPlotSelectedActiveColor()
{
  return this->Private->SelectedChartBGBrush->GetColorObject();
}

//----------------------------------------------------------------------------
svtkChart* svtkScatterPlotMatrix::GetMainChart()
{
  return this->Private->BigChart;
}

//----------------------------------------------------------------------------
void svtkScatterPlotMatrix::PrintSelf(ostream& os, svtkIndent indent)
{
  Superclass::PrintSelf(os, indent);

  os << indent << "NumberOfBins: " << this->NumberOfBins << endl;
  os << indent << "Title: " << this->Title << endl;
  os << indent << "SelectionMode: " << this->SelectionMode << endl;
}
