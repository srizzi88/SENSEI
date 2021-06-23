/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPlotPie.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkPlotPie.h"

#include "svtkBrush.h"
#include "svtkColorSeries.h"
#include "svtkContext2D.h"
#include "svtkContextMapper2D.h"
#include "svtkMath.h"
#include "svtkPen.h"
#include "svtkPoints2D.h"
#include "svtkRect.h"
#include "svtkTable.h"

#include "svtkObjectFactory.h"

#include <algorithm>

namespace
{

template <class A>
A SumData(A* a, int n)
{
  A sum = 0;
  for (int i = 0; i < n; ++i)
  {
    sum += a[i];
  }
  return sum;
}

template <class A>
void CopyToPoints(svtkPoints2D* points, A* a, int n)
{
  points->SetNumberOfPoints(n);

  A sum = SumData(a, n);
  float* data = static_cast<float*>(points->GetVoidPointer(0));
  float startAngle = 0.0;

  for (int i = 0; i < n; ++i)
  {
    data[2 * i] = startAngle;
    data[2 * i + 1] = startAngle + ((static_cast<float>(a[i]) / sum) * 360.0);
    startAngle = data[2 * i + 1];
  }
}
}

class svtkPlotPiePrivate
{
public:
  svtkPlotPiePrivate()
  {
    this->CenterX = 0;
    this->CenterY = 0;
    this->Radius = 0;
  }

  float CenterX;
  float CenterY;
  float Radius;
};

//-----------------------------------------------------------------------------
svtkStandardNewMacro(svtkPlotPie);

//-----------------------------------------------------------------------------
svtkPlotPie::svtkPlotPie()
{
  this->ColorSeries = svtkSmartPointer<svtkColorSeries>::New();
  this->Points = nullptr;
  this->Private = new svtkPlotPiePrivate();
  this->Dimensions[0] = this->Dimensions[1] = this->Dimensions[2] = this->Dimensions[3] = 0;
}

//-----------------------------------------------------------------------------
svtkPlotPie::~svtkPlotPie()
{
  delete this->Private;
  if (this->Points)
  {
    this->Points->Delete();
    this->Points = nullptr;
  }
  this->Private = nullptr;
}

//-----------------------------------------------------------------------------
bool svtkPlotPie::Paint(svtkContext2D* painter)
{
  if (!this->Visible)
  {
    return false;
  }

  // First check if we have an input
  svtkTable* table = this->Data->GetInput();
  if (!table)
  {
    svtkDebugMacro(<< "Paint event called with no input table set.");
    return false;
  }
  else if (this->Data->GetMTime() > this->BuildTime || table->GetMTime() > this->BuildTime ||
    this->MTime > this->BuildTime)
  {
    svtkDebugMacro(<< "Paint event called with outdated table cache. Updating.");
    this->UpdateTableCache(table);
  }

  float* data = static_cast<float*>(this->Points->GetVoidPointer(0));

  for (int i = 0; i < this->Points->GetNumberOfPoints(); ++i)
  {
    painter->GetBrush()->SetColor(this->ColorSeries->GetColorRepeating(i).GetData());

    painter->DrawEllipseWedge(this->Private->CenterX, this->Private->CenterY, this->Private->Radius,
      this->Private->Radius, 0.0, 0.0, data[2 * i], data[2 * i + 1]);
  }

  this->PaintChildren(painter);
  return true;
}

//-----------------------------------------------------------------------------

bool svtkPlotPie::PaintLegend(svtkContext2D* painter, const svtkRectf& rect, int legendIndex)
{
  if (this->ColorSeries)
    this->Brush->SetColor(this->ColorSeries->GetColorRepeating(legendIndex).GetData());

  painter->ApplyPen(this->Pen);
  painter->ApplyBrush(this->Brush);
  painter->DrawRect(rect[0], rect[1], rect[2], rect[3]);
  return true;
}

//-----------------------------------------------------------------------------

