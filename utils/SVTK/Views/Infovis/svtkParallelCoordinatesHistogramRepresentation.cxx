/*=========================================================================

Program:   Visualization Toolkit
Module:    svtkParallelCoordinatesHistogramRepresentation.cxx

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
#include "svtkParallelCoordinatesHistogramRepresentation.h"
//------------------------------------------------------------------------------
#include "svtkActor2D.h"
#include "svtkArrayIteratorIncludes.h"
#include "svtkCellArray.h"
#include "svtkCellData.h"
#include "svtkComputeHistogram2DOutliers.h"
#include "svtkDoubleArray.h"
#include "svtkExtractHistogram2D.h"
#include "svtkExtractSelectedPolyDataIds.h"
#include "svtkFloatArray.h"
#include "svtkIdList.h"
#include "svtkIdTypeArray.h"
#include "svtkImageData.h"
#include "svtkImageGaussianSmooth.h"
#include "svtkImageMedian3D.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkLookupTable.h"
#include "svtkMath.h"
#include "svtkMultiBlockDataSet.h"
#include "svtkObjectFactory.h"
#include "svtkPairwiseExtractHistogram2D.h"
#include "svtkPointData.h"
#include "svtkPoints.h"
#include "svtkPolyData.h"
#include "svtkPolyDataMapper2D.h"
#include "svtkPropCollection.h"
#include "svtkProperty2D.h"
#include "svtkRenderView.h"
#include "svtkRenderer.h"
#include "svtkSCurveSpline.h"
#include "svtkSelection.h"
#include "svtkSelectionNode.h"
#include "svtkStringArray.h"
#include "svtkTable.h"
#include "svtkUnsignedIntArray.h"
#include "svtkViewTheme.h"
//------------------------------------------------------------------------------
svtkStandardNewMacro(svtkParallelCoordinatesHistogramRepresentation);
//------------------------------------------------------------------------------
svtkParallelCoordinatesHistogramRepresentation::svtkParallelCoordinatesHistogramRepresentation()
{
  this->SetNumberOfInputPorts(svtkParallelCoordinatesHistogramRepresentation::NUM_INPUT_PORTS);

  this->UseHistograms = 0;
  this->HistogramLookupTableRange[0] = 0.0;
  this->HistogramLookupTableRange[1] = -1.0;

  this->HistogramFilter = svtkSmartPointer<svtkPairwiseExtractHistogram2D>::New();
  this->HistogramFilter->SetInputData(this->InputArrayTable);

  this->HistogramLookupTable = svtkSmartPointer<svtkLookupTable>::New();
  this->HistogramLookupTable->SetAlphaRange(0, 1);
  this->HistogramLookupTable->SetHueRange(1, 1);
  this->HistogramLookupTable->SetValueRange(1, 1);
  this->HistogramLookupTable->SetSaturationRange(0, 0);
  this->HistogramLookupTable->ForceBuild();

  this->PlotMapper->SetScalarModeToUseCellData();
  this->PlotMapper->UseLookupTableScalarRangeOn();
  this->PlotMapper->SetLookupTable(this->HistogramLookupTable);
  this->PlotMapper->ScalarVisibilityOff();

  this->ShowOutliers = 0;

  this->OutlierFilter = svtkSmartPointer<svtkComputeHistogram2DOutliers>::New();
  this->OutlierFilter->SetInputData(
    svtkComputeHistogram2DOutliers::INPUT_TABLE_DATA, this->InputArrayTable);
  //                                          this->HistogramFilter->GetOutputPort(svtkPairwiseExtractHistogram2D::REORDERED_INPUT));
  this->OutlierFilter->SetInputConnection(
    svtkComputeHistogram2DOutliers::INPUT_HISTOGRAMS_MULTIBLOCK,
    this->HistogramFilter->GetOutputPort(svtkPairwiseExtractHistogram2D::HISTOGRAM_IMAGE));

  this->OutlierData = svtkSmartPointer<svtkPolyData>::New();
  this->OutlierActor = svtkSmartPointer<svtkActor2D>::New();
  this->OutlierActor->GetProperty()->SetColor(1, 1, 1);
  this->OutlierMapper = svtkSmartPointer<svtkPolyDataMapper2D>::New();
  this->OutlierMapper.TakeReference(
    this->InitializePlotMapper(this->OutlierData, this->OutlierActor));

  this->SetHistogramLookupTableRange(0, 10);
  this->SetPreferredNumberOfOutliers(100);
  this->SetNumberOfHistogramBins(10, 10);

  // Apply default theme
  // you would think that calling this in the superclass would take care of it,
  // but it turns out that the superclass constructor will only call its own
  // version there.  So I have to call it again to make sure that the local
  // version gets called.
  svtkViewTheme* theme = svtkViewTheme::New();
  theme->SetCellOpacity(1.0);
  theme->SetCellColor(1.0, 1.0, 1.0);
  theme->SetEdgeLabelColor(1.0, .8, .3);
  this->ApplyViewTheme(theme);
  theme->Delete();
}
//------------------------------------------------------------------------------
svtkParallelCoordinatesHistogramRepresentation::~svtkParallelCoordinatesHistogramRepresentation() =
  default;
//------------------------------------------------------------------------------
// Histogram quad color is defined by theme->CellColor
void svtkParallelCoordinatesHistogramRepresentation::ApplyViewTheme(svtkViewTheme* theme)
{
  this->Superclass::ApplyViewTheme(theme);

  double* c = theme->GetCellColor();
  double hsv[3] = { 0, 0, 0 };
  svtkMath::RGBToHSV(c, hsv);
  this->HistogramLookupTable->SetHueRange(hsv[0], hsv[0]);
  this->HistogramLookupTable->SetSaturationRange(hsv[1], hsv[1]);
  this->HistogramLookupTable->SetValueRange(hsv[2], hsv[2]);
  this->HistogramLookupTable->ForceBuild();
}
//------------------------------------------------------------------------------
// Make sure all of the histogram/outlier stuff is up-to-date.  Also, if not using
// histograms, make sure that lookup table for the plot data mapper is
// disabled, since that's the behavior for the parent class.
int svtkParallelCoordinatesHistogramRepresentation::ComputeDataProperties()
{
  if (!this->Superclass::ComputeDataProperties())
    return 0;

  if (this->UseHistograms)
  {
    this->GetHistogramImage(0);
    this->SetHistogramLookupTableRange(0, this->HistogramFilter->GetMaximumBinCount());
    this->HistogramLookupTable->SetRange(
      this->HistogramLookupTableRange[0], this->HistogramLookupTableRange[1]);
    this->PlotMapper->ScalarVisibilityOn();
  }
  else
  {
    this->PlotMapper->ScalarVisibilityOff();
  }

  if (this->ShowOutliers)
  {
    this->OutlierActor->VisibilityOn();
  }
  else
  {
    this->OutlierActor->VisibilityOff();
  }

  return 1;
}
//------------------------------------------------------------------------------
// outliers have the same properties as plot lines.
int svtkParallelCoordinatesHistogramRepresentation::UpdatePlotProperties(svtkStringArray* inputTitles)
{
  if (!this->Superclass::UpdatePlotProperties(inputTitles))
    return 0;

  this->OutlierActor->GetProperty()->SetOpacity(this->LineOpacity);
  this->OutlierActor->GetProperty()->SetColor(this->LineColor);

  return 1;
}
//------------------------------------------------------------------------------
int svtkParallelCoordinatesHistogramRepresentation::RequestData(
  svtkInformation* request, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // do everything the superclass does
  //  * histogram quad computation happens automatically since this
  //  class overrides the plotting functions.
  if (!this->Superclass::RequestData(request, inputVector, outputVector))
    return 0;

  // but also show outliers
  if (this->ShowOutliers)
  {
    svtkTable* outlierTable = this->GetOutlierData();

    if (this->UseCurves)
    {
      svtkParallelCoordinatesRepresentation::PlaceCurves(this->OutlierData, outlierTable, nullptr);
    }
    else
    {
      svtkParallelCoordinatesRepresentation::PlaceLines(this->OutlierData, outlierTable, nullptr);
    }
  }

  this->BuildTime.Modified();

  return 1;
}
//------------------------------------------------------------------------------
bool svtkParallelCoordinatesHistogramRepresentation::AddToView(svtkView* view)
{
  this->Superclass::AddToView(view);
  svtkRenderView* rv = svtkRenderView::SafeDownCast(view);
  if (rv)
  {
    rv->GetRenderer()->AddActor(this->OutlierActor);

    // not sure what these are for
    // rv->RegisterProgress(...);
    return true;
  }
  return false;
}
//------------------------------------------------------------------------------
bool svtkParallelCoordinatesHistogramRepresentation::RemoveFromView(svtkView* view)
{
  this->Superclass::RemoveFromView(view);
  svtkRenderView* rv = svtkRenderView::SafeDownCast(view);
  if (rv)
  {
    rv->GetRenderer()->RemoveActor(this->OutlierActor);

    // not sure what these are for
    // rv->UnRegisterProgress(this->OutlineMapper);
    return true;
  }
  return false;
}
//------------------------------------------------------------------------------
// redirect the line plotting function to the histogram plotting function,
// if histograms are enabled
int svtkParallelCoordinatesHistogramRepresentation::PlaceLines(
  svtkPolyData* polyData, svtkTable* data, svtkIdTypeArray* idsToPlot)
{
  if (this->UseHistograms)
  {
    return this->PlaceHistogramLineQuads(polyData);
  }
  else
  {
    return this->Superclass::PlaceLines(polyData, data, idsToPlot);
  }
}
//------------------------------------------------------------------------------
// redirect the line plotting function to the histogram plotting function,
// if histograms are enabled
int svtkParallelCoordinatesHistogramRepresentation::PlaceCurves(
  svtkPolyData* polyData, svtkTable* data, svtkIdTypeArray* idsToPlot)
{
  if (this->UseHistograms)
  {
    return this->PlaceHistogramCurveQuads(polyData);
  }
  else
  {
    return this->Superclass::PlaceCurves(polyData, data, idsToPlot);
  }
}
//------------------------------------------------------------------------------
// this is a bit tricky.  This class plots selections as lines, regardless
// of whether or not histograms are enabled.  That means it needs to explicitly
// call the superclass plotting functions on the selection so that the
// histogram plotting functions don't get used.
int svtkParallelCoordinatesHistogramRepresentation::PlaceSelection(
  svtkPolyData* polyData, svtkTable* data, svtkSelectionNode* selectionNode)
{
  svtkIdTypeArray* selectedIds = svtkArrayDownCast<svtkIdTypeArray>(selectionNode->GetSelectionList());
  if (!selectedIds)
    return 1;

  if (this->UseCurves)
  {
    this->Superclass::PlaceCurves(polyData, data, selectedIds);
  }
  else
  {
    this->Superclass::PlaceLines(polyData, data, selectedIds);
  }

  return 1;
}
//------------------------------------------------------------------------------
int svtkParallelCoordinatesHistogramRepresentation::PlaceHistogramLineQuads(svtkPolyData* polyData)
{
  // figure out how many samples there are by looking at each of the
  // histograms and counting the bins.
  int numberOfQuads = 0;
  for (int i = 0; i < this->NumberOfAxes - 1; i++)
  {
    svtkImageData* histogram = this->GetHistogramImage(i);
    if (histogram)
      numberOfQuads += histogram->GetPointData()->GetScalars()->GetNumberOfTuples();
  }

  if (this->UseCurves)
    numberOfQuads *= this->CurveResolution;

  this->AllocatePolyData(polyData, 0, 0, 0, 0, numberOfQuads, numberOfQuads * 4, numberOfQuads, 0);

  svtkPoints* points = polyData->GetPoints();
  float* pointsp = svtkArrayDownCast<svtkFloatArray>(points->GetData())->GetPointer(0);

  svtkDoubleArray* scalars = svtkArrayDownCast<svtkDoubleArray>(polyData->GetCellData()->GetScalars());
  double* scalarsp = scalars->GetPointer(
    0); // svtkArrayDownCast<svtkDoubleArray>(polyData->GetCellData()->GetScalars());

  // for each histogram, draw a quad for each bin.
  svtkIdType ptId = 0;
  //  svtkIdType quadId=0;
  for (int pos = 0; pos < this->NumberOfAxes - 1; pos++)
  {
    int dims[3] = { 0, 0, 0 };
    double spacing[3] = { 0, 0, 0 };
    svtkImageData* image = this->GetHistogramImage(pos);
    if (!image)
      continue;
    image->GetDimensions(dims);
    image->GetSpacing(spacing);

    double binWidth[] = { (this->YMax - this->YMin) / static_cast<double>(dims[0]),
      (this->YMax - this->YMin) / static_cast<double>(dims[1]) };

    double x1[3] = { 0., 0., 0. };
    double x2[3] = { 0., 0., 0. };
    x1[0] = this->Xs[pos];
    x2[0] = this->Xs[pos + 1];

    // for each bin, draw a quad
    for (int y = 0; y < dims[1]; y++)
    {
      x2[1] = this->YMin + y * binWidth[1];

      for (int x = 0; x < dims[0]; x++)
      {
        x1[1] = this->YMin + x * binWidth[0];

        // the number of rows that fit into this bin
        double v = image->GetScalarComponentAsDouble(x, y, 0, 0);

        *(pointsp++) = x1[0];
        *(pointsp++) = x1[1] + binWidth[0];
        *(pointsp++) = 0.;
        ptId++;
        *(pointsp++) = x1[0];
        *(pointsp++) = x1[1];
        *(pointsp++) = 0.;
        ptId++;
        *(pointsp++) = x2[0];
        *(pointsp++) = x2[1];
        *(pointsp++) = 0.;
        ptId++;
        *(pointsp++) = x2[0];
        *(pointsp++) = x2[1] + binWidth[1];
        *(pointsp++) = 0.;
        ptId++;
        //      points->SetPoint(ptId++, x1[0], x1[1]+binWidth[0], 0.);
        //        points->SetPoint(ptId++, x1[0], x1[1], 0.);
        //        points->SetPoint(ptId++, x2[0], x2[1], 0.);
        //        points->SetPoint(ptId++, x2[0], x2[1]+binWidth[1], 0.);

        // scalars used for lookup table mapping.  More rows
        // in a bin means bright quad.
        // scalars->SetTuple1(quadId++,v);
        *(scalarsp++) = v; //->SetTuple1(quadId++,v);
      }
    }
  }

  polyData->Modified();
  return 1;
}
//------------------------------------------------------------------------------
int svtkParallelCoordinatesHistogramRepresentation::PlaceHistogramCurveQuads(svtkPolyData* polyData)
{
  // figure out how many samples there are by looking at each of the
  // histograms and counting the bins.
  int numberOfStrips = 0;
  for (int i = 0; i < this->NumberOfAxes - 1; i++)
  {
    svtkImageData* histogram = this->GetHistogramImage(i);
    if (histogram)
      numberOfStrips += histogram->GetPointData()->GetScalars()->GetNumberOfTuples();
  }

  int numberOfPointsPerStrip = this->CurveResolution * 2;

  this->AllocatePolyData(polyData, 0, 0, numberOfStrips, numberOfPointsPerStrip, 0,
    numberOfStrips * numberOfPointsPerStrip, numberOfStrips, 0);

  svtkPoints* points = polyData->GetPoints();
  float* pointsp = svtkArrayDownCast<svtkFloatArray>(points->GetData())->GetPointer(0);

  svtkDoubleArray* scalars = svtkArrayDownCast<svtkDoubleArray>(polyData->GetCellData()->GetScalars());
  double* scalarsp = scalars->GetPointer(0);

  // build the default spline
  svtkSmartPointer<svtkDoubleArray> defSplineValues = svtkSmartPointer<svtkDoubleArray>::New();
  this->BuildDefaultSCurve(defSplineValues, this->CurveResolution);

  svtkIdType ptId = 0;
  // svtkIdType stripId = 0;
  for (int pos = 0; pos < this->NumberOfAxes - 1; pos++)
  {
    int dims[3] = { 0, 0, 0 };
    double spacing[3] = { 0, 0, 0 };
    svtkImageData* image = this->GetHistogramImage(pos);
    if (!image)
      continue;
    image->GetDimensions(dims);
    image->GetSpacing(spacing);

    double binWidth[] = { (this->YMax - this->YMin) / static_cast<double>(dims[0]),
      (this->YMax - this->YMin) / static_cast<double>(dims[1]) };

    double x1[3] = { 0., 0., 0. };
    double x2[3] = { 0., 0., 0. };
    double xc[3] = { 0., 0., 0. };
    double w = 0.0;

    x1[0] = this->Xs[pos];
    x2[0] = this->Xs[pos + 1];

    double dx = (x2[0] - x1[0]) / static_cast<double>(this->CurveResolution - 1);
    double dw = binWidth[1] - binWidth[0];

    for (int y = 0; y < dims[1]; y++)
    {
      x2[1] = this->YMin + y * binWidth[1];

      for (int x = 0; x < dims[0]; x++)
      {
        x1[1] = this->YMin + x * binWidth[0];

        double v = image->GetScalarComponentAsDouble(x, y, 0, 0);
        double dy = x2[1] - x1[1];

        for (int c = 0; c < this->CurveResolution; c++)
        {
          xc[0] = this->Xs[pos] + dx * c;
          xc[1] = defSplineValues->GetValue(c) * dy +
            x1[1]; // spline->Evaluate(x1[0]);//spline->Evaluate(x1[0]);
          w = defSplineValues->GetValue(c) * dw + binWidth[0]; // bwspline->Evaluate(x1[0]);

          //          points->SetPoint(ptId++,   xc[0], xc[1]+w, 0.);
          //          points->SetPoint(ptId++, xc[0], xc[1], 0.);
          *(pointsp++) = xc[0];
          *(pointsp++) = xc[1] + w;
          *(pointsp++) = 0.0;
          ptId++;
          *(pointsp++) = xc[0];
          *(pointsp++) = xc[1];
          *(pointsp++) = 0.0;
          ptId++;
        }

        *(scalarsp++) = v; //->SetTuple1(stripId++,v);
      }
    }
  }

  polyData->Modified();
  return 1;
}
//------------------------------------------------------------------------------
void svtkParallelCoordinatesHistogramRepresentation::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << "UseHistograms: " << this->UseHistograms << endl;
  os << "HistogramLookupTableRange: " << this->HistogramLookupTableRange[0] << ","
     << this->HistogramLookupTableRange[1] << endl;
  os << "NumberOfHistogramBins: " << this->NumberOfHistogramBins[0] << ","
     << this->NumberOfHistogramBins[1] << endl;
  os << "ShowOutliers: " << this->ShowOutliers << endl;
  os << "PreferredNumberOfOutliers: " << this->PreferredNumberOfOutliers << endl;
}
//------------------------------------------------------------------------------
int svtkParallelCoordinatesHistogramRepresentation::SwapAxisPositions(int position1, int position2)
{
  if (this->Superclass::SwapAxisPositions(position1, position2))
  {
    this->HistogramFilter->Modified();
    if (this->ShowOutliers)
      this->OutlierFilter->Modified();

    return 1;
  }

  return 0;
}
//------------------------------------------------------------------------------
int svtkParallelCoordinatesHistogramRepresentation::SetRangeAtPosition(int position, double range[2])
{
  if (this->Superclass::SetRangeAtPosition(position, range))
  {
    this->HistogramFilter->SetCustomColumnRange(position, range);
    this->HistogramFilter->Modified();

    if (ShowOutliers)
      this->OutlierFilter->Modified();

    return 1;
  }
  return 0;
}
//------------------------------------------------------------------------------
void svtkParallelCoordinatesHistogramRepresentation::SetUseHistograms(svtkTypeBool use)
{
  if (use && this->UseHistograms != use)
  {
    this->HistogramFilter->Modified();

    if (this->ShowOutliers)
      this->OutlierFilter->Modified();
  }

  this->UseHistograms = use;
  this->Modified();
}
//------------------------------------------------------------------------------
void svtkParallelCoordinatesHistogramRepresentation::SetShowOutliers(svtkTypeBool show)
{
  if (show && this->ShowOutliers != show)
  {
    this->HistogramFilter->Modified();
    this->OutlierFilter->Modified();
  }

  this->ShowOutliers = show;
  this->Modified();
}
//------------------------------------------------------------------------------
void svtkParallelCoordinatesHistogramRepresentation::SetPreferredNumberOfOutliers(int num)
{
  if (num >= 0)
  {
    this->PreferredNumberOfOutliers = num;
    this->OutlierFilter->SetPreferredNumberOfOutliers(num);
    this->Modified();
  }
}
//------------------------------------------------------------------------------
void svtkParallelCoordinatesHistogramRepresentation::SetNumberOfHistogramBins(int nx, int ny)
{
  if (nx > 0 && ny > 0)
  {
    this->NumberOfHistogramBins[0] = nx;
    this->NumberOfHistogramBins[1] = ny;

    this->HistogramFilter->SetNumberOfBins(nx, ny);

    this->Modified();
  }
}
//------------------------------------------------------------------------------
void svtkParallelCoordinatesHistogramRepresentation::SetNumberOfHistogramBins(int* n)
{
  this->SetNumberOfHistogramBins(n[0], n[1]);
}
//------------------------------------------------------------------------------
svtkImageData* svtkParallelCoordinatesHistogramRepresentation::GetHistogramImage(int idx)
{
  return this->HistogramFilter->GetOutputHistogramImage(idx);
}
//------------------------------------------------------------------------------
svtkTable* svtkParallelCoordinatesHistogramRepresentation::GetOutlierData()
{
  return this->OutlierFilter->GetOutputTable();
}
