/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPlotPoints3D.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkPlotPoints3D.h"
#include "svtkChartXYZ.h"
#include "svtkContext2D.h"
#include "svtkContext3D.h"
#include "svtkIdTypeArray.h"
#include "svtkNew.h"
#include "svtkObjectFactory.h"
#include "svtkPen.h"
#include "svtkUnsignedCharArray.h"

//-----------------------------------------------------------------------------
svtkStandardNewMacro(svtkPlotPoints3D);

//-----------------------------------------------------------------------------
svtkPlotPoints3D::svtkPlotPoints3D()
{
  this->Pen->SetWidth(5);
  this->Pen->SetColor(0, 0, 0, 255);
  this->SelectionPen->SetWidth(7);
}

//-----------------------------------------------------------------------------
svtkPlotPoints3D::~svtkPlotPoints3D() = default;

//-----------------------------------------------------------------------------
void svtkPlotPoints3D::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//-----------------------------------------------------------------------------
bool svtkPlotPoints3D::Paint(svtkContext2D* painter)
{
  if (!this->Visible || this->Points.empty())
  {
    return false;
  }

  // Get the 3D context.
  svtkContext3D* context = painter->GetContext3D();

  if (!context)
  {
    return false;
  }

  this->Update();

  if (!this->Points.empty())
  {

    // Draw the points in 3d.
    context->ApplyPen(this->Pen);
    if (this->NumberOfComponents == 0)
    {
      context->DrawPoints(this->Points[0].GetData(), static_cast<int>(this->Points.size()));
    }
    else
    {
      context->DrawPoints(this->Points[0].GetData(), static_cast<int>(this->Points.size()),
        this->Colors->GetPointer(0), this->NumberOfComponents);
    }
  }

  // Now add some decorations for our selected points...
  if (this->Selection && this->Selection->GetNumberOfTuples())
  {
    if (this->Selection->GetMTime() > this->SelectedPointsBuildTime ||
      this->GetMTime() > this->SelectedPointsBuildTime)
    {
      size_t nSelected(static_cast<size_t>(this->Selection->GetNumberOfTuples()));
      this->SelectedPoints.reserve(nSelected);
      for (size_t i = 0; i < nSelected; ++i)
      {
        this->SelectedPoints.push_back(
          this->Points[this->Selection->GetValue(static_cast<svtkIdType>(i))]);
      }
      this->SelectedPointsBuildTime.Modified();
    }

    // Now to render the selected points.
    if (!this->SelectedPoints.empty())
    {
      context->ApplyPen(this->SelectionPen);
      context->DrawPoints(
        this->SelectedPoints[0].GetData(), static_cast<int>(this->SelectedPoints.size()));
    }
  }

  return true;
}
