/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPlotArea.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkPlotArea.h"

#include "svtkArrayDispatch.h"
#include "svtkAxis.h"
#include "svtkBoundingBox.h"
#include "svtkBrush.h"
#include "svtkCharArray.h"
#include "svtkContext2D.h"
#include "svtkContextMapper2D.h"
#include "svtkFloatArray.h"
#include "svtkMath.h"
#include "svtkNew.h"
#include "svtkObjectFactory.h"
#include "svtkPen.h"
#include "svtkPoints2D.h"
#include "svtkTable.h"
#include "svtkVectorOperators.h"
#include "svtkWeakPointer.h"

#include <algorithm>
#include <cmath>
#include <set>
#include <vector>

namespace
{
inline bool svtkIsBadPoint(const svtkVector2f& vec)
{
  return (svtkMath::IsNan(vec.GetX()) || svtkMath::IsInf(vec.GetX()) || svtkMath::IsNan(vec.GetY()) ||
    svtkMath::IsInf(vec.GetY()));
}
} // end of namespace

// Keeps all data-dependent meta-data that's updated in
// svtkPlotArea::Update.
class svtkPlotArea::svtkTableCache
{
  // PIMPL for STL vector...
  struct svtkIndexedVector2f
  {
    size_t index;
    svtkVector2f pos;
    static bool compVector3fX(const svtkIndexedVector2f& v1, const svtkIndexedVector2f& v2)
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
    static bool inRange(
      const svtkVector2f& point, const svtkVector2f& tol, const svtkVector2f& current)
    {
      if (current.GetX() > point.GetX() - tol.GetX() &&
        current.GetX() < point.GetX() + tol.GetX() && current.GetY() > point.GetY() - tol.GetY() &&
        current.GetY() < point.GetY() + tol.GetY())
      {
        return true;
      }
      else
      {
        return false;
      }
    }
  };

  // DataStructure used to store sorted points.
  class VectorPIMPL : public std::vector<svtkIndexedVector2f>
  {
  public:
    void Initialize(svtkVector2f* array, size_t n)
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

