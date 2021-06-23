/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkChartLegend.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkChartLegend.h"

#include "svtkBrush.h"
#include "svtkChart.h"
#include "svtkContext2D.h"
#include "svtkContextMouseEvent.h"
#include "svtkContextScene.h"
#include "svtkPen.h"
#include "svtkPlot.h"
#include "svtkSmartPointer.h"
#include "svtkStdString.h"
#include "svtkStringArray.h"
#include "svtkTextProperty.h"
#include "svtkVector.h"
#include "svtkVectorOperators.h"
#include "svtkWeakPointer.h"

#include "svtkObjectFactory.h"

#include <vector>

//-----------------------------------------------------------------------------
class svtkChartLegend::Private
{
public:
  Private()
    : Point(0, 0)
  {
  }
  ~Private() = default;

  svtkVector2f Point;
  svtkWeakPointer<svtkChart> Chart;
  std::vector<svtkPlot*> ActivePlots;
};

//-----------------------------------------------------------------------------
svtkStandardNewMacro(svtkChartLegend);

//-----------------------------------------------------------------------------
svtkChartLegend::svtkChartLegend()
{
  this->Storage = new svtkChartLegend::Private;
  this->Point = this->Storage->Point.GetData();
  this->Rect.Set(0, 0, 0, 0);
  // Defaults to 12pt text, with top, right alignment to the specified point.
  this->LabelProperties->SetFontSize(12);
  this->LabelProperties->SetColor(0.0, 0.0, 0.0);
  this->LabelProperties->SetJustificationToLeft();
  this->LabelProperties->SetVerticalJustificationToBottom();

  this->Pen->SetColor(0, 0, 0);
  this->Brush->SetColor(255, 255, 255, 255);
  this->HorizontalAlignment = svtkChartLegend::RIGHT;
  this->VerticalAlignment = svtkChartLegend::TOP;

  this->Padding = 5;
  this->SymbolWidth = 25;
  this->Inline = true;
  this->Button = -1;
  this->DragEnabled = true;
  this->CacheBounds = true;
}

//-----------------------------------------------------------------------------
svtkChartLegend::~svtkChartLegend()
{
  delete this->Storage;
  this->Storage = nullptr;
  this->Point = nullptr;
}

//-----------------------------------------------------------------------------
void svtkChartLegend::Update()
{
  this->Storage->ActivePlots.clear();
  for (int i = 0; i < this->Storage->Chart->GetNumberOfPlots(); ++i)
  {
    if (this->Storage->Chart->GetPlot(i)->GetVisible() &&
      this->Storage->Chart->GetPlot(i)->GetLabel().length() > 0)
    {
      this->Storage->ActivePlots.push_back(this->Storage->Chart->GetPlot(i));
    }
    // If we have a plot with multiple labels, we generally only want to show
    // the labels/legend symbols for the first one. So truncate at the first
    // one we encounter.
    if (this->Storage->Chart->GetPlot(i)->GetLabels() &&
      this->Storage->Chart->GetPlot(i)->GetLabels()->GetNumberOfTuples() > 1)
    {
      break;
    }
  }
  this->PlotTime.Modified();
}

//-----------------------------------------------------------------------------
bool svtkChartLegend::Paint(svtkContext2D* painter)
{
  // This is where everything should be drawn, or dispatched to other methods.
  svtkDebugMacro(<< "Paint event called in svtkChartLegend.");

  if (!this->Visible || this->Storage->ActivePlots.empty())
  {
    return true;
  }

  this->GetBoundingRect(painter);

  // Now draw a box for the legend.
  painter->ApplyPen(this->Pen);
  painter->ApplyBrush(this->Brush);
  painter->DrawRect(
    this->Rect.GetX(), this->Rect.GetY(), this->Rect.GetWidth(), this->Rect.GetHeight());

  painter->ApplyTextProp(this->LabelProperties);

  svtkVector2f stringBounds[2];
  painter->ComputeStringBounds("Tgyf", stringBounds->GetData());
  float height = stringBounds[1].GetY();
  painter->ComputeStringBounds("The", stringBounds->GetData());
  float baseHeight = stringBounds[1].GetY();

  svtkVector2f pos(this->Rect.GetX() + this->Padding + this->SymbolWidth,
    this->Rect.GetY() + this->Rect.GetHeight() - this->Padding - floor(height));
  svtkRectf rect(this->Rect.GetX() + this->Padding, pos.GetY(), this->SymbolWidth - 3, ceil(height));

  // Draw all of the legend labels and marks
  for (size_t i = 0; i < this->Storage->ActivePlots.size(); ++i)
  {
    if (!this->Storage->ActivePlots[i]->GetLegendVisibility())
    {
      // skip if legend is not visible.
      continue;
    }

    svtkStringArray* labels = this->Storage->ActivePlots[i]->GetLabels();
    for (svtkIdType l = 0; labels && (l < labels->GetNumberOfValues()); ++l)
    {
      // This is fairly hackish, but gets the text looking reasonable...
      // Calculate a height for a "normal" string, then if this height is greater
      // that offset is used to move it down. Effectively hacking in a text
      // base line until better support is in the text rendering code...
      // There are still several one pixel glitches, but it looks better than
      // using the default vertical alignment. FIXME!
      svtkStdString testString = labels->GetValue(l);
      testString += "T";
      painter->ComputeStringBounds(testString, stringBounds->GetData());
      painter->DrawString(
        pos.GetX(), rect.GetY() + (baseHeight - stringBounds[1].GetY()), labels->GetValue(l));

      // Paint the legend mark and increment out y value.
      this->Storage->ActivePlots[i]->PaintLegend(painter, rect, l);
      rect.SetY(rect.GetY() - height - this->Padding);
    }
  }

  return true;
}

