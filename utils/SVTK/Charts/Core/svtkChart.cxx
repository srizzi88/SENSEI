/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkChart.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkChart.h"
#include "svtkAxis.h"
#include "svtkBrush.h"
#include "svtkContextMouseEvent.h"
#include "svtkTransform2D.h"

#include "svtkAnnotationLink.h"
#include "svtkContextScene.h"
#include "svtkObjectFactory.h"
#include "svtkTextProperty.h"

//-----------------------------------------------------------------------------
svtkChart::MouseActions::MouseActions()
{
  this->Pan() = svtkContextMouseEvent::LEFT_BUTTON;
  this->Zoom() = svtkContextMouseEvent::MIDDLE_BUTTON;
  this->Select() = svtkContextMouseEvent::RIGHT_BUTTON;
  this->ZoomAxis() = -1;
  this->SelectPolygon() = -1;
  this->ClickAndDrag() = -1;
}

//-----------------------------------------------------------------------------
svtkChart::MouseClickActions::MouseClickActions()
{
  this->Data[0] = svtkContextMouseEvent::LEFT_BUTTON;
  this->Data[1] = svtkContextMouseEvent::RIGHT_BUTTON;
}

//-----------------------------------------------------------------------------
svtkCxxSetObjectMacro(svtkChart, AnnotationLink, svtkAnnotationLink);

//-----------------------------------------------------------------------------
svtkChart::svtkChart()
{
  this->Geometry[0] = 0;
  this->Geometry[1] = 0;
  this->Point1[0] = 0;
  this->Point1[1] = 0;
  this->Point2[0] = 0;
  this->Point2[1] = 0;
  this->Size.Set(0, 0, 0, 0);
  this->ShowLegend = false;
  this->TitleProperties = svtkTextProperty::New();
  this->TitleProperties->SetJustificationToCentered();
  this->TitleProperties->SetColor(0.0, 0.0, 0.0);
  this->TitleProperties->SetFontSize(12);
  this->TitleProperties->SetFontFamilyToArial();
  this->AnnotationLink = nullptr;
  this->LayoutStrategy = svtkChart::FILL_SCENE;
  this->RenderEmpty = false;
  this->BackgroundBrush = svtkSmartPointer<svtkBrush>::New();
  this->BackgroundBrush->SetColorF(1, 1, 1, 0);
  this->SelectionMode = svtkContextScene::SELECTION_NONE;
  this->SelectionMethod = svtkChart::SELECTION_ROWS;
}

//-----------------------------------------------------------------------------
svtkChart::~svtkChart()
{
  for (int i = 0; i < 4; i++)
  {
    if (this->GetAxis(i))
    {
      this->GetAxis(i)->RemoveObservers(svtkChart::UpdateRange);
    }
  }
  this->TitleProperties->Delete();
  if (this->AnnotationLink)
  {
    this->AnnotationLink->Delete();
  }
}

//-----------------------------------------------------------------------------
svtkPlot* svtkChart::AddPlot(int)
{
  return nullptr;
}

//-----------------------------------------------------------------------------
svtkIdType svtkChart::AddPlot(svtkPlot*)
{
  return -1;
}

//-----------------------------------------------------------------------------
bool svtkChart::RemovePlot(svtkIdType)
{
  return false;
}

//-----------------------------------------------------------------------------
bool svtkChart::RemovePlotInstance(svtkPlot* plot)
{
  if (plot)
  {
    svtkIdType numberOfPlots = this->GetNumberOfPlots();
    for (svtkIdType i = 0; i < numberOfPlots; ++i)
    {
      if (this->GetPlot(i) == plot)
      {
        return this->RemovePlot(i);
      }
    }
  }
  return false;
}

//-----------------------------------------------------------------------------
void svtkChart::ClearPlots() {}

//-----------------------------------------------------------------------------
svtkPlot* svtkChart::GetPlot(svtkIdType)
{
  return nullptr;
}

//-----------------------------------------------------------------------------
svtkIdType svtkChart::GetNumberOfPlots()
{
  return 0;
}

//-----------------------------------------------------------------------------
svtkAxis* svtkChart::GetAxis(int)
{
  return nullptr;
}

//-----------------------------------------------------------------------------
void svtkChart::SetAxis(int, svtkAxis*) {}

//-----------------------------------------------------------------------------
svtkIdType svtkChart::GetNumberOfAxes()
{
  return 0;
}

//-----------------------------------------------------------------------------
void svtkChart::RecalculateBounds() {}

//-----------------------------------------------------------------------------
void svtkChart::SetSelectionMethod(int method)
{
  if (method == this->SelectionMethod)
  {
    return;
  }
  this->SelectionMethod = method;
  this->Modified();
}

//-----------------------------------------------------------------------------
int svtkChart::GetSelectionMethod()
{
  return this->SelectionMethod;
}

