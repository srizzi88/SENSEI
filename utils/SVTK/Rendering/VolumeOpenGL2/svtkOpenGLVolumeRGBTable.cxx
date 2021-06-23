/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkOpenGLVolumeRGBTable.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkOpenGLVolumeRGBTable.h"

#include "svtkColorTransferFunction.h"
#include "svtkObjectFactory.h"
#include "svtkOpenGLRenderWindow.h"
#include "svtkTextureObject.h"

svtkStandardNewMacro(svtkOpenGLVolumeRGBTable);

//--------------------------------------------------------------------------
svtkOpenGLVolumeRGBTable::svtkOpenGLVolumeRGBTable()
{
  this->NumberOfColorComponents = 3;
}

//--------------------------------------------------------------------------
void svtkOpenGLVolumeRGBTable::InternalUpdate(svtkObject* func, int svtkNotUsed(blendMode),
  double svtkNotUsed(sampleDistance), double svtkNotUsed(unitDistance), int filterValue)
{
  svtkColorTransferFunction* scalarRGB = svtkColorTransferFunction::SafeDownCast(func);
  if (!scalarRGB)
  {
    return;
  }
  scalarRGB->GetTable(this->LastRange[0], this->LastRange[1], this->TextureWidth, this->Table);
  this->TextureObject->SetWrapS(svtkTextureObject::ClampToEdge);
  this->TextureObject->SetWrapT(svtkTextureObject::ClampToEdge);
  this->TextureObject->SetMagnificationFilter(filterValue);
  this->TextureObject->SetMinificationFilter(filterValue);
  this->TextureObject->Create2DFromRaw(
    this->TextureWidth, 1, this->NumberOfColorComponents, SVTK_FLOAT, this->Table);
}

//--------------------------------------------------------------------------
void svtkOpenGLVolumeRGBTable::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
