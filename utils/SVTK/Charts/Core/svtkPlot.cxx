/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPlot.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkPlot.h"

#include "svtkAxis.h"
#include "svtkBrush.h"
#include "svtkContextMapper2D.h"
#include "svtkDataObject.h"
#include "svtkIdTypeArray.h"
#include "svtkNew.h"
#include "svtkObjectFactory.h"
#include "svtkPen.h"
#include "svtkStringArray.h"
#include "svtkTable.h"
#include <sstream>

svtkCxxSetObjectMacro(svtkPlot, XAxis, svtkAxis);
svtkCxxSetObjectMacro(svtkPlot, YAxis, svtkAxis);

//-----------------------------------------------------------------------------
svtkPlot::svtkPlot()
  : ShiftScale(0.0, 0.0, 1.0, 1.0)
{
  this->Pen = svtkSmartPointer<svtkPen>::New();
  this->Pen->SetWidth(2.0);
  this->Brush = svtkSmartPointer<svtkBrush>::New();

  this->SelectionPen = svtkSmartPointer<svtkPen>::New();
  this->SelectionPen->SetColor(255, 50, 0, 150);
  this->SelectionPen->SetWidth(4.0);
  this->SelectionBrush = svtkSmartPointer<svtkBrush>::New();
  this->SelectionBrush->SetColor(255, 50, 0, 150);

  this->Labels = nullptr;
  this->UseIndexForXSeries = false;
  this->Data = svtkSmartPointer<svtkContextMapper2D>::New();
  this->Selectable = true;
  this->Selection = nullptr;
  this->XAxis = nullptr;
  this->YAxis = nullptr;

  this->TooltipDefaultLabelFormat = "%l: %x,  %y";
  this->TooltipNotation = svtkAxis::STANDARD_NOTATION;
  this->TooltipPrecision = 6;

  this->LegendVisibility = true;
}

//-----------------------------------------------------------------------------
svtkPlot::~svtkPlot()
{
  if (this->Selection)
  {
    this->Selection->Delete();
    this->Selection = nullptr;
  }
  this->SetLabels(nullptr);
  this->SetXAxis(nullptr);
  this->SetYAxis(nullptr);
}

//-----------------------------------------------------------------------------
bool svtkPlot::PaintLegend(svtkContext2D*, const svtkRectf&, int)
{
  return false;
}

#ifndef SVTK_LEGACY_REMOVE
//-----------------------------------------------------------------------------
svtkIdType svtkPlot::GetNearestPoint(
  const svtkVector2f& point, const svtkVector2f& tolerance, svtkVector2f* location)
{
  // When using legacy code, we need to make sure old override are still called
  // and old call are still working. This is the more generic way to achieve that
  // The flag is here to ensure that the two implementation
  // do not call each other in an infinite loop.
  if (!this->LegacyRecursionFlag)
  {
    svtkIdType segmentId;
    this->LegacyRecursionFlag = true;
    svtkIdType ret = this->GetNearestPoint(point, tolerance, location, &segmentId);
    this->LegacyRecursionFlag = false;
    return ret;
  }
  else
  {
    return -1;
  }
}
#endif // SVTK_LEGACY_REMOVE

//-----------------------------------------------------------------------------
svtkIdType svtkPlot::GetNearestPoint(
#ifndef SVTK_LEGACY_REMOVE
  const svtkVector2f& point, const svtkVector2f& tolerance, svtkVector2f* location,
#else
  const svtkVector2f& svtkNotUsed(point), const svtkVector2f& svtkNotUsed(tolerance),
  svtkVector2f* svtkNotUsed(location),
#endif // SVTK_LEGACY_REMOVE
  svtkIdType* svtkNotUsed(segmentId))
{
#ifndef SVTK_LEGACY_REMOVE
  if (!this->LegacyRecursionFlag)
  {
    this->LegacyRecursionFlag = true;
    int ret = this->GetNearestPoint(point, tolerance, location);
    this->LegacyRecursionFlag = false;
    if (ret != -1)
    {
      SVTK_LEGACY_REPLACED_BODY(svtkPlot::GetNearestPoint(const svtkVector2f& point,
                                 const svtkVector2f& tol, svtkVector2f* location),
        "SVTK 9.0",
        svtkPlot::GetNearestPoint(const svtkVector2f& point, const svtkVector2f& tol,
          svtkVector2f* location, svtkIdType* segmentId));
    }
    return ret;
  }
#endif // SVTK_LEGACY_REMOVE
  return -1;
}

