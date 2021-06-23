/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkBrush.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkBrush.h"

#include "svtkImageData.h"
#include "svtkObjectFactory.h"

//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
svtkStandardNewMacro(svtkBrush);

//-----------------------------------------------------------------------------
svtkBrush::svtkBrush()
  : BrushColor(0, 0, 0, 255)
{
  this->Color = this->BrushColor.GetData();
  this->Texture = nullptr;
  this->TextureProperties = svtkBrush::Nearest | svtkBrush::Stretch;
}

//-----------------------------------------------------------------------------
svtkBrush::~svtkBrush()
{
  if (this->Texture)
  {
    this->Texture->Delete();
    this->Texture = nullptr;
  }
}

//-----------------------------------------------------------------------------
void svtkBrush::SetColorF(double color[3])
{
  this->Color[0] = static_cast<unsigned char>(color[0] * 255.0);
  this->Color[1] = static_cast<unsigned char>(color[1] * 255.0);
  this->Color[2] = static_cast<unsigned char>(color[2] * 255.0);
}

//-----------------------------------------------------------------------------
void svtkBrush::SetColorF(double r, double g, double b)
{
  this->Color[0] = static_cast<unsigned char>(r * 255.0);
  this->Color[1] = static_cast<unsigned char>(g * 255.0);
  this->Color[2] = static_cast<unsigned char>(b * 255.0);
}

//-----------------------------------------------------------------------------
void svtkBrush::SetColorF(double r, double g, double b, double a)
{
  this->Color[0] = static_cast<unsigned char>(r * 255.0);
  this->Color[1] = static_cast<unsigned char>(g * 255.0);
  this->Color[2] = static_cast<unsigned char>(b * 255.0);
  this->Color[3] = static_cast<unsigned char>(a * 255.0);
}

//-----------------------------------------------------------------------------
void svtkBrush::SetOpacityF(double a)
{
  this->Color[3] = static_cast<unsigned char>(a * 255.0);
}

//-----------------------------------------------------------------------------
double svtkBrush::GetOpacityF()
{
  return this->Color[3] / 255.0;
}

//-----------------------------------------------------------------------------
void svtkBrush::SetColor(unsigned char color[3])
{
  this->Color[0] = color[0];
  this->Color[1] = color[1];
  this->Color[2] = color[2];
}

//-----------------------------------------------------------------------------
void svtkBrush::SetColor(unsigned char r, unsigned char g, unsigned char b)
{
  this->Color[0] = r;
  this->Color[1] = g;
  this->Color[2] = b;
}

//-----------------------------------------------------------------------------
void svtkBrush::SetColor(unsigned char r, unsigned char g, unsigned char b, unsigned char a)
{
  this->Color[0] = r;
  this->Color[1] = g;
  this->Color[2] = b;
  this->Color[3] = a;
}

//-----------------------------------------------------------------------------
void svtkBrush::SetColor(const svtkColor4ub& color)
{
  this->BrushColor = color;
}

//-----------------------------------------------------------------------------
void svtkBrush::SetOpacity(unsigned char a)
{
  this->Color[3] = a;
}

//-----------------------------------------------------------------------------
unsigned char svtkBrush::GetOpacity()
{
  return this->Color[3];
}

//-----------------------------------------------------------------------------
void svtkBrush::GetColorF(double color[4])
{
  for (int i = 0; i < 4; ++i)
  {
    color[i] = this->Color[i] / 255.0;
  }
}

//-----------------------------------------------------------------------------
void svtkBrush::GetColor(unsigned char color[4])
{
  for (int i = 0; i < 4; ++i)
  {
    color[i] = this->Color[i];
  }
}

//-----------------------------------------------------------------------------
svtkColor4ub svtkBrush::GetColorObject()
{
  return this->BrushColor;
}

//-----------------------------------------------------------------------------
void svtkBrush::SetTexture(svtkImageData* image)
{
  svtkSetObjectBodyMacro(Texture, svtkImageData, image);
}

//-----------------------------------------------------------------------------
void svtkBrush::DeepCopy(svtkBrush* brush)
{
  if (!brush)
  {
    return;
  }
  this->BrushColor = brush->BrushColor;
  this->TextureProperties = brush->TextureProperties;
  this->SetTexture(brush->Texture);
}

//-----------------------------------------------------------------------------
void svtkBrush::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Color: " << this->Color[0] << ", " << this->Color[1] << ", " << this->Color[2]
     << ", " << this->Color[3] << endl;
  os << indent << "Texture: " << reinterpret_cast<void*>(this->Texture) << endl;
  os << indent << "Texture Properties: " << this->TextureProperties << endl;
}
