/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPlotBar.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkPlotBar.h"

#include "svtkAxis.h"
#include "svtkBrush.h"
#include "svtkColorSeries.h"
#include "svtkContext2D.h"
#include "svtkContextDevice2D.h"
#include "svtkContextMapper2D.h"
#include "svtkExecutive.h"
#include "svtkFloatArray.h"
#include "svtkIdTypeArray.h"
#include "svtkInformation.h"
#include "svtkLookupTable.h"
#include "svtkNew.h"
#include "svtkPen.h"
#include "svtkPoints2D.h"
#include "svtkRect.h"
#include "svtkSmartPointer.h"
#include "svtkStringArray.h"
#include "svtkTable.h"
#include "svtkTimeStamp.h"

#include "svtkObjectFactory.h"

#include <algorithm>
#include <map>
#include <set>
#include <vector>

//-----------------------------------------------------------------------------
namespace
{

// Copy the two arrays into the points array
template <class A, class B>
void CopyToPoints(svtkPoints2D* points, svtkPoints2D* previousPoints, A* a, B* b, int n, int logScale,
  const svtkRectd& ss)
{
  points->SetNumberOfPoints(n);
  float* data = static_cast<float*>(points->GetVoidPointer(0));
  float* prevData = nullptr;
  if (previousPoints && static_cast<int>(previousPoints->GetNumberOfPoints()) == n)
  {
    prevData = static_cast<float*>(previousPoints->GetVoidPointer(0));
  }
  float prev = 0.0;
  for (int i = 0; i < n; ++i)
  {
    if (prevData)
    {
      prev = prevData[2 * i + 1];
    }
    A tmpA(static_cast<A>((a[i] + ss[0]) * ss[2]));
    B tmpB(static_cast<B>((b[i] + ss[1]) * ss[3]));
    data[2 * i] = static_cast<float>((logScale & 1) ? log10(static_cast<double>(tmpA)) : tmpA);
    data[2 * i + 1] =
      static_cast<float>((logScale & 2) ? log10(static_cast<double>(tmpB + prev)) : (tmpB + prev));
  }
}

// Copy one array into the points array, use the index of that array as x
template <class A>
void CopyToPoints(
  svtkPoints2D* points, svtkPoints2D* previousPoints, A* a, int n, int logScale, const svtkRectd& ss)
{
  points->SetNumberOfPoints(n);
  float* data = static_cast<float*>(points->GetVoidPointer(0));
  float* prevData = nullptr;
  if (previousPoints && static_cast<int>(previousPoints->GetNumberOfPoints()) == n)
  {
    prevData = static_cast<float*>(previousPoints->GetVoidPointer(0));
  }
  float prev = 0.0;
  for (int i = 0; i < n; ++i)
  {
    if (prevData)
    {
      prev = prevData[2 * i + 1];
    }
    A tmpA(static_cast<A>((a[i] + ss[1]) * ss[3]));
    data[2 * i] = static_cast<float>((logScale & 1) ? log10(static_cast<double>(i + 1.0)) : i);
    data[2 * i + 1] =
      static_cast<float>((logScale & 2) ? log10(static_cast<double>(tmpA + prev)) : (tmpA + prev));
  }
}

// Copy the two arrays into the points array
template <class A>
void CopyToPointsSwitch(svtkPoints2D* points, svtkPoints2D* previousPoints, A* a, svtkDataArray* b,
  int n, int logScale, const svtkRectd& ss)
{
  switch (b->GetDataType())
  {
    svtkTemplateMacro(CopyToPoints(
      points, previousPoints, a, static_cast<SVTK_TT*>(b->GetVoidPointer(0)), n, logScale, ss));
  }
}

} // namespace

//-----------------------------------------------------------------------------

class svtkPlotBarSegment : public svtkObject
{
public:
  svtkTypeMacro(svtkPlotBarSegment, svtkObject);
  static svtkPlotBarSegment* New();

  svtkPlotBarSegment()
  {
    this->Bar = nullptr;
    this->Points = nullptr;
    this->Sorted = nullptr;
    this->Previous = nullptr;
    this->Colors = nullptr;
  }

  ~svtkPlotBarSegment() override { delete this->Sorted; }