//-----------------------------------------------------------------------------
svtkStdString svtkPlot::GetTooltipLabel(const svtkVector2d& plotPos, svtkIdType seriesIndex, svtkIdType)
{
  svtkStdString tooltipLabel;
  svtkStdString& format =
    this->TooltipLabelFormat.empty() ? this->TooltipDefaultLabelFormat : this->TooltipLabelFormat;
  // Parse TooltipLabelFormat and build tooltipLabel
  bool escapeNext = false;
  for (size_t i = 0; i < format.length(); ++i)
  {
    if (escapeNext)
    {
      switch (format[i])
      {
        case 'x':
          tooltipLabel += this->GetNumber(plotPos.GetX(), this->XAxis);
          break;
        case 'y':
          tooltipLabel += this->GetNumber(plotPos.GetY(), this->YAxis);
          break;
        case 'i':
          if (this->IndexedLabels && seriesIndex >= 0 &&
            seriesIndex < this->IndexedLabels->GetNumberOfTuples())
          {
            tooltipLabel += this->IndexedLabels->GetValue(seriesIndex);
          }
          break;
        case 'l':
          // GetLabel() is GetLabel(0) in this implementation
          tooltipLabel += this->GetLabel();
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

//-----------------------------------------------------------------------------
svtkStdString svtkPlot::GetNumber(double position, svtkAxis* axis)
{
  // Determine and format the X and Y position in the chart
  std::ostringstream ostr;
  ostr.imbue(std::locale::classic());
  ostr.precision(this->GetTooltipPrecision());

  if (this->GetTooltipNotation() == svtkAxis::SCIENTIFIC_NOTATION)
  {
    ostr.setf(ios::scientific, ios::floatfield);
  }
  else if (this->GetTooltipNotation() == svtkAxis::FIXED_NOTATION)
  {
    ostr.setf(ios::fixed, ios::floatfield);
  }

  if (axis && axis->GetLogScaleActive())
  {
    // If axes are set to logarithmic scale we need to convert the
    // axis value using 10^(axis value)
    ostr << pow(double(10.0), double(position));
  }
  else
  {
    ostr << position;
  }
  return ostr.str();
}

//-----------------------------------------------------------------------------
bool svtkPlot::SelectPoints(const svtkVector2f&, const svtkVector2f&)
{
  if (this->Selection)
  {
    this->Selection->SetNumberOfTuples(0);
  }
  return false;
}

//-----------------------------------------------------------------------------
bool svtkPlot::SelectPointsInPolygon(const svtkContextPolygon&)
{
  if (this->Selection)
  {
    this->Selection->SetNumberOfTuples(0);
  }
  return false;
}

//-----------------------------------------------------------------------------
void svtkPlot::SetColor(unsigned char r, unsigned char g, unsigned char b, unsigned char a)
{
  this->Pen->SetColor(r, g, b, a);
}

//-----------------------------------------------------------------------------
void svtkPlot::SetColor(double r, double g, double b)
{
  this->Pen->SetColorF(r, g, b);
}

//-----------------------------------------------------------------------------
void svtkPlot::GetColor(double rgb[3])
{
  this->Pen->GetColorF(rgb);
}

//-----------------------------------------------------------------------------
void svtkPlot::GetColor(unsigned char rgb[3])
{
  double rgbF[3];
  this->GetColor(rgbF);
  rgb[0] = static_cast<unsigned char>(255. * rgbF[0] + 0.5);
  rgb[1] = static_cast<unsigned char>(255. * rgbF[1] + 0.5);
  rgb[2] = static_cast<unsigned char>(255. * rgbF[2] + 0.5);
}

//-----------------------------------------------------------------------------
void svtkPlot::SetWidth(float width)
{
  this->Pen->SetWidth(width);
}

//-----------------------------------------------------------------------------
float svtkPlot::GetWidth()
{
  return this->Pen->GetWidth();
}

//-----------------------------------------------------------------------------
void svtkPlot::SetPen(svtkPen* pen)
{
  if (this->Pen != pen)
  {
    this->Pen = pen;
    this->Modified();
  }
}

//-----------------------------------------------------------------------------
svtkPen* svtkPlot::GetPen()
{
  return this->Pen;
}

//-----------------------------------------------------------------------------
void svtkPlot::SetBrush(svtkBrush* brush)
{
  if (this->Brush != brush)
  {
    this->Brush = brush;
    this->Modified();
  }
}

//-----------------------------------------------------------------------------
svtkBrush* svtkPlot::GetBrush()
{
  return this->Brush;
}

//-----------------------------------------------------------------------------
void svtkPlot::SetSelectionPen(svtkPen* pen)
{
  if (this->SelectionPen != pen)
  {
    this->SelectionPen = pen;
    this->Modified();
  }
}

//-----------------------------------------------------------------------------
svtkPen* svtkPlot::GetSelectionPen()
{
  return this->SelectionPen;
}

//-----------------------------------------------------------------------------
void svtkPlot::SetSelectionBrush(svtkBrush* brush)
{
  if (this->SelectionBrush != brush)
  {
    this->SelectionBrush = brush;
    this->Modified();
  }
}

//-----------------------------------------------------------------------------
svtkBrush* svtkPlot::GetSelectionBrush()
{
  return this->SelectionBrush;
}

//-----------------------------------------------------------------------------
void svtkPlot::SetLabel(const svtkStdString& label)
{
  svtkNew<svtkStringArray> labels;
  labels->InsertNextValue(label);
  this->SetLabels(labels);
}

//-----------------------------------------------------------------------------
svtkStdString svtkPlot::GetLabel()
{
  return this->GetLabel(0);
}

//-----------------------------------------------------------------------------
void svtkPlot::SetLabels(svtkStringArray* labels)
{
  if (this->Labels == labels)
  {
    return;
  }

  this->Labels = labels;
  this->Modified();
}

//-----------------------------------------------------------------------------
svtkStringArray* svtkPlot::GetLabels()
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
    return this->AutoLabels;
  }
  else
  {
    return nullptr;
  }
}
//-----------------------------------------------------------------------------
int svtkPlot::GetNumberOfLabels()
{
  svtkStringArray* labels = this->GetLabels();
  if (labels)
  {
    return labels->GetNumberOfValues();
  }
  else
  {
    return 0;
  }
}

//-----------------------------------------------------------------------------
void svtkPlot::SetIndexedLabels(svtkStringArray* labels)
{
  if (this->IndexedLabels == labels)
  {
    return;
  }

  if (labels)
  {
    this->TooltipDefaultLabelFormat = "%i: %x,  %y";
  }
  else
  {
    this->TooltipDefaultLabelFormat = "%l: %x,  %y";
  }

  this->IndexedLabels = labels;
  this->Modified();
}

//-----------------------------------------------------------------------------
svtkStringArray* svtkPlot::GetIndexedLabels()
{
  return this->IndexedLabels;
}

//-----------------------------------------------------------------------------
svtkContextMapper2D* svtkPlot::GetData()
{
  return this->Data;
}

//-----------------------------------------------------------------------------
void svtkPlot::SetTooltipLabelFormat(const svtkStdString& labelFormat)
{
  if (this->TooltipLabelFormat == labelFormat)
  {
    return;
  }

  this->TooltipLabelFormat = labelFormat;
  this->Modified();
}

//-----------------------------------------------------------------------------
svtkStdString svtkPlot::GetTooltipLabelFormat()
{
  return this->TooltipLabelFormat;
}

//-----------------------------------------------------------------------------
void svtkPlot::SetTooltipNotation(int notation)
{
  this->TooltipNotation = notation;
  this->Modified();
}

//-----------------------------------------------------------------------------
int svtkPlot::GetTooltipNotation()
{
  return this->TooltipNotation;
}

//-----------------------------------------------------------------------------
void svtkPlot::SetTooltipPrecision(int precision)
{
  this->TooltipPrecision = precision;
  this->Modified();
}

//-----------------------------------------------------------------------------
int svtkPlot::GetTooltipPrecision()
{
  return this->TooltipPrecision;
}

//-----------------------------------------------------------------------------
svtkStdString svtkPlot::GetLabel(svtkIdType index)
{
  svtkStringArray* labels = this->GetLabels();
  if (labels && index >= 0 && index < labels->GetNumberOfValues())
  {
    return labels->GetValue(index);
  }
  else
  {
    return svtkStdString();
  }
}
//-----------------------------------------------------------------------------
void svtkPlot::SetInputData(svtkTable* table)
{
  this->Data->SetInputData(table);
  this->AutoLabels = nullptr; // No longer valid
}

//-----------------------------------------------------------------------------
void svtkPlot::SetInputData(
  svtkTable* table, const svtkStdString& xColumn, const svtkStdString& yColumn)
{
  svtkDebugMacro(<< "Setting input, X column = \"" << xColumn.c_str() << "\", "
                << "Y column = \"" << yColumn.c_str() << "\"");

  this->Data->SetInputData(table);
  this->Data->SetInputArrayToProcess(
    0, 0, 0, svtkDataObject::FIELD_ASSOCIATION_ROWS, xColumn.c_str());
  this->Data->SetInputArrayToProcess(
    1, 0, 0, svtkDataObject::FIELD_ASSOCIATION_ROWS, yColumn.c_str());
  this->AutoLabels = nullptr; // No longer valid
}

//-----------------------------------------------------------------------------
void svtkPlot::SetInputData(svtkTable* table, svtkIdType xColumn, svtkIdType yColumn)
{
  this->SetInputData(table, table->GetColumnName(xColumn), table->GetColumnName(yColumn));
}

//-----------------------------------------------------------------------------
svtkTable* svtkPlot::GetInput()
{
  return this->Data->GetInput();
}

//-----------------------------------------------------------------------------
void svtkPlot::SetInputArray(int index, const svtkStdString& name)
{
  this->Data->SetInputArrayToProcess(
    index, 0, 0, svtkDataObject::FIELD_ASSOCIATION_ROWS, name.c_str());
  this->AutoLabels = nullptr; // No longer valid
}

//-----------------------------------------------------------------------------
void svtkPlot::SetSelection(svtkIdTypeArray* id)
{
  if (!this->GetSelectable())
  {
    return;
  }
  svtkSetObjectBodyMacro(Selection, svtkIdTypeArray, id);
}

//-----------------------------------------------------------------------------
void svtkPlot::SetShiftScale(const svtkRectd& shiftScale)
{
  if (shiftScale != this->ShiftScale)
  {
    this->Modified();
    this->ShiftScale = shiftScale;
  }
}

//-----------------------------------------------------------------------------
svtkRectd svtkPlot::GetShiftScale()
{
  return this->ShiftScale;
}

//-----------------------------------------------------------------------------
void svtkPlot::SetProperty(const svtkStdString&, const svtkVariant&) {}

//-----------------------------------------------------------------------------
svtkVariant svtkPlot::GetProperty(const svtkStdString&)
{
  return svtkVariant();
}

//-----------------------------------------------------------------------------
void svtkPlot::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "LegendVisibility: " << this->LegendVisibility << endl;
}

//-----------------------------------------------------------------------------
void svtkPlot::TransformScreenToData(const svtkVector2f& in, svtkVector2f& out)
{
  double tmp[2] = { in.GetX(), in.GetY() };

  this->TransformScreenToData(tmp[0], tmp[1], tmp[0], tmp[1]);

  out.Set(static_cast<float>(tmp[0]), static_cast<float>(tmp[1]));
}

//-----------------------------------------------------------------------------
void svtkPlot::TransformDataToScreen(const svtkVector2f& in, svtkVector2f& out)
{
  double tmp[2] = { in.GetX(), in.GetY() };

  this->TransformDataToScreen(tmp[0], tmp[1], tmp[0], tmp[1]);

  out.Set(static_cast<float>(tmp[0]), static_cast<float>(tmp[1]));
}

//-----------------------------------------------------------------------------
void svtkPlot::TransformScreenToData(const double inX, const double inY, double& outX, double& outY)
{
  // inverse shift/scale from screen space.
  const svtkRectd& ss = this->ShiftScale;
  outX = (inX / ss[2]) - ss[0];
  outY = (inY / ss[3]) - ss[1];

  const bool logX = this->GetXAxis() && this->GetXAxis()->GetLogScaleActive();
  const bool logY = this->GetYAxis() && this->GetYAxis()->GetLogScaleActive();

  if (logX)
  {
    outX = std::pow(10., outX);
  }
  if (logY)
  {
    outY = std::pow(10., outY);
  }
}

//-----------------------------------------------------------------------------
void svtkPlot::TransformDataToScreen(const double inX, const double inY, double& outX, double& outY)
{
  outX = inX;
  outY = inY;

  const bool logX = this->GetXAxis() && this->GetXAxis()->GetLogScaleActive();
  const bool logY = this->GetYAxis() && this->GetYAxis()->GetLogScaleActive();

  if (logX)
  {
    outX = std::log10(outX);
  }
  if (logY)
  {
    outY = std::log10(outY);
  }

  // now, shift/scale to screen space.
  const svtkRectd& ss = this->ShiftScale;
  outX = (outX + ss[0]) * ss[2];
  outY = (outY + ss[1]) * ss[3];
}

//-----------------------------------------------------------------------------
bool svtkPlot::ClampPos(double pos[2], double bounds[4])
{
  if (bounds[1] < bounds[0] || bounds[3] < bounds[2])
  {
    // bounds are not valid. Don't clamp.
    return false;
  }
  bool clamped = false;
  if (pos[0] < bounds[0] || svtkMath::IsNan(pos[0]))
  {
    pos[0] = bounds[0];
    clamped = true;
  }
  if (pos[0] > bounds[1])
  {
    pos[0] = bounds[1];
    clamped = true;
  }
  if (pos[1] < 0. || svtkMath::IsNan(pos[0]))
  {
    pos[1] = 0.;
    clamped = true;
  }
  if (pos[1] > 1.)
  {
    pos[1] = 1.;
    clamped = true;
  }
  return clamped;
}

//-----------------------------------------------------------------------------
bool svtkPlot::ClampPos(double pos[2])
{
  double bounds[4];
  this->GetBounds(bounds);
  return svtkPlot::ClampPos(pos, bounds);
}
