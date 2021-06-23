/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkScalarsToColorsItem.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkScalarsToColorsItem.h"
#include "svtkAxis.h"
#include "svtkBrush.h"
#include "svtkCallbackCommand.h"
#include "svtkContext2D.h"
#include "svtkContextDevice2D.h"
#include "svtkContextScene.h"
#include "svtkDoubleArray.h"
#include "svtkFloatArray.h"
#include "svtkImageData.h"
#include "svtkNew.h"
#include "svtkObjectFactory.h"
#include "svtkPen.h"
#include "svtkPlotBar.h"
#include "svtkPoints2D.h"
#include "svtkSmartPointer.h"
#include "svtkTable.h"
#include "svtkTransform2D.h"

#include <cassert>

//-----------------------------------------------------------------------------
svtkCxxSetObjectMacro(svtkScalarsToColorsItem, HistogramTable, svtkTable);

//-----------------------------------------------------------------------------
svtkScalarsToColorsItem::svtkScalarsToColorsItem()
{
  this->PolyLinePen->SetWidth(2.);
  this->PolyLinePen->SetColor(64, 64, 72); // Payne's grey, why not
  this->PolyLinePen->SetLineType(svtkPen::NO_PEN);

  this->Shape->SetDataTypeToFloat();
  this->Shape->SetNumberOfPoints(0);

  this->Callback->SetClientData(this);
  this->Callback->SetCallback(svtkScalarsToColorsItem::OnScalarsToColorsModified);

  this->MaskAboveCurve = false;

  this->UserBounds[0] = this->UserBounds[2] = 0.0;
  this->UserBounds[1] = this->UserBounds[3] = -1.0;

  this->PlotBar->GetPen()->SetLineType(svtkPen::NO_PEN);
  this->PlotBar->SelectableOn();
  this->PlotBar->SetInteractive(false);
  this->PlotBar->ScalarVisibilityOn();
  this->PlotBar->EnableOpacityMappingOff();
  this->PlotBar->SetOffset(0);
  this->AddItem(this->PlotBar);
}

//-----------------------------------------------------------------------------
svtkScalarsToColorsItem::~svtkScalarsToColorsItem()
{
  if (this->HistogramTable)
  {
    this->HistogramTable->Delete();
    this->HistogramTable = nullptr;
  }
  if (this->Texture)
  {
    this->Texture->Delete();
    this->Texture = nullptr;
  }
}

//-----------------------------------------------------------------------------
void svtkScalarsToColorsItem::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Interpolate: " << this->Interpolate << endl;
}

//-----------------------------------------------------------------------------
void svtkScalarsToColorsItem::GetBounds(double bounds[4])
{
  if (this->UserBounds[1] > this->UserBounds[0] && this->UserBounds[3] > this->UserBounds[2])
  {
    bounds[0] = this->UserBounds[0];
    bounds[1] = this->UserBounds[1];
    bounds[2] = this->UserBounds[2];
    bounds[3] = this->UserBounds[3];
    return;
  }
  this->ComputeBounds(bounds);
}

//-----------------------------------------------------------------------------
void svtkScalarsToColorsItem::ComputeBounds(double bounds[4])
{
  bounds[0] = 0.;
  bounds[1] = 1.;
  bounds[2] = 0.;
  bounds[3] = 1.;
}

//-----------------------------------------------------------------------------
bool svtkScalarsToColorsItem::Paint(svtkContext2D* painter)
{
  this->TextureWidth = this->GetScene()->GetViewWidth();
  if (this->Texture == nullptr || this->Texture->GetMTime() < this->GetMTime())
  {
    this->ComputeTexture();
  }

  const int size = this->Shape->GetNumberOfPoints();
  if (this->ConfigurePlotBar())
  {
    // The superclass will take care of painting the plot bar which is a child item.
    this->Superclass::Paint(painter);
  }
  else
  {
    if (this->Texture == nullptr)
    {
      return false;
    }
    svtkSmartPointer<svtkPen> transparentPen = svtkSmartPointer<svtkPen>::New();
    transparentPen->SetLineType(svtkPen::NO_PEN);
    painter->ApplyPen(transparentPen);
    painter->GetBrush()->SetColorF(0., 0., 0., 1.);
    painter->GetBrush()->SetColorF(1., 1., 1., 1.);
    painter->GetBrush()->SetTexture(this->Texture);
    painter->GetBrush()->SetTextureProperties(
      (this->Interpolate ? svtkBrush::Nearest : svtkBrush::Linear) | svtkBrush::Stretch);
    if (!this->MaskAboveCurve || size < 2)
    {
      double dbounds[4];
      this->GetBounds(dbounds);
      painter->DrawQuad(dbounds[0], dbounds[2], dbounds[0], dbounds[3], dbounds[1], dbounds[3],
        dbounds[1], dbounds[2]);
    }
    else
    {
      const svtkRectd& ss = this->ShiftScale;

      svtkNew<svtkPoints2D> trapezoids;
      trapezoids->SetNumberOfPoints(2 * size);
      double point[2];
      svtkIdType j = -1;
      for (svtkIdType i = 0; i < size; ++i)
      {
        this->Shape->GetPoint(i, point);

        // shift/scale to scale from data space to rendering space.
        point[0] = (point[0] + ss[0]) * ss[2];
        point[1] = (point[1] + ss[1]) * ss[3];
        trapezoids->SetPoint(++j, point[0], 0.);
        trapezoids->SetPoint(++j, point);
      }
      painter->DrawQuadStrip(trapezoids);
    }
  }

  if (this->PolyLinePen->GetLineType() != svtkPen::NO_PEN && size >= 2)
  {
    const svtkRectd& ss = this->ShiftScale;

    svtkNew<svtkPoints2D> transformedShape;
    transformedShape->SetNumberOfPoints(size);
    for (svtkIdType i = 0; i < size; ++i)
    {
      double point[2];
      this->Shape->GetPoint(i, point);
      // shift/scale to scale from data space to rendering space.
      point[0] = (point[0] + ss[0]) * ss[2];
      point[1] = (point[1] + ss[1]) * ss[3];
      transformedShape->SetPoint(i, point);
    }
    painter->ApplyPen(this->PolyLinePen);
    painter->DrawPoly(transformedShape);
  }

  return true;
}