  void Configure(svtkPlotBar* bar, svtkDataArray* xArray, svtkDataArray* yArray, svtkAxis* xAxis,
    svtkAxis* yAxis, svtkPlotBarSegment* prev)
  {
    this->Bar = bar;
    this->Previous = prev;
    if (!this->Points)
    {
      this->Points = svtkSmartPointer<svtkPoints2D>::New();
    }
    // For the atypical case that Configure is called on a non-fresh "this"
    delete this->Sorted;

    int logScale = (xAxis->GetLogScaleActive() ? 1 : 0) + (yAxis->GetLogScaleActive() ? 2 : 0);
    if (xArray)
    {
      switch (xArray->GetDataType())
      {
        svtkTemplateMacro(
          CopyToPointsSwitch(this->Points, this->Previous ? this->Previous->Points : nullptr,
            static_cast<SVTK_TT*>(xArray->GetVoidPointer(0)), yArray, xArray->GetNumberOfTuples(),
            logScale, this->Bar->GetShiftScale()));
      }
    }
    else
    { // Using Index for X Series
      switch (yArray->GetDataType())
      {
        svtkTemplateMacro(
          CopyToPoints(this->Points, this->Previous ? this->Previous->Points : nullptr,
            static_cast<SVTK_TT*>(yArray->GetVoidPointer(0)), yArray->GetNumberOfTuples(), logScale,
            this->Bar->GetShiftScale()));
      }
    }
  }

  void Paint(
    svtkContext2D* painter, svtkPen* pen, svtkBrush* brush, float width, float offset, int orientation)
  {
    painter->ApplyPen(pen);
    painter->ApplyBrush(brush);
    int n = this->Points->GetNumberOfPoints();
    float* f = svtkArrayDownCast<svtkFloatArray>(this->Points->GetData())->GetPointer(0);
    float* p = nullptr;
    if (this->Previous)
    {
      p = svtkArrayDownCast<svtkFloatArray>(this->Previous->Points->GetData())->GetPointer(0);
    }

    for (int i = 0; i < n; ++i)
    {
      if (this->Colors)
      {
        if (this->Colors->GetNumberOfComponents() == 3)
        {
          painter->GetBrush()->SetColor(svtkColor3ub(this->Colors->GetPointer(i * 3)));
        }
        else if (this->Colors->GetNumberOfComponents() == 4)
        {
          painter->GetBrush()->SetColor(svtkColor4ub(this->Colors->GetPointer(i * 4)));
        }
        else
        {
          svtkErrorMacro(<< "Number of components not supported: "
                        << this->Colors->GetNumberOfComponents());
        }
      }
      if (orientation == svtkPlotBar::VERTICAL)
      {
        if (p)
        {
          painter->DrawRect(
            f[2 * i] - (width / 2) - offset, p[2 * i + 1], width, f[2 * i + 1] - p[2 * i + 1]);
        }
        else
        {
          painter->DrawRect(f[2 * i] - (width / 2) - offset, 0.0, width, f[2 * i + 1]);
        }
      }
      else // HORIZONTAL orientation
      {
        if (p)
        {
          painter->DrawRect(
            p[2 * i + 1], f[2 * i] - (width / 2) - offset, f[2 * i + 1] - p[2 * i + 1], width);
        }
        else
        {
          painter->DrawRect(0.0, f[2 * i] - (width / 2) - offset, f[2 * i + 1], width);
        }
      }
    }
    // Paint selections if there are any.
    svtkIdTypeArray* selection = this->Bar->GetSelection();
    if (!selection)
    {
      return;
    }
    painter->ApplyBrush(this->Bar->GetSelectionBrush());
    for (svtkIdType j = 0; j < selection->GetNumberOfTuples(); ++j)
    {
      int i = selection->GetValue(j);
      if (orientation == svtkPlotBar::VERTICAL)
      {
        if (p)
        {
          painter->DrawRect(
            f[2 * i] - (width / 2) - offset, p[2 * i + 1], width, f[2 * i + 1] - p[2 * i + 1]);
        }
        else
        {
          painter->DrawRect(f[2 * i] - (width / 2) - offset, 0.0, width, f[2 * i + 1]);
        }
      }
      else // HORIZONTAL orientation
      {
        if (p)
        {
          painter->DrawRect(
            p[2 * i + 1], f[2 * i] - (width / 2) - offset, f[2 * i + 1] - p[2 * i + 1], width);
        }
        else
        {
          painter->DrawRect(0.0, f[2 * i] - (width / 2) - offset, f[2 * i + 1], width);
        }
      }
    }
  }

