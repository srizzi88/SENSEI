/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkChart2DHistogram.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkChartHistogram2D.h"

#include "svtkAxis.h"
#include "svtkBrush.h"
#include "svtkColorLegend.h"
#include "svtkContext2D.h"
#include "svtkContextMouseEvent.h"
#include "svtkContextScene.h"
#include "svtkObjectFactory.h"
#include "svtkPen.h"
#include "svtkPlotHistogram2D.h"
#include "svtkSmartPointer.h"
#include "svtkTextProperty.h"
#include "svtkTooltipItem.h"

//-----------------------------------------------------------------------------
svtkStandardNewMacro(svtkChartHistogram2D);

//-----------------------------------------------------------------------------
svtkChartHistogram2D::svtkChartHistogram2D()
{
  // Now for the 2D histogram
  this->Histogram = svtkSmartPointer<svtkPlotHistogram2D>::New();
  this->AddPlot(this->Histogram);

  this->RemoveItem(this->Legend);
  this->Legend = svtkSmartPointer<svtkColorLegend>::New();
  this->AddItem(this->Legend);

  // Re-add tooltip, making it the last ContextItem to be painted
  this->RemoveItem(this->Tooltip);
  this->AddItem(this->Tooltip);
}

//-----------------------------------------------------------------------------
svtkChartHistogram2D::~svtkChartHistogram2D() = default;

//-----------------------------------------------------------------------------
void svtkChartHistogram2D::Update()
{
  this->Histogram->Update();
  this->Legend->Update();
  this->svtkChartXY::Update();
}

//-----------------------------------------------------------------------------
void svtkChartHistogram2D::SetInputData(svtkImageData* data, svtkIdType z)
{
  this->Histogram->SetInputData(data, z);
}

//-----------------------------------------------------------------------------
void svtkChartHistogram2D::SetTransferFunction(svtkScalarsToColors* function)
{
  this->Histogram->SetTransferFunction(function);
  svtkColorLegend* legend = svtkColorLegend::SafeDownCast(this->Legend);
  if (legend)
  {
    legend->SetTransferFunction(function);
  }
}

//-----------------------------------------------------------------------------
bool svtkChartHistogram2D::UpdateLayout(svtkContext2D* painter)
{
  this->svtkChartXY::UpdateLayout(painter);
  svtkColorLegend* legend = svtkColorLegend::SafeDownCast(this->Legend);
  if (legend)
  {
    legend->SetPosition(svtkRectf(this->Point2[0] + 5, this->Point1[1],
      this->Legend->GetSymbolWidth(), this->Point2[1] - this->Point1[1]));
  }
  this->Legend->Update();
  return true;
}

//-----------------------------------------------------------------------------
bool svtkChartHistogram2D::Hit(const svtkContextMouseEvent& mouse)
{
  svtkVector2i pos(mouse.GetScreenPos());
  if (pos[0] > this->Point1[0] - 10 && pos[0] < this->Point2[0] + 10 && pos[1] > this->Point1[1] &&
    pos[1] < this->Point2[1])
  {
    return true;
  }
  else
  {
    return false;
  }
}

//-----------------------------------------------------------------------------
svtkPlot* svtkChartHistogram2D::GetPlot(svtkIdType index)
{
  if (index == 0)
  {
    return this->Histogram;
  }

  return nullptr;
}

//-----------------------------------------------------------------------------
void svtkChartHistogram2D::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
