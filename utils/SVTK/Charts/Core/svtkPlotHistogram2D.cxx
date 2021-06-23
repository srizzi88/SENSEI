/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtk2DHistogramItem.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkPlotHistogram2D.h"

#include "svtkAxis.h"
#include "svtkContext2D.h"
#include "svtkDoubleArray.h"
#include "svtkImageData.h"
#include "svtkMath.h"
#include "svtkScalarsToColors.h"
#include "svtkStringArray.h"

#include "svtkObjectFactory.h"

#include <algorithm>

//-----------------------------------------------------------------------------
svtkStandardNewMacro(svtkPlotHistogram2D);

//-----------------------------------------------------------------------------
svtkPlotHistogram2D::svtkPlotHistogram2D()
{
  this->TooltipDefaultLabelFormat = "%x,  %y:  %v";
}

//-----------------------------------------------------------------------------
svtkPlotHistogram2D::~svtkPlotHistogram2D() = default;

void svtkPlotHistogram2D::Update()
{
  this->GenerateHistogram();
}

//-----------------------------------------------------------------------------
bool svtkPlotHistogram2D::Paint(svtkContext2D* painter)
{
  if (this->Output)
  {
    if (this->Input)
    {
      double bounds[4];
      this->GetBounds(bounds);
      this->Position = svtkRectf(bounds[0], bounds[2], bounds[1] - bounds[0], bounds[3] - bounds[2]);
    }
    painter->DrawImage(this->Position, this->Output);
  }
  return true;
}

//-----------------------------------------------------------------------------
void svtkPlotHistogram2D::SetInputData(svtkImageData* data, svtkIdType)
{
  // FIXME: Store the z too, for slices.
  this->Input = data;
}

//-----------------------------------------------------------------------------
svtkImageData* svtkPlotHistogram2D::GetInputImageData()
{
  return this->Input;
}

//-----------------------------------------------------------------------------
void svtkPlotHistogram2D::SetTransferFunction(svtkScalarsToColors* function)
{
  this->TransferFunction = function;
}

//-----------------------------------------------------------------------------
svtkScalarsToColors* svtkPlotHistogram2D::GetTransferFunction()
{
  return this->TransferFunction;
}

//-----------------------------------------------------------------------------
void svtkPlotHistogram2D::GetBounds(double bounds[4])
{
  if (this->Input)
  {
    std::copy(this->Input->GetBounds(), this->Input->GetBounds() + 4, bounds);

    // Adding a spacing increment is necessary in order to draw in the context 2D correctly
    double* spacing = this->Input->GetSpacing();
    bounds[1] += spacing[0];
    bounds[3] += spacing[1];
  }
  else
  {
    std::fill(bounds, bounds + 4, 0.);
  }
}

//-----------------------------------------------------------------------------
void svtkPlotHistogram2D::SetPosition(const svtkRectf& pos)
{
  this->Position = pos;
}

//-----------------------------------------------------------------------------
svtkRectf svtkPlotHistogram2D::GetPosition()
{
  return this->Position;
}