  svtkIdType GetNearestPoint(
    const svtkVector2f& point, svtkVector2f* location, float width, float offset, int orientation)
  {
    if (!this->Points && this->Points->GetNumberOfPoints())
    {
      return -1;
    }

    // The extent of any given bar is half a width on either
    // side of the point with which it is associated.
    float halfWidth = width / 2.0;

    // If orientation is VERTICAL, search normally. For HORIZONTAL,
    // simply transpose the X and Y coordinates of the target, as the rest
    // of the search uses the assumption that X = bar position, Y = bar
    // value; swapping the target X and Y is simpler than swapping the
    // X and Y of all the other references to the bar data.
    svtkVector2f targetPoint(point);
    if (orientation == svtkPlotBar::HORIZONTAL)
    {
      targetPoint.Set(point.GetY(), point.GetX()); // Swap x and y
    }

    this->CreateSortedPoints();

    // Get the left-most bar we might hit
    svtkIndexedVector2f lowPoint;
    lowPoint.index = 0;
    lowPoint.pos = svtkVector2f(targetPoint.GetX() - (offset * -1) - halfWidth, 0.0f);

    // Set up our search array, use the STL lower_bound algorithm
    VectorPIMPL::iterator low;
    VectorPIMPL& v = *this->Sorted;
    low = std::lower_bound(v.begin(), v.end(), lowPoint);

    while (low != v.end())
    {
      // Does the bar surround the point?
      if (low->pos.GetX() - halfWidth - offset < targetPoint.GetX() &&
        low->pos.GetX() + halfWidth - offset > targetPoint.GetX())
      {
        // Is the point within the vertical extent of the bar?
        if ((targetPoint.GetY() >= 0 && targetPoint.GetY() < low->pos.GetY()) ||
          (targetPoint.GetY() < 0 && targetPoint.GetY() > low->pos.GetY()))
        {
          *location = low->pos;
          svtkRectd ss = this->Bar->GetShiftScale();
          location->SetX((location->GetX() - ss.GetX()) / ss.GetWidth());
          location->SetY((location->GetY() - ss.GetY()) / ss.GetHeight());
          return static_cast<svtkIdType>(low->index);
        }
      }
      // Is the left side of the bar beyond the point?
      if (low->pos.GetX() - offset - halfWidth > targetPoint.GetX())
      {
        break;
      }
      ++low;
    }
    return -1;
  }

  void CreateSortedPoints()
  {
    // Sorted points, used when searching for the nearest point.
    if (!this->Sorted)
    {
      svtkIdType n = this->Points->GetNumberOfPoints();
      svtkVector2f* data = static_cast<svtkVector2f*>(this->Points->GetVoidPointer(0));
      this->Sorted = new VectorPIMPL(data, n);
      std::sort(this->Sorted->begin(), this->Sorted->end());
    }
  }

  bool SelectPoints(
    const svtkVector2f& min, const svtkVector2f& max, float width, float offset, int orientation)
  {
    if (!this->Points)
    {
      return false;
    }

    this->CreateSortedPoints();

    // If orientation is VERTICAL, search normally. For HORIZONTAL,
    // transpose the selection box.
    svtkVector2f targetMin(min);
    svtkVector2f targetMax(max);
    if (orientation == svtkPlotBar::HORIZONTAL)
    {
      targetMin.Set(min.GetY(), min.GetX());
      targetMax.Set(max.GetY(), max.GetX());
    }

    // The extent of any given bar is half a width on either
    // side of the point with which it is associated.
    float halfWidth = width / 2.0;

    // Get the lowest X coordinate we might hit
    svtkIndexedVector2f lowPoint;
    lowPoint.index = 0;
    lowPoint.pos = svtkVector2f(targetMin.GetX() - (offset * -1) - halfWidth, 0.0f);

    // Set up our search array, use the STL lower_bound algorithm
    VectorPIMPL::iterator low;
    VectorPIMPL& v = *this->Sorted;
    low = std::lower_bound(v.begin(), v.end(), lowPoint);

    std::vector<svtkIdType> selected;

    while (low != v.end())
    {
      // Is the bar's X coordinates at least partially within the box?
      if (low->pos.GetX() + halfWidth - offset > targetMin.GetX() &&
        low->pos.GetX() - halfWidth - offset < targetMax.GetX())
      {
        // Is the bar within the vertical extent of the box?
        if ((targetMin.GetY() > 0 && low->pos.GetY() >= targetMin.GetY()) ||
          (targetMax.GetY() < 0 && low->pos.GetY() <= targetMax.GetY()) ||
          (targetMin.GetY() < 0 && targetMax.GetY() > 0))
        {
          selected.push_back(static_cast<int>(low->index));
        }
      }
      // Is the left side of the bar beyond the box?
      if (low->pos.GetX() - offset - halfWidth > targetMax.GetX())
      {
        break;
      }
      ++low;
    }

    if (selected.empty())
    {
      return false;
    }
    else
    {
      this->Bar->GetSelection()->SetNumberOfTuples(static_cast<svtkIdType>(selected.size()));
      svtkIdType* ptr = static_cast<svtkIdType*>(this->Bar->GetSelection()->GetVoidPointer(0));
      for (size_t i = 0; i < selected.size(); ++i)
      {
        ptr[i] = selected[i];
      }
      this->Bar->GetSelection()->Modified();
      return true;
    }
  }

