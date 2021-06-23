//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_worklet_colorconversion_ScalarsToColors_h
#define svtk_m_worklet_colorconversion_ScalarsToColors_h

#include <svtkm/worklet/colorconversion/Conversions.h>

namespace svtkm
{
namespace worklet
{
namespace colorconversion
{

struct ConvertToRGBA : public svtkm::worklet::WorkletMapField
{
  using ControlSignature = void(FieldIn in, FieldOut out);
  using ExecutionSignature = _2(_1);

  ConvertToRGBA(svtkm::Float32 alpha)
    : Alpha(alpha)
  {
  }

  template <typename T>
  SVTKM_EXEC svtkm::Vec4ui_8 operator()(const T& in) const
  { //svtkScalarsToColorsLuminanceToRGBA
    const svtkm::UInt8 l = colorconversion::ColorToUChar(in);
    return svtkm::Vec<UInt8, 4>(l, l, l, colorconversion::ColorToUChar(this->Alpha));
  }

  template <typename T>
  SVTKM_EXEC svtkm::Vec4ui_8 operator()(const svtkm::Vec<T, 2>& in) const
  { //svtkScalarsToColorsLuminanceAlphaToRGBA
    const svtkm::UInt8 l = colorconversion::ColorToUChar(in[0]);
    const svtkm::UInt8 a = colorconversion::ColorToUChar(in[1]);
    return svtkm::Vec<UInt8, 4>(l, l, l, static_cast<svtkm::UInt8>(a * this->Alpha + 0.5f));
  }

  template <typename T>
  SVTKM_EXEC svtkm::Vec4ui_8 operator()(const svtkm::Vec<T, 3>& in) const
  { //svtkScalarsToColorsRGBToRGBA
    return svtkm::Vec<UInt8, 4>(colorconversion::ColorToUChar(in[0]),
                               colorconversion::ColorToUChar(in[1]),
                               colorconversion::ColorToUChar(in[2]),
                               colorconversion::ColorToUChar(this->Alpha));
  }

  template <typename T>
  SVTKM_EXEC svtkm::Vec4ui_8 operator()(const svtkm::Vec<T, 4>& in) const
  { //svtkScalarsToColorsRGBAToRGBA
    const svtkm::UInt8 a = colorconversion::ColorToUChar(in[3]);
    return svtkm::Vec<UInt8, 4>(colorconversion::ColorToUChar(in[0]),
                               colorconversion::ColorToUChar(in[1]),
                               colorconversion::ColorToUChar(in[2]),
                               static_cast<svtkm::UInt8>(a * this->Alpha + 0.5f));
  }

  const svtkm::Float32 Alpha = 1.0f;
};
}
}
}
#endif
