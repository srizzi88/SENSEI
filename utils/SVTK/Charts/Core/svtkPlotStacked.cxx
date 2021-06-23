/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPlotStacked.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkPlotStacked.h"

#include "svtkAxis.h"
#include "svtkBrush.h"
#include "svtkChartXY.h"
#include "svtkColorSeries.h"
#include "svtkContext2D.h"
#include "svtkContextMapper2D.h"
#include "svtkDataArray.h"
#include "svtkFloatArray.h"
#include "svtkIdTypeArray.h"
#include "svtkImageData.h"
#include "svtkMath.h"
#include "svtkNew.h"
#include "svtkObjectFactory.h"
#include "svtkPen.h"
#include "svtkPoints2D.h"
#include "svtkSmartPointer.h"
#include "svtkStringArray.h"
#include "svtkTable.h"

#include <algorithm>
#include <map>
#include <vector>

//-----------------------------------------------------------------------------
namespace
{

// Compare the two vectors, in X component only
bool compVector2fX(const svtkVector2f& v1, const svtkVector2f& v2)
{
  if (v1.GetX() < v2.GetX())
  {
    return true;
  }
  else
  {
    return false;
  }
}

// Copy the two arrays into the points array
template <class A, class B>
void CopyToPoints(
  svtkPoints2D* points, svtkPoints2D* previous_points, A* a, B* b, int n, double bds[4])
{
  points->SetNumberOfPoints(n);
  for (int i = 0; i < n; ++i)
  {
    double prev[] = { 0.0, 0.0 };
    if (previous_points)
      previous_points->GetPoint(i, prev);
    double yi = b[i] + prev[1];
    points->SetPoint(i, a[i], yi);

    bds[0] = bds[0] < a[i] ? bds[0] : a[i];
    bds[1] = bds[1] > a[i] ? bds[1] : a[i];

    bds[2] = bds[2] < yi ? bds[2] : yi;
    bds[3] = bds[3] > yi ? bds[3] : yi;
  }
}

// Copy one array into the points array, use the index of that array as x
template <class A>
void CopyToPoints(svtkPoints2D* points, svtkPoints2D* previous_points, A* a, int n, double bds[4])
{
  bds[0] = 0.;
  bds[1] = n - 1.;
  points->SetNumberOfPoints(n);
  for (int i = 0; i < n; ++i)
  {
    double prev[] = { 0.0, 0.0 };
    if (previous_points)
      previous_points->GetPoint(i, prev);
    double yi = a[i] + prev[1];
    points->SetPoint(i, i, yi);

    bds[2] = bds[2] < yi ? bds[2] : yi;
    bds[3] = bds[3] > yi ? bds[3] : yi;
  }
}

// Copy the two arrays into the points array
template <class A>
void CopyToPointsSwitch(
  svtkPoints2D* points, svtkPoints2D* previous_points, A* a, svtkDataArray* b, int n, double bds[4])
{
  switch (b->GetDataType())
  {
    svtkTemplateMacro(
      CopyToPoints(points, previous_points, a, static_cast<SVTK_TT*>(b->GetVoidPointer(0)), n, bds));
  }
}

} // namespace

class svtkPlotStackedSegment : public svtkObject
{
public:
  svtkTypeMacro(svtkPlotStackedSegment, svtkObject);
  static svtkPlotStackedSegment* New();

  svtkPlotStackedSegment()
  {
    this->Stacked = nullptr;
    this->Points = nullptr;
    this->BadPoints = nullptr;
    this->Previous = nullptr;
    this->Sorted = false;
  }

