/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkIcicleView.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/

#include "svtkIcicleView.h"

#include "svtkObjectFactory.h"
#include "svtkSmartPointer.h"
#include "svtkStackedTreeLayoutStrategy.h"
#include "svtkTreeMapToPolyData.h"

svtkStandardNewMacro(svtkIcicleView);
//----------------------------------------------------------------------------
svtkIcicleView::svtkIcicleView()
{
  svtkSmartPointer<svtkStackedTreeLayoutStrategy> strategy =
    svtkSmartPointer<svtkStackedTreeLayoutStrategy>::New();
  double shrink = this->GetShrinkPercentage();
  strategy->SetUseRectangularCoordinates(true);
  strategy->SetRootStartAngle(0.0);
  strategy->SetRootEndAngle(15.0);
  strategy->SetReverse(true);
  strategy->SetShrinkPercentage(shrink);
  this->SetLayoutStrategy(strategy);
  svtkSmartPointer<svtkTreeMapToPolyData> poly = svtkSmartPointer<svtkTreeMapToPolyData>::New();
  this->SetAreaToPolyData(poly);
  this->SetUseRectangularCoordinates(true);
}

//----------------------------------------------------------------------------
svtkIcicleView::~svtkIcicleView() = default;

//----------------------------------------------------------------------------
void svtkIcicleView::SetTopToBottom(bool reversed)
{
  svtkStackedTreeLayoutStrategy* st =
    svtkStackedTreeLayoutStrategy::SafeDownCast(this->GetLayoutStrategy());
  if (st)
  {
    st->SetReverse(reversed);
  }
}

//----------------------------------------------------------------------------
bool svtkIcicleView::GetTopToBottom()
{
  svtkStackedTreeLayoutStrategy* st =
    svtkStackedTreeLayoutStrategy::SafeDownCast(this->GetLayoutStrategy());
  if (st)
  {
    return st->GetReverse();
  }
  return false;
}

//----------------------------------------------------------------------------
void svtkIcicleView::SetRootWidth(double width)
{
  svtkStackedTreeLayoutStrategy* st =
    svtkStackedTreeLayoutStrategy::SafeDownCast(this->GetLayoutStrategy());
  if (st)
  {
    st->SetRootStartAngle(0.0);
    st->SetRootEndAngle(width);
  }
}

//----------------------------------------------------------------------------
double svtkIcicleView::GetRootWidth()
{
  svtkStackedTreeLayoutStrategy* st =
    svtkStackedTreeLayoutStrategy::SafeDownCast(this->GetLayoutStrategy());
  if (st)
  {
    return st->GetRootEndAngle();
  }
  return 0.0;
}

//----------------------------------------------------------------------------
void svtkIcicleView::SetLayerThickness(double thickness)
{
  svtkStackedTreeLayoutStrategy* st =
    svtkStackedTreeLayoutStrategy::SafeDownCast(this->GetLayoutStrategy());
  if (st)
  {
    st->SetRingThickness(thickness);
  }
}

//----------------------------------------------------------------------------
double svtkIcicleView::GetLayerThickness()
{
  svtkStackedTreeLayoutStrategy* st =
    svtkStackedTreeLayoutStrategy::SafeDownCast(this->GetLayoutStrategy());
  if (st)
  {
    return st->GetRingThickness();
  }
  return 0.0;
}

//----------------------------------------------------------------------------
void svtkIcicleView::SetUseGradientColoring(bool value)
{
  svtkTreeMapToPolyData* tm = svtkTreeMapToPolyData::SafeDownCast(this->GetAreaToPolyData());
  if (tm)
  {
    tm->SetAddNormals(value);
  }
}

//----------------------------------------------------------------------------
bool svtkIcicleView::GetUseGradientColoring()
{
  svtkTreeMapToPolyData* tm = svtkTreeMapToPolyData::SafeDownCast(this->GetAreaToPolyData());
  if (tm)
  {
    return tm->GetAddNormals();
  }
  return false;
}

//----------------------------------------------------------------------------
void svtkIcicleView::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