//-----------------------------------------------------------------------------
void svtkChart::SetShowLegend(bool visible)
{
  if (this->ShowLegend != visible)
  {
    this->ShowLegend = visible;
    this->Modified();
  }
}

//-----------------------------------------------------------------------------
bool svtkChart::GetShowLegend()
{
  return this->ShowLegend;
}

svtkChartLegend* svtkChart::GetLegend()
{
  return nullptr;
}

//-----------------------------------------------------------------------------
void svtkChart::SetTitle(const svtkStdString& title)
{
  if (this->Title != title)
  {
    this->Title = title;
    this->Modified();
  }
}

//-----------------------------------------------------------------------------
svtkStdString svtkChart::GetTitle()
{
  return this->Title;
}

//-----------------------------------------------------------------------------
bool svtkChart::CalculatePlotTransform(svtkAxis* x, svtkAxis* y, svtkTransform2D* transform)
{
  if (!x || !y || !transform)
  {
    svtkWarningMacro("Called with null arguments.");
    return false;
  }

  svtkVector2d origin(x->GetMinimum(), y->GetMinimum());
  svtkVector2d scale(x->GetMaximum() - x->GetMinimum(), y->GetMaximum() - y->GetMinimum());
  svtkVector2d shift(0.0, 0.0);
  svtkVector2d factor(1.0, 1.0);

  for (int i = 0; i < 2; ++i)
  {
    double safeScale;
    if (scale[i] != 0.0)
    {
      safeScale = fabs(scale[i]);
    }
    else
    {
      safeScale = 1.0;
    }
    double safeOrigin;
    if (origin[i] != 0.0)
    {
      safeOrigin = fabs(origin[i]);
    }
    else
    {
      safeOrigin = 1.0;
    }

    if (fabs(log10(safeOrigin / safeScale)) > 2)
    {
      // the line below probably was meant to be something like
      // scale[i] = pow(10.0, floor(log10(safeOrigin / safeScale) / 3.0) * 3.0);
      // but instead was set to the following
      // shift[i] = floor(log10(safeOrigin / safeScale) / 3.0) * 3.0;
      // which makes no sense as the next line overwrites shift[i] ala
      shift[i] = -origin[i];
    }
    if (fabs(log10(safeScale)) > 10)
    {
      // We need to scale the transform to show all data, do this in blocks.
      factor[i] = pow(10.0, floor(log10(safeScale) / 10.0) * -10.0);
      scale[i] = scale[i] * factor[i];
    }
  }
  x->SetScalingFactor(factor[0]);
  x->SetShift(shift[0]);
  y->SetScalingFactor(factor[1]);
  y->SetShift(shift[1]);

  // Get the scale for the plot area from the x and y axes
  float* min = x->GetPoint1();
  float* max = x->GetPoint2();
  if (fabs(max[0] - min[0]) == 0.0)
  {
    return false;
  }
  float xScale = scale[0] / (max[0] - min[0]);

  // Now the y axis
  min = y->GetPoint1();
  max = y->GetPoint2();
  if (fabs(max[1] - min[1]) == 0.0)
  {
    return false;
  }
  float yScale = scale[1] / (max[1] - min[1]);

  transform->Identity();
  transform->Translate(this->Point1[0], this->Point1[1]);
  // Get the scale for the plot area from the x and y axes
  transform->Scale(1.0 / xScale, 1.0 / yScale);
  transform->Translate(
    -(x->GetMinimum() + shift[0]) * factor[0], -(y->GetMinimum() + shift[1]) * factor[1]);
  return true;
}

//-----------------------------------------------------------------------------
bool svtkChart::CalculateUnscaledPlotTransform(svtkAxis* x, svtkAxis* y, svtkTransform2D* transform)
{
  if (!x || !y || !transform)
  {
    svtkWarningMacro("Called with null arguments.");
    return false;
  }

  svtkVector2d scale(x->GetMaximum() - x->GetMinimum(), y->GetMaximum() - y->GetMinimum());

  // Get the scale for the plot area from the x and y axes
  float* min = x->GetPoint1();
  float* max = x->GetPoint2();
  if (fabs(max[0] - min[0]) == 0.0)
  {
    return false;
  }
  double xScale = scale[0] / (max[0] - min[0]);

  // Now the y axis
  min = y->GetPoint1();
  max = y->GetPoint2();
  if (fabs(max[1] - min[1]) == 0.0)
  {
    return false;
  }
  double yScale = scale[1] / (max[1] - min[1]);

  transform->Identity();
  transform->Translate(this->Point1[0], this->Point1[1]);
  // Get the scale for the plot area from the x and y axes
  transform->Scale(1.0 / xScale, 1.0 / yScale);
  transform->Translate(-x->GetMinimum(), -y->GetMinimum());
  return true;
}

