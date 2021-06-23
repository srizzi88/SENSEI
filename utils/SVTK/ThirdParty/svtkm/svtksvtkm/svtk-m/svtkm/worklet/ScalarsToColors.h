//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_worklet_ScalarsToColors_h
#define svtk_m_worklet_ScalarsToColors_h

#include <svtkm/Range.h>
#include <svtkm/cont/ArrayHandle.h>

namespace svtkm
{
namespace worklet
{

namespace colorconversion
{
inline void ComputeShiftScale(const svtkm::Range& range, svtkm::Float32& shift, svtkm::Float32& scale)
{
  //This scale logic seems to be unduly complicated
  shift = static_cast<svtkm::Float32>(-range.Min);
  scale = static_cast<svtkm::Float32>(range.Length());

  if (range.Length() <= 0)
  {
    scale = -1e17f;
  }
  if (scale * scale > 1e-30f)
  {
    scale = 1.0f / scale;
  }
  scale *= 255.0f;
}
}

class ScalarsToColors
{
  svtkm::Range ValueRange = { 0.0f, 255.0f };
  svtkm::Float32 Alpha = 1.0f;
  svtkm::Float32 Shift = 0.0f;
  svtkm::Float32 Scale = 1.0f;

public:
  ScalarsToColors() {}

  ScalarsToColors(const svtkm::Range& range, svtkm::Float32 alpha)
    : ValueRange(range)
    , Alpha(svtkm::Min(svtkm::Max(alpha, 0.0f), 1.0f))
  {
    colorconversion::ComputeShiftScale(range, this->Shift, this->Scale);
  }

  ScalarsToColors(const svtkm::Range& range)
    : ValueRange(range)
  {
    colorconversion::ComputeShiftScale(range, this->Shift, this->Scale);
  }

  ScalarsToColors(svtkm::Float32 alpha)
    : ValueRange(0.0f, 255.0f)
    , Alpha(svtkm::Min(svtkm::Max(alpha, 0.0f), 1.0f))
  {
  }

  void SetRange(const svtkm::Range& range)
  {
    this->ValueRange = range;
    colorconversion::ComputeShiftScale(range, this->Shift, this->Scale);
  }

  svtkm::Range GetRange() const { return this->ValueRange; }

  void SetAlpha(svtkm::Float32 alpha) { this->Alpha = svtkm::Min(svtkm::Max(alpha, 0.0f), 1.0f); }

  svtkm::Float32 GetAlpha() const { return this->Alpha; }

  /// \brief Use each component to generate RGBA colors
  ///
  template <typename T, typename S>
  void Run(const svtkm::cont::ArrayHandle<T, S>& values,
           svtkm::cont::ArrayHandle<svtkm::Vec4ui_8>& rgbaOut) const;

  /// \brief Use each component to generate RGB colors
  ///
  template <typename T, typename S>
  void Run(const svtkm::cont::ArrayHandle<T, S>& values,
           svtkm::cont::ArrayHandle<svtkm::Vec3ui_8>& rgbOut) const;


  /// \brief Use magnitude of a vector to generate RGBA colors
  ///
  template <typename T, int N, typename S>
  void RunMagnitude(const svtkm::cont::ArrayHandle<svtkm::Vec<T, N>, S>& values,
                    svtkm::cont::ArrayHandle<svtkm::Vec4ui_8>& rgbaOut) const;

  /// \brief Use magnitude of a vector to generate RGB colors
  ///
  template <typename T, int N, typename S>
  void RunMagnitude(const svtkm::cont::ArrayHandle<svtkm::Vec<T, N>, S>& values,
                    svtkm::cont::ArrayHandle<svtkm::Vec3ui_8>& rgbOut) const;

  /// \brief Use a single component of a vector to generate RGBA colors
  ///
  template <typename T, int N, typename S>
  void RunComponent(const svtkm::cont::ArrayHandle<svtkm::Vec<T, N>, S>& values,
                    svtkm::IdComponent comp,
                    svtkm::cont::ArrayHandle<svtkm::Vec4ui_8>& rgbaOut) const;

  /// \brief Use a single component of a vector to generate RGB colors
  ///
  template <typename T, int N, typename S>
  void RunComponent(const svtkm::cont::ArrayHandle<svtkm::Vec<T, N>, S>& values,
                    svtkm::IdComponent comp,
                    svtkm::cont::ArrayHandle<svtkm::Vec3ui_8>& rgbOut) const;
};
}
}

#include <svtkm/worklet/ScalarsToColors.hxx>

#endif
