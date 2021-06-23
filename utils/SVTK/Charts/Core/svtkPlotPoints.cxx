/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPlotPoints.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkPlotPoints.h"

#include "svtkAxis.h"
#include "svtkBrush.h"
#include "svtkCharArray.h"
#include "svtkContext2D.h"
#include "svtkContextMapper2D.h"
#include "svtkFloatArray.h"
#include "svtkIdTypeArray.h"
#include "svtkImageData.h"
#include "svtkLookupTable.h"
#include "svtkMath.h"
#include "svtkNew.h"
#include "svtkObjectFactory.h"
#include "svtkPen.h"
#include "svtkPoints2D.h"
#include "svtkTable.h"
#include "svtkUnsignedCharArray.h"

#include <algorithm>
#include <limits>
#include <set>
#include <vector>

// PIMPL for STL vector...
struct svtkIndexedVector2f
{
  size_t index;
  svtkVector2f pos;
};

class svtkPlotPoints::VectorPIMPL : public std::vector<svtkIndexedVector2f>
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

//-----------------------------------------------------------------------------
svtkStandardNewMacro(svtkPlotPoints);

//-----------------------------------------------------------------------------
svtkPlotPoints::svtkPlotPoints()
{
  this->Points = nullptr;
  this->Sorted = nullptr;
  this->BadPoints = nullptr;
  this->ValidPointMask = nullptr;
  this->MarkerStyle = svtkPlotPoints::CIRCLE;
  this->MarkerSize = -1.0;
  this->LogX = false;
  this->LogY = false;

  this->LookupTable = nullptr;
  this->Colors = nullptr;
  this->ScalarVisibility = 0;

  this->UnscaledInputBounds[0] = this->UnscaledInputBounds[2] = svtkMath::Inf();
  this->UnscaledInputBounds[1] = this->UnscaledInputBounds[3] = -svtkMath::Inf();
}

//-----------------------------------------------------------------------------
svtkPlotPoints::~svtkPlotPoints()
{
  if (this->Points)
  {
    this->Points->Delete();
    this->Points = nullptr;
  }
  delete this->Sorted;
  if (this->BadPoints)
  {
    this->BadPoints->Delete();
    this->BadPoints = nullptr;
  }
  if (this->LookupTable)
  {
    this->LookupTable->UnRegister(this);
  }
  if (this->Colors != nullptr)
  {
    this->Colors->UnRegister(this);
  }
}

//-----------------------------------------------------------------------------
void svtkPlotPoints::Update()
{
  if (!this->Visible)
  {
    return;
  }
  // Check if we have an input
  svtkTable* table = this->Data->GetInput();

  if (table && !this->ValidPointMaskName.empty() &&
    table->GetColumnByName(this->ValidPointMaskName))
  {
    this->ValidPointMask =
      svtkArrayDownCast<svtkCharArray>(table->GetColumnByName(this->ValidPointMaskName));
  }
  else
  {
    this->ValidPointMask = nullptr;
  }

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
  else if (this->XAxis && this->YAxis &&
    ((this->XAxis->GetMTime() > this->BuildTime) || (this->YAxis->GetMTime() > this->BuildTime)))
  {
    if ((this->LogX != this->XAxis->GetLogScale()) || (this->LogY != this->YAxis->GetLogScale()))
    {
      this->UpdateTableCache(table);
    }
  }
}

