/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkTreeMapView.cxx

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

#include "svtkTreeMapView.h"

#include "svtkBoxLayoutStrategy.h"
#include "svtkDataRepresentation.h"
#include "svtkInteractorStyleTreeMapHover.h"
#include "svtkLabeledTreeMapDataMapper.h"
#include "svtkObjectFactory.h"
#include "svtkSliceAndDiceLayoutStrategy.h"
#include "svtkSmartPointer.h"
#include "svtkSquarifyLayoutStrategy.h"
#include "svtkTextProperty.h"
#include "svtkTreeMapToPolyData.h"

svtkStandardNewMacro(svtkTreeMapView);
//----------------------------------------------------------------------------
svtkTreeMapView::svtkTreeMapView()
{
  this->BoxLayout = svtkSmartPointer<svtkBoxLayoutStrategy>::New();
  this->SquarifyLayout = svtkSmartPointer<svtkSquarifyLayoutStrategy>::New();
  this->SliceAndDiceLayout = svtkSmartPointer<svtkSliceAndDiceLayoutStrategy>::New();

  this->SetLayoutStrategyToSquarify();
  svtkSmartPointer<svtkTreeMapToPolyData> poly = svtkSmartPointer<svtkTreeMapToPolyData>::New();
  this->SetAreaToPolyData(poly);
  this->SetUseRectangularCoordinates(true);
  svtkSmartPointer<svtkLabeledTreeMapDataMapper> mapper =
    svtkSmartPointer<svtkLabeledTreeMapDataMapper>::New();
  this->SetAreaLabelMapper(mapper);
}

//----------------------------------------------------------------------------
svtkTreeMapView::~svtkTreeMapView() = default;

//----------------------------------------------------------------------------
void svtkTreeMapView::SetLayoutStrategyToBox()
{
  this->SetLayoutStrategy("Box");
}

//----------------------------------------------------------------------------
void svtkTreeMapView::SetLayoutStrategyToSliceAndDice()
{
  this->SetLayoutStrategy("Slice And Dice");
}

//----------------------------------------------------------------------------
void svtkTreeMapView::SetLayoutStrategyToSquarify()
{
  this->SetLayoutStrategy("Squarify");
}

//----------------------------------------------------------------------------
void svtkTreeMapView::SetLayoutStrategy(svtkAreaLayoutStrategy* s)
{
  if (!svtkTreeMapLayoutStrategy::SafeDownCast(s))
  {
    svtkErrorMacro("Strategy must be a treemap layout strategy.");
    return;
  }
  this->Superclass::SetLayoutStrategy(s);
}

//----------------------------------------------------------------------------
void svtkTreeMapView::SetLayoutStrategy(const char* name)
{
  if (!strcmp(name, "Box"))
  {
    this->BoxLayout->SetShrinkPercentage(this->GetShrinkPercentage());
    this->SetLayoutStrategy(this->BoxLayout);
  }
  else if (!strcmp(name, "Slice And Dice"))
  {
    this->SliceAndDiceLayout->SetShrinkPercentage(this->GetShrinkPercentage());
    this->SetLayoutStrategy(this->SliceAndDiceLayout);
  }
  else if (!strcmp(name, "Squarify"))
  {
    this->SquarifyLayout->SetShrinkPercentage(this->GetShrinkPercentage());
    this->SetLayoutStrategy(this->SquarifyLayout);
  }
  else
  {
    svtkErrorMacro("Unknown layout name: " << name);
  }
}

//----------------------------------------------------------------------------
void svtkTreeMapView::SetFontSizeRange(const int maxSize, const int minSize, const int delta)
{
  svtkLabeledTreeMapDataMapper* mapper =
    svtkLabeledTreeMapDataMapper::SafeDownCast(this->GetAreaLabelMapper());
  if (mapper)
  {
    mapper->SetFontSizeRange(maxSize, minSize, delta);
  }
}

//----------------------------------------------------------------------------
void svtkTreeMapView::GetFontSizeRange(int range[3])
{
  svtkLabeledTreeMapDataMapper* mapper =
    svtkLabeledTreeMapDataMapper::SafeDownCast(this->GetAreaLabelMapper());
  if (mapper)
  {
    mapper->GetFontSizeRange(range);
  }
}

//----------------------------------------------------------------------------
void svtkTreeMapView::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
