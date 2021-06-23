//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_worklet_colorconversion_ShiftScaleToRGB_h
#define svtk_m_worklet_colorconversion_ShiftScaleToRGB_h

#include <svtkm/worklet/colorconversion/Conversions.h>

#include <svtkm/Math.h>

namespace svtkm
{
namespace worklet
{
namespace colorconversion
{

struct ShiftScaleToRGB : public svtkm::worklet::WorkletMapField
{
  using ControlSignature = void(FieldIn in, FieldOut out);
  using ExecutionSignature = _2(_1);

  ShiftScaleToRGB(svtkm::Float32 shift, svtkm::Float32 scale)
    : Shift(shift)
    , Scale(scale)
  {
  }

  template <typename T>
  SVTKM_EXEC svtkm::Vec3ui_8 operator()(const T& in) const
  { //svtkScalarsToColorsLuminanceToRGB
    svtkm::Float32 l = (static_cast<svtkm::Float32>(in) + this->Shift) * this->Scale;
    colorconversion::Clamp(l);
    return svtkm::Vec3ui_8{ static_cast<svtkm::UInt8>(l + 0.5f) };
  }

  template <typename T>
  SVTKM_EXEC svtkm::Vec3ui_8 operator()(const svtkm::Vec<T, 2>& in) const
  { //svtkScalarsToColorsLuminanceAlphaToRGB (which actually doesn't exist in svtk)
    return this->operator()(in[0]);
  }

  template <typename T>
  SVTKM_EXEC svtkm::Vec3ui_8 operator()(const svtkm::Vec<T, 3>& in) const
  { //svtkScalarsToColorsRGBToRGB
    svtkm::Vec3f_32 rgb(in);
    rgb = (rgb + svtkm::Vec3f_32(this->Shift)) * this->Scale;
    colorconversion::Clamp(rgb);
    return svtkm::Vec3ui_8{ static_cast<svtkm::UInt8>(rgb[0] + 0.5f),
                           static_cast<svtkm::UInt8>(rgb[1] + 0.5f),
                           static_cast<svtkm::UInt8>(rgb[2] + 0.5f) };
  }

  template <typename T>
  SVTKM_EXEC svtkm::Vec3ui_8 operator()(const svtkm::Vec<T, 4>& in) const
  { //svtkScalarsToColorsRGBAToRGB
    return this->operator()(svtkm::Vec<T, 3>{ in[0], in[1], in[2] });
  }

private:
  const svtkm::Float32 Shift;
  const svtkm::Float32 Scale;
};
}
}
}
#endif
