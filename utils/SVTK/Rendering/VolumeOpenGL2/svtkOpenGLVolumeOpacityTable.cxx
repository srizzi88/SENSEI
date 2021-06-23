/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkOpenGLVolumeOpacityTable.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkOpenGLVolumeOpacityTable.h"

#include "svtkObjectFactory.h"
#include "svtkOpenGLRenderWindow.h"
#include "svtkPiecewiseFunction.h"
#include "svtkTextureObject.h"

svtkStandardNewMacro(svtkOpenGLVolumeOpacityTable);

// Update opacity transfer function texture.
//--------------------------------------------------------------------------
void svtkOpenGLVolumeOpacityTable::InternalUpdate(
  svtkObject* func, int blendMode, double sampleDistance, double unitDistance, int filterValue)
{
  svtkPiecewiseFunction* scalarOpacity = svtkPiecewiseFunction::SafeDownCast(func);
  if (!scalarOpacity)
  {
    return;
  }

  scalarOpacity->GetTable(this->LastRange[0], this->LastRange[1], this->TextureWidth, this->Table);

  // Correct the opacity array for the spacing between the planes if we
  // are using a composite blending operation
  // TODO Fix this code for sample distance in three dimensions
  if (this->LastBlendMode == svtkVolumeMapper::COMPOSITE_BLEND)
  {
    float* ptr = this->Table;
    double factor = sampleDistance / unitDistance;
    int i = 0;
    while (i < this->TextureWidth)
    {
      if (*ptr > 0.0001f)
      {
        *ptr = static_cast<float>(1.0 - pow(1.0 - static_cast<double>(*ptr), factor));
      }
      ++ptr;
      ++i;
    }
  }
  else if (blendMode == svtkVolumeMapper::ADDITIVE_BLEND)
  {
    float* ptr = this->Table;
    double factor = sampleDistance / unitDistance;
    int i = 0;
    while (i < this->TextureWidth)
    {
      if (*ptr > 0.0001f)
      {
        *ptr = static_cast<float>(static_cast<double>(*ptr) * factor);
      }
      ++ptr;
      ++i;
    }
  }

  this->TextureObject->SetWrapS(svtkTextureObject::ClampToEdge);
  this->TextureObject->SetMagnificationFilter(filterValue);
  this->TextureObject->SetMinificationFilter(filterValue);
  this->TextureObject->Create2DFromRaw(
    this->TextureWidth, 1, this->NumberOfColorComponents, SVTK_FLOAT, this->Table);
}

//-----------------------------------------------------------------------------
bool svtkOpenGLVolumeOpacityTable::NeedsUpdate(
  svtkObject* func, double scalarRange[2], int blendMode, double sampleDistance)
{
  if (this->Superclass::NeedsUpdate(func, scalarRange, blendMode, sampleDistance) ||
    this->LastBlendMode != blendMode || this->LastSampleDistance != sampleDistance)
  {
    this->LastBlendMode = blendMode;
    this->LastSampleDistance = sampleDistance;
    return true;
  }
  return false;
}

//-----------------------------------------------------------------------------
void svtkOpenGLVolumeOpacityTable::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Last Blend Mode: " << this->LastBlendMode << endl;
  os << indent << "Last Sample Distance: " << this->LastSampleDistance << endl;
}