//-----------------------------------------------------------------------------
svtkRectf svtkChartLegend::GetBoundingRect(svtkContext2D* painter)
{
  if (this->CacheBounds && this->RectTime > this->GetMTime() && this->RectTime > this->PlotTime)
  {
    return this->Rect;
  }

  painter->ApplyTextProp(this->LabelProperties);

  svtkVector2f stringBounds[2];
  painter->ComputeStringBounds("Tgyf", stringBounds->GetData());
  float height = stringBounds[1].GetY();
  float maxWidth = 0.0f;

  // Calculate the widest legend label - needs the context to calculate font
  // metrics, but these could be cached.
  for (size_t i = 0; i < this->Storage->ActivePlots.size(); ++i)
  {
    if (!this->Storage->ActivePlots[i]->GetLegendVisibility())
    {
      // skip if legend is not visible.
      continue;
    }
    svtkStringArray* labels = this->Storage->ActivePlots[i]->GetLabels();
    for (svtkIdType l = 0; labels && (l < labels->GetNumberOfTuples()); ++l)
    {
      painter->ComputeStringBounds(labels->GetValue(l), stringBounds->GetData());
      if (stringBounds[1].GetX() > maxWidth)
      {
        maxWidth = stringBounds[1].GetX();
      }
    }
  }

  // Figure out the size of the legend box and store locally.
  int numLabels = 0;
  for (size_t i = 0; i < this->Storage->ActivePlots.size(); ++i)
  {
    if (!this->Storage->ActivePlots[i]->GetLegendVisibility())
    {
      // skip if legend is not visible.
      continue;
    }
    numLabels += this->Storage->ActivePlots[i]->GetNumberOfLabels();
  }

  // Default point placement is bottom left.
  this->Rect = svtkRectf(floor(this->Storage->Point.GetX()), floor(this->Storage->Point.GetY()),
    ceil(maxWidth + 2 * this->Padding + this->SymbolWidth),
    ceil((numLabels * (height + this->Padding)) + this->Padding));

  this->RectTime.Modified();
  return this->Rect;
}

//-----------------------------------------------------------------------------
void svtkChartLegend::SetPoint(const svtkVector2f& point)
{
  this->Storage->Point = point;
  this->Modified();
}

//-----------------------------------------------------------------------------
const svtkVector2f& svtkChartLegend::GetPointVector()
{
  return this->Storage->Point;
}

//-----------------------------------------------------------------------------
void svtkChartLegend::SetLabelSize(int size)
{
  this->LabelProperties->SetFontSize(size);
}

//-----------------------------------------------------------------------------
int svtkChartLegend::GetLabelSize()
{
  return this->LabelProperties->GetFontSize();
}

//-----------------------------------------------------------------------------
svtkPen* svtkChartLegend::GetPen()
{
  return this->Pen;
}

//-----------------------------------------------------------------------------
svtkBrush* svtkChartLegend::GetBrush()
{
  return this->Brush;
}

//-----------------------------------------------------------------------------
svtkTextProperty* svtkChartLegend::GetLabelProperties()
{
  return this->LabelProperties;
}

//-----------------------------------------------------------------------------
void svtkChartLegend::SetChart(svtkChart* chart)
{
  if (this->Storage->Chart == chart)
  {
    return;
  }
  else
  {
    this->Storage->Chart = chart;
    this->Modified();
  }
}

//-----------------------------------------------------------------------------
svtkChart* svtkChartLegend::GetChart()
{
  return this->Storage->Chart;
}

//-----------------------------------------------------------------------------
bool svtkChartLegend::Hit(const svtkContextMouseEvent& mouse)
{
  if (!this->GetVisible())
  {
    return false;
  }
  if (this->DragEnabled && mouse.GetPos().GetX() > this->Rect.GetX() &&
    mouse.GetPos().GetX() < this->Rect.GetX() + this->Rect.GetWidth() &&
    mouse.GetPos().GetY() > this->Rect.GetY() &&
    mouse.GetPos().GetY() < this->Rect.GetY() + this->Rect.GetHeight())
  {
    return true;
  }
  else
  {
    return false;
  }
}

//-----------------------------------------------------------------------------
bool svtkChartLegend::MouseMoveEvent(const svtkContextMouseEvent& mouse)
{
  if (this->Button == svtkContextMouseEvent::LEFT_BUTTON)
  {
    svtkVector2f delta = mouse.GetPos() - mouse.GetLastPos();
    this->Storage->Point = this->Storage->Point + delta;
    this->GetScene()->SetDirty(true);
    this->Modified();
  }
  return true;
}

//-----------------------------------------------------------------------------
bool svtkChartLegend::MouseButtonPressEvent(const svtkContextMouseEvent& mouse)
{
  if (mouse.GetButton() == svtkContextMouseEvent::LEFT_BUTTON)
  {
    this->Button = svtkContextMouseEvent::LEFT_BUTTON;
    return true;
  }
  return false;
}

//-----------------------------------------------------------------------------
bool svtkChartLegend::MouseButtonReleaseEvent(const svtkContextMouseEvent&)
{
  this->Button = -1;
  return true;
}

//-----------------------------------------------------------------------------
void svtkChartLegend::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
