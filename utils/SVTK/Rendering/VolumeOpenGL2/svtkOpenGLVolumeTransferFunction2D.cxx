/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkOpenGLVolumeTransferFunction2D.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkOpenGLVolumeTransferFunction2D.h"

#include "svtkDataArray.h"
#include "svtkImageData.h"
#include "svtkImageResize.h"
#include "svtkObjectFactory.h"
#include "svtkOpenGLRenderWindow.h"
#include "svtkPointData.h"
#include "svtkTextureObject.h"

svtkStandardNewMacro(svtkOpenGLVolumeTransferFunction2D);

//--------------------------------------------------------------------------
svtkOpenGLVolumeTransferFunction2D::svtkOpenGLVolumeTransferFunction2D()
{
  this->NumberOfColorComponents = 4;
}

//--------------------------------------------------------------------------
void svtkOpenGLVolumeTransferFunction2D::InternalUpdate(svtkObject* func, int svtkNotUsed(blendMode),
  double svtkNotUsed(sampleDistance), double svtkNotUsed(unitDistance), int filterValue)
{
  svtkImageData* transfer2D = svtkImageData::SafeDownCast(func);
  if (!transfer2D)
  {
    return;
  }
  int* dims = transfer2D->GetDimensions();
  // Resample if there is a size restriction
  void* data = transfer2D->GetPointData()->GetScalars()->GetVoidPointer(0);
  if (dims[0] != this->TextureWidth || dims[1] != this->TextureHeight)
  {
    this->ResizeFilter->SetInputData(transfer2D);
    this->ResizeFilter->SetResizeMethodToOutputDimensions();
    this->ResizeFilter->SetOutputDimensions(this->TextureWidth, this->TextureHeight, 1);
    this->ResizeFilter->Update();
    data = this->ResizeFilter->GetOutput()->GetPointData()->GetScalars()->GetVoidPointer(0);
  }

  this->TextureObject->SetWrapS(svtkTextureObject::ClampToEdge);
  this->TextureObject->SetWrapT(svtkTextureObject::ClampToEdge);
  this->TextureObject->SetMagnificationFilter(filterValue);
  this->TextureObject->SetMinificationFilter(filterValue);
  this->TextureObject->Create2DFromRaw(
    this->TextureWidth, this->TextureHeight, this->NumberOfColorComponents, SVTK_FLOAT, data);
}

//-----------------------------------------------------------------------------
bool svtkOpenGLVolumeTransferFunction2D::NeedsUpdate(svtkObject* func,
  double[2] svtkNotUsed(scalarRange), int svtkNotUsed(blendMode), double svtkNotUsed(sampleDistance))
{
  if (!func)
  {
    return false;
  }
  if (func->GetMTime() > this->BuildTime || this->TextureObject->GetMTime() > this->BuildTime ||
    !this->TextureObject->GetHandle())
  {
    return true;
  }
  return false;
}

//-----------------------------------------------------------------------------
void svtkOpenGLVolumeTransferFunction2D::AllocateTable() {}

//-----------------------------------------------------------------------------
void svtkOpenGLVolumeTransferFunction2D::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
