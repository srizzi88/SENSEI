//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_worklet_colorconversion_ShiftScaleToRGBA_h
#define svtk_m_worklet_colorconversion_ShiftScaleToRGBA_h

#include <svtkm/worklet/WorkletMapField.h>
#include <svtkm/worklet/colorconversion/Conversions.h>

#include <svtkm/Math.h>

namespace svtkm
{
namespace worklet
{
namespace colorconversion
{

struct ShiftScaleToRGBA : public svtkm::worklet::WorkletMapField
{
  const svtkm::Float32 Shift;
  const svtkm::Float32 Scale;
  const svtkm::Float32 Alpha;

  using ControlSignature = void(FieldIn in, FieldOut out);
  using ExecutionSignature = _2(_1);

  ShiftScaleToRGBA(svtkm::Float32 shift, svtkm::Float32 scale, svtkm::Float32 alpha)
    : WorkletMapField()
    , Shift(shift)
    , Scale(scale)
    , Alpha(alpha)
  {
  }

  template <typename T>
  SVTKM_EXEC svtkm::Vec4ui_8 operator()(const T& in) const
  { //svtkScalarsToColorsLuminanceToRGBA
    svtkm::Float32 l = (static_cast<svtkm::Float32>(in) + this->Shift) * this->Scale;
    colorconversion::Clamp(l);
    const svtkm::UInt8 lc = static_cast<svtkm::UInt8>(l + 0.5);
    return svtkm::Vec4ui_8{ lc, lc, lc, colorconversion::ColorToUChar(this->Alpha) };
  }

  template <typename T>
  SVTKM_EXEC svtkm::Vec4ui_8 operator()(const svtkm::Vec<T, 2>& in) const
  { //svtkScalarsToColorsLuminanceAlphaToRGBA
    svtkm::Vec2f_32 la(in);
    la = (la + svtkm::Vec2f_32(this->Shift)) * this->Scale;
    colorconversion::Clamp(la);

    const svtkm::UInt8 lc = static_cast<svtkm::UInt8>(la[0] + 0.5f);
    return svtkm::Vec4ui_8{ lc, lc, lc, static_cast<svtkm::UInt8>((la[1] * this->Alpha) + 0.5f) };
  }

  template <typename T>
  SVTKM_EXEC svtkm::Vec4ui_8 operator()(const svtkm::Vec<T, 3>& in) const
  { //svtkScalarsToColorsRGBToRGBA
    svtkm::Vec3f_32 rgb(in);
    rgb = (rgb + svtkm::Vec3f_32(this->Shift)) * this->Scale;
    colorconversion::Clamp(rgb);
    return svtkm::Vec4ui_8{ static_cast<svtkm::UInt8>(rgb[0] + 0.5f),
                           static_cast<svtkm::UInt8>(rgb[1] + 0.5f),
                           static_cast<svtkm::UInt8>(rgb[2] + 0.5f),
                           colorconversion::ColorToUChar(this->Alpha) };
  }

  template <typename T>
  SVTKM_EXEC svtkm::Vec4ui_8 operator()(const svtkm::Vec<T, 4>& in) const
  { //svtkScalarsToColorsRGBAToRGBA
    svtkm::Vec4f_32 rgba(in);
    rgba = (rgba + svtkm::Vec4f_32(this->Shift)) * this->Scale;
    colorconversion::Clamp(rgba);

    rgba[3] *= this->Alpha;
    return svtkm::Vec4ui_8{ static_cast<svtkm::UInt8>(rgba[0] + 0.5f),
                           static_cast<svtkm::UInt8>(rgba[1] + 0.5f),
                           static_cast<svtkm::UInt8>(rgba[2] + 0.5f),
                           static_cast<svtkm::UInt8>(rgba[3] + 0.5f) };
  }
};
}
}
}
#endif
