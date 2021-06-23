//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <svtkm/worklet/ScalarsToColors.h>

#include <svtkm/VecTraits.h>
#include <svtkm/cont/ArrayHandleExtractComponent.h>
#include <svtkm/cont/ArrayHandleTransform.h>

#include <svtkm/worklet/DispatcherMapField.h>
#include <svtkm/worklet/colorconversion/ConvertToRGB.h>
#include <svtkm/worklet/colorconversion/ConvertToRGBA.h>
#include <svtkm/worklet/colorconversion/Portals.h>
#include <svtkm/worklet/colorconversion/ShiftScaleToRGB.h>
#include <svtkm/worklet/colorconversion/ShiftScaleToRGBA.h>

namespace svtkm
{
namespace worklet
{
namespace colorconversion
{
inline bool needShiftScale(svtkm::Float32, svtkm::Float32 shift, svtkm::Float32 scale)
{
  return !((shift == -0.0f || shift == 0.0f) && (scale == 255.0f));
}
inline bool needShiftScale(svtkm::Float64, svtkm::Float32 shift, svtkm::Float32 scale)
{
  return !((shift == -0.0f || shift == 0.0f) && (scale == 255.0f));
}
inline bool needShiftScale(svtkm::UInt8, svtkm::Float32 shift, svtkm::Float32 scale)
{
  return !((shift == -0.0f || shift == 0.0f) && (scale == 1.0f));
}

template <typename T>
inline bool needShiftScale(T, svtkm::Float32, svtkm::Float32)
{
  return true;
}
}
/// \brief Use each component to generate RGBA colors
///
template <typename T, typename S>
void ScalarsToColors::Run(const svtkm::cont::ArrayHandle<T, S>& values,
                          svtkm::cont::ArrayHandle<svtkm::Vec4ui_8>& rgbaOut) const
{
  using namespace svtkm::worklet::colorconversion;
  //If our shift is 0 and our scale == 1 no need to apply them
  using BaseT = typename svtkm::VecTraits<T>::BaseComponentType;
  const bool shiftscale = needShiftScale(BaseT{}, this->Shift, this->Scale);
  if (shiftscale)
  {
    svtkm::worklet::DispatcherMapField<ShiftScaleToRGBA> dispatcher(
      ShiftScaleToRGBA(this->Shift, this->Scale, this->Alpha));
    dispatcher.Invoke(values, rgbaOut);
  }
  else
  {
    svtkm::worklet::DispatcherMapField<ConvertToRGBA> dispatcher(ConvertToRGBA(this->Alpha));
    dispatcher.Invoke(values, rgbaOut);
  }
}

/// \brief Use each component to generate RGB colors
///
template <typename T, typename S>
void ScalarsToColors::Run(const svtkm::cont::ArrayHandle<T, S>& values,
                          svtkm::cont::ArrayHandle<svtkm::Vec3ui_8>& rgbOut) const
{
  using namespace svtkm::worklet::colorconversion;
  using BaseT = typename svtkm::VecTraits<T>::BaseComponentType;
  const bool shiftscale = needShiftScale(BaseT{}, this->Shift, this->Scale);
  if (shiftscale)
  {
    svtkm::worklet::DispatcherMapField<ShiftScaleToRGB> dispatcher(
      ShiftScaleToRGB(this->Shift, this->Scale));
    dispatcher.Invoke(values, rgbOut);
  }
  else
  {
    svtkm::worklet::DispatcherMapField<ConvertToRGB> dispatcher;
    dispatcher.Invoke(values, rgbOut);
  }
}

/// \brief Use magnitude of a vector to generate RGBA colors
///
template <typename T, int N, typename S>
void ScalarsToColors::RunMagnitude(const svtkm::cont::ArrayHandle<svtkm::Vec<T, N>, S>& values,
                                   svtkm::cont::ArrayHandle<svtkm::Vec4ui_8>& rgbaOut) const
{
  //magnitude is a complex situation. the default scale factor is incorrect
  //
  using namespace svtkm::worklet::colorconversion;
  //If our shift is 0 and our scale == 1 no need to apply them
  using BaseT = typename svtkm::VecTraits<T>::BaseComponentType;
  const bool shiftscale = needShiftScale(BaseT{}, this->Shift, this->Scale);
  if (shiftscale)
  {
    svtkm::worklet::DispatcherMapField<ShiftScaleToRGBA> dispatcher(
      ShiftScaleToRGBA(this->Shift, this->Scale, this->Alpha));
    dispatcher.Invoke(
      svtkm::cont::make_ArrayHandleTransform(values, colorconversion::MagnitudePortal()), rgbaOut);
  }
  else
  {
    svtkm::worklet::DispatcherMapField<ConvertToRGBA> dispatcher(ConvertToRGBA(this->Alpha));
    dispatcher.Invoke(
      svtkm::cont::make_ArrayHandleTransform(values, colorconversion::MagnitudePortal()), rgbaOut);
  }
}

/// \brief Use magnitude of a vector to generate RGB colors
///
template <typename T, int N, typename S>
void ScalarsToColors::RunMagnitude(const svtkm::cont::ArrayHandle<svtkm::Vec<T, N>, S>& values,
                                   svtkm::cont::ArrayHandle<svtkm::Vec3ui_8>& rgbOut) const
{

  using namespace svtkm::worklet::colorconversion;
  using BaseT = typename svtkm::VecTraits<T>::BaseComponentType;
  const bool shiftscale = needShiftScale(BaseT{}, this->Shift, this->Scale);
  if (shiftscale)
  {
    svtkm::worklet::DispatcherMapField<ShiftScaleToRGB> dispatcher(
      ShiftScaleToRGB(this->Shift, this->Scale));
    dispatcher.Invoke(
      svtkm::cont::make_ArrayHandleTransform(values, colorconversion::MagnitudePortal()), rgbOut);
  }
  else
  {
    svtkm::worklet::DispatcherMapField<ConvertToRGB> dispatcher;
    dispatcher.Invoke(
      svtkm::cont::make_ArrayHandleTransform(values, colorconversion::MagnitudePortal()), rgbOut);
  }
}

/// \brief Use a single component of a vector to generate RGBA colors
///
template <typename T, int N, typename S>
void ScalarsToColors::RunComponent(const svtkm::cont::ArrayHandle<svtkm::Vec<T, N>, S>& values,
                                   svtkm::IdComponent comp,
                                   svtkm::cont::ArrayHandle<svtkm::Vec4ui_8>& rgbaOut) const
{
  this->Run(svtkm::cont::make_ArrayHandleTransform(values, colorconversion::ComponentPortal(comp)),
            rgbaOut);
}

/// \brief Use a single component of a vector to generate RGB colors
///
template <typename T, int N, typename S>
void ScalarsToColors::RunComponent(const svtkm::cont::ArrayHandle<svtkm::Vec<T, N>, S>& values,
                                   svtkm::IdComponent comp,
                                   svtkm::cont::ArrayHandle<svtkm::Vec3ui_8>& rgbOut) const
{
  this->Run(svtkm::cont::make_ArrayHandleTransform(values, colorconversion::ComponentPortal(comp)),
            rgbOut);
}
}
}