  void Configure(svtkPlotStacked* stacked, svtkDataArray* x_array, svtkDataArray* y_array,
    svtkPlotStackedSegment* prev, double bds[4])
  {
    this->Stacked = stacked;
    this->Sorted = false;
    this->Previous = prev;

    if (!this->Points)
    {
      this->Points = svtkSmartPointer<svtkPoints2D>::New();
    }

    if (x_array)
    {
      switch (x_array->GetDataType())
      {
        svtkTemplateMacro(
          CopyToPointsSwitch(this->Points, this->Previous ? this->Previous->Points : nullptr,
            static_cast<SVTK_TT*>(x_array->GetVoidPointer(0)), y_array, x_array->GetNumberOfTuples(),
            bds));
      }
    }
    else
    { // Using Index for X Series
      switch (y_array->GetDataType())
      {
        svtkTemplateMacro(
          CopyToPoints(this->Points, this->Previous ? this->Previous->Points : nullptr,
            static_cast<SVTK_TT*>(y_array->GetVoidPointer(0)), y_array->GetNumberOfTuples(), bds));
      }
    }

    // Nothing works if we're not sorted on the X access
    svtkIdType n = this->Points->GetNumberOfPoints();
    svtkVector2f* data = static_cast<svtkVector2f*>(this->Points->GetVoidPointer(0));
    std::vector<svtkVector2f> v(data, data + n);
    std::sort(v.begin(), v.end(), compVector2fX);

    this->CalculateLogSeries();
    this->FindBadPoints();
  }

  void CalculateLogSeries()
  {
    svtkAxis* xAxis = this->Stacked->GetXAxis();
    svtkAxis* yAxis = this->Stacked->GetYAxis();

    if (!xAxis || !yAxis)
    {
      return;
    }

    bool logX = xAxis->GetLogScaleActive();
    bool logY = yAxis->GetLogScaleActive();

    float* data = static_cast<float*>(this->Points->GetVoidPointer(0));

    svtkIdType n = this->Points->GetNumberOfPoints();
    if (logX)
    {
      for (svtkIdType i = 0; i < n; ++i)
      {
        data[2 * i] = log10(data[2 * i]);
      }
    }
    if (logY)
    {
      for (svtkIdType i = 0; i < n; ++i)
      {
        data[2 * i + 1] = log10(data[2 * i + 1]);
      }
    }
  }

  void FindBadPoints()
  {
    // This should be run after CalculateLogSeries as a final step.
    float* data = static_cast<float*>(this->Points->GetVoidPointer(0));
    svtkIdType n = this->Points->GetNumberOfPoints();
    if (!this->BadPoints)
    {
      this->BadPoints = svtkSmartPointer<svtkIdTypeArray>::New();
    }
    else
    {
      this->BadPoints->SetNumberOfTuples(0);
    }

    // Scan through and find any bad points.
    for (svtkIdType i = 0; i < n; ++i)
    {
      svtkIdType p = 2 * i;
      if (svtkMath::IsInf(data[p]) || svtkMath::IsInf(data[p + 1]) || svtkMath::IsNan(data[p]) ||
        svtkMath::IsNan(data[p + 1]))
      {
        this->BadPoints->InsertNextValue(i);
      }
    }

    if (this->BadPoints->GetNumberOfTuples() == 0)
    {
      this->BadPoints = nullptr;
    }
  }

  void GetBounds(double bounds[4])
  {
    bounds[0] = bounds[1] = bounds[2] = bounds[3] = 0.0;
    if (this->Points)
    {
      if (!this->BadPoints)
      {
        this->Points->GetBounds(bounds);
      }
      else
      {
        // There are bad points in the series - need to do this ourselves.
        this->CalculateBounds(bounds);
      }
    }
  }

  void CalculateBounds(double bounds[4])
  {
    // We can use the BadPoints array to skip the bad points
    if (!this->Points || !this->BadPoints)
    {
      return;
    }
    svtkIdType start = 0;
    svtkIdType end = 0;
    svtkIdType i = 0;
    svtkIdType nBad = this->BadPoints->GetNumberOfTuples();
    svtkIdType nPoints = this->Points->GetNumberOfPoints();
    if (this->BadPoints->GetValue(i) == 0)
    {
      while (i < nBad && i == this->BadPoints->GetValue(i))
      {
        start = this->BadPoints->GetValue(i++) + 1;
      }
      if (start >= nPoints)
      {
        // They are all bad points
        return;
      }
    }
    if (i < nBad)
    {
      end = this->BadPoints->GetValue(i++);
    }
    else
    {
      end = nPoints;
    }
    svtkVector2f* pts = static_cast<svtkVector2f*>(this->Points->GetVoidPointer(0));

    // Initialize our min/max
    bounds[0] = bounds[1] = pts[start].GetX();
    bounds[2] = bounds[3] = pts[start++].GetY();

    while (start < nPoints)
    {
      // Calculate the min/max in this range
      while (start < end)
      {
        float x = pts[start].GetX();
        float y = pts[start++].GetY();
        if (x < bounds[0])
        {
          bounds[0] = x;
        }
        else if (x > bounds[1])
        {
          bounds[1] = x;
        }
        if (y < bounds[2])
        {
          bounds[2] = y;
        }
        else if (y > bounds[3])
        {
          bounds[3] = y;
        }
      }
      // Now figure out the next range
      start = end + 1;
      if (++i < nBad)
      {
        end = this->BadPoints->GetValue(i);
      }
      else
      {
        end = nPoints;
      }
    }
  }

