/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkScalarBarRepresentation.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/*
 * Copyright 2008 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

#include "svtkScalarBarRepresentation.h"

#include "svtkObjectFactory.h"
#include "svtkPropCollection.h"
#include "svtkScalarBarActor.h"
#include "svtkSmartPointer.h"
#include "svtkTextProperty.h"

#include <algorithm>

//=============================================================================
svtkStandardNewMacro(svtkScalarBarRepresentation);
//-----------------------------------------------------------------------------
svtkScalarBarRepresentation::svtkScalarBarRepresentation()
{
  this->PositionCoordinate->SetValue(0.82, 0.1);
  this->Position2Coordinate->SetValue(0.17, 0.8);

  this->AutoOrient = true;

  this->ScalarBarActor = nullptr;
  svtkScalarBarActor* actor = svtkScalarBarActor::New();
  this->SetScalarBarActor(actor);
  actor->Delete();

  this->SetShowBorder(svtkBorderRepresentation::BORDER_ACTIVE);
}

//-----------------------------------------------------------------------------
svtkScalarBarRepresentation::~svtkScalarBarRepresentation()
{
  this->SetScalarBarActor(nullptr);
}

//-----------------------------------------------------------------------------
void svtkScalarBarRepresentation::SetScalarBarActor(svtkScalarBarActor* actor)
{
  if (this->ScalarBarActor != actor)
  {
    svtkSmartPointer<svtkScalarBarActor> oldActor = this->ScalarBarActor;
    svtkSetObjectBodyMacro(ScalarBarActor, svtkScalarBarActor, actor);
    if (actor && oldActor)
    {
      actor->SetOrientation(oldActor->GetOrientation());
      if (actor->GetOrientation())
      {
        this->ShowHorizontalBorder = 2;
        this->ShowVerticalBorder = 0;
      }
      else
      {
        this->ShowHorizontalBorder = 0;
        this->ShowVerticalBorder = 2;
      }
      this->UpdateShowBorder();
    }
  }
}

//-----------------------------------------------------------------------------
void svtkScalarBarRepresentation::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "ScalarBarActor: " << this->ScalarBarActor << endl;
}

//-----------------------------------------------------------------------------
void svtkScalarBarRepresentation::SetOrientation(int orientation)
{
  if (this->ScalarBarActor && this->ScalarBarActor->GetOrientation() != orientation)
  {
    this->SwapOrientation();
  }
}

//-----------------------------------------------------------------------------
int svtkScalarBarRepresentation::GetOrientation()
{
  if (this->ScalarBarActor)
  {
    return this->ScalarBarActor->GetOrientation();
  }
  svtkErrorMacro("No scalar bar");
  return 0;
}

//-----------------------------------------------------------------------------
void svtkScalarBarRepresentation::BuildRepresentation()
{
  if (this->ScalarBarActor)
  {
    this->ScalarBarActor->SetPosition(this->GetPosition());
    this->ScalarBarActor->SetPosition2(this->GetPosition2());
  }

  this->Superclass::BuildRepresentation();
}

//-----------------------------------------------------------------------------
void svtkScalarBarRepresentation::WidgetInteraction(double eventPos[2])
{
  // Let superclass move things around.
  this->Superclass::WidgetInteraction(eventPos);

  // Check to see if we need to change the orientation.
  if (this->Moving && this->AutoOrient)
  {
    double* fpos1 = this->PositionCoordinate->GetValue();
    double* fpos2 = this->Position2Coordinate->GetValue();
    double center[2];
    center[0] = fpos1[0] + 0.5 * fpos2[0];
    center[1] = fpos1[1] + 0.5 * fpos2[1];

    if (fabs(center[0] - 0.5) > 0.2 + fabs(center[1] - 0.5))
    {
      // Close enough to left/right to be swapped to vertical
      if (this->ScalarBarActor->GetOrientation() == SVTK_ORIENT_HORIZONTAL)
      {
        this->SwapOrientation();
      }
    }
    else if (fabs(center[1] - 0.5) > 0.2 + fabs(center[0] - 0.5))
    {
      // Close enough to left/right to be swapped to horizontal
      if (this->ScalarBarActor->GetOrientation() == SVTK_ORIENT_VERTICAL)
      {
        this->SwapOrientation();
      }
    }
  } // if this->AutoOrient
}

//-----------------------------------------------------------------------------
void svtkScalarBarRepresentation::SwapOrientation()
{
  double* fpos1 = this->PositionCoordinate->GetValue();
  double* fpos2 = this->Position2Coordinate->GetValue();
  double par1[2];
  double par2[2];
  par1[0] = fpos1[0];
  par1[1] = fpos1[1];
  par2[0] = fpos1[0] + fpos2[0];
  par2[1] = fpos1[1] + fpos2[1];
  double center[2];
  center[0] = fpos1[0] + 0.5 * fpos2[0];
  center[1] = fpos1[1] + 0.5 * fpos2[1];

  // Change the corners to effectively rotate 90 degrees.
  par2[0] = center[0] + center[1] - par1[1];
  par2[1] = center[1] + center[0] - par1[0];
  par1[0] = 2 * center[0] - par2[0];
  par1[1] = 2 * center[1] - par2[1];

  if (this->ScalarBarActor->GetOrientation() == SVTK_ORIENT_HORIZONTAL)
  {
    this->ScalarBarActor->SetOrientation(SVTK_ORIENT_VERTICAL);
  }
  else
  {
    this->ScalarBarActor->SetOrientation(SVTK_ORIENT_HORIZONTAL);
  }

  this->PositionCoordinate->SetValue(par1[0], par1[1]);
  this->Position2Coordinate->SetValue(par2[0] - par1[0], par2[1] - par1[1]);

  std::swap(this->ShowHorizontalBorder, this->ShowVerticalBorder);

  this->Modified();
  this->UpdateShowBorder();
  this->BuildRepresentation();
}

//-----------------------------------------------------------------------------
svtkTypeBool svtkScalarBarRepresentation::GetVisibility()
{
  return this->ScalarBarActor->GetVisibility();
}

//-----------------------------------------------------------------------------
void svtkScalarBarRepresentation::SetVisibility(svtkTypeBool vis)
{
  this->ScalarBarActor->SetVisibility(vis);
  this->Superclass::SetVisibility(vis);
}

//-----------------------------------------------------------------------------
void svtkScalarBarRepresentation::GetActors2D(svtkPropCollection* collection)
{
  if (this->ScalarBarActor)
  {
    collection->AddItem(this->ScalarBarActor);
  }
  this->Superclass::GetActors2D(collection);
}

//-----------------------------------------------------------------------------
void svtkScalarBarRepresentation::ReleaseGraphicsResources(svtkWindow* w)
{
  if (this->ScalarBarActor)
  {
    this->ScalarBarActor->ReleaseGraphicsResources(w);
  }
  this->Superclass::ReleaseGraphicsResources(w);
}

//-------------------------------------------------------------------------
int svtkScalarBarRepresentation::RenderOverlay(svtkViewport* w)
{
  int count = this->Superclass::RenderOverlay(w);
  if (this->ScalarBarActor)
  {
    count += this->ScalarBarActor->RenderOverlay(w);
  }
  return count;
}

//-------------------------------------------------------------------------
int svtkScalarBarRepresentation::RenderOpaqueGeometry(svtkViewport* w)
{
  int count = this->Superclass::RenderOpaqueGeometry(w);
  if (this->ScalarBarActor)
  {
    count += this->ScalarBarActor->RenderOpaqueGeometry(w);
  }
  return count;
}

//-------------------------------------------------------------------------
int svtkScalarBarRepresentation::RenderTranslucentPolygonalGeometry(svtkViewport* w)
{
  int count = this->Superclass::RenderTranslucentPolygonalGeometry(w);
  if (this->ScalarBarActor)
  {
    count += this->ScalarBarActor->RenderTranslucentPolygonalGeometry(w);
  }
  return count;
}

//-------------------------------------------------------------------------
svtkTypeBool svtkScalarBarRepresentation::HasTranslucentPolygonalGeometry()
{
  int result = this->Superclass::HasTranslucentPolygonalGeometry();
  if (this->ScalarBarActor)
  {
    result |= this->ScalarBarActor->HasTranslucentPolygonalGeometry();
  }
  return result;
}