void svtkPlotPie::SetDimensions(int arg1, int arg2, int arg3, int arg4)
{
  if (arg1 != this->Dimensions[0] || arg2 != this->Dimensions[1] || arg3 != this->Dimensions[2] ||
    arg4 != this->Dimensions[3])
  {
    this->Dimensions[0] = arg1;
    this->Dimensions[1] = arg2;
    this->Dimensions[2] = arg3;
    this->Dimensions[3] = arg4;

    this->Private->CenterX = this->Dimensions[0] + 0.5 * this->Dimensions[2];
    this->Private->CenterY = this->Dimensions[1] + 0.5 * this->Dimensions[3];
    this->Private->Radius = this->Dimensions[2] < this->Dimensions[3] ? 0.5 * this->Dimensions[2]
                                                                      : 0.5 * this->Dimensions[3];
    this->Modified();
  }
}

void svtkPlotPie::SetDimensions(const int arg[4])
{
  this->SetDimensions(arg[0], arg[1], arg[2], arg[3]);
}

//-----------------------------------------------------------------------------
void svtkPlotPie::SetColorSeries(svtkColorSeries* colorSeries)
{
  if (this->ColorSeries == colorSeries)
  {
    return;
  }
  this->ColorSeries = colorSeries;
  this->Modified();
}

//-----------------------------------------------------------------------------
svtkColorSeries* svtkPlotPie::GetColorSeries()
{
  return this->ColorSeries;
}

//-----------------------------------------------------------------------------
svtkIdType svtkPlotPie::GetNearestPoint(const svtkVector2f& point,
#ifndef SVTK_LEGACY_REMOVE
  const svtkVector2f& tolerance,
#else
  const svtkVector2f& svtkNotUsed(tolerance),
#endif // SVTK_LEGACY_REMOVE
  svtkVector2f* value, svtkIdType* svtkNotUsed(segmentId))
{
#ifndef SVTK_LEGACY_REMOVE
  if (!this->LegacyRecursionFlag)
  {
    this->LegacyRecursionFlag = true;
    svtkIdType retLegacy = this->GetNearestPoint(point, tolerance, value);
    this->LegacyRecursionFlag = false;
    if (retLegacy != -1)
    {
      SVTK_LEGACY_REPLACED_BODY(svtkPlotPie::GetNearestPoint(const svtkVector2f& point,
                                 const svtkVector2f& tolerance, svtkVector2f* value),
        "SVTK 9.0",
        svtkPlotPie::GetNearestPoint(const svtkVector2f& point, const svtkVector2f& tolerance,
          svtkVector2f* value, svtkIdType* segmentId));
      return retLegacy;
    }
  }
#endif // SVTK_LEGACY_REMOVE

  float x = point.GetX() - this->Private->CenterX;
  float y = point.GetY() - this->Private->CenterY;

  if (sqrt((x * x) + (y * y)) <= this->Private->Radius)
  {
    float* angles = static_cast<float*>(this->Points->GetVoidPointer(0));
    float pointAngle = svtkMath::DegreesFromRadians(atan2(y, x));
    if (pointAngle < 0)
    {
      pointAngle = 180.0 + (180.0 + pointAngle);
    }
    float* lbound =
      std::lower_bound(angles, angles + (this->Points->GetNumberOfPoints() * 2), pointAngle);
    // Location in the array
    int ret = lbound - angles;
    // There are two of each angle in the array (start,end for each point)
    ret = ret / 2;

    svtkTable* table = this->Data->GetInput();
    svtkDataArray* data = this->Data->GetInputArrayToProcess(0, table);
    value->SetX(ret);
    value->SetY(data->GetTuple1(ret));
    return ret;
  }

  return -1;
}

//-----------------------------------------------------------------------------
void svtkPlotPie::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//-----------------------------------------------------------------------------
bool svtkPlotPie::UpdateTableCache(svtkTable* table)
{
  // Get the x and y arrays (index 0 and 1 respectively)
  svtkDataArray* data = this->Data->GetInputArrayToProcess(0, table);

  if (!data)
  {
    svtkErrorMacro(<< "No data set (index 0).");
    return false;
  }

  if (!this->Points)
  {
    this->Points = svtkPoints2D::New();
  }

  switch (data->GetDataType())
  {
    svtkTemplateMacro(CopyToPoints(
      this->Points, static_cast<SVTK_TT*>(data->GetVoidPointer(0)), data->GetNumberOfTuples()));
  }

  this->BuildTime.Modified();
  return true;
}
