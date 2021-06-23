//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_cont_ColorTableSamples_h
#define svtk_m_cont_ColorTableSamples_h

#include <svtkm/Range.h>
#include <svtkm/cont/ArrayHandle.h>

namespace svtkm
{
namespace cont
{

/// \brief Color Sample Table used with svtkm::cont::ColorTable for fast coloring
///
/// Holds a special layout of sampled values with the pattern of
/// [Below Color, samples, last sample value again, Above Color, Nan Color ]
///
/// This layout has been chosen as it allows for efficient access for values
/// inside the range, and values outside the range. The last value being duplicated
/// a second time is an optimization for fast interpolation of values that are
/// very near to the Max value of the range.
///
///
class ColorTableSamplesRGBA
{
public:
  svtkm::Range SampleRange = { 1.0, 0.0 };
  svtkm::Int32 NumberOfSamples = 0; // this will not include end padding, NaN, Below or Above Range
  svtkm::cont::ArrayHandle<svtkm::Vec4ui_8> Samples;
};

/// \brief Color Sample Table used with svtkm::cont::ColorTable for fast coloring
///
/// Holds a special layout of sampled values with the pattern of
/// [Below Color, samples, last sample value again, Above Color ]
///
/// This layout has been chosen as it allows for efficient access for values
/// inside the range, and values outside the range. The last value being duplicated
/// a second time is an optimization for fast interpolation of values that are
/// very near to the Max value of the range.
///
///
class ColorTableSamplesRGB
{
public:
  svtkm::Range SampleRange = { 1.0, 0.0 };
  svtkm::Int32 NumberOfSamples = 0; // this will not include end padding, NaN, Below or Above Range
  svtkm::cont::ArrayHandle<svtkm::Vec3ui_8> Samples;
};
}
}

#endif
