/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPlotFunctionalBag.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkPlotFunctionalBag.h"

#include "svtkAbstractArray.h"
#include "svtkAxis.h"
#include "svtkBrush.h"
#include "svtkContext2D.h"
#include "svtkContextMapper2D.h"
#include "svtkDoubleArray.h"
#include "svtkIdTypeArray.h"
#include "svtkLookupTable.h"
#include "svtkObjectFactory.h"
#include "svtkPen.h"
#include "svtkPlotLine.h"
#include "svtkPoints2D.h"
#include "svtkRect.h"
#include "svtkScalarsToColors.h"
#include "svtkTable.h"

//-----------------------------------------------------------------------------
svtkStandardNewMacro(svtkPlotFunctionalBag);

//-----------------------------------------------------------------------------
svtkPlotFunctionalBag::svtkPlotFunctionalBag()
{
  this->LookupTable = nullptr;
  this->TooltipDefaultLabelFormat = "%l (%x, %y)";
  this->LogX = false;
  this->LogY = false;
}

//-----------------------------------------------------------------------------
svtkPlotFunctionalBag::~svtkPlotFunctionalBag()
{
  if (this->LookupTable)
  {
    this->LookupTable->UnRegister(this);
  }
}

//-----------------------------------------------------------------------------
bool svtkPlotFunctionalBag::IsBag()
{
  this->Update();
  return (this->BagPoints->GetNumberOfPoints() > 0);
}

//-----------------------------------------------------------------------------
bool svtkPlotFunctionalBag::GetVisible()
{
  return this->Superclass::GetVisible() || this->GetSelection() != nullptr;
}

//-----------------------------------------------------------------------------
void svtkPlotFunctionalBag::Update()
{
  if (!this->GetVisible())
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
    (this->LookupTable && this->LookupTable->GetMTime() > this->BuildTime) ||
    this->MTime > this->BuildTime)
  {
    svtkDebugMacro(<< "Updating cached values.");
    this->UpdateTableCache(table);
  }
  else if ((this->XAxis->GetMTime() > this->BuildTime) ||
    (this->YAxis->GetMTime() > this->BuildTime))
  {
    if ((this->LogX != this->XAxis->GetLogScale()) || (this->LogY != this->YAxis->GetLogScale()))
    {
      this->UpdateTableCache(table);
    }
  }
}

//-----------------------------------------------------------------------------
bool svtkPlotFunctionalBag::UpdateTableCache(svtkTable* table)
{
  if (!this->LookupTable)
  {
    this->CreateDefaultLookupTable();
    this->LookupTable->SetRange(0, table->GetNumberOfColumns());
    this->LookupTable->Build();
  }

  this->BagPoints->Reset();

  svtkDataArray* array[2] = { nullptr, nullptr };
  if (!this->GetDataArrays(table, array))
  {
    this->BuildTime.Modified();
    return false;
  }

  if (array[1]->GetNumberOfComponents() == 1)
  {
    // The input array has one component, manage it as a line
    this->Line->SetInputData(table, array[0] ? array[0]->GetName() : "", array[1]->GetName());
    this->Line->SetUseIndexForXSeries(this->UseIndexForXSeries);
    this->Line->SetMarkerStyle(svtkPlotPoints::NONE);
    this->Line->SetPen(this->Pen);
    this->Line->SetBrush(this->Brush);
    this->Line->Update();
  }
  else if (array[1]->GetNumberOfComponents() == 2)
  {
    // The input array has 2 components, this must be a bag
    // with {miny,maxy} tuples
    svtkDoubleArray* darr = svtkArrayDownCast<svtkDoubleArray>(array[1]);

    this->LogX = this->XAxis->GetLogScaleActive();
    this->LogY = this->YAxis->GetLogScaleActive();
    bool xAbs = this->XAxis->GetUnscaledMinimum() < 0.;
    bool yAbs = this->YAxis->GetUnscaledMinimum() < 0.;
    if (darr)
    {
      svtkIdType nbRows = array[1]->GetNumberOfTuples();
      this->BagPoints->SetNumberOfPoints(2 * nbRows);
      for (svtkIdType i = 0; i < nbRows; i++)
      {
        double y[2];
        darr->GetTuple(i, y);

        double x = (!this->UseIndexForXSeries && array[0]) ? array[0]->GetVariantValue(i).ToDouble()
                                                           : static_cast<double>(i);
        if (this->LogX)
        {
          x = xAbs ? log10(fabs(x)) : log10(x);
        }

        if (this->LogY)
        {
          y[0] = yAbs ? log10(fabs(y[0])) : log10(y[0]);
          y[1] = yAbs ? log10(fabs(y[1])) : log10(y[1]);
        }

        this->BagPoints->SetPoint(2 * i, x, y[0]);
        this->BagPoints->SetPoint(2 * i + 1, x, y[1]);
      }
      this->BagPoints->Modified();
    }
  }

  this->BuildTime.Modified();

  return true;
}

