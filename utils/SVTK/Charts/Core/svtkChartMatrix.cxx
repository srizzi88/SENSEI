/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkChartMatrix.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkChartMatrix.h"

#include "svtkAxis.h"
#include "svtkChartXY.h"
#include "svtkContext2D.h"
#include "svtkContextScene.h"
#include "svtkNew.h"
#include "svtkObjectFactory.h"
#include "svtkSmartPointer.h"

#include <vector>

class svtkChartMatrix::PIMPL
{
public:
  PIMPL()
    : Geometry(0, 0)
  {
  }
  ~PIMPL() = default;

  // Container for the svtkChart objects that make up the matrix.
  std::vector<svtkSmartPointer<svtkChart> > Charts;
  // Spans of the charts in the matrix, default is 1x1.
  std::vector<svtkVector2i> Spans;
  svtkVector2i Geometry;
};

svtkStandardNewMacro(svtkChartMatrix);

svtkChartMatrix::svtkChartMatrix()
  : Size(0, 0)
  , Gutter(15.0, 15.0)
{
  this->Private = new PIMPL;
  this->Borders[svtkAxis::LEFT] = 50;
  this->Borders[svtkAxis::BOTTOM] = 40;
  this->Borders[svtkAxis::RIGHT] = 50;
  this->Borders[svtkAxis::TOP] = 40;
  this->LayoutIsDirty = true;
}

svtkChartMatrix::~svtkChartMatrix()
{
  delete this->Private;
}

void svtkChartMatrix::Update() {}

bool svtkChartMatrix::Paint(svtkContext2D* painter)
{
  if (this->LayoutIsDirty || this->GetScene()->GetSceneWidth() != this->Private->Geometry.GetX() ||
    this->GetScene()->GetSceneHeight() != this->Private->Geometry.GetY())
  {
    // Update the chart element positions
    this->Private->Geometry.Set(
      this->GetScene()->GetSceneWidth(), this->GetScene()->GetSceneHeight());
    if (this->Size.GetX() > 0 && this->Size.GetY() > 0)
    {
      // Calculate the increments without the gutters/borders that must be left
      svtkVector2f increments;
      increments.SetX(
        (this->Private->Geometry.GetX() - (this->Size.GetX() - 1) * this->Gutter.GetX() -
          this->Borders[svtkAxis::LEFT] - this->Borders[svtkAxis::RIGHT]) /
        this->Size.GetX());
      increments.SetY(
        (this->Private->Geometry.GetY() - (this->Size.GetY() - 1) * this->Gutter.GetY() -
          this->Borders[svtkAxis::TOP] - this->Borders[svtkAxis::BOTTOM]) /
        this->Size.GetY());

      float x = this->Borders[svtkAxis::LEFT];
      float y = this->Borders[svtkAxis::BOTTOM];
      for (int i = 0; i < this->Size.GetX(); ++i)
      {
        if (i > 0)
        {
          x += increments.GetX() + this->Gutter.GetX();
        }
        for (int j = 0; j < this->Size.GetY(); ++j)
        {
          if (j > 0)
          {
            y += increments.GetY() + this->Gutter.GetY();
          }
          else
          {
            y = this->Borders[svtkAxis::BOTTOM];
          }
          svtkVector2f resize(0., 0.);
          svtkVector2i key(i, j);
          if (this->SpecificResize.find(key) != this->SpecificResize.end())
          {
            resize = this->SpecificResize[key];
          }
          size_t index = j * this->Size.GetX() + i;
          if (this->Private->Charts[index])
          {
            svtkChart* chart = this->Private->Charts[index];
            svtkVector2i& span = this->Private->Spans[index];
            svtkRectf chartRect(x + resize.GetX(), y + resize.GetY(),
              increments.GetX() * span.GetX() - resize.GetX() +
                (span.GetX() - 1) * this->Gutter.GetX(),
              increments.GetY() * span.GetY() - resize.GetY() +
                (span.GetY() - 1) * this->Gutter.GetY());
            // ensure that the size is valid. If not, make the rect and empty rect.
            if (chartRect.GetWidth() < 0)
            {
              chartRect.SetWidth(0);
            }
            if (chartRect.GetHeight() < 0)
            {
              chartRect.SetHeight(0);
            }
            chart->SetSize(chartRect);
          }
        }
      }
    }
    this->LayoutIsDirty = false;
  }
  return Superclass::Paint(painter);
}

