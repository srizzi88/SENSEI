//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef svtk_m_source_Wavelet_h
#define svtk_m_source_Wavelet_h

#include <svtkm/source/Source.h>

namespace svtkm
{
namespace source
{
/**
 * @brief The Wavelet source creates a dataset similar to SVTK's
 * svtkRTAnalyticSource.
 *
 * This class generates a predictable structured dataset with a smooth yet
 * interesting set of scalars, which is useful for testing and benchmarking.
 *
 * The Execute method creates a complete structured dataset that have a
 * point field names 'scalars'
 *
 * The scalars are computed as:
 *
 * ```
 * MaxVal * Gauss + MagX * sin(FrqX*x) + MagY * sin(FrqY*y) + MagZ * cos(FrqZ*z)
 * ```
 *
 * The dataset properties are determined by:
 * - `Minimum/MaximumExtent`: The logical point extents of the dataset.
 * - `Spacing`: The distance between points of the dataset.
 * - `Center`: The center of the dataset.
 *
 * The scalar functions is control via:
 * - `Center`: The center of a Gaussian contribution to the scalars.
 * - `StandardDeviation`: The unscaled width of a Gaussian contribution.
 * - `MaximumValue`: Upper limit of the scalar range.
 * - `Frequency`: The Frq[XYZ] parameters of the periodic contributions.
 * - `Magnitude`: The Mag[XYZ] parameters of the periodic contributions.
 *
 * By default, the following parameters are used:
 * - `Extents`: { -10, -10, -10 } `-->` { 10, 10, 10 }
 * - `Spacing`: { 1, 1, 1 }
 * - `Center`: { 0, 0, 0 }
 * - `StandardDeviation`: 0.5
 * - `MaximumValue`: 255
 * - `Frequency`: { 60, 30, 40 }
 * - `Magnitude`: { 10, 18, 5 }
 */
class SVTKM_SOURCE_EXPORT Wavelet final : public svtkm::source::Source
{
public:
  SVTKM_CONT
  Wavelet(svtkm::Id3 minExtent = { -10 }, svtkm::Id3 maxExtent = { 10 });

  SVTKM_CONT void SetCenter(const svtkm::Vec<FloatDefault, 3>& center) { this->Center = center; }

  SVTKM_CONT void SetSpacing(const svtkm::Vec<FloatDefault, 3>& spacing) { this->Spacing = spacing; }

  SVTKM_CONT void SetFrequency(const svtkm::Vec<FloatDefault, 3>& frequency)
  {
    this->Frequency = frequency;
  }

  SVTKM_CONT void SetMagnitude(const svtkm::Vec<FloatDefault, 3>& magnitude)
  {
    this->Magnitude = magnitude;
  }

  SVTKM_CONT void SetMinimumExtent(const svtkm::Id3& minExtent) { this->MinimumExtent = minExtent; }

  SVTKM_CONT void SetMaximumExtent(const svtkm::Id3& maxExtent) { this->MaximumExtent = maxExtent; }

  SVTKM_CONT void SetExtent(const svtkm::Id3& minExtent, const svtkm::Id3& maxExtent)
  {
    this->MinimumExtent = minExtent;
    this->MaximumExtent = maxExtent;
  }

  SVTKM_CONT void SetMaximumValue(const svtkm::FloatDefault& maxVal) { this->MaximumValue = maxVal; }

  SVTKM_CONT void SetStandardDeviation(const svtkm::FloatDefault& stdev)
  {
    this->StandardDeviation = stdev;
  }

  svtkm::cont::DataSet Execute() const;

private:
  svtkm::cont::Field GeneratePointField(const svtkm::cont::CellSetStructured<3>& cellset,
                                       const std::string& name) const;

  svtkm::Vec3f Center;
  svtkm::Vec3f Spacing;
  svtkm::Vec3f Frequency;
  svtkm::Vec3f Magnitude;
  svtkm::Id3 MinimumExtent;
  svtkm::Id3 MaximumExtent;
  svtkm::FloatDefault MaximumValue;
  svtkm::FloatDefault StandardDeviation;
};
} //namespace source
} //namespace svtkm

#endif //svtk_m_source_Wavelet_h
