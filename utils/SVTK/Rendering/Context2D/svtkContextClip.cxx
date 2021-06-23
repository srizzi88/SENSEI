/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkContextClip.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkContextClip.h"
#include "svtkContext2D.h"
#include "svtkContextDevice2D.h"
#include "svtkContextScenePrivate.h"
#include "svtkObjectFactory.h"
#include "svtkTransform2D.h"
#include "svtkVector.h"

#include <cassert>

svtkStandardNewMacro(svtkContextClip);

//-----------------------------------------------------------------------------
svtkContextClip::svtkContextClip()
{
  this->Dims[0] = 0.0;
  this->Dims[1] = 0.0;
  this->Dims[2] = 100.0;
  this->Dims[3] = 100.0;
}

//-----------------------------------------------------------------------------
svtkContextClip::~svtkContextClip() = default;

//-----------------------------------------------------------------------------
bool svtkContextClip::Paint(svtkContext2D* painter)
{
  // Clip rendering for all child items.
  // Check whether the scene has a transform - use it if so
  float* clipBy = this->Dims;

  int clipi[] = { svtkContext2D::FloatToInt(clipBy[0]), svtkContext2D::FloatToInt(clipBy[1]),
    svtkContext2D::FloatToInt(clipBy[2]), svtkContext2D::FloatToInt(clipBy[3]) };

  painter->GetDevice()->SetClipping(clipi);
  painter->GetDevice()->EnableClipping(true);
  bool result = this->PaintChildren(painter);
  painter->GetDevice()->EnableClipping(false);
  return result;
}

//-----------------------------------------------------------------------------
void svtkContextClip::Update() {}

//-----------------------------------------------------------------------------
void svtkContextClip::SetClip(float x, float y, float width, float height)
{
  this->Dims[0] = x;
  this->Dims[1] = y;
  this->Dims[2] = width;
  this->Dims[3] = height;
  assert(width >= 0 && height >= 0);
}

//-----------------------------------------------------------------------------
void svtkContextClip::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