  void Paint(svtkContext2D* painter, svtkPen* pen, svtkBrush* brush)
  {
    painter->ApplyPen(pen);
    painter->ApplyBrush(brush);
    int n = this->Points->GetNumberOfPoints();
    float* data_extent = svtkArrayDownCast<svtkFloatArray>(this->Points->GetData())->GetPointer(0);
    float* data_base = nullptr;
    if (this->Previous)
      data_base = svtkArrayDownCast<svtkFloatArray>(this->Previous->Points->GetData())->GetPointer(0);

    if (n >= 2)
    {
      float poly_points[8];

      for (int i = 0; i < (n - 1); ++i)
      {
        if (data_base)
        {
          poly_points[0] = data_base[2 * i];
          poly_points[1] = data_base[2 * i + 1];
          poly_points[2] = data_base[2 * i + 2];
          poly_points[3] = data_base[2 * i + 3];
        }
        else
        {
          poly_points[0] = data_extent[2 * i]; // Use the same X as extent
          poly_points[1] = 0.0;
          poly_points[2] = data_extent[2 * i + 2]; // Use the same X as extent
          poly_points[3] = 0.0;
        }
        poly_points[4] = data_extent[2 * i + 2];
        poly_points[5] = data_extent[2 * i + 3];
        poly_points[6] = data_extent[2 * i];
        poly_points[7] = data_extent[2 * i + 1];

        painter->DrawQuad(poly_points);
      }
    }
  }

  bool GetNearestPoint(const svtkVector2f& point, const svtkVector2f& tol, svtkVector2f* location)
  {
    // Right now doing a simple bisector search of the array. This should be
    // revisited. Assumes the x axis is sorted, which should always be true for
    // line plots.
    if (!this->Points)
    {
      return false;
    }
    svtkIdType n = this->Points->GetNumberOfPoints();
    if (n < 2)
    {
      return false;
    }

    // Set up our search array, use the STL lower_bound algorithm
    // When searching, invert the behavior of the offset and
    // compensate for the half width overlap.
    std::vector<svtkVector2f>::iterator low;
    svtkVector2f lowPoint(point.GetX() - tol.GetX(), 0.0f);

    svtkVector2f* data = static_cast<svtkVector2f*>(this->Points->GetVoidPointer(0));
    std::vector<svtkVector2f> v(data, data + n);

    low = std::lower_bound(v.begin(), v.end(), lowPoint, compVector2fX);

    // Now consider the y axis.  We only worry about our extent
    // to the base because each segment is called in order and the
    // first positive wins.
    while (low != v.end())
    {
      if (low->GetX() - tol.GetX() > point.GetX())
      {
        break;
      }
      else if (low->GetX() - tol.GetX() < point.GetX() && low->GetX() + tol.GetX() > point.GetX())
      {
        if ((point.GetY() >= 0 && point.GetY() < low->GetY()) ||
          (point.GetY() < 0 && point.GetY() > low->GetY()))
        {
          *location = *low;
          return true;
        }
      }
      ++low;
    }
    return false;
  }

  void SelectPoints(const svtkVector2f& min, const svtkVector2f& max, svtkIdTypeArray* selection)
  {
    if (!this->Points)
    {
      return;
    }

    // Iterate through all points and check whether any are in range
    svtkVector2f* data = static_cast<svtkVector2f*>(this->Points->GetVoidPointer(0));
    svtkIdType n = this->Points->GetNumberOfPoints();

    for (svtkIdType i = 0; i < n; ++i)
    {
      if (data[i].GetX() >= min.GetX() && data[i].GetX() <= max.GetX() &&
        data[i].GetY() >= min.GetY() && data[i].GetY() <= max.GetY())
      {
        selection->InsertNextValue(i);
      }
    }
  }

