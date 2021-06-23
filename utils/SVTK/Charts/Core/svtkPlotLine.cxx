/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPlotLine.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkPlotLine.h"

#include "svtkContext2D.h"
#include "svtkIdTypeArray.h"
#include "svtkPen.h"
#include "svtkPoints2D.h"
#include "svtkRect.h"

#include "svtkObjectFactory.h"

//-----------------------------------------------------------------------------
svtkStandardNewMacro(svtkPlotLine);

//-----------------------------------------------------------------------------
svtkPlotLine::svtkPlotLine()
{
  this->MarkerStyle = svtkPlotPoints::NONE;
  this->PolyLine = true;
}

//-----------------------------------------------------------------------------
svtkPlotLine::~svtkPlotLine() = default;

//-----------------------------------------------------------------------------
bool svtkPlotLine::Paint(svtkContext2D* painter)
{
  // This is where everything should be drawn, or dispatched to other methods.
  svtkDebugMacro(<< "Paint event called in svtkPlotLine.");

  if (!this->Visible || !this->Points)
  {
    return false;
  }

  // Draw the line between the points
  painter->ApplyPen(this->Pen);

  if (this->BadPoints && this->BadPoints->GetNumberOfTuples() > 0)
  {
    // draw lines skipping bad points
    float* points = static_cast<float*>(this->Points->GetVoidPointer(0));
    const int pointSize = 2;
    svtkIdType lastGood = 0;
    svtkIdType bpIdx = 0;
    svtkIdType lineIncrement = this->PolyLine ? 1 : 2;
    svtkIdType nPoints = this->Points->GetNumberOfPoints();
    svtkIdType nBadPoints = this->BadPoints->GetNumberOfTuples();

    while (lastGood < nPoints)
    {
      svtkIdType id =
        bpIdx < nBadPoints ? this->BadPoints->GetValue(bpIdx) : this->Points->GetNumberOfPoints();

      // With non polyline, we discard a line if any of its points are bad
      if (!this->PolyLine && id % 2 == 1)
      {
        id--;
      }

      // render from last good point to one before this bad point
      if (id - lastGood > 1)
      {
        int start = lastGood;
        int numberOfPoints = id - start;
        if (this->PolyLine)
        {
          painter->DrawPoly(points + pointSize * start, numberOfPoints);
        }
        else
        {
          painter->DrawLines(points + pointSize * start, numberOfPoints);
        }
      }
      lastGood = id + lineIncrement;
      bpIdx++;
    }
  }
  else
  {
    // draw lines between all points
    if (this->PolyLine)
    {
      painter->DrawPoly(this->Points);
    }
    else
    {
      painter->DrawLines(this->Points);
    }
  }

  return this->svtkPlotPoints::Paint(painter);
}

//-----------------------------------------------------------------------------
bool svtkPlotLine::PaintLegend(svtkContext2D* painter, const svtkRectf& rect, int)
{
  painter->ApplyPen(this->Pen);
  painter->DrawLine(rect[0], rect[1] + 0.5 * rect[3], rect[0] + rect[2], rect[1] + 0.5 * rect[3]);
  this->Superclass::PaintLegend(painter, rect, 0);
  return true;
}

//-----------------------------------------------------------------------------
void svtkPlotLine::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
