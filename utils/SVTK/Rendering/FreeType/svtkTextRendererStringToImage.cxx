/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkTextRendererStringToImage.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkTextRendererStringToImage.h"

#include "svtkImageData.h"
#include "svtkObjectFactory.h"
#include "svtkStdString.h"
#include "svtkTextProperty.h"
#include "svtkTextRenderer.h"
#include "svtkUnicodeString.h"
#include "svtkVector.h"

class svtkTextRendererStringToImage::Internals
{
public:
  Internals() { this->TextRenderer = svtkTextRenderer::GetInstance(); }
  svtkTextRenderer* TextRenderer;
};

//-----------------------------------------------------------------------------
svtkStandardNewMacro(svtkTextRendererStringToImage);

//-----------------------------------------------------------------------------
svtkTextRendererStringToImage::svtkTextRendererStringToImage()
{
  this->Implementation = new Internals;
}

//-----------------------------------------------------------------------------
svtkTextRendererStringToImage::~svtkTextRendererStringToImage()
{
  delete this->Implementation;
}

//-----------------------------------------------------------------------------
svtkVector2i svtkTextRendererStringToImage::GetBounds(
  svtkTextProperty* property, const svtkUnicodeString& string, int dpi)
{
  int tmp[4] = { 0, 0, 0, 0 };
  svtkVector2i recti(tmp);
  if (!property || string.empty())
  {
    return recti;
  }

  this->Implementation->TextRenderer->GetBoundingBox(property, string, tmp, dpi);

  recti.Set(tmp[1] - tmp[0] + 1, tmp[3] - tmp[2] + 1);

  return recti;
}

//-----------------------------------------------------------------------------
svtkVector2i svtkTextRendererStringToImage::GetBounds(
  svtkTextProperty* property, const svtkStdString& string, int dpi)
{
  svtkVector2i recti(0, 0);
  int tmp[4];
  if (!property || string.empty())
  {
    return recti;
  }

  this->Implementation->TextRenderer->GetBoundingBox(property, string, tmp, dpi);

  recti.Set(tmp[1] - tmp[0] + 1, tmp[3] - tmp[2] + 1);

  return recti;
}

//-----------------------------------------------------------------------------
int svtkTextRendererStringToImage::RenderString(svtkTextProperty* property,
  const svtkUnicodeString& string, int dpi, svtkImageData* data, int textDims[2])
{
  return this->Implementation->TextRenderer->RenderString(property, string, data, textDims, dpi);
}

//-----------------------------------------------------------------------------
int svtkTextRendererStringToImage::RenderString(svtkTextProperty* property,
  const svtkStdString& string, int dpi, svtkImageData* data, int textDims[2])
{
  return this->Implementation->TextRenderer->RenderString(property, string, data, textDims, dpi);
}

//-----------------------------------------------------------------------------
void svtkTextRendererStringToImage::SetScaleToPowerOfTwo(bool scale)
{
  this->svtkStringToImage::SetScaleToPowerOfTwo(scale);
  this->Implementation->TextRenderer->SetScaleToPowerOfTwo(scale);
}

//-----------------------------------------------------------------------------
void svtkTextRendererStringToImage::DeepCopy(svtkTextRendererStringToImage*) {}

//-----------------------------------------------------------------------------
void svtkTextRendererStringToImage::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
