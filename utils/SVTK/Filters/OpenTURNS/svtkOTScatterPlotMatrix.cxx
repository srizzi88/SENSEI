/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkOTScatterPlotMatrix.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkOTScatterPlotMatrix.h"

#include "svtkAxis.h"
#include "svtkBrush.h"
#include "svtkChart.h"
#include "svtkChartXY.h"
#include "svtkColor.h"
#include "svtkColorTransferFunction.h"
#include "svtkCompositeDataIterator.h"
#include "svtkExecutive.h"
#include "svtkImageData.h"
#include "svtkInformation.h"
#include "svtkMultiBlockDataSet.h"
#include "svtkNew.h"
#include "svtkOTDensityMap.h"
#include "svtkObjectFactory.h"
#include "svtkPen.h"
#include "svtkPlotHistogram2D.h"
#include "svtkPlotPoints.h"
#include "svtkTable.h"
#include "svtkTextProperty.h"

#include <vector>

// Static density values for now
static const int nDensityValues = 3;
static const double densityValues[3] = { 0.1, 0.5, 0.9 };

// An internal class to store density map settings
class svtkOTScatterPlotMatrix::DensityMapSettings
{
public:
  DensityMapSettings()
  {
    this->PlotPen->SetColor(0, 0, 0, 255);
    this->ShowDensityMap = false;
    this->DensityLineSize = 2;
    for (int i = 0; i < nDensityValues; i++)
    {
      this->DensityMapValues.push_back(densityValues[i]);
      double r, g, b;
      svtkMath::HSVToRGB(densityValues[i], 1, 0.75, &r, &g, &b);
      this->DensityMapColorMap.insert(
        std::make_pair(densityValues[i], svtkColor4ub(r * 255, g * 255, b * 255)));
    }
  }
  ~DensityMapSettings() {}

  svtkNew<svtkPen> PlotPen;
  bool ShowDensityMap;
  float DensityLineSize;
  std::vector<double> DensityMapValues;
  std::map<double, svtkColor4ub> DensityMapColorMap;
};

svtkStandardNewMacro(svtkOTScatterPlotMatrix);

svtkOTScatterPlotMatrix::svtkOTScatterPlotMatrix()
{
  this->DensityMapsSettings[svtkScatterPlotMatrix::SCATTERPLOT] =
    new svtkOTScatterPlotMatrix::DensityMapSettings;
  this->DensityMapsSettings[svtkScatterPlotMatrix::ACTIVEPLOT] = new DensityMapSettings();
}

svtkOTScatterPlotMatrix::~svtkOTScatterPlotMatrix()
{
  delete this->DensityMapsSettings[svtkScatterPlotMatrix::SCATTERPLOT];
  delete this->DensityMapsSettings[svtkScatterPlotMatrix::ACTIVEPLOT];
}