void svtkChartMatrix::SetSize(const svtkVector2i& size)
{
  if (this->Size.GetX() != size.GetX() || this->Size.GetY() != size.GetY())
  {
    this->Size = size;
    if (size.GetX() * size.GetY() < static_cast<int>(this->Private->Charts.size()))
    {
      for (int i = static_cast<int>(this->Private->Charts.size() - 1);
           i >= size.GetX() * size.GetY(); --i)
      {
        this->RemoveItem(this->Private->Charts[i]);
      }
    }
    this->Private->Charts.resize(size.GetX() * size.GetY());
    this->Private->Spans.resize(size.GetX() * size.GetY(), svtkVector2i(1, 1));
    this->LayoutIsDirty = true;
  }
}

void svtkChartMatrix::SetBorders(int left, int bottom, int right, int top)
{
  this->Borders[svtkAxis::LEFT] = left;
  this->Borders[svtkAxis::BOTTOM] = bottom;
  this->Borders[svtkAxis::RIGHT] = right;
  this->Borders[svtkAxis::TOP] = top;
  this->LayoutIsDirty = true;
}

void svtkChartMatrix::SetBorderLeft(int value)
{
  this->Borders[svtkAxis::LEFT] = value;
  this->LayoutIsDirty = true;
}

void svtkChartMatrix::SetBorderBottom(int value)
{
  this->Borders[svtkAxis::BOTTOM] = value;
  this->LayoutIsDirty = true;
}

void svtkChartMatrix::SetBorderRight(int value)
{
  this->Borders[svtkAxis::RIGHT] = value;
  this->LayoutIsDirty = true;
}

void svtkChartMatrix::SetBorderTop(int value)
{
  this->Borders[svtkAxis::TOP] = value;
  this->LayoutIsDirty = true;
}

void svtkChartMatrix::SetGutter(const svtkVector2f& gutter)
{
  this->Gutter = gutter;
  this->LayoutIsDirty = true;
}

void svtkChartMatrix::SetGutterX(float value)
{
  this->Gutter.SetX(value);
  this->LayoutIsDirty = true;
}

void svtkChartMatrix::SetGutterY(float value)
{
  this->Gutter.SetY(value);
  this->LayoutIsDirty = true;
}

void svtkChartMatrix::SetSpecificResize(const svtkVector2i& index, const svtkVector2f& resize)
{
  if (this->SpecificResize.find(index) == this->SpecificResize.end() ||
    this->SpecificResize[index] != resize)
  {
    this->SpecificResize[index] = resize;
    this->LayoutIsDirty = true;
  }
}

void svtkChartMatrix::ClearSpecificResizes()
{
  if (!this->SpecificResize.empty())
  {
    this->SpecificResize.clear();
    this->LayoutIsDirty = true;
  }
}

void svtkChartMatrix::Allocate()
{
  // Force allocation of all objects as svtkChartXY.
}

bool svtkChartMatrix::SetChart(const svtkVector2i& position, svtkChart* chart)
{
  if (position.GetX() < this->Size.GetX() && position.GetY() < this->Size.GetY())
  {
    size_t index = position.GetY() * this->Size.GetX() + position.GetX();
    if (this->Private->Charts[index])
    {
      this->RemoveItem(this->Private->Charts[index]);
    }
    this->Private->Charts[index] = chart;
    this->AddItem(chart);
    chart->SetLayoutStrategy(svtkChart::AXES_TO_RECT);
    return true;
  }
  else
  {
    return false;
  }
}