//-----------------------------------------------------------------------------
bool svtkPlotPoints::Paint(svtkContext2D* painter)
{
  // This is where everything should be drawn, or dispatched to other methods.
  svtkDebugMacro(<< "Paint event called in svtkPlotPoints.");

  if (!this->Visible || !this->Points || this->Points->GetNumberOfPoints() == 0)
  {
    return false;
  }

  // Maintain legacy behavior (using pen width) if MarkerSize was not set
  float width = this->MarkerSize;
  if (width < 0.0f)
  {
    width = this->Pen->GetWidth() * 2.3;
    if (width < 8.0)
    {
      width = 8.0;
    }
  }

  // If there is a marker style, then draw the marker for each point too
  if (this->MarkerStyle != SVTK_MARKER_NONE)
  {
    painter->ApplyPen(this->Pen);
    painter->ApplyBrush(this->Brush);
    painter->GetPen()->SetWidth(width);

    float* points = static_cast<float*>(this->Points->GetVoidPointer(0));
    unsigned char* colors = nullptr;
    int nColorComponents = 0;
    if (this->ScalarVisibility && this->Colors)
    {
      colors = this->Colors->GetPointer(0);
      nColorComponents = static_cast<int>(this->Colors->GetNumberOfComponents());
    }

    if (this->BadPoints && this->BadPoints->GetNumberOfTuples() > 0)
    {
      svtkIdType lastGood = 0;
      svtkIdType bpIdx = 0;
      svtkIdType nPoints = this->Points->GetNumberOfPoints();
      svtkIdType nBadPoints = this->BadPoints->GetNumberOfTuples();

      while (lastGood < nPoints)
      {
        svtkIdType id =
          bpIdx < nBadPoints ? this->BadPoints->GetValue(bpIdx) : this->Points->GetNumberOfPoints();

        // render from last good point to one before this bad point
        if (id - lastGood > 0)
        {
          painter->DrawMarkers(this->MarkerStyle, false, points + 2 * (lastGood), id - lastGood,
            colors ? colors + 4 * (lastGood) : nullptr, nColorComponents);
        }
        lastGood = id + 1;
        bpIdx++;
      }
    }
    else
    {
      // draw all of the points
      painter->DrawMarkers(this->MarkerStyle, false, points, this->Points->GetNumberOfPoints(),
        colors, nColorComponents);
    }
  }

  // Now add some decorations for our selected points...
  if (this->Selection && this->Selection->GetNumberOfTuples())
  {
    if (this->Selection->GetMTime() > this->SelectedPoints->GetMTime() ||
      this->GetMTime() > this->SelectedPoints->GetMTime())
    {
      float* f = svtkArrayDownCast<svtkFloatArray>(this->Points->GetData())->GetPointer(0);
      int nSelected(static_cast<int>(this->Selection->GetNumberOfTuples()));
      this->SelectedPoints->SetNumberOfComponents(2);
      this->SelectedPoints->SetNumberOfTuples(nSelected);
      float* selectedPtr = static_cast<float*>(this->SelectedPoints->GetVoidPointer(0));
      for (int i = 0; i < nSelected; ++i)
      {
        *(selectedPtr++) = f[2 * this->Selection->GetValue(i)];
        *(selectedPtr++) = f[2 * this->Selection->GetValue(i) + 1];
      }
    }
    svtkDebugMacro(<< "Selection set " << this->Selection->GetNumberOfTuples());
    painter->GetPen()->SetColor(this->SelectionPen->GetColor());
    painter->GetPen()->SetOpacity(this->SelectionPen->GetOpacity());
    painter->GetPen()->SetWidth(width + 2.7);

    if (this->MarkerStyle == SVTK_MARKER_NONE)
    {
      painter->DrawMarkers(SVTK_MARKER_PLUS, false,
        static_cast<float*>(this->SelectedPoints->GetVoidPointer(0)),
        this->SelectedPoints->GetNumberOfTuples());
    }
    else
    {
      painter->DrawMarkers(this->MarkerStyle, true,
        static_cast<float*>(this->SelectedPoints->GetVoidPointer(0)),
        this->SelectedPoints->GetNumberOfTuples());
    }
  }

  return true;
}

//-----------------------------------------------------------------------------
bool svtkPlotPoints::PaintLegend(svtkContext2D* painter, const svtkRectf& rect, int)
{
  if (this->MarkerStyle)
  {
    float width = this->Pen->GetWidth() * 2.3;
    if (width < 8.0)
    {
      width = 8.0;
    }
    painter->ApplyPen(this->Pen);
    painter->ApplyBrush(this->Brush);
    painter->GetPen()->SetWidth(width);

    float point[] = { rect[0] + 0.5f * rect[2], rect[1] + 0.5f * rect[3] };
    painter->DrawMarkers(this->MarkerStyle, false, point, 1);
  }
  return true;
}

