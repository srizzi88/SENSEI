/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkTreeRingView.cxx

  -------------------------------------------------------------------------
    Copyright 2008 Sandia Corporation.
    Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
    the U.S. Government retains certain rights in this software.
  -------------------------------------------------------------------------

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkTreeRingView.h"

#include "svtkObjectFactory.h"
#include "svtkRenderedTreeAreaRepresentation.h"
#include "svtkStackedTreeLayoutStrategy.h"
#include "svtkTreeRingToPolyData.h"

svtkStandardNewMacro(svtkTreeRingView);
//----------------------------------------------------------------------------
svtkTreeRingView::svtkTreeRingView() = default;

//----------------------------------------------------------------------------
svtkTreeRingView::~svtkTreeRingView() = default;

//----------------------------------------------------------------------------
void svtkTreeRingView::SetRootAngles(double start, double end)
{
  svtkStackedTreeLayoutStrategy* s =
    svtkStackedTreeLayoutStrategy::SafeDownCast(this->GetLayoutStrategy());
  if (s)
  {
    s->SetRootStartAngle(start);
    s->SetRootEndAngle(end);
  }
}

//----------------------------------------------------------------------------
void svtkTreeRingView::SetRootAtCenter(bool center)
{
  svtkStackedTreeLayoutStrategy* st =
    svtkStackedTreeLayoutStrategy::SafeDownCast(this->GetLayoutStrategy());
  if (st)
  {
    st->SetReverse(!center);
  }
}

//----------------------------------------------------------------------------
bool svtkTreeRingView::GetRootAtCenter()
{
  svtkStackedTreeLayoutStrategy* st =
    svtkStackedTreeLayoutStrategy::SafeDownCast(this->GetLayoutStrategy());
  if (st)
  {
    return !st->GetReverse();
  }
  return false;
}

//----------------------------------------------------------------------------
void svtkTreeRingView::SetLayerThickness(double thickness)
{
  svtkStackedTreeLayoutStrategy* st =
    svtkStackedTreeLayoutStrategy::SafeDownCast(this->GetLayoutStrategy());
  if (st)
  {
    st->SetRingThickness(thickness);
  }
}

//----------------------------------------------------------------------------
double svtkTreeRingView::GetLayerThickness()
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
void svtkTreeRingView::SetInteriorRadius(double rad)
{
  svtkStackedTreeLayoutStrategy* st =
    svtkStackedTreeLayoutStrategy::SafeDownCast(this->GetLayoutStrategy());
  if (st)
  {
    st->SetInteriorRadius(rad);
  }
}

//----------------------------------------------------------------------------
double svtkTreeRingView::GetInteriorRadius()
{
  svtkStackedTreeLayoutStrategy* st =
    svtkStackedTreeLayoutStrategy::SafeDownCast(this->GetLayoutStrategy());
  if (st)
  {
    return st->GetInteriorRadius();
  }
  return 0.0;
}

//----------------------------------------------------------------------------
void svtkTreeRingView::SetInteriorLogSpacingValue(double value)
{
  svtkStackedTreeLayoutStrategy* st =
    svtkStackedTreeLayoutStrategy::SafeDownCast(this->GetLayoutStrategy());
  if (st)
  {
    st->SetInteriorLogSpacingValue(value);
  }
}

//----------------------------------------------------------------------------
double svtkTreeRingView::GetInteriorLogSpacingValue()
{
  svtkStackedTreeLayoutStrategy* st =
    svtkStackedTreeLayoutStrategy::SafeDownCast(this->GetLayoutStrategy());
  if (st)
  {
    return st->GetInteriorLogSpacingValue();
  }
  return 0.0;
}

//----------------------------------------------------------------------------
void svtkTreeRingView::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
