//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <svtkm/cont/ArrayCopy.h>
#include <svtkm/source/Tangle.h>

namespace svtkm
{
namespace source
{
namespace tangle
{
class TangleField : public svtkm::worklet::WorkletVisitPointsWithCells
{
public:
  using ControlSignature = void(CellSetIn, FieldOut v);
  using ExecutionSignature = void(ThreadIndices, _2);
  using InputDomain = _1;

  const svtkm::Vec3f CellDimsf;
  const svtkm::Vec3f Mins;
  const svtkm::Vec3f Maxs;

  SVTKM_CONT
  TangleField(const svtkm::Id3& cdims, const svtkm::Vec3f& mins, const svtkm::Vec3f& maxs)
    : CellDimsf(static_cast<svtkm::FloatDefault>(cdims[0]),
                static_cast<svtkm::FloatDefault>(cdims[1]),
                static_cast<svtkm::FloatDefault>(cdims[2]))
    , Mins(mins)
    , Maxs(maxs)
  {
  }

  template <typename ThreadIndexType>
  SVTKM_EXEC void operator()(const ThreadIndexType& threadIndex, svtkm::Float32& v) const
  {
    //We are operating on a 3d structured grid. This means that the threadIndex has
    //efficiently computed the i,j,k of the point current point for us
    const svtkm::Id3 ijk = threadIndex.GetInputIndex3D();
    const svtkm::Vec3f xyzf = static_cast<svtkm::Vec3f>(ijk) / this->CellDimsf;

    const svtkm::Vec3f_32 values = 3.0f * svtkm::Vec3f_32(Mins + (Maxs - Mins) * xyzf);
    const svtkm::Float32& xx = values[0];
    const svtkm::Float32& yy = values[1];
    const svtkm::Float32& zz = values[2];

    v = (xx * xx * xx * xx - 5.0f * xx * xx + yy * yy * yy * yy - 5.0f * yy * yy +
         zz * zz * zz * zz - 5.0f * zz * zz + 11.8f) *
        0.2f +
      0.5f;
  }
};
} // namespace tangle

svtkm::cont::DataSet Tangle::Execute() const
{
  SVTKM_LOG_SCOPE_FUNCTION(svtkm::cont::LogLevel::Perf);

  svtkm::cont::DataSet dataSet;

  const svtkm::Id3 pdims{ this->Dims + svtkm::Id3{ 1, 1, 1 } };
  const svtkm::Vec3f mins = { -1.0f, -1.0f, -1.0f };
  const svtkm::Vec3f maxs = { 1.0f, 1.0f, 1.0f };

  svtkm::cont::CellSetStructured<3> cellSet;
  cellSet.SetPointDimensions(pdims);
  dataSet.SetCellSet(cellSet);

  svtkm::cont::ArrayHandle<svtkm::Float32> pointFieldArray;
  this->Invoke(tangle::TangleField{ this->Dims, mins, maxs }, cellSet, pointFieldArray);

  svtkm::cont::ArrayHandle<svtkm::FloatDefault> cellFieldArray;
  svtkm::cont::ArrayCopy(
    svtkm::cont::make_ArrayHandleCounting<svtkm::Id>(0, 1, cellSet.GetNumberOfCells()),
    cellFieldArray);

  const svtkm::Vec3f origin(0.0f, 0.0f, 0.0f);
  const svtkm::Vec3f spacing(1.0f / static_cast<svtkm::FloatDefault>(this->Dims[0]),
                            1.0f / static_cast<svtkm::FloatDefault>(this->Dims[1]),
                            1.0f / static_cast<svtkm::FloatDefault>(this->Dims[2]));

  svtkm::cont::ArrayHandleUniformPointCoordinates coordinates(pdims, origin, spacing);
  dataSet.AddCoordinateSystem(svtkm::cont::CoordinateSystem("coordinates", coordinates));
  dataSet.AddField(svtkm::cont::make_FieldPoint("nodevar", pointFieldArray));
  dataSet.AddField(svtkm::cont::make_FieldCell("cellvar", cellFieldArray));

  return dataSet;
}

} // namespace source
} // namespace svtkm