//-----------------------------------------------------------------------------
void svtkPlotPoints::GetBounds(double bounds[4])
{
  if (this->Points)
  {
    // There are bad points in the series - need to do this ourselves.
    this->CalculateBounds(bounds);
  }
  svtkDebugMacro(<< "Bounds: " << bounds[0] << "\t" << bounds[1] << "\t" << bounds[2] << "\t"
                << bounds[3]);
}

//-----------------------------------------------------------------------------
void svtkPlotPoints::GetUnscaledInputBounds(double bounds[4])
{
  this->CalculateUnscaledInputBounds();
  for (int i = 0; i < 4; ++i)
  {
    bounds[i] = this->UnscaledInputBounds[i];
  }
  svtkDebugMacro(<< "Bounds: " << bounds[0] << "\t" << bounds[1] << "\t" << bounds[2] << "\t"
                << bounds[3]);
}

namespace
{

bool compVector3fX(const svtkIndexedVector2f& v1, const svtkIndexedVector2f& v2)
{
  if (v1.pos.GetX() < v2.pos.GetX())
  {
    return true;
  }
  else
  {
    return false;
  }
}

// See if the point is within tolerance.
bool inRange(const svtkVector2f& point, const svtkVector2f& tol, const svtkVector2f& current)
{
  if (current.GetX() > point.GetX() - tol.GetX() && current.GetX() < point.GetX() + tol.GetX() &&
    current.GetY() > point.GetY() - tol.GetY() && current.GetY() < point.GetY() + tol.GetY())
  {
    return true;
  }
  else
  {
    return false;
  }
}

}

//-----------------------------------------------------------------------------
void svtkPlotPoints::CreateSortedPoints()
{
  // Sort the data if it has not been done already...
  if (!this->Sorted)
  {
    svtkIdType n = this->Points->GetNumberOfPoints();
    svtkVector2f* data = static_cast<svtkVector2f*>(this->Points->GetVoidPointer(0));
    this->Sorted = new VectorPIMPL(data, n);
    std::sort(this->Sorted->begin(), this->Sorted->end(), compVector3fX);
  }
}

//-----------------------------------------------------------------------------
svtkIdType svtkPlotPoints::GetNearestPoint(const svtkVector2f& point, const svtkVector2f& tol,
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
      SVTK_LEGACY_REPLACED_BODY(svtkPlotPoints::GetNearestPoint(const svtkVector2f& point,
                                 const svtkVector2f& tol, svtkVector2f* location),
        "SVTK 9.0",
        svtkPlotPoints::GetNearestPoint(const svtkVector2f& point, const svtkVector2f& tol,
          svtkVector2f* location, svtkIdType* segmentId));
      return ret;
    }
  }
#endif // SVTK_LEGACY_REMOVE

  // Right now doing a simple bisector search of the array.
  if (!this->Points)
  {
    return -1;
  }
  this->CreateSortedPoints();

  // Set up our search array, use the STL lower_bound algorithm
  VectorPIMPL::iterator low;
  VectorPIMPL& v = *this->Sorted;

  // Get the lowest point we might hit within the supplied tolerance
  svtkIndexedVector2f lowPoint;
  lowPoint.index = 0;
  lowPoint.pos = svtkVector2f(point.GetX() - tol.GetX(), 0.0f);
  low = std::lower_bound(v.begin(), v.end(), lowPoint, compVector3fX);

  // Now consider the y axis
  float highX = point.GetX() + tol.GetX();
  while (low != v.end())
  {
    if (inRange(point, tol, (*low).pos))
    {
      *location = (*low).pos;
      svtkRectd ss = this->GetShiftScale();
      location->SetX((location->GetX() - ss.GetX()) / ss.GetWidth());
      location->SetY((location->GetY() - ss.GetY()) / ss.GetHeight());
      return static_cast<int>((*low).index);
    }
    else if (low->pos.GetX() > highX)
    {
      break;
    }
    ++low;
  }
  return -1;
}

