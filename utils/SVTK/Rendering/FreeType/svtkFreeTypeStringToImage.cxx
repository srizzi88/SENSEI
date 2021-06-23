/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkFreeTypeStringToImage.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkFreeTypeStringToImage.h"

#include "svtkImageData.h"
#include "svtkStdString.h"
#include "svtkTextProperty.h"
#include "svtkUnicodeString.h"
#include "svtkVector.h"

#include "svtkFreeTypeTools.h"

#include "svtkObjectFactory.h"

class svtkFreeTypeStringToImage::Internals
{
public:
  Internals() { this->FreeType = svtkFreeTypeTools::GetInstance(); }
  svtkFreeTypeTools* FreeType;
};

//-----------------------------------------------------------------------------
svtkStandardNewMacro(svtkFreeTypeStringToImage);

//-----------------------------------------------------------------------------
svtkFreeTypeStringToImage::svtkFreeTypeStringToImage()
{
  this->Implementation = new Internals;
}

//-----------------------------------------------------------------------------
svtkFreeTypeStringToImage::~svtkFreeTypeStringToImage()
{
  delete this->Implementation;
}

//-----------------------------------------------------------------------------
svtkVector2i svtkFreeTypeStringToImage::GetBounds(
  svtkTextProperty* property, const svtkUnicodeString& string, int dpi)
{
  int tmp[4] = { 0, 0, 0, 0 };
  svtkVector2i recti(tmp);
  if (!property)
  {
    return recti;
  }

  this->Implementation->FreeType->GetBoundingBox(property, string, dpi, tmp);

  recti.Set(tmp[1] - tmp[0], tmp[3] - tmp[2]);

  return recti;
}

//-----------------------------------------------------------------------------
svtkVector2i svtkFreeTypeStringToImage::GetBounds(
  svtkTextProperty* property, const svtkStdString& string, int dpi)
{
  svtkVector2i recti(0, 0);
  int tmp[4];
  if (!property || string.empty())
  {
    return recti;
  }

  this->Implementation->FreeType->GetBoundingBox(property, string, dpi, tmp);

  recti.Set(tmp[1] - tmp[0], tmp[3] - tmp[2]);

  return recti;
}

//-----------------------------------------------------------------------------
int svtkFreeTypeStringToImage::RenderString(svtkTextProperty* property,
  const svtkUnicodeString& string, int dpi, svtkImageData* data, int textDims[2])
{
  return this->Implementation->FreeType->RenderString(property, string, dpi, data, textDims);
}

//-----------------------------------------------------------------------------
int svtkFreeTypeStringToImage::RenderString(svtkTextProperty* property, const svtkStdString& string,
  int dpi, svtkImageData* data, int textDims[2])
{
  return this->Implementation->FreeType->RenderString(property, string, dpi, data, textDims);
}

//-----------------------------------------------------------------------------
void svtkFreeTypeStringToImage::SetScaleToPowerOfTwo(bool scale)
{
  this->svtkStringToImage::SetScaleToPowerOfTwo(scale);
  this->Implementation->FreeType->SetScaleToPowerTwo(scale);
}

//-----------------------------------------------------------------------------
void svtkFreeTypeStringToImage::DeepCopy(svtkFreeTypeStringToImage*) {}

//-----------------------------------------------------------------------------
void svtkFreeTypeStringToImage::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