  // Indexed vector for sorting
  struct svtkIndexedVector2f
  {
    size_t index;
    svtkVector2f pos;

    // Compare two svtkIndexedVector2f, in X component only
    bool operator<(const svtkIndexedVector2f& v2) const
    {
      return (this->pos.GetX() < v2.pos.GetX());
    }
  };

  class VectorPIMPL : public std::vector<svtkIndexedVector2f>
  {
  public:
    VectorPIMPL(svtkVector2f* array, size_t n)
      : std::vector<svtkIndexedVector2f>()
    {
      this->reserve(n);
      for (size_t i = 0; i < n; ++i)
      {
        svtkIndexedVector2f tmp;
        tmp.index = i;
        tmp.pos = array[i];
        this->push_back(tmp);
      }
    }
  };

  svtkSmartPointer<svtkPlotBarSegment> Previous;
  svtkSmartPointer<svtkPoints2D> Points;
  svtkPlotBar* Bar;
  VectorPIMPL* Sorted;
  svtkVector2d ScalingFactor;
  svtkUnsignedCharArray* Colors;
};

svtkStandardNewMacro(svtkPlotBarSegment);

class svtkPlotBarPrivate
{
public:
  svtkPlotBarPrivate(svtkPlotBar* bar)
    : Bar(bar)
  {
  }

  void Update() { this->Segments.clear(); }

  svtkPlotBarSegment* AddSegment(svtkDataArray* xArray, svtkDataArray* yArray, svtkAxis* xAxis,
    svtkAxis* yAxis, svtkPlotBarSegment* prev = nullptr)
  {
    svtkNew<svtkPlotBarSegment> segment;
    segment->Configure(this->Bar, xArray, yArray, xAxis, yAxis, prev);
    this->Segments.push_back(segment.GetPointer());
    return segment.GetPointer();
  }

  void PaintSegments(svtkContext2D* painter, svtkColorSeries* colorSeries, svtkPen* pen,
    svtkBrush* brush, float width, float offset, int orientation)
  {
    int colorInSeries = 0;
    bool useColorSeries = this->Segments.size() > 1;
    for (std::vector<svtkSmartPointer<svtkPlotBarSegment> >::iterator it = this->Segments.begin();
         it != this->Segments.end(); ++it)
    {
      if (useColorSeries && colorSeries)
      {
        brush->SetColor(colorSeries->GetColorRepeating(colorInSeries++).GetData());
      }
      (*it)->Paint(painter, pen, brush, width, offset, orientation);
    }
  }

  svtkIdType GetNearestPoint(const svtkVector2f& point, svtkVector2f* location, float width,
    float offset, int orientation, svtkIdType* segmentIndex)
  {
    svtkIdType segmentIndexCtr = 0;
    for (std::vector<svtkSmartPointer<svtkPlotBarSegment> >::iterator it = this->Segments.begin();
         it != this->Segments.end(); ++it)
    {
      int barIndex = (*it)->GetNearestPoint(point, location, width, offset, orientation);
      if (barIndex != -1)
      {
        if (segmentIndex)
        {
          *segmentIndex = segmentIndexCtr;
        }
        return barIndex;
      }
      ++segmentIndexCtr;
    }
    if (segmentIndex)
    {
      *segmentIndex = -1;
    }
    return -1;
  }

  bool SelectPoints(
    const svtkVector2f& min, const svtkVector2f& max, float width, float offset, int orientation)
  {
    // Selection functionality not supported for stacked plots (yet)...
    if (this->Segments.size() != 1)
    {
      return false;
    }

    return this->Segments[0]->SelectPoints(min, max, width, offset, orientation);
  }

  std::vector<svtkSmartPointer<svtkPlotBarSegment> > Segments;
  svtkPlotBar* Bar;
  std::map<int, std::string> AdditionalSeries;
  svtkStdString GroupName;
};

//-----------------------------------------------------------------------------
svtkStandardNewMacro(svtkPlotBar);

//-----------------------------------------------------------------------------
svtkPlotBar::svtkPlotBar()
{
  this->Private = new svtkPlotBarPrivate(this);
  this->Points = nullptr;
  this->AutoLabels = nullptr;
  this->Width = 1.0;
  this->Pen->SetWidth(1.0);
  this->Offset = 1.0;
  this->ColorSeries = nullptr;
  this->Orientation = svtkPlotBar::VERTICAL;
  this->ScalarVisibility = false;
  this->EnableOpacityMapping = true;
  this->LogX = false;
  this->LogY = false;
}