//-----------------------------------------------------------------------------
bool svtkPlotPoints::SelectPoints(const svtkVector2f& min, const svtkVector2f& max)
{
  if (!this->Points)
  {
    return false;
  }
  this->CreateSortedPoints();

  if (!this->Selection)
  {
    this->Selection = svtkIdTypeArray::New();
  }
  this->Selection->SetNumberOfTuples(0);

  // Set up our search array, use the STL lower_bound algorithm
  VectorPIMPL::iterator low;
  VectorPIMPL& v = *this->Sorted;

  // Get the lowest point we might hit within the supplied tolerance
  svtkIndexedVector2f lowPoint;
  lowPoint.index = 0;
  lowPoint.pos = min;
  low = std::lower_bound(v.begin(), v.end(), lowPoint, compVector3fX);

  // Output a sorted selection list too.
  std::vector<svtkIdType> selected;
  // Iterate until we are out of range in X
  while (low != v.end())
  {
    if (low->pos.GetX() >= min.GetX() && low->pos.GetX() <= max.GetX() &&
      low->pos.GetY() >= min.GetY() && low->pos.GetY() <= max.GetY())
    {
      selected.push_back(static_cast<int>(low->index));
    }
    else if (low->pos.GetX() > max.GetX())
    {
      break;
    }
    ++low;
  }
  this->Selection->SetNumberOfTuples(static_cast<svtkIdType>(selected.size()));
  svtkIdType* ptr = static_cast<svtkIdType*>(this->Selection->GetVoidPointer(0));
  for (size_t i = 0; i < selected.size(); ++i)
  {
    ptr[i] = selected[i];
  }
  std::sort(ptr, ptr + selected.size());
  this->Selection->Modified();
  return this->Selection->GetNumberOfTuples() > 0;
}

//-----------------------------------------------------------------------------
bool svtkPlotPoints::SelectPointsInPolygon(const svtkContextPolygon& polygon)
{
  if (!this->Points)
  {
    // nothing to select
    return false;
  }

  if (!this->Selection)
  {
    // create selection object
    this->Selection = svtkIdTypeArray::New();
  }
  else
  {
    // clear previous selection
    this->Selection->SetNumberOfValues(0);
  }

  for (svtkIdType pointId = 0; pointId < this->Points->GetNumberOfPoints(); pointId++)
  {
    // get point location
    double point[3];
    this->Points->GetPoint(pointId, point);

    if (polygon.Contains(svtkVector2f(point[0], point[1])))
    {
      this->Selection->InsertNextValue(pointId);
    }
  }
  this->Selection->Modified();

  // return true if we selected any points
  return this->Selection->GetNumberOfTuples() > 0;
}

