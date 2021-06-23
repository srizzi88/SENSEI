//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_worklet_colorconversion_ConvertToRGB_h
#define svtk_m_worklet_colorconversion_ConvertToRGB_h

#include <svtkm/worklet/colorconversion/Conversions.h>

namespace svtkm
{
namespace worklet
{
namespace colorconversion
{

struct ConvertToRGB : public svtkm::worklet::WorkletMapField
{
  using ControlSignature = void(FieldIn in, FieldOut out);
  using ExecutionSignature = _2(_1);

  template <typename T>
  SVTKM_EXEC svtkm::Vec3ui_8 operator()(const T& in) const
  { //svtkScalarsToColorsLuminanceToRGB
    const svtkm::UInt8 la = colorconversion::ColorToUChar(in);
    return svtkm::Vec<UInt8, 3>(la, la, la);
  }

  template <typename T>
  SVTKM_EXEC svtkm::Vec3ui_8 operator()(const svtkm::Vec<T, 2>& in) const
  { //svtkScalarsToColorsLuminanceAlphaToRGB (which actually doesn't exist in svtk)
    return this->operator()(in[0]);
  }

  template <typename T>
  SVTKM_EXEC svtkm::Vec3ui_8 operator()(const svtkm::Vec<T, 3>& in) const
  { //svtkScalarsToColorsRGBToRGB
    return svtkm::Vec<UInt8, 3>(colorconversion::ColorToUChar(in[0]),
                               colorconversion::ColorToUChar(in[1]),
                               colorconversion::ColorToUChar(in[2]));
  }

  SVTKM_EXEC svtkm::Vec3ui_8 operator()(const svtkm::Vec3ui_8& in) const
  { //svtkScalarsToColorsRGBToRGB
    return in;
  }

  template <typename T>
  SVTKM_EXEC svtkm::Vec3ui_8 operator()(const svtkm::Vec<T, 4>& in) const
  { //svtkScalarsToColorsRGBAToRGB
    return svtkm::Vec<UInt8, 3>(colorconversion::ColorToUChar(in[0]),
                               colorconversion::ColorToUChar(in[1]),
                               colorconversion::ColorToUChar(in[2]));
  }
};
}
}
}
#endif