//-----------------------------------------------------------------------------
bool svtkPlotFunctionalBag::GetDataArrays(svtkTable* table, svtkDataArray* array[2])
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
bool svtkPlotFunctionalBag::Paint(svtkContext2D* painter)
{
  // This is where everything should be drawn, or dispatched to other methods.
  svtkDebugMacro(<< "Paint event called in svtkPlotFunctionalBag.");

  if (!this->GetVisible())
  {
    return false;
  }

  svtkPen* pen = this->GetSelection() ? this->SelectionPen : this->Pen;

  if (this->IsBag())
  {
    double pwidth = pen->GetWidth();
    pen->SetWidth(0.);
    painter->ApplyPen(pen);
    unsigned char pcolor[4];
    pen->GetColor(pcolor);
    this->Brush->SetColor(pcolor);
    painter->ApplyBrush(this->Brush);
    painter->DrawQuadStrip(this->BagPoints);
    pen->SetWidth(pwidth);
  }
  else
  {
    this->Line->SetPen(pen);
    this->Line->Paint(painter);
  }

  return true;
}

//-----------------------------------------------------------------------------
bool svtkPlotFunctionalBag::PaintLegend(svtkContext2D* painter, const svtkRectf& rect, int index)
{
  if (this->BagPoints->GetNumberOfPoints() > 0)
  {
    svtkNew<svtkPen> blackPen;
    blackPen->SetWidth(1.0);
    blackPen->SetColor(0, 0, 0, 255);
    painter->ApplyPen(blackPen);
    painter->ApplyBrush(this->Brush);
    painter->DrawRect(rect[0], rect[1], rect[2], rect[3]);
  }
  else
  {
    this->Line->PaintLegend(painter, rect, index);
  }
  return true;
}

//-----------------------------------------------------------------------------
svtkIdType svtkPlotFunctionalBag::GetNearestPoint(
  const svtkVector2f& point, const svtkVector2f& tol, svtkVector2f* location, svtkIdType* segmentId)
{
#ifndef SVTK_LEGACY_REMOVE
  if (!this->LegacyRecursionFlag)
  {
    this->LegacyRecursionFlag = true;
    svtkIdType ret = this->GetNearestPoint(point, tol, location);
    this->LegacyRecursionFlag = false;
    if (ret != -1)
    {
      SVTK_LEGACY_REPLACED_BODY(svtkPlotFunctionalBag::GetNearestPoint(const svtkVector2f& point,
                                 const svtkVector2f& tol, svtkVector2f* location),
        "SVTK 9.0",
        svtkPlotFunctionalBag::GetNearestPoint(const svtkVector2f& point, const svtkVector2f& tol,
          svtkVector2f* location, svtkIdType* segmentId));
      return ret;
    }
  }
#endif // SVTK_LEGACY_REMOVE

  if (this->BagPoints->GetNumberOfPoints() == 0)
  {
    return this->Line->GetNearestPoint(point, tol, location, segmentId);
  }
  return -1;
}

//-----------------------------------------------------------------------------
bool svtkPlotFunctionalBag::SelectPoints(const svtkVector2f& min, const svtkVector2f& max)
{
  if (!this->IsBag())
  {
    return this->Line->SelectPoints(min, max);
  }
  return false;
}

//-----------------------------------------------------------------------------
bool svtkPlotFunctionalBag::SelectPointsInPolygon(const svtkContextPolygon& polygon)
{
  if (!this->IsBag())
  {
    return this->Line->SelectPointsInPolygon(polygon);
  }
  return false;
}

//-----------------------------------------------------------------------------
void svtkPlotFunctionalBag::GetBounds(double bounds[4])
{
  if (this->BagPoints->GetNumberOfPoints() > 0)
  {
    this->BagPoints->GetBounds(bounds);
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
  else
  {
    this->Line->GetBounds(bounds);
  }

  svtkDebugMacro(<< "Bounds: " << bounds[0] << "\t" << bounds[1] << "\t" << bounds[2] << "\t"
                << bounds[3]);
}

//-----------------------------------------------------------------------------
void svtkPlotFunctionalBag::GetUnscaledInputBounds(double bounds[4])
{
  if (this->BagPoints->GetNumberOfPoints() > 0)
  {
    this->BagPoints->GetBounds(bounds);
  }
  else
  {
    this->Line->GetUnscaledInputBounds(bounds);
  }

  svtkDebugMacro(<< "Bounds: " << bounds[0] << "\t" << bounds[1] << "\t" << bounds[2] << "\t"
                << bounds[3]);
}

//-----------------------------------------------------------------------------
void svtkPlotFunctionalBag::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//-----------------------------------------------------------------------------
void svtkPlotFunctionalBag::SetLookupTable(svtkScalarsToColors* lut)
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
svtkScalarsToColors* svtkPlotFunctionalBag::GetLookupTable()
{
  if (this->LookupTable == nullptr)
  {
    this->CreateDefaultLookupTable();
  }
  return this->LookupTable;
}

//-----------------------------------------------------------------------------
void svtkPlotFunctionalBag::CreateDefaultLookupTable()
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