//-----------------------------------------------------------------------------
namespace
{

// Find any bad points in the supplied array.
template <typename T>
void SetBadPoints(T* data, svtkIdType n, std::set<svtkIdType>& bad)
{
  for (svtkIdType i = 0; i < n; ++i)
  {
    if (svtkMath::IsInf(data[i]) || svtkMath::IsNan(data[i]))
    {
      bad.insert(i);
    }
  }
}

// Calculate the bounds from the original data.
template <typename A>
void ComputeBounds(A* a, int n, double bounds[2])
{
  bounds[0] = std::numeric_limits<double>::max();
  bounds[1] = -std::numeric_limits<double>::max();
  for (int i = 0; i < n; ++a, ++i)
  {
    bounds[0] = bounds[0] < *a ? bounds[0] : *a;
    bounds[1] = bounds[1] > *a ? bounds[1] : *a;
  }
}

template <typename A>
void ComputeBounds(A* a, int n, svtkIdTypeArray* bad, double bounds[2])
{
  // If possible, use the simpler code without any bad points.
  if (!bad || bad->GetNumberOfTuples() == 0)
  {
    ComputeBounds(a, n, bounds);
    return;
  }

  // Initialize the first range of points.
  svtkIdType start = 0;
  svtkIdType end = 0;
  svtkIdType i = 0;
  svtkIdType nBad = bad->GetNumberOfTuples();
  if (bad->GetValue(i) == 0)
  {
    while (i < nBad && i == bad->GetValue(i))
    {
      start = bad->GetValue(i++) + 1;
    }
    if (start >= n)
    {
      // They are all bad points, return early.
      return;
    }
  }
  if (i < nBad)
  {
    end = bad->GetValue(i++);
  }
  else
  {
    end = n;
  }

  bounds[0] = std::numeric_limits<double>::max();
  bounds[1] = -std::numeric_limits<double>::max();
  while (start < n)
  {
    // Calculate the min/max in this range.
    while (start < end)
    {
      bounds[0] = bounds[0] < a[start] ? bounds[0] : a[start];
      bounds[1] = bounds[1] > a[start] ? bounds[1] : a[start];
      ++start;
    }
    // Now figure out the next range to be evaluated.
    start = end + 1;
    while (i < nBad && start == bad->GetValue(i))
    {
      start = bad->GetValue(i++) + 1;
    }
    if (i < nBad)
    {
      end = bad->GetValue(i++);
    }
    else
    {
      end = n;
    }
  }
}

// Dispatch this call off to the right function.
template <typename A>
void ComputeBounds(A* a, svtkDataArray* b, int n, svtkIdTypeArray* bad, double bounds[4])
{
  ComputeBounds(a, n, bad, bounds);
  switch (b->GetDataType())
  {
    svtkTemplateMacro(ComputeBounds(static_cast<SVTK_TT*>(b->GetVoidPointer(0)), n, bad, &bounds[2]));
  }
}

// Copy the two arrays into the points array
template <typename A, typename B>
void CopyToPoints(svtkPoints2D* points, A* a, B* b, int n, const svtkRectd& ss)
{
  points->SetNumberOfPoints(n);
  float* data = static_cast<float*>(points->GetVoidPointer(0));
  for (int i = 0; i < n; ++i)
  {
    data[2 * i] = static_cast<float>((a[i] + ss[0]) * ss[2]);
    data[2 * i + 1] = static_cast<float>((b[i] + ss[1]) * ss[3]);
  }
}

// Copy one array into the points array, use the index of that array as x
template <typename A>
void CopyToPoints(svtkPoints2D* points, A* a, int n, const svtkRectd& ss)
{
  points->SetNumberOfPoints(n);
  float* data = static_cast<float*>(points->GetVoidPointer(0));
  for (int i = 0; i < n; ++i)
  {
    data[2 * i] = static_cast<float>((i + ss[0]) * ss[2]);
    data[2 * i + 1] = static_cast<float>((a[i] + ss[1]) * ss[3]);
  }
}

// Copy the two arrays into the points array
template <typename A>
void CopyToPointsSwitch(svtkPoints2D* points, A* a, svtkDataArray* b, int n, const svtkRectd& ss)
{
  switch (b->GetDataType())
  {
    svtkTemplateMacro(CopyToPoints(points, a, static_cast<SVTK_TT*>(b->GetVoidPointer(0)), n, ss));
  }
}

}

//-----------------------------------------------------------------------------
bool svtkPlotPoints::GetDataArrays(svtkTable* table, svtkDataArray* array[2])
{
  if (!table)
  {
    return false;
  }

  // Get the x and y arrays (index 0 and 1 respectively)
  array[0] = this->UseIndexForXSeries ? nullptr : this->Data->GetInputArrayToProcess(0, table);
  array[1] = this->Data->GetInputArrayToProcess(1, table);

  if (!array[0] && !this->UseIndexForXSeries)
  {
    svtkErrorMacro(<< "No X column is set (index 0).");
    return false;
  }
  else if (!array[1])
  {
    svtkErrorMacro(<< "No Y column is set (index 1).");
    return false;
  }
  else if (!this->UseIndexForXSeries &&
    array[0]->GetNumberOfTuples() != array[1]->GetNumberOfTuples())
  {
    svtkErrorMacro("The x and y columns must have the same number of elements. "
      << array[0]->GetNumberOfTuples() << ", " << array[1]->GetNumberOfTuples());
    return false;
  }
  return true;
}