//-----------------------------------------------------------------------------
svtkIdType svtkPlotHistogram2D::GetNearestPoint(const svtkVector2f& point,
  const svtkVector2f& tolerance, svtkVector2f* location, svtkIdType* svtkNotUsed(segmentId))
{
#ifndef SVTK_LEGACY_REMOVE
  if (!this->LegacyRecursionFlag)
  {
    this->LegacyRecursionFlag = true;
    svtkIdType ret = this->GetNearestPoint(point, tolerance, location);
    this->LegacyRecursionFlag = false;
    if (ret != -1)
    {
      SVTK_LEGACY_REPLACED_BODY(svtkPlotHistogram2D::GetNearestPoint(const svtkVector2f& point,
                                 const svtkVector2f& tolerance, svtkVector2f* location),
        "SVTK 9.0",
        svtkPlotHistogram2D::GetNearestPoint(const svtkVector2f& point, const svtkVector2f& tolerance,
          svtkVector2f* location, svtkIdType* segmentId));
      return ret;
    }
  }
#endif // SVTK_LEGACY_REMOVE

  if (!this->Input)
  {
    return -1;
  }

  (void)tolerance;
  double bounds[4];
  this->GetBounds(bounds);
  double spacing[3];

  this->Input->GetSpacing(spacing);

  if (point.GetX() < bounds[0] || point.GetX() > bounds[1] + spacing[0] ||
    point.GetY() < bounds[2] || point.GetY() > bounds[3] + spacing[1])
  {
    return -1;
  }

  // Can't use svtkImageData::FindPoint() / GetPoint(), as ImageData points are
  // rendered as the bottom left corner of a histogram cell, not the center
  int locX = svtkMath::Floor((point.GetX() - bounds[0]) / spacing[0]);
  int locY = svtkMath::Floor((point.GetY() - bounds[2]) / spacing[1]);
  int width = this->Input->GetExtent()[1] - this->Input->GetExtent()[0] + 1;

  // Discretize to ImageData point values
  location->SetX(locX * spacing[0] + bounds[0]);
  location->SetY(locY * spacing[1] + bounds[2]);

  return (locX + (locY * width));
}

//-----------------------------------------------------------------------------
svtkStdString svtkPlotHistogram2D::GetTooltipLabel(
  const svtkVector2d& plotPos, svtkIdType seriesIndex, svtkIdType)
{
  // This does not call the Superclass svtkPlot::GetTooltipLabel(), since the
  // format tags internally refer to different values
  svtkStdString tooltipLabel;
  svtkStdString& format =
    this->TooltipLabelFormat.empty() ? this->TooltipDefaultLabelFormat : this->TooltipLabelFormat;

  if (!this->Input)
  {
    return tooltipLabel;
  }

  double bounds[4];
  this->GetBounds(bounds);
  int width = this->Input->GetExtent()[1] - this->Input->GetExtent()[0] + 1;
  int height = this->Input->GetExtent()[3] - this->Input->GetExtent()[2] + 1;
  int pointX = seriesIndex % width + this->Input->GetExtent()[0];
  int pointY = seriesIndex / width + this->Input->GetExtent()[2];

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
          if (this->XAxis->GetTickLabels() && pointX >= 0 &&
            pointX < this->XAxis->GetTickLabels()->GetNumberOfTuples())
          {
            tooltipLabel += this->XAxis->GetTickLabels()->GetValue(pointX);
          }
          break;
        case 'j':
          if (this->YAxis->GetTickLabels() && pointY >= 0 &&
            pointY < this->YAxis->GetTickLabels()->GetNumberOfTuples())
          {
            tooltipLabel += this->YAxis->GetTickLabels()->GetValue(pointY);
          }
          break;
        case 'v':
          if (pointX >= 0 && pointX < width && pointY >= 0 && pointY < height)
          {
            tooltipLabel += this->GetNumber(
              this->Input->GetScalarComponentAsDouble(pointX, pointY, 0, 0), nullptr);
          }
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
void svtkPlotHistogram2D::GenerateHistogram()
{
  if (!this->Input)
  {
    return;
  }
  if (!this->Output)
  {
    this->Output = svtkSmartPointer<svtkImageData>::New();
  }
  this->Output->SetExtent(this->Input->GetExtent());
  this->Output->AllocateScalars(SVTK_UNSIGNED_CHAR, 4);

  int dimension = this->Input->GetDimensions()[0] * this->Input->GetDimensions()[1];
  void* const input = this->Input->GetScalarPointer();
  const int inputType = this->Input->GetScalarType();
  unsigned char* output = reinterpret_cast<unsigned char*>(this->Output->GetScalarPointer());

  if (this->TransferFunction)
  {
    this->TransferFunction->MapScalarsThroughTable2(input, output, inputType, dimension, 1, 4);
  }
}

//-----------------------------------------------------------------------------
void svtkPlotHistogram2D::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