//-----------------------------------------------------------------------------
void svtkScalarsToColorsItem::OnScalarsToColorsModified(
  svtkObject* caller, unsigned long eid, void* clientdata, void* calldata)
{
  svtkScalarsToColorsItem* self = reinterpret_cast<svtkScalarsToColorsItem*>(clientdata);
  self->ScalarsToColorsModified(caller, eid, calldata);
}

//-----------------------------------------------------------------------------
void svtkScalarsToColorsItem::ScalarsToColorsModified(
  svtkObject* svtkNotUsed(object), unsigned long svtkNotUsed(eid), void* svtkNotUsed(calldata))
{
  this->Modified();
}

//-----------------------------------------------------------------------------
bool svtkScalarsToColorsItem::ConfigurePlotBar()
{
  bool visible = this->HistogramTable && this->HistogramTable->GetNumberOfColumns() >= 2 &&
    this->GetXAxis() && this->GetYAxis();
  if (visible)
  {
    // Configure the plot bar
    this->PlotBar->SetInputData(this->HistogramTable, this->HistogramTable->GetColumnName(0),
      this->HistogramTable->GetColumnName(1));
    this->PlotBar->SelectColorArray(this->HistogramTable->GetColumnName(0));
    this->PlotBar->SetXAxis(this->GetXAxis());
    this->PlotBar->SetYAxis(this->GetYAxis());

    // Configure the plot bar Y Axis
    svtkDoubleArray* valueArray = svtkDoubleArray::SafeDownCast(this->HistogramTable->GetColumn(1));
    if (!valueArray)
    {
      svtkErrorMacro("HistogramTable is not containing expected data");
      return false;
    }
    double valueRange[2];
    valueArray->GetRange(valueRange);
    double val = 1 / valueRange[1];
    svtkRectd shiftScale = this->ShiftScale;
    shiftScale.SetHeight(shiftScale.GetHeight() * val);
    this->PlotBar->SetShiftScale(shiftScale);

    // Recover the range of the histogram
    svtkDoubleArray* binExtent = svtkDoubleArray::SafeDownCast(this->HistogramTable->GetColumn(0));
    if (binExtent)
    {
      // It is necessary to extract the actual range of computation of the histogram
      // as it can be different to the range of the scalar to colors item.
      int nBin = this->HistogramTable->GetNumberOfRows();
      double range = binExtent->GetValue(nBin - 1) - binExtent->GetValue(0);
      double delta = range / (nBin - 1);
      this->PlotBar->SetWidth((range + delta) / nBin);
    }
    else
    {
      svtkWarningMacro("Could not find the bin extent array, histogram width has not been set");
    }
  }
  this->PlotBar->SetVisible(visible);
  this->PlotBar->Update();
  return visible;
}

//-----------------------------------------------------------------------------
svtkIdType svtkScalarsToColorsItem::GetNearestPoint(const svtkVector2f& point,
  const svtkVector2f& tolerance, svtkVector2f* location, svtkIdType* segmentIndex)
{
  if (this->PlotBar->GetVisible())
  {
    return this->PlotBar->GetNearestPoint(point, tolerance, location, segmentIndex);
  }
  return -1;
}

//-----------------------------------------------------------------------------
svtkStdString svtkScalarsToColorsItem::GetTooltipLabel(
  const svtkVector2d& plotPos, svtkIdType seriesIndex, svtkIdType segmentIndex)
{
  if (this->PlotBar->GetVisible())
  {
    return this->PlotBar->GetTooltipLabel(plotPos, seriesIndex, segmentIndex);
  }
  return "";
}