  svtkSmartPointer<svtkPlotStackedSegment> Previous;
  svtkSmartPointer<svtkPoints2D> Points;
  svtkSmartPointer<svtkIdTypeArray> BadPoints;
  svtkPlotStacked* Stacked;
  bool Sorted;
};

svtkStandardNewMacro(svtkPlotStackedSegment);

//-----------------------------------------------------------------------------

class svtkPlotStackedPrivate
{
public:
  svtkPlotStackedPrivate(svtkPlotStacked* stacked)
    : Stacked(stacked)
  {
  }
  void Update()
  {
    this->Segments.clear();
    this->UnscaledInputBounds[0] = this->UnscaledInputBounds[2] = svtkMath::Inf();
    this->UnscaledInputBounds[1] = this->UnscaledInputBounds[3] = -svtkMath::Inf();
  }

  svtkPlotStackedSegment* AddSegment(
    svtkDataArray* x_array, svtkDataArray* y_array, svtkPlotStackedSegment* prev = nullptr)
  {
    svtkSmartPointer<svtkPlotStackedSegment> segment = svtkSmartPointer<svtkPlotStackedSegment>::New();
    segment->Configure(this->Stacked, x_array, y_array, prev, this->UnscaledInputBounds);
    this->Segments.push_back(segment);
    return segment;
  }

  void PaintSegments(
    svtkContext2D* painter, svtkColorSeries* colorSeries, svtkPen* pen, svtkBrush* brush)
  {
    int colorInSeries = 0;
    bool useColorSeries = this->Segments.size() > 1;
    for (std::vector<svtkSmartPointer<svtkPlotStackedSegment> >::iterator it = this->Segments.begin();
         it != this->Segments.end(); ++it)
    {
      if (useColorSeries && colorSeries)
        brush->SetColor(colorSeries->GetColorRepeating(colorInSeries++).GetData());
      (*it)->Paint(painter, pen, brush);
    }
  }

  svtkIdType GetNearestPoint(const svtkVector2f& point, const svtkVector2f& tol, svtkVector2f* location)
  {
    // Depends on the fact that we check the segments in order. Each
    // Segment only worrys about its own total extent from the base.
    int index = 0;
    for (std::vector<svtkSmartPointer<svtkPlotStackedSegment> >::iterator it = this->Segments.begin();
         it != this->Segments.end(); ++it)
    {
      if ((*it)->GetNearestPoint(point, tol, location))
      {
        return index;
      }
      ++index;
    }
    return -1;
  }

  void GetBounds(double bounds[4])
  {
    // Depends on the fact that we check the segments in order. Each
    // Segment only worrys about its own total extent from the base.
    double segment_bounds[4];
    for (std::vector<svtkSmartPointer<svtkPlotStackedSegment> >::iterator it = this->Segments.begin();
         it != this->Segments.end(); ++it)
    {
      (*it)->GetBounds(segment_bounds);
      if (segment_bounds[0] < bounds[0])
      {
        bounds[0] = segment_bounds[0];
      }
      if (segment_bounds[1] > bounds[1])
      {
        bounds[1] = segment_bounds[1];
      }
      if (segment_bounds[2] < bounds[2])
      {
        bounds[2] = segment_bounds[2];
      }
      if (segment_bounds[3] > bounds[3])
      {
        bounds[3] = segment_bounds[3];
      }
    }
  }

  void SelectPoints(const svtkVector2f& min, const svtkVector2f& max, svtkIdTypeArray* selection)
  {
    for (std::vector<svtkSmartPointer<svtkPlotStackedSegment> >::iterator it = this->Segments.begin();
         it != this->Segments.end(); ++it)
    {
      (*it)->SelectPoints(min, max, selection);
    }
  }

  std::vector<svtkSmartPointer<svtkPlotStackedSegment> > Segments;
  svtkPlotStacked* Stacked;
  std::map<int, std::string> AdditionalSeries;
  double UnscaledInputBounds[4];
};