//---------------------------------------------------------------------------
void svtkOTScatterPlotMatrix::AddSupplementaryPlot(
  svtkChart* chart, int plotType, svtkStdString row, svtkStdString column, int plotCorner)
{
  svtkChartXY* xy = svtkChartXY::SafeDownCast(chart);

  if (plotType != NOPLOT && plotType != HISTOGRAM &&
    this->DensityMapsSettings[plotType]->ShowDensityMap && !this->Animating)
  {
    DensityMapCacheMap::iterator it = this->DensityMapCache.find(std::make_pair(row, column));
    svtkOTDensityMap* density;
    if (it != this->DensityMapCache.end())
    {
      density = it->second;
    }
    else
    {
      svtkSmartPointer<svtkOTDensityMap> densityPt = svtkSmartPointer<svtkOTDensityMap>::New();
      this->DensityMapCache[std::make_pair(row, column)] = densityPt;
      density = densityPt;
    }

    // Compute density map
    density->SetInputData(this->Input);
    density->SetNumberOfContours(3);
    density->SetValue(0, 0.1);
    density->SetValue(1, 0.5);
    density->SetValue(2, 0.9);
    density->SetInputArrayToProcess(0, 0, 0, svtkDataObject::FIELD_ASSOCIATION_ROWS, row);
    density->SetInputArrayToProcess(1, 0, 0, svtkDataObject::FIELD_ASSOCIATION_ROWS, column);
    density->Update();

    // Iterate over multiblock output to drow the density maps
    svtkMultiBlockDataSet* mb = svtkMultiBlockDataSet::SafeDownCast(density->GetOutput());
    svtkCompositeDataIterator* iter = mb->NewIterator();
    for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
    {
      svtkTable* densityLineTable = svtkTable::SafeDownCast(iter->GetCurrentDataObject());
      if (densityLineTable)
      {
        svtkPlot* densityPlot = chart->AddPlot(svtkChart::LINE);
        if (xy)
        {
          xy->AutoAxesOff();
          xy->SetPlotCorner(densityPlot, plotCorner);
          xy->RaisePlot(densityPlot);
        }
        densityPlot->SetInputData(densityLineTable, densityLineTable->GetColumnName(1), row);
        double densityVal = iter->GetCurrentMetaData()->Get(svtkOTDensityMap::DENSITY());
        svtkPen* plotPen = svtkPen::New();
        plotPen->DeepCopy(this->DensityMapsSettings[plotType]->PlotPen);
        plotPen->SetColor(this->DensityMapsSettings[plotType]->DensityMapColorMap[densityVal]);
        densityPlot->SetPen(plotPen);
        plotPen->Delete();
        svtkPlotPoints* plotPoints = svtkPlotPoints::SafeDownCast(densityPlot);
        plotPoints->SetWidth(this->DensityMapsSettings[plotType]->DensityLineSize);
      }
    }
    iter->Delete();

    // Draw the density map image as well
    svtkImageData* image = svtkImageData::SafeDownCast(density->GetExecutive()->GetOutputData(1));
    if (image)
    {
      svtkNew<svtkPlotHistogram2D> histo;
      histo->SetInputData(image);
      if (this->TransferFunction.Get() == nullptr)
      {
        double* range = image->GetScalarRange();
        svtkNew<svtkColorTransferFunction> stc;
        stc->SetColorSpaceToDiverging();
        stc->AddRGBPoint(range[0], 59. / 255., 76. / 255., 192. / 255.);
        stc->AddRGBPoint(0.5 * (range[0] + range[1]), 221. / 255., 221. / 255., 221. / 255.);
        stc->AddRGBPoint(range[1], 180. / 255., 4. / 255., 38. / 255.);
        stc->Build();
        histo->SetTransferFunction(stc);
      }
      else
      {
        histo->SetTransferFunction(this->TransferFunction);
      }
      histo->Update();
      chart->AddPlot(histo);
      if (xy)
      {
        xy->LowerPlot(histo); // push the plot in background
      }
    }
    else
    {
      svtkWarningMacro("Density image is not found.");
    }
  }
}

//----------------------------------------------------------------------------
void svtkOTScatterPlotMatrix::SetDensityMapVisibility(int plotType, bool visible)
{
  if (plotType != NOPLOT && plotType != HISTOGRAM &&
    this->DensityMapsSettings[plotType]->ShowDensityMap != visible)
  {
    this->DensityMapsSettings[plotType]->ShowDensityMap = visible;
    this->Modified();
    if (plotType == ACTIVEPLOT)
    {
      this->ActivePlotValid = false;
    }
  }
}

//----------------------------------------------------------------------------
void svtkOTScatterPlotMatrix::SetDensityLineSize(int plotType, float size)
{
  if (plotType != NOPLOT && plotType != HISTOGRAM &&
    this->DensityMapsSettings[plotType]->DensityLineSize != size)
  {
    this->DensityMapsSettings[plotType]->DensityLineSize = size;
    this->Modified();
    if (plotType == ACTIVEPLOT)
    {
      this->ActivePlotValid = false;
    }
  }
}

//----------------------------------------------------------------------------
void svtkOTScatterPlotMatrix::SetDensityMapColor(
  int plotType, unsigned int densityLineIndex, const svtkColor4ub& color)
{
  if (plotType != NOPLOT && plotType != HISTOGRAM &&
    densityLineIndex < this->DensityMapsSettings[plotType]->DensityMapValues.size())
  {
    double density = this->DensityMapsSettings[plotType]->DensityMapValues[densityLineIndex];
    if (this->DensityMapsSettings[plotType]->DensityMapColorMap[density] != color)
    {
      this->DensityMapsSettings[plotType]->DensityMapColorMap[density] = color;
      this->Modified();
      if (plotType == ACTIVEPLOT)
      {
        this->ActivePlotValid = false;
      }
    }
  }
}

//----------------------------------------------------------------------------
void svtkOTScatterPlotMatrix::PrintSelf(ostream& os, svtkIndent indent)
{
  Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
svtkScalarsToColors* svtkOTScatterPlotMatrix::GetTransferFunction()
{
  return this->TransferFunction;
}

//----------------------------------------------------------------------------
void svtkOTScatterPlotMatrix::SetTransferFunction(svtkScalarsToColors* stc)
{
  if (this->TransferFunction.Get() != stc)
  {
    this->TransferFunction = stc;
    this->Modified();
  }
}