svtkChart* svtkChartMatrix::GetChart(const svtkVector2i& position)
{
  if (position.GetX() < this->Size.GetX() && position.GetY() < this->Size.GetY())
  {
    size_t index = position.GetY() * this->Size.GetX() + position.GetX();
    if (this->Private->Charts[index] == nullptr)
    {
      svtkNew<svtkChartXY> chart;
      this->Private->Charts[index] = chart;
      this->AddItem(chart);
      chart->SetLayoutStrategy(svtkChart::AXES_TO_RECT);
    }
    return this->Private->Charts[index];
  }
  else
  {
    return nullptr;
  }
}

bool svtkChartMatrix::SetChartSpan(const svtkVector2i& position, const svtkVector2i& span)
{
  if (this->Size.GetX() - position.GetX() - span.GetX() < 0 ||
    this->Size.GetY() - position.GetY() - span.GetY() < 0)
  {
    return false;
  }
  else
  {
    this->Private->Spans[position.GetY() * this->Size.GetX() + position.GetX()] = span;
    this->LayoutIsDirty = true;
    return true;
  }
}

svtkVector2i svtkChartMatrix::GetChartSpan(const svtkVector2i& position)
{
  size_t index = position.GetY() * this->Size.GetX() + position.GetX();
  if (position.GetX() < this->Size.GetX() && position.GetY() < this->Size.GetY())
  {
    return this->Private->Spans[index];
  }
  else
  {
    return svtkVector2i(0, 0);
  }
}

svtkVector2i svtkChartMatrix::GetChartIndex(const svtkVector2f& position)
{
  if (this->Size.GetX() > 0 && this->Size.GetY() > 0)
  {
    // Calculate the increments without the gutters/borders that must be left.
    svtkVector2f increments;
    increments.SetX(
      (this->Private->Geometry.GetX() - (this->Size.GetX() - 1) * this->Gutter.GetX() -
        this->Borders[svtkAxis::LEFT] - this->Borders[svtkAxis::RIGHT]) /
      this->Size.GetX());
    increments.SetY(
      (this->Private->Geometry.GetY() - (this->Size.GetY() - 1) * this->Gutter.GetY() -
        this->Borders[svtkAxis::TOP] - this->Borders[svtkAxis::BOTTOM]) /
      this->Size.GetY());

    float x = this->Borders[svtkAxis::LEFT];
    float y = this->Borders[svtkAxis::BOTTOM];
    for (int i = 0; i < this->Size.GetX(); ++i)
    {
      if (i > 0)
      {
        x += increments.GetX() + this->Gutter.GetX();
      }
      for (int j = 0; j < this->Size.GetY(); ++j)
      {
        if (j > 0)
        {
          y += increments.GetY() + this->Gutter.GetY();
        }
        else
        {
          y = this->Borders[svtkAxis::BOTTOM];
        }
        svtkVector2f resize(0., 0.);
        svtkVector2i key(i, j);
        if (this->SpecificResize.find(key) != this->SpecificResize.end())
        {
          resize = this->SpecificResize[key];
        }
        size_t index = j * this->Size.GetX() + i;
        if (this->Private->Charts[index])
        {
          svtkVector2i& span = this->Private->Spans[index];
          // Check if the supplied location is within this charts area.
          float x2 = x + resize.GetX();
          float y2 = y + resize.GetY();
          if (position.GetX() > x2 &&
            position.GetX() < (x2 + increments.GetX() * span.GetX() - resize.GetY() +
                                (span.GetX() - 1) * this->Gutter.GetX()) &&
            position.GetY() > y2 &&
            position.GetY() < (y2 + increments.GetY() * span.GetY() - resize.GetY() +
                                (span.GetY() - 1) * this->Gutter.GetY()))
            return svtkVector2i(i, j);
        }
      }
    }
  }
  return svtkVector2i(-1, -1);
}

void svtkChartMatrix::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