    //-----------------------------------------------------------------------------
    svtkIdType GetNearestPoint(
      const svtkVector2f& point, const svtkVector2f& tol, const svtkRectd& ss, svtkVector2f* location)
    {
      // Set up our search array, use the STL lower_bound algorithm
      VectorPIMPL::iterator low;
      VectorPIMPL& v = *this;

      // Get the lowest point we might hit within the supplied tolerance
      svtkIndexedVector2f lowPoint;
      lowPoint.index = 0;
      lowPoint.pos = svtkVector2f(point.GetX() - tol.GetX(), 0.0f);
      low = std::lower_bound(v.begin(), v.end(), lowPoint, svtkIndexedVector2f::compVector3fX);

      // Now consider the y axis
      float highX = point.GetX() + tol.GetX();
      while (low != v.end())
      {
        if (svtkIndexedVector2f::inRange(point, tol, (*low).pos))
        {
          *location = (*low).pos;
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
  };

private:
  svtkTimeStamp DataMTime;
  svtkTimeStamp BoundsMTime;

  // Unscaled data bounds.
  svtkBoundingBox DataBounds;

  svtkRectd ShiftScale;

  svtkTuple<double, 2> GetDataRange(svtkDataArray* array)
  {
    assert(array);

    if (this->ValidPointMask)
    {
      assert(array->GetNumberOfTuples() == this->ValidPointMask->GetNumberOfTuples());
      assert(array->GetNumberOfComponents() == this->ValidPointMask->GetNumberOfComponents());

      using Dispatcher =
        svtkArrayDispatch::Dispatch2ByArray<svtkArrayDispatch::Arrays, // First array is input, can be
                                                                     // anything.
          svtkTypeList::Create<svtkCharArray> // Second is always svtkCharArray.
          >;
      ComputeArrayRange worker;

      if (!Dispatcher::Execute(array, this->ValidPointMask, worker))
      {
        svtkGenericWarningMacro(<< "Error computing range. Unsupported array type: "
                               << array->GetClassName() << " (" << array->GetDataTypeAsString()
                               << ").");
      }

      return worker.Result;
    }
    else
    {
      using Dispatcher = svtkArrayDispatch::Dispatch;
      ComputeArrayRange worker;

      if (!Dispatcher::Execute(array, worker))
      {
        svtkGenericWarningMacro(<< "Error computing range. Unsupported array type: "
                               << array->GetClassName() << " (" << array->GetDataTypeAsString()
                               << ").");
      }

      return worker.Result;
    }
  }

  struct ComputeArrayRange
  {
    svtkTuple<double, 2> Result;

    ComputeArrayRange()
    {
      Result[0] = SVTK_DOUBLE_MAX;
      Result[1] = SVTK_DOUBLE_MIN;
    }

    // Use mask:
    template <class ArrayT>
    void operator()(ArrayT* array, svtkCharArray* mask)
    {
      svtkIdType numTuples = array->GetNumberOfTuples();
      int numComps = array->GetNumberOfComponents();

      for (svtkIdType tupleIdx = 0; tupleIdx < numTuples; ++tupleIdx)
      {
        for (int compIdx = 0; compIdx < numComps; ++compIdx)
        {
          if (mask->GetTypedComponent(tupleIdx, compIdx) != 0)
          {
            typename ArrayT::ValueType val = array->GetTypedComponent(tupleIdx, compIdx);
            Result[0] = std::min(Result[0], static_cast<double>(val));
            Result[1] = std::max(Result[1], static_cast<double>(val));
          }
        }
      }
    }

    // No mask:
    template <class ArrayT>
    void operator()(ArrayT* array)
    {
      svtkIdType numTuples = array->GetNumberOfTuples();
      int numComps = array->GetNumberOfComponents();

      for (svtkIdType tupleIdx = 0; tupleIdx < numTuples; ++tupleIdx)
      {
        for (int compIdx = 0; compIdx < numComps; ++compIdx)
        {
          typename ArrayT::ValueType val = array->GetTypedComponent(tupleIdx, compIdx);
          Result[0] = std::min(Result[0], static_cast<double>(val));
          Result[1] = std::max(Result[1], static_cast<double>(val));
        }
      }
    }
  };

  struct CopyToPoints
  {
    float* Data;
    int DataIncrement;
    svtkIdType NumValues;
    svtkVector2d Transform;
    bool UseLog;

    CopyToPoints(
      float* data, int data_increment, svtkIdType numValues, const svtkVector2d& ss, bool useLog)
      : Data(data)
      , DataIncrement(data_increment)
      , NumValues(numValues)
      , Transform(ss)
      , UseLog(useLog)
    {
    }

    CopyToPoints& operator=(const CopyToPoints&) = delete;

    // Use input array:
    template <class ArrayT>
    void operator()(ArrayT* array)
    {
      if (this->UseLog)
      {
        float* data = this->Data;
        for (svtkIdType valIdx = 0; valIdx < this->NumValues; ++valIdx, data += this->DataIncrement)
        {
          *data = log10(static_cast<float>(
            (array->GetValue(valIdx) + this->Transform[0]) * this->Transform[1]));
        }
      }
      else
      {
        float* data = this->Data;
        for (svtkIdType valIdx = 0; valIdx < this->NumValues; ++valIdx, data += this->DataIncrement)
        {
          *data =
            static_cast<float>((array->GetValue(valIdx) + this->Transform[0]) * this->Transform[1]);
        }
      }
    }

    // No array, just iterate to number of values
    void operator()()
    {
      if (this->UseLog)
      {
        float* data = this->Data;
        for (svtkIdType valIdx = 0; valIdx < this->NumValues; ++valIdx, data += this->DataIncrement)
        {
          *data = log10(static_cast<float>((valIdx + this->Transform[0]) * this->Transform[1]));
        }
      }
      else
      {
        float* data = this->Data;
        for (svtkIdType valIdx = 0; valIdx < this->NumValues; ++valIdx, data += this->DataIncrement)
        {
          *data = static_cast<float>((valIdx + this->Transform[0]) * this->Transform[1]);
        }
      }
    }
  };

  VectorPIMPL SortedPoints;

public:
  // Array which marks valid points in the array. If nullptr (the default), all
  // points in the input array are considered valid.
  svtkWeakPointer<svtkCharArray> ValidPointMask;

  // References to input arrays.
  svtkTuple<svtkWeakPointer<svtkDataArray>, 3> InputArrays;

  // Array for the points. These maintain the points that form the QuadStrip for
  // the area plot.
  svtkNew<svtkPoints2D> Points;

  // Set of point ids that are invalid or masked out.
  std::vector<svtkIdType> BadPoints;

  svtkTableCache() { this->Reset(); }

  void Reset()
  {
    this->ValidPointMask = nullptr;
    this->Points->Initialize();
    this->Points->SetDataTypeToFloat();
    this->BadPoints.clear();
  }

  bool IsInputDataValid() const
  {
    return this->InputArrays[1] != nullptr && this->InputArrays[2] != nullptr;
  }

  bool SetPoints(svtkDataArray* x, svtkDataArray* y1, svtkDataArray* y2)
  {
    if (y1 == nullptr || y2 == nullptr)
    {
      return false;
    }

    svtkIdType numTuples = y1->GetNumberOfTuples();

    // sanity check.
    assert((x == nullptr || x->GetNumberOfTuples() == numTuples) &&
      y2->GetNumberOfTuples() == numTuples);

    this->InputArrays[0] = x;
    this->InputArrays[1] = y1;
    this->InputArrays[2] = y2;
    this->Points->SetNumberOfPoints(numTuples * 2);
    this->SortedPoints.clear();
    this->DataMTime.Modified();
    return true;
  }

  void GetDataBounds(double bounds[4])
  {
    if (this->DataMTime > this->BoundsMTime)
    {
      svtkTuple<double, 2> rangeX, rangeY1, rangeY2;
      if (this->InputArrays[0])
      {
        rangeX = this->GetDataRange(this->InputArrays[0]);
      }
      else
      {
        rangeX[0] = 0;
        rangeX[1] = (this->Points->GetNumberOfPoints() / 2 - 1);
      }
      rangeY1 = this->GetDataRange(this->InputArrays[1]);
      rangeY2 = this->GetDataRange(this->InputArrays[2]);

      this->DataBounds.Reset();
      this->DataBounds.SetMinPoint(rangeX[0], std::min(rangeY1[0], rangeY2[0]), 0);
      this->DataBounds.SetMaxPoint(rangeX[1], std::max(rangeY1[1], rangeY2[1]), 0);
      this->BoundsMTime.Modified();
    }
    double bds[6];
    this->DataBounds.GetBounds(bds);
    std::copy(bds, bds + 4, bounds);
  }

  void UpdateCache(svtkPlotArea* self)
  {
    const svtkRectd& ss = self->GetShiftScale();
    svtkAxis* xaxis = self->GetXAxis();
    svtkAxis* yaxis = self->GetYAxis();

    if (this->Points->GetMTime() > this->DataMTime &&
      this->Points->GetMTime() > xaxis->GetMTime() &&
      this->Points->GetMTime() > yaxis->GetMTime() && ss == this->ShiftScale)
    {
      // nothing to do.
      return;
    }

    svtkTuple<bool, 2> useLog;
    useLog[0] = xaxis->GetLogScaleActive();
    useLog[1] = yaxis->GetLogScaleActive();

    svtkIdType numTuples = this->InputArrays[1]->GetNumberOfTuples();
    assert(this->Points->GetNumberOfPoints() == 2 * numTuples);

    float* data = reinterpret_cast<float*>(this->Points->GetVoidPointer(0));
    if (this->InputArrays[0])
    {
      svtkDataArray* array = this->InputArrays[0];
      svtkIdType numValues = array->GetNumberOfTuples() * array->GetNumberOfComponents();

      CopyToPoints worker1(data, 4, numValues, svtkVector2d(ss[0], ss[2]), useLog[0]);
      CopyToPoints worker2(&data[2], 4, numValues, svtkVector2d(ss[0], ss[2]), useLog[0]);

      using Dispatcher = svtkArrayDispatch::Dispatch;

      if (!Dispatcher::Execute(array, worker1) || !Dispatcher::Execute(array, worker2))
      {
        svtkGenericWarningMacro("Error creating points, unsupported array type: "
          << array->GetClassName() << " (" << array->GetDataTypeAsString() << ").");
      }
    }
    else
    {
      CopyToPoints worker1(data, 4, numTuples, svtkVector2d(ss[0], ss[2]), useLog[0]);
      CopyToPoints worker2(&data[2], 4, numTuples, svtkVector2d(ss[0], ss[2]), useLog[0]);
      // No array, no need to dispatch:
      worker1();
      worker2();
    }

    svtkDataArray* array1 = this->InputArrays[1];
    svtkDataArray* array2 = this->InputArrays[2];
    svtkIdType numValues1 = array1->GetNumberOfTuples() * array1->GetNumberOfComponents();
    svtkIdType numValues2 = array2->GetNumberOfTuples() * array2->GetNumberOfComponents();

    CopyToPoints worker1(&data[1], 4, numValues1, svtkVector2d(ss[1], ss[3]), useLog[1]);
    CopyToPoints worker2(&data[3], 4, numValues2, svtkVector2d(ss[1], ss[3]), useLog[1]);

    using Dispatcher = svtkArrayDispatch::Dispatch;

    if (!Dispatcher::Execute(array1, worker1) || !Dispatcher::Execute(array2, worker2))
    {
      svtkGenericWarningMacro("Error creating points: Array dispatch failed.");
    }

    // Set the bad-points mask.
    svtkVector2f* vec2f = reinterpret_cast<svtkVector2f*>(this->Points->GetVoidPointer(0));
    for (svtkIdType cc = 0; cc < numTuples; cc++)
    {
      bool is_bad = (this->ValidPointMask && this->ValidPointMask->GetValue(cc) == 0);
      is_bad = is_bad || svtkIsBadPoint(vec2f[2 * cc]);
      is_bad = is_bad || svtkIsBadPoint(vec2f[2 * cc + 1]);
      if (is_bad)
      {
        // this ensures that the GetNearestPoint() fails for masked out points.
        vec2f[2 * cc] = svtkVector2f(svtkMath::Nan(), svtkMath::Nan());
        vec2f[2 * cc + 1] = svtkVector2f(svtkMath::Nan(), svtkMath::Nan());
        this->BadPoints.push_back(cc);
      }
    }

    this->ShiftScale = ss;
    this->Points->Modified();
    this->SortedPoints.clear();
  }

  svtkIdType GetNearestPoint(const svtkVector2f& point, const svtkVector2f& tol, svtkVector2f* location)
  {
    if (this->Points->GetNumberOfPoints() == 0)
    {
      return -1;
    }

    if (this->SortedPoints.empty())
    {
      float* data = reinterpret_cast<float*>(this->Points->GetVoidPointer(0));
      this->SortedPoints.Initialize(
        reinterpret_cast<svtkVector2f*>(data), this->Points->GetNumberOfPoints());
      std::sort(
        this->SortedPoints.begin(), this->SortedPoints.end(), svtkIndexedVector2f::compVector3fX);
    }
    return this->SortedPoints.GetNearestPoint(point, tol, this->ShiftScale, location);
  }
};

svtkStandardNewMacro(svtkPlotArea);
//----------------------------------------------------------------------------
svtkPlotArea::svtkPlotArea()
  : TableCache(new svtkPlotArea::svtkTableCache())
{
  this->TooltipDefaultLabelFormat = "%l: %x:(%a, %b)";
}

//----------------------------------------------------------------------------
svtkPlotArea::~svtkPlotArea()
{
  delete this->TableCache;
  this->TableCache = nullptr;
}

//----------------------------------------------------------------------------
void svtkPlotArea::Update()
{
  if (!this->Visible)
  {
    return;
  }

  svtkTable* table = this->GetInput();
  if (!table)
  {
    svtkDebugMacro("Update event called with no input table set.");
    this->TableCache->Reset();
    return;
  }

  if (this->Data->GetMTime() > this->UpdateTime || table->GetMTime() > this->UpdateTime ||
    this->GetMTime() > this->UpdateTime)
  {
    svtkTableCache& cache = (*this->TableCache);

    cache.Reset();
    cache.ValidPointMask = (this->ValidPointMaskName.empty() == false)
      ? svtkArrayDownCast<svtkCharArray>(table->GetColumnByName(this->ValidPointMaskName))
      : nullptr;
    cache.SetPoints(
      this->UseIndexForXSeries ? nullptr : this->Data->GetInputArrayToProcess(0, table),
      this->Data->GetInputArrayToProcess(1, table), this->Data->GetInputArrayToProcess(2, table));
    this->UpdateTime.Modified();
  }
}

//----------------------------------------------------------------------------
void svtkPlotArea::UpdateCache()
{
  svtkTableCache& cache = (*this->TableCache);
  if (!this->Visible || !cache.IsInputDataValid())
  {
    return;
  }
  cache.UpdateCache(this);
}

//----------------------------------------------------------------------------
void svtkPlotArea::GetBounds(double bounds[4])
{
  svtkTableCache& cache = (*this->TableCache);
  if (!this->Visible || !cache.IsInputDataValid())
  {
    return;
  }
  cache.GetDataBounds(bounds);
}

//----------------------------------------------------------------------------
bool svtkPlotArea::Paint(svtkContext2D* painter)
{
  svtkTableCache& cache = (*this->TableCache);
  if (!this->Visible || !cache.IsInputDataValid() || cache.Points->GetNumberOfPoints() == 0)
  {
    return false;
  }
  painter->ApplyPen(this->Pen);
  painter->ApplyBrush(this->Brush);

  svtkIdType start = 0;
  for (std::vector<svtkIdType>::iterator iter = cache.BadPoints.begin();
       iter != cache.BadPoints.end(); ++iter)
  {
    svtkIdType end = *iter;
    if ((end - start) >= 2)
    {
      painter->DrawQuadStrip(
        reinterpret_cast<float*>(cache.Points->GetVoidPointer(2 * 2 * start)), (end - start) * 2);
    }
    start = end;
  }
  if (cache.Points->GetNumberOfPoints() - (2 * start) > 4)
  {
    painter->DrawQuadStrip(reinterpret_cast<float*>(cache.Points->GetVoidPointer(2 * 2 * start)),
      cache.Points->GetNumberOfPoints() - (2 * start));
  }
  return true;
}

//-----------------------------------------------------------------------------
bool svtkPlotArea::PaintLegend(
  svtkContext2D* painter, const svtkRectf& rect, int svtkNotUsed(legendIndex))
{
  painter->ApplyPen(this->Pen);
  painter->ApplyBrush(this->Brush);
  painter->DrawRect(rect[0], rect[1], rect[2], rect[3]);
  return true;
}

//----------------------------------------------------------------------------
svtkIdType svtkPlotArea::GetNearestPoint(const svtkVector2f& point, const svtkVector2f& tolerance,
  svtkVector2f* location, svtkIdType* svtkNotUsed(segmentId))
{

#ifndef SVTK_LEGACY_REMOVE
  if (!this->LegacyRecursionFlag)
  {
    this->LegacyRecursionFlag = true;
    svtkIdType ret = this->GetNearestPoint(point, tolerance, location);
    this->LegacyRecursionFlag = false;
    if (ret != -1)
    {
      SVTK_LEGACY_REPLACED_BODY(svtkPlotArea::GetNearestPoint(const svtkVector2f& point,
                                 const svtkVector2f& tolerance, svtkVector2f* location),
        "SVTK 9.0",
        svtkPlotArea::GetNearestPoint(const svtkVector2f& point, const svtkVector2f& tolerance,
          svtkVector2f* location, svtkIdType* segmentId));
      return ret;
    }
  }
#endif // SVTK_LEGACY_REMOVE

  svtkTableCache& cache = (*this->TableCache);
  if (!this->Visible || !cache.IsInputDataValid() || cache.Points->GetNumberOfPoints() == 0)
  {
    return -1;
  }
  return cache.GetNearestPoint(point, tolerance, location);
}

//-----------------------------------------------------------------------------
svtkStdString svtkPlotArea::GetTooltipLabel(
  const svtkVector2d& plotPos, svtkIdType seriesIndex, svtkIdType segmentIndex)
{
  svtkStdString tooltipLabel;
  svtkStdString format = this->Superclass::GetTooltipLabel(plotPos, seriesIndex, segmentIndex);

  svtkIdType idx = (seriesIndex / 2) * 2;

  svtkTableCache& cache = (*this->TableCache);
  const svtkVector2f* data = reinterpret_cast<svtkVector2f*>(cache.Points->GetVoidPointer(0));

  // Parse TooltipLabelFormat and build tooltipLabel
  bool escapeNext = false;
  for (size_t i = 0; i < format.length(); ++i)
  {
    if (escapeNext)
    {
      switch (format[i])
      {
        case 'a':
          tooltipLabel += this->GetNumber(data[idx].GetY(), this->YAxis);
          break;
        case 'b':
          tooltipLabel += this->GetNumber(data[idx + 1].GetY(), this->YAxis);
          break;
        default: // If no match, insert the entire format tag
          tooltipLabel += "%";
          tooltipLabel += format[i];
          break;
      }
      escapeNext = false;
    }
    else
    {
      if (format[i] == '%')
      {
        escapeNext = true;
      }
      else
      {
        tooltipLabel += format[i];
      }
    }
  }
  return tooltipLabel;
}

//----------------------------------------------------------------------------
void svtkPlotArea::SetColor(unsigned char r, unsigned char g, unsigned char b, unsigned char a)
{
  this->Brush->SetColor(r, g, b, a);
  this->Superclass::SetColor(r, g, b, a);
}

//----------------------------------------------------------------------------
void svtkPlotArea::SetColor(double r, double g, double b)
{
  this->Brush->SetColorF(r, g, b);
  this->Superclass::SetColor(r, g, b);
}

//----------------------------------------------------------------------------
void svtkPlotArea::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
