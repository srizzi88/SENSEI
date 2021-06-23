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

#include "svtkPlotLine3D.h"

#include "svtkContext2D.h"
#include "svtkContext3D.h"
#include "svtkPen.h"

#include "svtkObjectFactory.h"

//-----------------------------------------------------------------------------
svtkStandardNewMacro(svtkPlotLine3D);

//-----------------------------------------------------------------------------
svtkPlotLine3D::svtkPlotLine3D() = default;

//-----------------------------------------------------------------------------
svtkPlotLine3D::~svtkPlotLine3D() = default;

//-----------------------------------------------------------------------------
bool svtkPlotLine3D::Paint(svtkContext2D* painter)
{
  // This is where everything should be drawn, or dispatched to other methods.
  svtkDebugMacro(<< "Paint event called in svtkPlotLine3D.");

  if (!this->Visible || this->Points.empty())
  {
    return false;
  }

  // Get the 3D context.
  svtkContext3D* context = painter->GetContext3D();
  if (context == nullptr)
  {
    return false;
  }

  // Draw the line between the points
  context->ApplyPen(this->Pen);
  context->DrawPoly(this->Points[0].GetData(), static_cast<int>(this->Points.size()));

  return this->svtkPlotPoints3D::Paint(painter);
}

//-----------------------------------------------------------------------------
void svtkPlotLine3D::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