//-----------------------------------------------------------------------------
bool svtkPlotPoints::UpdateTableCache(svtkTable* table)
{
  svtkDataArray* array[2] = { nullptr, nullptr };
  if (!this->GetDataArrays(table, array))
  {
    this->BuildTime.Modified();
    return false;
  }

  if (!this->Points)
  {
    this->Points = svtkPoints2D::New();
  }
  svtkDataArray* x = array[0];
  svtkDataArray* y = array[1];

  // Now copy the components into their new columns
  if (this->UseIndexForXSeries)
  {
    switch (y->GetDataType())
    {
      svtkTemplateMacro(CopyToPoints(this->Points, static_cast<SVTK_TT*>(y->GetVoidPointer(0)),
        y->GetNumberOfTuples(), this->ShiftScale));
    }
  }
  else
  {
    switch (x->GetDataType())
    {
      svtkTemplateMacro(CopyToPointsSwitch(this->Points, static_cast<SVTK_TT*>(x->GetVoidPointer(0)),
        y, x->GetNumberOfTuples(), this->ShiftScale));
    }
  }
  this->CalculateLogSeries();
  this->FindBadPoints();
  this->Points->Modified();
  delete this->Sorted;
  this->Sorted = nullptr;

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
      if (this->Colors)
      {
        this->Colors->UnRegister(this);
      }
      this->Colors = this->LookupTable->MapScalars(c, SVTK_COLOR_MODE_MAP_SCALARS, -1);
      // Consistent register and unregisters
      this->Colors->Register(this);
      this->Colors->Delete();
    }
    else
    {
      this->Colors->UnRegister(this);
      this->Colors = nullptr;
    }
  }

  this->BuildTime.Modified();

  return true;
}

//-----------------------------------------------------------------------------
void svtkPlotPoints::CalculateUnscaledInputBounds()
{
  svtkTable* table = this->Data->GetInput();
  svtkDataArray* array[2] = { nullptr, nullptr };
  if (!this->GetDataArrays(table, array))
  {
    return;
  }
  // Now copy the components into their new columns
  if (this->UseIndexForXSeries)
  {
    this->UnscaledInputBounds[0] = 0.0;
    this->UnscaledInputBounds[1] = array[1]->GetNumberOfTuples() - 1;
    switch (array[1]->GetDataType())
    {
      svtkTemplateMacro(ComputeBounds(static_cast<SVTK_TT*>(array[1]->GetVoidPointer(0)),
        array[1]->GetNumberOfTuples(), this->BadPoints, &this->UnscaledInputBounds[2]));
    }
  }
  else
  {
    switch (array[0]->GetDataType())
    {
      svtkTemplateMacro(ComputeBounds(static_cast<SVTK_TT*>(array[0]->GetVoidPointer(0)), array[1],
        array[0]->GetNumberOfTuples(), this->BadPoints, this->UnscaledInputBounds));
    }
  }
}

//-----------------------------------------------------------------------------
void svtkPlotPoints::CalculateLogSeries()
{
  if (!this->XAxis || !this->YAxis)
  {
    return;
  }
  this->LogX = this->XAxis->GetLogScaleActive();
  this->LogY = this->YAxis->GetLogScaleActive();
  float* data = static_cast<float*>(this->Points->GetVoidPointer(0));
  svtkIdType n = this->Points->GetNumberOfPoints();
  if (this->LogX)
  {
    if (this->XAxis->GetUnscaledMinimum() < 0.)
    {
      for (svtkIdType i = 0; i < n; ++i)
      {
        data[2 * i] = log10(fabs(data[2 * i]));
      }
    }
    else
    {
      for (svtkIdType i = 0; i < n; ++i)
      {
        data[2 * i] = log10(data[2 * i]);
      }
    }
  }
  if (this->LogY)
  {
    if (this->YAxis->GetUnscaledMinimum() < 0.)
    {
      for (svtkIdType i = 0; i < n; ++i)
      {
        data[2 * i + 1] = log10(fabs(data[2 * i + 1]));
      }
    }
    else
    {
      for (svtkIdType i = 0; i < n; ++i)
      {
        data[2 * i + 1] = log10(data[2 * i + 1]);
      }
    }
  }
}

