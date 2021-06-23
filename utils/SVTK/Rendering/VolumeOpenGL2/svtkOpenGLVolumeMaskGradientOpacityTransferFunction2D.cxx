/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkOpenGLVolumeMaskGradientOpacityTransferFunction2D.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkOpenGLVolumeMaskGradientOpacityTransferFunction2D.h"

#include "svtkObjectFactory.h"
#include "svtkOpenGLRenderWindow.h"
#include "svtkPiecewiseFunction.h"
#include "svtkTextureObject.h"
#include "svtkVolumeProperty.h"

#include <algorithm>

svtkStandardNewMacro(svtkOpenGLVolumeMaskGradientOpacityTransferFunction2D);

//----------------------------------------------------------------------------
svtkOpenGLVolumeMaskGradientOpacityTransferFunction2D::
  svtkOpenGLVolumeMaskGradientOpacityTransferFunction2D()
{
  this->NumberOfColorComponents = 1;
}

//----------------------------------------------------------------------------
void svtkOpenGLVolumeMaskGradientOpacityTransferFunction2D::InternalUpdate(svtkObject* func,
  int svtkNotUsed(blendMode), double svtkNotUsed(sampleDistance), double svtkNotUsed(unitDistance),
  int filterValue)
{
  if (!func)
  {
    return;
  }
  svtkVolumeProperty* prop = svtkVolumeProperty::SafeDownCast(func);
  if (!prop)
  {
    return;
  }

  std::set<int> labels = prop->GetLabelMapLabels();
  std::fill(this->Table, this->Table + this->TextureWidth * 1, 0.0f);
  for (auto i = 1; i < this->TextureHeight; ++i)
  {
    float* tmpGradOp = new float[this->TextureWidth];
    std::fill(tmpGradOp, tmpGradOp + this->TextureWidth, 1.0f);
    svtkPiecewiseFunction* gradOp = prop->GetLabelGradientOpacity(i);
    if (gradOp)
    {
      gradOp->GetTable(
        0, (this->LastRange[1] - this->LastRange[0]) * 0.25, this->TextureWidth, tmpGradOp);
    }

    float* tablePtr = this->Table;
    tablePtr += i * this->TextureWidth;
    memcpy(tablePtr, tmpGradOp, this->TextureWidth * sizeof(float));
    delete[] tmpGradOp;
  }
  this->TextureObject->SetWrapS(svtkTextureObject::ClampToEdge);
  this->TextureObject->SetWrapT(svtkTextureObject::ClampToEdge);
  this->TextureObject->SetMagnificationFilter(filterValue);
  this->TextureObject->SetMinificationFilter(filterValue);
  this->TextureObject->Create2DFromRaw(
    this->TextureWidth, this->TextureHeight, this->NumberOfColorComponents, SVTK_FLOAT, this->Table);
}

//----------------------------------------------------------------------------
void svtkOpenGLVolumeMaskGradientOpacityTransferFunction2D::ComputeIdealTextureSize(
  svtkObject* func, int& width, int& height, svtkOpenGLRenderWindow* svtkNotUsed(renWin))
{
  svtkVolumeProperty* prop = svtkVolumeProperty::SafeDownCast(func);
  if (!prop)
  {
    return;
  }
  width = 1024;
  // Set the height to one more than the max label value. The extra row will be
  // for the special label 0 that represents un-masked values. It is also
  // necessary to ensure that the shader indexing is correct.
  std::set<int> const labels = prop->GetLabelMapLabels();
  height = labels.empty() ? 1 : *(labels.crbegin()) + 1;
}

//----------------------------------------------------------------------------
void svtkOpenGLVolumeMaskGradientOpacityTransferFunction2D::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