//-----------------------------------------------------------------------------
svtkPlotBar::~svtkPlotBar()
{
  if (this->Points)
  {
    this->Points->Delete();
    this->Points = nullptr;
  }
  delete this->Private;
}

//-----------------------------------------------------------------------------
void svtkPlotBar::Update()
{
  if (!this->Visible)
  {
    return;
  }
  // First check if we have an input
  svtkTable* table = this->Data->GetInput();
  if (!table)
  {
    svtkDebugMacro(<< "Update event called with no input table set.");
    return;
  }
  else if (this->Data->GetMTime() > this->BuildTime || table->GetMTime() > this->BuildTime ||
    (this->LookupTable && this->LookupTable->GetMTime() > this->BuildTime) ||
    this->MTime > this->BuildTime)
  {
    svtkDebugMacro(<< "Updating cached values.");
    this->UpdateTableCache(table);
  }
  else if ((this->XAxis->GetMTime() > this->BuildTime) ||
    (this->YAxis->GetMTime() > this->BuildTime))
  {
    if ((this->LogX != this->XAxis->GetLogScale()) || (this->LogY != this->YAxis->GetLogScale()))
    {
      this->LogX = this->XAxis->GetLogScale();
      this->LogY = this->YAxis->GetLogScale();
      this->UpdateTableCache(table);
    }
  }
}

//-----------------------------------------------------------------------------
bool svtkPlotBar::Paint(svtkContext2D* painter)
{
  // This is where everything should be drawn, or dispatched to other methods.
  svtkDebugMacro(<< "Paint event called in svtkPlotBar.");

  if (!this->Visible)
  {
    return false;
  }

  this->Private->PaintSegments(painter, this->ColorSeries, this->Pen, this->Brush, this->Width,
    this->Offset, this->Orientation);

  return true;
}

//-----------------------------------------------------------------------------
bool svtkPlotBar::PaintLegend(svtkContext2D* painter, const svtkRectf& rect, int legendIndex)
{
  if (this->ColorSeries)
  {
    this->Brush->SetColor(this->ColorSeries->GetColorRepeating(legendIndex).GetData());
  }

  painter->ApplyPen(this->Pen);
  painter->ApplyBrush(this->Brush);
  painter->DrawRect(rect[0], rect[1], rect[2], rect[3]);
  return true;
}

//-----------------------------------------------------------------------------
void svtkPlotBar::GetBounds(double bounds[4], bool unscaled)
{
  int seriesLow, seriesHigh, valuesLow, valuesHigh;
  // Don't re-orient the axes for vertical plots or unscaled bounds:
  if (this->Orientation == svtkPlotBar::VERTICAL || unscaled)
  {
    seriesLow = 0;  // Xmin
    seriesHigh = 1; // Xmax
    valuesLow = 2;  // Ymin
    valuesHigh = 3; // Ymax
  }
  else // HORIZONTAL orientation
  {
    seriesLow = 2;  // Ymin
    seriesHigh = 3; // Ymax
    valuesLow = 0;  // Xmin
    valuesHigh = 1; // Xmax
  }

  // Get the x and y arrays (index 0 and 1 respectively)
  svtkTable* table = this->Data->GetInput();
  svtkDataArray* x =
    this->UseIndexForXSeries ? nullptr : this->Data->GetInputArrayToProcess(0, table);
  svtkDataArray* y = this->Data->GetInputArrayToProcess(1, table);
  if (!y)
  {
    return;
  }

  if (this->UseIndexForXSeries)
  {
    bounds[seriesLow] = 0 - (this->Width / 2);
    bounds[seriesHigh] = y->GetNumberOfTuples() + (this->Width / 2);
  }
  else if (x)
  {
    x->GetRange(&bounds[seriesLow]);
    // We surround our point by Width/2 on either side
    bounds[seriesLow] -= this->Width / 2.0 + this->Offset;
    bounds[seriesHigh] += this->Width / 2.0 - this->Offset;
  }
  else
  {
    return;
  }

  y->GetRange(&bounds[valuesLow]);

  double yRange[2];
  std::map<int, std::string>::iterator it;
  for (it = this->Private->AdditionalSeries.begin(); it != this->Private->AdditionalSeries.end();
       ++it)
  {
    y = svtkArrayDownCast<svtkDataArray>(table->GetColumnByName((*it).second.c_str()));
    y->GetRange(yRange);
    bounds[valuesHigh] += yRange[1];
  }

  // Bar plots always have one of the value bounds at the origin
  if (bounds[valuesLow] > 0.0f)
  {
    bounds[valuesLow] = 0.0;
  }
  else if (bounds[valuesHigh] < 0.0f)
  {
    bounds[valuesHigh] = 0.0;
  }

  if (unscaled)
  {
    svtkAxis* axes[2];
    axes[seriesLow / 2] = this->GetXAxis();
    axes[valuesLow / 2] = this->GetYAxis();
    if (axes[0]->GetLogScaleActive())
    {
      bounds[0] = log10(fabs(bounds[0]));
      bounds[1] = log10(fabs(bounds[1]));
    }
    if (axes[1]->GetLogScaleActive())
    {
      bounds[2] = log10(fabs(bounds[2]));
      bounds[3] = log10(fabs(bounds[3]));
    }
  }
  svtkDebugMacro(<< "Bounds: " << bounds[0] << "\t" << bounds[1] << "\t" << bounds[2] << "\t"
                << bounds[3]);
}

