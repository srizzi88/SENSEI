//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_exec_ColorTable_h
#define svtk_m_exec_ColorTable_h

#include <svtkm/VirtualObjectBase.h>

namespace svtkm
{
namespace exec
{

class SVTKM_ALWAYS_EXPORT ColorTableBase : public svtkm::VirtualObjectBase
{
public:
  inline SVTKM_EXEC svtkm::Vec<float, 3> MapThroughColorSpace(double) const;

  SVTKM_EXEC virtual svtkm::Vec<float, 3> MapThroughColorSpace(const svtkm::Vec<float, 3>& rgb1,
                                                             const svtkm::Vec<float, 3>& rgb2,
                                                             float weight) const = 0;

  inline SVTKM_EXEC float MapThroughOpacitySpace(double value) const;

  double const* ColorNodes = nullptr;
  svtkm::Vec<float, 3> const* RGB = nullptr;

  double const* ONodes = nullptr;
  float const* Alpha = nullptr;
  svtkm::Vec<float, 2> const* MidSharp = nullptr;

  svtkm::Int32 ColorSize = 0;
  svtkm::Int32 OpacitySize = 0;

  svtkm::Vec<float, 3> NaNColor = { 0.5f, 0.0f, 0.0f };
  svtkm::Vec<float, 3> BelowRangeColor = { 0.0f, 0.0f, 0.0f };
  svtkm::Vec<float, 3> AboveRangeColor = { 0.0f, 0.0f, 0.0f };

  bool UseClamping = true;

private:
  inline SVTKM_EXEC void FindColors(double value,
                                   svtkm::Vec<float, 3>& first,
                                   svtkm::Vec<float, 3>& second,
                                   float& weight) const;
};

class SVTKM_ALWAYS_EXPORT ColorTableRGB final : public ColorTableBase
{
public:
  inline SVTKM_EXEC svtkm::Vec<float, 3> MapThroughColorSpace(const svtkm::Vec<float, 3>& rgb1,
                                                            const svtkm::Vec<float, 3>& rgb2,
                                                            float weight) const;
};

class SVTKM_ALWAYS_EXPORT ColorTableHSV final : public ColorTableBase
{
public:
  inline SVTKM_EXEC svtkm::Vec<float, 3> MapThroughColorSpace(const svtkm::Vec<float, 3>& rgb1,
                                                            const svtkm::Vec<float, 3>& rgb2,
                                                            float weight) const;
};

class SVTKM_ALWAYS_EXPORT ColorTableHSVWrap final : public ColorTableBase
{
public:
  inline SVTKM_EXEC svtkm::Vec<float, 3> MapThroughColorSpace(const svtkm::Vec<float, 3>& rgb1,
                                                            const svtkm::Vec<float, 3>& rgb2,
                                                            float weight) const;
};

class SVTKM_ALWAYS_EXPORT ColorTableLab final : public ColorTableBase
{
public:
  inline SVTKM_EXEC svtkm::Vec<float, 3> MapThroughColorSpace(const svtkm::Vec<float, 3>& rgb1,
                                                            const svtkm::Vec<float, 3>& rgb2,
                                                            float weight) const;
};

class SVTKM_ALWAYS_EXPORT ColorTableDiverging final : public ColorTableBase
{
public:
  inline SVTKM_EXEC svtkm::Vec<float, 3> MapThroughColorSpace(const svtkm::Vec<float, 3>& rgb1,
                                                            const svtkm::Vec<float, 3>& rgb2,
                                                            float weight) const;
};
}
}

#include <svtkm/exec/ColorTable.hxx>

#endif
