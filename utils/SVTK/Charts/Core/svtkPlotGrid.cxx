/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPlotGrid.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkPlotGrid.h"

#include "svtkAxis.h"
#include "svtkContext2D.h"
#include "svtkFloatArray.h"
#include "svtkPen.h"
#include "svtkPoints2D.h"
#include "svtkVector.h"

#include "svtkObjectFactory.h"

//-----------------------------------------------------------------------------
svtkCxxSetObjectMacro(svtkPlotGrid, XAxis, svtkAxis);
svtkCxxSetObjectMacro(svtkPlotGrid, YAxis, svtkAxis);
//-----------------------------------------------------------------------------
svtkStandardNewMacro(svtkPlotGrid);

//-----------------------------------------------------------------------------
svtkPlotGrid::svtkPlotGrid()
{
  this->XAxis = nullptr;
  this->YAxis = nullptr;
}

//-----------------------------------------------------------------------------
svtkPlotGrid::~svtkPlotGrid()
{
  this->SetXAxis(nullptr);
  this->SetYAxis(nullptr);
}

//-----------------------------------------------------------------------------
bool svtkPlotGrid::Paint(svtkContext2D* painter)
{
  if (!this->XAxis || !this->YAxis)
  {
    // Need axes to define where our grid lines should be drawn
    svtkDebugMacro(<< "No axes set and so grid lines cannot be drawn.");
    return false;
  }

  svtkVector2f x1, x2, y1, y2;
  this->XAxis->GetPoint1(x1.GetData());
  this->XAxis->GetPoint2(x2.GetData());
  this->YAxis->GetPoint1(y1.GetData());
  this->YAxis->GetPoint2(y2.GetData());

  // in x
  if (this->XAxis->GetVisible() && this->XAxis->GetGridVisible())
  {
    svtkFloatArray* xLines = this->XAxis->GetTickScenePositions();
    painter->ApplyPen(this->XAxis->GetGridPen());
    float* xPositions = xLines->GetPointer(0);
    for (int i = 0; i < xLines->GetNumberOfTuples(); ++i)
    {
      painter->DrawLine(xPositions[i], y1.GetY(), xPositions[i], y2.GetY());
    }
  }

  // in y
  if (this->YAxis->GetVisible() && this->YAxis->GetGridVisible())
  {
    svtkFloatArray* yLines = this->YAxis->GetTickScenePositions();
    painter->ApplyPen(this->YAxis->GetGridPen());
    float* yPositions = yLines->GetPointer(0);
    for (int i = 0; i < yLines->GetNumberOfTuples(); ++i)
    {
      painter->DrawLine(x1.GetX(), yPositions[i], x2.GetX(), yPositions[i]);
    }
  }

  return true;
}

//-----------------------------------------------------------------------------
void svtkPlotGrid::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
