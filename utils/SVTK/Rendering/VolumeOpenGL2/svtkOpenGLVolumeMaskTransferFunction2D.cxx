/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkOpenGLVolumeMaskTransferFunction2D.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkOpenGLVolumeMaskTransferFunction2D.h"

#include "svtkColorTransferFunction.h"
#include "svtkObjectFactory.h"
#include "svtkOpenGLRenderWindow.h"
#include "svtkPiecewiseFunction.h"
#include "svtkTextureObject.h"
#include "svtkVolumeProperty.h"

#include <algorithm>

svtkStandardNewMacro(svtkOpenGLVolumeMaskTransferFunction2D);

//----------------------------------------------------------------------------
svtkOpenGLVolumeMaskTransferFunction2D::svtkOpenGLVolumeMaskTransferFunction2D()
{
  this->NumberOfColorComponents = 4;
}

//----------------------------------------------------------------------------
void svtkOpenGLVolumeMaskTransferFunction2D::InternalUpdate(svtkObject* func,
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

  std::fill(this->Table, this->Table + this->TextureWidth * 4, 0.0f);
  for (auto i = 1; i < this->TextureHeight; ++i)
  {
    float* tmpColor = new float[this->TextureWidth * 3];
    std::fill(tmpColor, tmpColor + this->TextureWidth * 3, 1.0f);
    svtkColorTransferFunction* color = prop->GetLabelColor(i);
    if (!color)
    {
      // If no color function is provided for this label, just use the default
      // color transfer function (i.e. label 0)
      color = prop->GetRGBTransferFunction();
    }
    if (color)
    {
      color->GetTable(this->LastRange[0], this->LastRange[1], this->TextureWidth, tmpColor);
    }
    float* tmpOpacity = new float[this->TextureWidth];
    std::fill(tmpOpacity, tmpOpacity + this->TextureWidth, 1.0f);
    svtkPiecewiseFunction* opacity = prop->GetLabelScalarOpacity(i);
    if (!opacity)
    {
      // If no opacity function is provided for this label, just use the default
      // scalar opacity function (i.e. label 0)
      opacity = prop->GetScalarOpacity();
    }
    if (opacity)
    {
      opacity->GetTable(this->LastRange[0], this->LastRange[1], this->TextureWidth, tmpOpacity);
    }
    float* tmpTable = new float[this->TextureWidth * 4];
    float* tmpColorPtr = tmpColor;
    float* tmpOpacityPtr = tmpOpacity;
    float* tmpTablePtr = tmpTable;
    for (int j = 0; j < this->TextureWidth; ++j)
    {
      for (int k = 0; k < 3; ++k)
      {
        *tmpTablePtr++ = *tmpColorPtr++;
      }
      *tmpTablePtr++ = *tmpOpacityPtr++;
    }
    float* tablePtr = this->Table;
    tablePtr += i * this->TextureWidth * 4;
    memcpy(tablePtr, tmpTable, this->TextureWidth * 4 * sizeof(float));
    delete[] tmpColor;
    delete[] tmpOpacity;
    delete[] tmpTable;
  }
  this->TextureObject->SetWrapS(svtkTextureObject::ClampToEdge);
  this->TextureObject->SetWrapT(svtkTextureObject::ClampToEdge);
  this->TextureObject->SetMagnificationFilter(filterValue);
  this->TextureObject->SetMinificationFilter(filterValue);
  this->TextureObject->Create2DFromRaw(
    this->TextureWidth, this->TextureHeight, this->NumberOfColorComponents, SVTK_FLOAT, this->Table);
}

//----------------------------------------------------------------------------
void svtkOpenGLVolumeMaskTransferFunction2D::ComputeIdealTextureSize(
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
void svtkOpenGLVolumeMaskTransferFunction2D::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