//-----------------------------------------------------------------------------
void svtkPlotBar::GetBounds(double bounds[4])
{
  this->GetBounds(bounds, false);
}

//-----------------------------------------------------------------------------
void svtkPlotBar::GetUnscaledInputBounds(double bounds[4])
{
  this->GetBounds(bounds, true);
}

//-----------------------------------------------------------------------------
void svtkPlotBar::SetOrientation(int orientation)
{
  if (orientation < 0 || orientation > 1)
  {
    svtkErrorMacro("Error, invalid orientation value supplied: " << orientation);
    return;
  }
  this->Orientation = orientation;
}

//-----------------------------------------------------------------------------
void svtkPlotBar::SetColor(unsigned char r, unsigned char g, unsigned char b, unsigned char a)
{
  this->Brush->SetColor(r, g, b, a);
}

//-----------------------------------------------------------------------------
void svtkPlotBar::SetColor(double r, double g, double b)
{
  this->Brush->SetColorF(r, g, b);
}

//-----------------------------------------------------------------------------
void svtkPlotBar::GetColor(double rgb[3])
{
  double rgba[4];
  this->Brush->GetColorF(rgba);
  rgb[0] = rgba[0];
  rgb[1] = rgba[1];
  rgb[2] = rgba[2];
}

//-----------------------------------------------------------------------------
svtkIdType svtkPlotBar::GetNearestPoint(const svtkVector2f& point,
#ifndef SVTK_LEGACY_REMOVE
  const svtkVector2f& tolerance,
#else
  const svtkVector2f& svtkNotUsed(tolerance),
#endif // SVTK_LEGACY_REMOVE
  svtkVector2f* location, svtkIdType* segmentIndex)
{
#ifndef SVTK_LEGACY_REMOVE
  if (!this->LegacyRecursionFlag)
  {
    this->LegacyRecursionFlag = true;
    svtkIdType ret = this->GetNearestPoint(point, tolerance, location);
    this->LegacyRecursionFlag = false;
    if (ret != -1)
    {
      SVTK_LEGACY_REPLACED_BODY(svtkPlotBox::GetNearestPoint(const svtkVector2f& point,
                                 const svtkVector2f& tolerance, svtkVector2f* location),
        "SVTK 9.0",
        svtkPlotBox::GetNearestPoint(const svtkVector2f& point, const svtkVector2f& tolerance,
          svtkVector2f* location, svtkIdType* segmentId));
      return ret;
    }
  }
#endif // SVTK_LEGACY_REMOVE

  return this->Private->GetNearestPoint(
    point, location, this->Width, this->Offset, this->Orientation, segmentIndex);
}

//-----------------------------------------------------------------------------
svtkStringArray* svtkPlotBar::GetLabels()
{
  // If the label string is empty, return the y column name
  if (this->Labels)
  {
    return this->Labels;
  }
  else if (this->AutoLabels)
  {
    return this->AutoLabels;
  }
  else if (this->Data->GetInput() && this->Data->GetInputArrayToProcess(1, this->Data->GetInput()))
  {
    this->AutoLabels = svtkSmartPointer<svtkStringArray>::New();
    this->AutoLabels->InsertNextValue(
      this->Data->GetInputArrayToProcess(1, this->Data->GetInput())->GetName());

    std::map<int, std::string>::iterator it;
    for (it = this->Private->AdditionalSeries.begin(); it != this->Private->AdditionalSeries.end();
         ++it)
    {
      this->AutoLabels->InsertNextValue((*it).second);
    }
    return this->AutoLabels;
  }
  else
  {
    return nullptr;
  }
}