//-----------------------------------------------------------------------------
svtkStandardNewMacro(svtkPlotStacked);

//-----------------------------------------------------------------------------
svtkPlotStacked::svtkPlotStacked()
{
  this->Private = new svtkPlotStackedPrivate(this);
  this->BaseBadPoints = nullptr;
  this->ExtentBadPoints = nullptr;
  this->AutoLabels = nullptr;
  this->Pen->SetColor(0, 0, 0, 0);
  this->LogX = false;
  this->LogY = false;
}

//-----------------------------------------------------------------------------
svtkPlotStacked::~svtkPlotStacked()
{
  if (this->BaseBadPoints)
  {
    this->BaseBadPoints->Delete();
    this->BaseBadPoints = nullptr;
  }
  if (this->ExtentBadPoints)
  {
    this->ExtentBadPoints->Delete();
    this->ExtentBadPoints = nullptr;
  }

  delete this->Private;
}

//-----------------------------------------------------------------------------
void svtkPlotStacked::SetColor(unsigned char r, unsigned char g, unsigned char b, unsigned char a)
{
  this->Brush->SetColor(r, g, b, a);
}

//-----------------------------------------------------------------------------
void svtkPlotStacked::SetColor(double r, double g, double b)
{
  this->Brush->SetColorF(r, g, b);
}

//-----------------------------------------------------------------------------
void svtkPlotStacked::GetColor(double rgb[3])
{
  this->Brush->GetColorF(rgb);
}

//-----------------------------------------------------------------------------
void svtkPlotStacked::Update()
{
  if (!this->Visible)
  {
    return;
  }
  // Check if we have an input
  svtkTable* table = this->Data->GetInput();
  if (!table)
  {
    svtkDebugMacro(<< "Update event called with no input table set.");
    return;
  }
  else if (this->Data->GetMTime() > this->BuildTime || table->GetMTime() > this->BuildTime ||
    this->MTime > this->BuildTime)
  {
    svtkDebugMacro(<< "Updating cached values.");
    this->UpdateTableCache(table);
  }
  else if ((this->XAxis->GetMTime() > this->BuildTime) ||
    (this->YAxis->GetMTime() > this->BuildTime))
  {
    if (this->LogX != this->XAxis->GetLogScaleActive() ||
      this->LogY != this->YAxis->GetLogScaleActive())
    {
      this->UpdateTableCache(table);
    }
  }
}

//-----------------------------------------------------------------------------
bool svtkPlotStacked::Paint(svtkContext2D* painter)
{
  // This is where everything should be drawn, or dispatched to other methods.
  svtkDebugMacro(<< "Paint event called in svtkPlotStacked.");

  if (!this->Visible)
  {
    return false;
  }

  // Now add some decorations for our selected points...
  if (this->Selection)
  {
    svtkDebugMacro(<< "Selection set " << this->Selection->GetNumberOfTuples());
  }
  else
  {
    svtkDebugMacro("No selection set.");
  }

  this->Private->PaintSegments(painter, this->ColorSeries, this->Pen, this->Brush);

  return true;
}

//-----------------------------------------------------------------------------
bool svtkPlotStacked::PaintLegend(svtkContext2D* painter, const svtkRectf& rect, int legendIndex)
{
  if (this->ColorSeries)
  {
    svtkNew<svtkPen> pen;
    svtkNew<svtkBrush> brush;
    pen->SetColor(this->ColorSeries->GetColorRepeating(legendIndex).GetData());
    brush->SetColor(pen->GetColor());
    painter->ApplyPen(pen);
    painter->ApplyBrush(brush);
  }
  else
  {
    painter->ApplyPen(this->Pen);
    painter->ApplyBrush(this->Brush);
  }
  painter->DrawRect(rect[0], rect[1], rect[2], rect[3]);
  return true;
}

//-----------------------------------------------------------------------------
void svtkPlotStacked::GetBounds(double bounds[4])
{
  this->Private->GetBounds(bounds);
}

//-----------------------------------------------------------------------------
void svtkPlotStacked::GetUnscaledInputBounds(double bounds[4])
{
  for (int i = 0; i < 4; ++i)
  {
    bounds[i] = this->Private->UnscaledInputBounds[i];
  }
}

