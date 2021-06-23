//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <svtkm/source/Wavelet.h>
#include <svtkm/worklet/WorkletMapField.h>

namespace
{
inline svtkm::FloatDefault computeScaleFactor(svtkm::Id min, svtkm::Id max)
{
  return (min < max) ? (1.f / static_cast<svtkm::FloatDefault>(max - min))
                     : static_cast<svtkm::FloatDefault>(1.);
}
}
namespace svtkm
{
namespace source
{
namespace wavelet
{

struct WaveletField : public svtkm::worklet::WorkletVisitPointsWithCells
{
  using ControlSignature = void(CellSetIn, FieldOut v);
  using ExecutionSignature = void(ThreadIndices, _2);
  using InputDomain = _1;

  using Vec3F = svtkm::Vec3f;

  Vec3F Center;
  Vec3F Spacing;
  Vec3F Frequency;
  Vec3F Magnitude;
  Vec3F MinimumPoint;
  Vec3F Scale;
  svtkm::Id3 Offset;
  svtkm::Id3 Dims;
  svtkm::FloatDefault MaximumValue;
  svtkm::FloatDefault Temp2;

  WaveletField(const Vec3F& center,
               const Vec3F& spacing,
               const Vec3F& frequency,
               const Vec3F& magnitude,
               const Vec3F& minimumPoint,
               const Vec3F& scale,
               const svtkm::Id3& offset,
               const svtkm::Id3& dims,
               svtkm::FloatDefault maximumValue,
               svtkm::FloatDefault temp2)
    : Center(center)
    , Spacing(spacing)
    , Frequency(frequency)
    , Magnitude(magnitude)
    , MinimumPoint(minimumPoint)
    , Scale(scale)
    , Offset(offset)
    , Dims(dims)
    , MaximumValue(maximumValue)
    , Temp2(temp2)
  {
  }

  template <typename ThreadIndexType>
  SVTKM_EXEC void operator()(const ThreadIndexType& threadIndex, svtkm::FloatDefault& scalar) const
  {
    const svtkm::Id3 ijk = threadIndex.GetInputIndex3D();

    // map ijk to the point location, accounting for spacing:
    const Vec3F loc = Vec3F(ijk + this->Offset) * this->Spacing;

    // Compute the distance from the center of the gaussian:
    const Vec3F scaledLoc = (this->Center - loc) * this->Scale;
    svtkm::FloatDefault gaussSum = svtkm::Dot(scaledLoc, scaledLoc);

    const Vec3F periodicContribs{
      this->Magnitude[0] * svtkm::Sin(this->Frequency[0] * scaledLoc[0]),
      this->Magnitude[1] * svtkm::Sin(this->Frequency[1] * scaledLoc[1]),
      this->Magnitude[2] * svtkm::Cos(this->Frequency[2] * scaledLoc[2]),
    };

    // The svtkRTAnalyticSource documentation says the periodic contributions
    // should be multiplied in, but the implementation adds them. We'll do as
    // they do, not as they say.
    scalar =
      this->MaximumValue * svtkm::Exp(-gaussSum * this->Temp2) + svtkm::ReduceSum(periodicContribs);
  }
};
} // namespace wavelet

Wavelet::Wavelet(svtkm::Id3 minExtent, svtkm::Id3 maxExtent)
  : Center{ minExtent - ((minExtent - maxExtent) / 2) }
  , Spacing{ 1. }
  , Frequency{ 60., 30., 40. }
  , Magnitude{ 10., 18., 5. }
  , MinimumExtent{ minExtent }
  , MaximumExtent{ maxExtent }
  , MaximumValue{ 255. }
  , StandardDeviation{ 0.5 }
{
}

svtkm::cont::DataSet Wavelet::Execute() const
{
  SVTKM_LOG_SCOPE_FUNCTION(svtkm::cont::LogLevel::Perf);

  // Create points:
  const svtkm::Id3 dims{ this->MaximumExtent - this->MinimumExtent + svtkm::Id3{ 1 } };
  const svtkm::Vec3f origin{ this->MinimumExtent };
  svtkm::cont::CoordinateSystem coords{ "coordinates", dims, origin, this->Spacing };

  // And cells:
  svtkm::cont::CellSetStructured<3> cellSet;
  cellSet.SetPointDimensions(dims);

  // Compile the dataset:
  svtkm::cont::DataSet dataSet;
  dataSet.AddCoordinateSystem(coords);
  dataSet.SetCellSet(cellSet);

  // Scalars, too
  svtkm::cont::Field field = this->GeneratePointField(cellSet, "scalars");
  dataSet.AddField(field);

  return dataSet;
}

svtkm::cont::Field Wavelet::GeneratePointField(const svtkm::cont::CellSetStructured<3>& cellset,
                                              const std::string& name) const
{
  const svtkm::Id3 dims{ this->MaximumExtent - this->MinimumExtent + svtkm::Id3{ 1 } };
  svtkm::Vec3f minPt = svtkm::Vec3f(this->MinimumExtent) * this->Spacing;
  svtkm::FloatDefault temp2 = 1.f / (2.f * this->StandardDeviation * this->StandardDeviation);
  svtkm::Vec3f scale{ computeScaleFactor(this->MinimumExtent[0], this->MaximumExtent[0]),
                     computeScaleFactor(this->MinimumExtent[1], this->MaximumExtent[1]),
                     computeScaleFactor(this->MinimumExtent[2], this->MaximumExtent[2]) };

  svtkm::cont::ArrayHandle<svtkm::FloatDefault> output;
  wavelet::WaveletField worklet{ this->Center,
                                 this->Spacing,
                                 this->Frequency,
                                 this->Magnitude,
                                 minPt,
                                 scale,
                                 this->MinimumExtent,
                                 dims,
                                 this->MaximumValue,
                                 temp2 };
  this->Invoke(worklet, cellset, output);
  return svtkm::cont::make_FieldPoint(name, output);
}

} // namespace source
} // namespace svtkm