void svtkPlotBar::SetGroupName(const svtkStdString& name)
{
  if (this->Private->GroupName != name)
  {
    this->Private->GroupName = name;
    this->Modified();
  }
}

svtkStdString svtkPlotBar::GetGroupName()
{
  return this->Private->GroupName;
}

//-----------------------------------------------------------------------------
bool svtkPlotBar::UpdateTableCache(svtkTable* table)
{
  // Get the x and y arrays (index 0 and 1 respectively)
  svtkDataArray* x =
    this->UseIndexForXSeries ? nullptr : this->Data->GetInputArrayToProcess(0, table);
  svtkDataArray* y = this->Data->GetInputArrayToProcess(1, table);
  if (!x && !this->UseIndexForXSeries)
  {
    svtkErrorMacro(<< "No X column is set (index 0).");
    return false;
  }
  else if (!y)
  {
    svtkErrorMacro(<< "No Y column is set (index 1).");
    return false;
  }
  else if (!this->UseIndexForXSeries && x->GetNumberOfTuples() != y->GetNumberOfTuples())
  {
    svtkErrorMacro("The x and y columns must have the same number of elements.");
    return false;
  }

  this->Private->Update();

  svtkPlotBarSegment* prev = this->Private->AddSegment(x, y, this->GetXAxis(), this->GetYAxis());

  // Additions for color mapping
  if (this->ScalarVisibility && !this->ColorArrayName.empty())
  {
    svtkDataArray* c = svtkArrayDownCast<svtkDataArray>(table->GetColumnByName(this->ColorArrayName));
    // TODO: Should add support for categorical coloring & try enum lookup
    if (c)
    {
      if (!this->LookupTable)
      {
        this->CreateDefaultLookupTable();
      }

      int outputFormat = this->EnableOpacityMapping ? SVTK_RGBA : SVTK_RGB;
      this->Colors = this->LookupTable->MapScalars(c, SVTK_COLOR_MODE_MAP_SCALARS, -1, outputFormat);

      prev->Colors = this->Colors;
      this->Colors->Delete();
    }
    else
    {
      this->Colors = nullptr;
      prev->Colors = nullptr;
    }
  }

  std::map<int, std::string>::iterator it;

  for (it = this->Private->AdditionalSeries.begin(); it != this->Private->AdditionalSeries.end();
       ++it)
  {
    y = svtkArrayDownCast<svtkDataArray>(table->GetColumnByName((*it).second.c_str()));
    prev = this->Private->AddSegment(x, y, this->GetXAxis(), this->GetYAxis(), prev);
  }

  this->TooltipDefaultLabelFormat.clear();
  // Set the default tooltip according to the segments
  if (this->Private->Segments.size() > 1)
  {
    this->TooltipDefaultLabelFormat = "%s: ";
  }
  if (this->IndexedLabels)
  {
    this->TooltipDefaultLabelFormat += "%i: ";
  }
  this->TooltipDefaultLabelFormat += "%x,  %y";

  this->BuildTime.Modified();
  return true;
}

//-----------------------------------------------------------------------------
void svtkPlotBar::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//-----------------------------------------------------------------------------

void svtkPlotBar::SetInputArray(int index, const svtkStdString& name)
{
  if (index == 0 || index == 1)
  {
    svtkPlot::SetInputArray(index, name);
  }
  else
  {
    this->Private->AdditionalSeries[index] = name;
  }
  this->AutoLabels = nullptr; // No longer valid
}

//-----------------------------------------------------------------------------
void svtkPlotBar::SetColorSeries(svtkColorSeries* colorSeries)
{
  if (this->ColorSeries == colorSeries)
  {
    return;
  }
  this->ColorSeries = colorSeries;
  this->Modified();
}

//-----------------------------------------------------------------------------
svtkColorSeries* svtkPlotBar::GetColorSeries()
{
  return this->ColorSeries;
}

//-----------------------------------------------------------------------------
void svtkPlotBar::SetLookupTable(svtkScalarsToColors* lut)
{
  if (this->LookupTable != lut)
  {
    this->LookupTable = lut;
    this->Modified();
  }
}

//-----------------------------------------------------------------------------
svtkScalarsToColors* svtkPlotBar::GetLookupTable()
{
  if (!this->LookupTable)
  {
    this->CreateDefaultLookupTable();
  }
  return this->LookupTable;
}

//-----------------------------------------------------------------------------
void svtkPlotBar::CreateDefaultLookupTable()
{
  svtkSmartPointer<svtkLookupTable> lut = svtkSmartPointer<svtkLookupTable>::New();
  // rainbow - blue to red
  lut->SetHueRange(0.6667, 0.0);
  lut->Build();
  double bounds[4];
  this->GetBounds(bounds);
  lut->SetRange(bounds[0], bounds[1]);
  this->LookupTable = lut;
}