//-----------------------------------------------------------------------------
svtkIdType svtkPlotStacked::GetNearestPoint(const svtkVector2f& point, const svtkVector2f& tol,
  svtkVector2f* location, svtkIdType* svtkNotUsed(segmentId))
{
#ifndef SVTK_LEGACY_REMOVE
  if (!this->LegacyRecursionFlag)
  {
    this->LegacyRecursionFlag = true;
    svtkIdType ret = this->GetNearestPoint(point, tol, location);
    this->LegacyRecursionFlag = false;
    if (ret != -1)
    {
      SVTK_LEGACY_REPLACED_BODY(svtkPlotStacked::GetNearestPoint(const svtkVector2f& point,
                                 const svtkVector2f& tol, svtkVector2f* location),
        "SVTK 9.0",
        svtkPlotStacked::GetNearestPoint(const svtkVector2f& point, const svtkVector2f& tol,
          svtkVector2f* location, svtkIdType* segmentId));
      return ret;
    }
  }
#endif // SVTK_LEGACY_REMOVE

  return this->Private->GetNearestPoint(point, tol, location);
}

//-----------------------------------------------------------------------------
bool svtkPlotStacked::SelectPoints(const svtkVector2f& min, const svtkVector2f& max)
{
  if (!this->Selection)
  {
    this->Selection = svtkIdTypeArray::New();
  }
  this->Selection->SetNumberOfTuples(0);

  this->Private->SelectPoints(min, max, this->Selection);

  return this->Selection->GetNumberOfTuples() > 0;
}

//-----------------------------------------------------------------------------
svtkStringArray* svtkPlotStacked::GetLabels()
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

//-----------------------------------------------------------------------------
bool svtkPlotStacked::UpdateTableCache(svtkTable* table)
{
  // Get the x and ybase and yextent arrays (index 0 1 2 respectively)
  svtkDataArray* x =
    this->UseIndexForXSeries ? nullptr : this->Data->GetInputArrayToProcess(0, table);
  svtkDataArray* y = this->Data->GetInputArrayToProcess(1, table);

  if (!x && !this->UseIndexForXSeries)
  {
    svtkErrorMacro(<< "No X column is set (index 0).");
    this->BuildTime.Modified();
    return false;
  }
  else if (!y)
  {
    svtkErrorMacro(<< "No Y column is set (index 1).");
    this->BuildTime.Modified();
    return false;
  }
  else if (!this->UseIndexForXSeries && x->GetNumberOfTuples() != y->GetNumberOfTuples())
  {
    svtkErrorMacro("The x and y columns must have the same number of elements. "
      << x->GetNumberOfTuples() << ", " << y->GetNumberOfTuples() << ", "
      << y->GetNumberOfTuples());
    this->BuildTime.Modified();
    return false;
  }
  this->Private->Update();

  svtkPlotStackedSegment* prev = this->Private->AddSegment(x, y);

  std::map<int, std::string>::iterator it;

  for (it = this->Private->AdditionalSeries.begin(); it != this->Private->AdditionalSeries.end();
       ++it)
  {
    y = svtkArrayDownCast<svtkDataArray>(table->GetColumnByName((*it).second.c_str()));
    prev = this->Private->AddSegment(x, y, prev);
  }

  // Record if this update was done with Log scale.
  this->LogX = this->XAxis ? this->XAxis->GetLogScaleActive() : false;
  this->LogY = this->YAxis ? this->YAxis->GetLogScaleActive() : false;

  this->BuildTime.Modified();
  return true;
}

//-----------------------------------------------------------------------------
void svtkPlotStacked::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//-----------------------------------------------------------------------------

void svtkPlotStacked::SetInputArray(int index, const svtkStdString& name)
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
void svtkPlotStacked::SetColorSeries(svtkColorSeries* colorSeries)
{
  if (this->ColorSeries == colorSeries)
  {
    return;
  }
  this->ColorSeries = colorSeries;
  this->Modified();
}

//-----------------------------------------------------------------------------
svtkColorSeries* svtkPlotStacked::GetColorSeries()
{
  return this->ColorSeries;
}