//-----------------------------------------------------------------------------
void svtkPlotPoints::FindBadPoints()
{
  // This should be run after CalculateLogSeries as a final step.
  svtkIdType n = this->Points->GetNumberOfPoints();

  // Scan through and find any bad points.
  svtkTable* table = this->Data->GetInput();
  svtkDataArray* array[2] = { nullptr, nullptr };
  if (!this->GetDataArrays(table, array))
  {
    return;
  }
  std::set<svtkIdType> bad;
  if (!this->UseIndexForXSeries)
  {
    switch (array[0]->GetDataType())
    {
      svtkTemplateMacro(SetBadPoints(static_cast<SVTK_TT*>(array[0]->GetVoidPointer(0)), n, bad));
    }
  }
  switch (array[1]->GetDataType())
  {
    svtkTemplateMacro(SetBadPoints(static_cast<SVTK_TT*>(array[1]->GetVoidPointer(0)), n, bad));
  }

  // add points from the ValidPointMask
  if (this->ValidPointMask)
  {
    for (svtkIdType i = 0; i < n; i++)
    {
      if (this->ValidPointMask->GetValue(i) == 0)
      {
        bad.insert(i);
      }
    }
  }

  // If there are bad points copy them, if not ensure the pointer is null.
  if (!bad.empty())
  {
    if (!this->BadPoints)
    {
      this->BadPoints = svtkIdTypeArray::New();
    }
    else
    {
      this->BadPoints->SetNumberOfTuples(0);
    }
    for (std::set<svtkIdType>::const_iterator it = bad.begin(); it != bad.end(); ++it)
    {
      this->BadPoints->InsertNextValue(*it);
    }
  }
  else if (this->BadPoints)
  {
    this->BadPoints->Delete();
    this->BadPoints = nullptr;
  }
}

//-----------------------------------------------------------------------------
void svtkPlotPoints::CalculateBounds(double bounds[4])
{
  // We can use the BadPoints array to skip the bad points
  if (!this->Points)
  {
    return;
  }
  this->CalculateUnscaledInputBounds();
  for (int i = 0; i < 4; ++i)
  {
    bounds[i] = this->UnscaledInputBounds[i];
  }
  if (this->LogX)
  {
    bounds[0] = log10(bounds[0]);
    bounds[1] = log10(bounds[1]);
  }
  if (this->LogY)
  {
    bounds[2] = log10(bounds[2]);
    bounds[3] = log10(bounds[3]);
  }
}

//-----------------------------------------------------------------------------
void svtkPlotPoints::SetLookupTable(svtkScalarsToColors* lut)
{
  if (this->LookupTable != lut)
  {
    if (this->LookupTable)
    {
      this->LookupTable->UnRegister(this);
    }
    this->LookupTable = lut;
    if (lut)
    {
      lut->Register(this);
    }
    this->Modified();
  }
}

//-----------------------------------------------------------------------------
svtkScalarsToColors* svtkPlotPoints::GetLookupTable()
{
  if (this->LookupTable == nullptr)
  {
    this->CreateDefaultLookupTable();
  }
  return this->LookupTable;
}

//-----------------------------------------------------------------------------
void svtkPlotPoints::CreateDefaultLookupTable()
{
  if (this->LookupTable)
  {
    this->LookupTable->UnRegister(this);
  }
  this->LookupTable = svtkLookupTable::New();
  // Consistent Register/UnRegisters.
  this->LookupTable->Register(this);
  this->LookupTable->Delete();
}

//-----------------------------------------------------------------------------
void svtkPlotPoints::SelectColorArray(const svtkStdString& arrayName)
{
  svtkTable* table = this->Data->GetInput();
  if (!table)
  {
    svtkDebugMacro(<< "SelectColorArray called with no input table set.");
    return;
  }
  if (this->ColorArrayName == arrayName)
  {
    return;
  }
  for (svtkIdType c = 0; c < table->GetNumberOfColumns(); ++c)
  {
    if (arrayName == table->GetColumnName(c))
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
void svtkPlotPoints::SelectColorArray(svtkIdType arrayNum)
{
  svtkTable* table = this->Data->GetInput();
  if (!table)
  {
    svtkDebugMacro(<< "SelectColorArray called with no input table set.");
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
svtkStdString svtkPlotPoints::GetColorArrayName()
{
  return this->ColorArrayName;
}

//-----------------------------------------------------------------------------
void svtkPlotPoints::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