//-----------------------------------------------------------------------------
void svtkPlotBar::SelectColorArray(const svtkStdString& arrayName)
{
  if (this->ColorArrayName == arrayName)
  {
    return;
  }
  svtkTable* table = this->Data->GetInput();
  if (!table)
  {
    svtkWarningMacro(<< "SelectColorArray called with no input table set.");
    return;
  }
  for (svtkIdType i = 0; i < table->GetNumberOfColumns(); ++i)
  {
    if (arrayName == table->GetColumnName(i))
    {
      this->ColorArrayName = arrayName;
      this->Modified();
      return;
    }
  }
  svtkDebugMacro(<< "SelectColorArray called with invalid column name.");
  this->ColorArrayName = "";
  this->Modified();
}

//-----------------------------------------------------------------------------
void svtkPlotBar::SelectColorArray(svtkIdType arrayNum)
{
  svtkTable* table = this->Data->GetInput();
  if (!table)
  {
    svtkWarningMacro(<< "SelectColorArray called with no input table set.");
    return;
  }
  svtkDataArray* col = svtkArrayDownCast<svtkDataArray>(table->GetColumn(arrayNum));
  // TODO: Should add support for categorical coloring & try enum lookup
  if (!col)
  {
    svtkDebugMacro(<< "SelectColorArray called with invalid column index");
    return;
  }
  else
  {
    const char* arrayName = table->GetColumnName(arrayNum);
    if (this->ColorArrayName == arrayName || arrayName == nullptr)
    {
      return;
    }
    else
    {
      this->ColorArrayName = arrayName;
      this->Modified();
    }
  }
}

//-----------------------------------------------------------------------------
svtkStdString svtkPlotBar::GetColorArrayName()
{
  return this->ColorArrayName;
}

//-----------------------------------------------------------------------------
bool svtkPlotBar::SelectPoints(const svtkVector2f& min, const svtkVector2f& max)
{
  if (!this->Selection)
  {
    this->Selection = svtkIdTypeArray::New();
  }
  this->Selection->SetNumberOfTuples(0);

  return this->Private->SelectPoints(min, max, this->Width, this->Offset, this->Orientation);
}

//-----------------------------------------------------------------------------
svtkStdString svtkPlotBar::GetTooltipLabel(
  const svtkVector2d& plotPos, svtkIdType seriesIndex, svtkIdType segmentIndex)
{
  svtkStdString baseLabel = Superclass::GetTooltipLabel(plotPos, seriesIndex, segmentIndex);
  svtkStdString tooltipLabel;
  bool escapeNext = false;
  for (size_t i = 0; i < baseLabel.length(); ++i)
  {
    if (escapeNext)
    {
      switch (baseLabel[i])
      {
        case 's':
          if (segmentIndex >= 0 && this->GetLabels() &&
            segmentIndex < this->GetLabels()->GetNumberOfTuples())
          {
            tooltipLabel += this->GetLabels()->GetValue(segmentIndex);
          }
          break;
        default: // If no match, insert the entire format tag
          tooltipLabel += "%";
          tooltipLabel += baseLabel[i];
          break;
      }
      escapeNext = false;
    }
    else
    {
      if (baseLabel[i] == '%')
      {
        escapeNext = true;
      }
      else
      {
        tooltipLabel += baseLabel[i];
      }
    }
  }
  return tooltipLabel;
}

//-----------------------------------------------------------------------------
int svtkPlotBar::GetBarsCount()
{
  svtkTable* table = this->Data->GetInput();
  if (!table)
  {
    svtkWarningMacro(<< "GetBarsCount called with no input table set.");
    return 0;
  }
  svtkDataArray* x = this->Data->GetInputArrayToProcess(0, table);
  return x ? x->GetNumberOfTuples() : 0;
}

//-----------------------------------------------------------------------------
void svtkPlotBar::GetDataBounds(double bounds[2])
{
  assert(bounds);
  // Get the x and y arrays (index 0 and 1 respectively)
  svtkTable* table = this->Data->GetInput();
  if (!table)
  {
    svtkWarningMacro(<< "GetDataBounds called with no input table set.");
    bounds[0] = SVTK_DOUBLE_MAX;
    bounds[1] = SVTK_DOUBLE_MIN;
    return;
  }
  svtkDataArray* x = this->Data->GetInputArrayToProcess(0, table);
  if (x)
  {
    x->GetRange(bounds);
  }
}
