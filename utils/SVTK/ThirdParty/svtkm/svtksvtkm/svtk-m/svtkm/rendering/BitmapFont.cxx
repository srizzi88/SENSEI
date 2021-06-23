//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <svtkm/rendering/BitmapFont.h>

namespace svtkm
{
namespace rendering
{

BitmapFont::BitmapFont()
{
  for (int i = 0; i < 256; ++i)
    ShortMap[i] = 0;
  this->PadL = 0;
  this->PadR = 0;
  this->PadT = 0;
  this->PadB = 0;
}

const svtkm::rendering::BitmapFont::Character& BitmapFont::GetChar(char c) const
{
  std::size_t mappedCharIndex = static_cast<std::size_t>(this->ShortMap[(unsigned char)c]);
  return this->Chars[mappedCharIndex];
}

svtkm::Float32 BitmapFont::GetTextWidth(const std::string& text) const
{
  svtkm::Float32 width = 0;
  for (unsigned int i = 0; i < text.length(); ++i)
  {
    Character c = this->GetChar(text[i]);
    char nextchar = (i < text.length() - 1) ? text[i + 1] : 0;

    const bool kerning = true;
    if (kerning && nextchar > 0)
      width += svtkm::Float32(c.kern[int(nextchar)]) / svtkm::Float32(this->Height);
    width += svtkm::Float32(c.adv) / svtkm::Float32(this->Height);
  }
  return width;
}

void BitmapFont::GetCharPolygon(char character,
                                svtkm::Float32& x,
                                svtkm::Float32& y,
                                svtkm::Float32& vl,
                                svtkm::Float32& vr,
                                svtkm::Float32& vt,
                                svtkm::Float32& vb,
                                svtkm::Float32& tl,
                                svtkm::Float32& tr,
                                svtkm::Float32& tt,
                                svtkm::Float32& tb,
                                char nextchar) const
{
  Character c = this->GetChar(character);

  // By default, the origin for the font is at the
  // baseline.  That's nice, but we'd rather it
  // be at the actual bottom, so create an offset.
  svtkm::Float32 yoff = -svtkm::Float32(this->Descender) / svtkm::Float32(this->Height);

  tl = svtkm::Float32(c.x + this->PadL) / svtkm::Float32(this->ImgW);
  tr = svtkm::Float32(c.x + c.w - this->PadR) / svtkm::Float32(this->ImgW);
  tt = 1.f - svtkm::Float32(c.y + this->PadT) / svtkm::Float32(this->ImgH);
  tb = 1.f - svtkm::Float32(c.y + c.h - this->PadB) / svtkm::Float32(this->ImgH);

  vl = x + svtkm::Float32(c.offx + this->PadL) / svtkm::Float32(this->Height);
  vr = x + svtkm::Float32(c.offx + c.w - this->PadR) / svtkm::Float32(this->Height);
  vt = yoff + y + svtkm::Float32(c.offy - this->PadT) / svtkm::Float32(this->Height);
  vb = yoff + y + svtkm::Float32(c.offy - c.h + this->PadB) / svtkm::Float32(this->Height);

  const bool kerning = true;
  if (kerning && nextchar > 0)
    x += svtkm::Float32(c.kern[int(nextchar)]) / svtkm::Float32(this->Height);
  x += svtkm::Float32(c.adv) / svtkm::Float32(this->Height);
}
}
} // namespace svtkm::rendering