//-----------------------------------------------------------------------------
void svtkChart::SetBottomBorder(int border)
{
  this->Point1[1] = border >= 0 ? border : 0;
  this->Point1[1] += static_cast<int>(this->Size.GetY());
}

//-----------------------------------------------------------------------------
void svtkChart::SetTopBorder(int border)
{
  this->Point2[1] = border >= 0 ? this->Geometry[1] - border : this->Geometry[1];
  this->Point2[1] += static_cast<int>(this->Size.GetY());
}

//-----------------------------------------------------------------------------
void svtkChart::SetLeftBorder(int border)
{
  this->Point1[0] = border >= 0 ? border : 0;
  this->Point1[0] += static_cast<int>(this->Size.GetX());
}

//-----------------------------------------------------------------------------
void svtkChart::SetRightBorder(int border)
{
  this->Point2[0] = border >= 0 ? this->Geometry[0] - border : this->Geometry[0];
  this->Point2[0] += static_cast<int>(this->Size.GetX());
}

//-----------------------------------------------------------------------------
void svtkChart::SetBorders(int left, int bottom, int right, int top)
{
  this->SetLeftBorder(left);
  this->SetRightBorder(right);
  this->SetTopBorder(top);
  this->SetBottomBorder(bottom);
}

void svtkChart::SetSize(const svtkRectf& rect)
{
  this->Size = rect;
  this->Geometry[0] = static_cast<int>(rect.GetWidth());
  this->Geometry[1] = static_cast<int>(rect.GetHeight());
}

svtkRectf svtkChart::GetSize()
{
  return this->Size;
}

void svtkChart::SetActionToButton(int action, int button)
{
  if (action < -1 || action >= MouseActions::MaxAction)
  {
    svtkErrorMacro("Error, invalid action value supplied: " << action);
    return;
  }
  this->Actions[action] = button;
  for (int i = 0; i < MouseActions::MaxAction; ++i)
  {
    if (this->Actions[i] == button && i != action)
    {
      this->Actions[i] = -1;
    }
  }
}

int svtkChart::GetActionToButton(int action)
{
  return this->Actions[action];
}

void svtkChart::SetClickActionToButton(int action, int button)
{
  if (action != svtkChart::SELECT && action != svtkChart::NOTIFY)
  {
    svtkErrorMacro("Error, invalid click action value supplied: " << action);
    return;
  }

  if (action == svtkChart::NOTIFY)
  {
    this->ActionsClick[0] = button;
  }
  else if (action == svtkChart::SELECT)
  {
    this->ActionsClick[1] = button;
  }
}

int svtkChart::GetClickActionToButton(int action)
{
  if (action == svtkChart::NOTIFY)
  {
    return this->ActionsClick[0];
  }
  else if (action == svtkChart::SELECT)
  {
    return this->ActionsClick[1];
  }

  return -1;
}

//-----------------------------------------------------------------------------
void svtkChart::SetBackgroundBrush(svtkBrush* brush)
{
  if (brush == nullptr)
  {
    // set to transparent white if brush is null
    this->BackgroundBrush->SetColorF(1, 1, 1, 0);
  }
  else
  {
    this->BackgroundBrush = brush;
  }

  this->Modified();
}

//-----------------------------------------------------------------------------
svtkBrush* svtkChart::GetBackgroundBrush()
{
  return this->BackgroundBrush;
}

//-----------------------------------------------------------------------------
void svtkChart::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  // Print out the chart's geometry if it has been set
  os << indent << "Point1: " << this->Point1[0] << "\t" << this->Point1[1] << endl;
  os << indent << "Point2: " << this->Point2[0] << "\t" << this->Point2[1] << endl;
  os << indent << "Width: " << this->Geometry[0] << endl
     << indent << "Height: " << this->Geometry[1] << endl;
  os << indent << "SelectionMode: " << this->SelectionMode << endl;
}
//-----------------------------------------------------------------------------
void svtkChart::AttachAxisRangeListener(svtkAxis* axis)
{
  axis->AddObserver(svtkChart::UpdateRange, this, &svtkChart::AxisRangeForwarderCallback);
}

//-----------------------------------------------------------------------------
void svtkChart::AxisRangeForwarderCallback(svtkObject*, unsigned long, void*)
{
  double fullAxisRange[8];
  for (int i = 0; i < 4; i++)
  {
    this->GetAxis(i)->GetRange(&fullAxisRange[i * 2]);
  }
  this->InvokeEvent(svtkChart::UpdateRange, fullAxisRange);
}

//-----------------------------------------------------------------------------
void svtkChart::SetSelectionMode(int selMode)
{
  if (this->SelectionMode == selMode || selMode < svtkContextScene::SELECTION_NONE ||
    selMode > svtkContextScene::SELECTION_TOGGLE)
  {
    return;
  }
  this->SelectionMode = selMode;
  this->Modified();
}
