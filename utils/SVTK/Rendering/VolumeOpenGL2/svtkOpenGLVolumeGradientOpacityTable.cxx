/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkOpenGLVolumeGradientOpacityTable.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkOpenGLVolumeGradientOpacityTable.h"

#include "svtkObjectFactory.h"
#include "svtkOpenGLRenderWindow.h"
#include "svtkPiecewiseFunction.h"
#include "svtkTextureObject.h"

svtkStandardNewMacro(svtkOpenGLVolumeGradientOpacityTable);

// Update opacity transfer function texture.
//--------------------------------------------------------------------------
void svtkOpenGLVolumeGradientOpacityTable::InternalUpdate(svtkObject* func, int svtkNotUsed(blendMode),
  double svtkNotUsed(sampleDistance), double svtkNotUsed(unitDistance), int filterValue)
{
  svtkPiecewiseFunction* gradientOpacity = svtkPiecewiseFunction::SafeDownCast(func);
  if (!gradientOpacity)
  {
    return;
  }
  gradientOpacity->GetTable(
    0, (this->LastRange[1] - this->LastRange[0]) * 0.25, this->TextureWidth, this->Table);

  this->TextureObject->Create2DFromRaw(
    this->TextureWidth, 1, this->NumberOfColorComponents, SVTK_FLOAT, this->Table);

  this->TextureObject->SetWrapS(svtkTextureObject::ClampToEdge);
  this->TextureObject->SetMagnificationFilter(filterValue);
  this->TextureObject->SetMinificationFilter(filterValue);
  this->BuildTime.Modified();
}

//--------------------------------------------------------------------------
void svtkOpenGLVolumeGradientOpacityTable::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
