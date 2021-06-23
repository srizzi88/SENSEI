//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_cont_testing_TestingCellLocatorUniformBins_h
#define svtk_m_cont_testing_TestingCellLocatorUniformBins_h

#include <svtkm/cont/ArrayCopy.h>
#include <svtkm/cont/CellLocatorUniformBins.h>
#include <svtkm/cont/DataSetBuilderUniform.h>
#include <svtkm/cont/testing/Testing.h>

#include <svtkm/exec/ParametricCoordinates.h>

#include <svtkm/worklet/DispatcherMapField.h>
#include <svtkm/worklet/DispatcherMapTopology.h>
#include <svtkm/worklet/ScatterPermutation.h>
#include <svtkm/worklet/Tetrahedralize.h>
#include <svtkm/worklet/Triangulate.h>
#include <svtkm/worklet/WorkletMapField.h>
#include <svtkm/worklet/WorkletMapTopology.h>

#include <svtkm/CellShape.h>

#include <ctime>
#include <random>

namespace
{

using PointType = svtkm::Vec3f;

std::default_random_engine RandomGenerator;

class ParametricToWorldCoordinates : public svtkm::worklet::WorkletVisitCellsWithPoints
{
public:
  using ControlSignature = void(CellSetIn cellset,
                                FieldInPoint coords,
                                FieldInOutCell pcs,
                                FieldOutCell wcs);
  using ExecutionSignature = void(CellShape, _2, _3, _4);

  using ScatterType = svtkm::worklet::ScatterPermutation<>;

  SVTKM_CONT
  static ScatterType MakeScatter(const svtkm::cont::ArrayHandle<svtkm::Id>& cellIds)
  {
    return ScatterType(cellIds);
  }

  template <typename CellShapeTagType, typename PointsVecType>
  SVTKM_EXEC void operator()(CellShapeTagType cellShape,
                            PointsVecType points,
                            const PointType& pc,
                            PointType& wc) const
  {
    wc = svtkm::exec::ParametricCoordinatesToWorldCoordinates(points, pc, cellShape, *this);
  }
};

template <svtkm::IdComponent DIMENSIONS>
svtkm::cont::DataSet MakeTestDataSet(const svtkm::Vec<svtkm::Id, DIMENSIONS>& dims)
{
  auto uniformDs =
    svtkm::cont::DataSetBuilderUniform::Create(dims,
                                              svtkm::Vec<svtkm::FloatDefault, DIMENSIONS>(0.0f),
                                              svtkm::Vec<svtkm::FloatDefault, DIMENSIONS>(1.0f));

  // copy points
  svtkm::cont::ArrayHandle<PointType> points;
  svtkm::cont::ArrayCopy(uniformDs.GetCoordinateSystem().GetData(), points);

  auto uniformCs =
    uniformDs.GetCellSet().template Cast<svtkm::cont::CellSetStructured<DIMENSIONS>>();

  // triangulate the cellset
  svtkm::cont::CellSetSingleType<> cellset;
  switch (DIMENSIONS)
  {
    case 2:
      cellset = svtkm::worklet::Triangulate().Run(uniformCs);
      break;
    case 3:
      cellset = svtkm::worklet::Tetrahedralize().Run(uniformCs);
      break;
    default:
      SVTKM_ASSERT(false);
  }

  // Warp the coordinates
  std::uniform_real_distribution<svtkm::FloatDefault> warpFactor(-0.10f, 0.10f);
  auto pointsPortal = points.GetPortalControl();
  for (svtkm::Id i = 0; i < pointsPortal.GetNumberOfValues(); ++i)
  {
    PointType warpVec(0);
    for (svtkm::IdComponent c = 0; c < DIMENSIONS; ++c)
    {
      warpVec[c] = warpFactor(RandomGenerator);
    }
    pointsPortal.Set(i, pointsPortal.Get(i) + warpVec);
  }

  // build dataset
  svtkm::cont::DataSet out;
  out.AddCoordinateSystem(svtkm::cont::CoordinateSystem("coords", points));
  out.SetCellSet(cellset);
  return out;
}

template <svtkm::IdComponent DIMENSIONS>
void GenerateRandomInput(const svtkm::cont::DataSet& ds,
                         svtkm::Id count,
                         svtkm::cont::ArrayHandle<svtkm::Id>& cellIds,
                         svtkm::cont::ArrayHandle<PointType>& pcoords,
                         svtkm::cont::ArrayHandle<PointType>& wcoords)
{
  svtkm::Id numberOfCells = ds.GetNumberOfCells();

  std::uniform_int_distribution<svtkm::Id> cellIdGen(0, numberOfCells - 1);

  cellIds.Allocate(count);
  pcoords.Allocate(count);
  wcoords.Allocate(count);

  for (svtkm::Id i = 0; i < count; ++i)
  {
    cellIds.GetPortalControl().Set(i, cellIdGen(RandomGenerator));

    PointType pc(0.0f);
    svtkm::FloatDefault minPc = 1e-2f;
    svtkm::FloatDefault sum = 0.0f;
    for (svtkm::IdComponent c = 0; c < DIMENSIONS; ++c)
    {
      svtkm::FloatDefault maxPc =
        1.0f - (static_cast<svtkm::FloatDefault>(DIMENSIONS - c) * minPc) - sum;
      std::uniform_real_distribution<svtkm::FloatDefault> pcoordGen(minPc, maxPc);
      pc[c] = pcoordGen(RandomGenerator);
      sum += pc[c];
    }
    pcoords.GetPortalControl().Set(i, pc);
  }

  svtkm::worklet::DispatcherMapTopology<ParametricToWorldCoordinates> dispatcher(
    ParametricToWorldCoordinates::MakeScatter(cellIds));
  dispatcher.Invoke(ds.GetCellSet(), ds.GetCoordinateSystem().GetData(), pcoords, wcoords);
}

class FindCellWorklet : public svtkm::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn points,
                                ExecObject locator,
                                FieldOut cellIds,
                                FieldOut pcoords);
  using ExecutionSignature = void(_1, _2, _3, _4);

  template <typename LocatorType>
  SVTKM_EXEC void operator()(const svtkm::Vec3f& point,
                            const LocatorType& locator,
                            svtkm::Id& cellId,
                            svtkm::Vec3f& pcoords) const
  {
    locator->FindCell(point, cellId, pcoords, *this);
  }
};

template <svtkm::IdComponent DIMENSIONS>
void TestCellLocator(const svtkm::Vec<svtkm::Id, DIMENSIONS>& dim, svtkm::Id numberOfPoints)
{
  auto ds = MakeTestDataSet(dim);

  std::cout << "Testing " << DIMENSIONS << "D dataset with " << ds.GetNumberOfCells() << " cells\n";

  svtkm::cont::CellLocatorUniformBins locator;
  locator.SetDensityL1(64.0f);
  locator.SetDensityL2(1.0f);
  locator.SetCellSet(ds.GetCellSet());
  locator.SetCoordinates(ds.GetCoordinateSystem());
  locator.Update();

  svtkm::cont::ArrayHandle<svtkm::Id> expCellIds;
  svtkm::cont::ArrayHandle<PointType> expPCoords;
  svtkm::cont::ArrayHandle<PointType> points;
  GenerateRandomInput<DIMENSIONS>(ds, numberOfPoints, expCellIds, expPCoords, points);

  std::cout << "Finding cells for " << numberOfPoints << " points\n";
  svtkm::cont::ArrayHandle<svtkm::Id> cellIds;
  svtkm::cont::ArrayHandle<PointType> pcoords;

  svtkm::worklet::DispatcherMapField<FindCellWorklet> dispatcher;
  dispatcher.Invoke(points, locator, cellIds, pcoords);

  for (svtkm::Id i = 0; i < numberOfPoints; ++i)
  {
    SVTKM_TEST_ASSERT(cellIds.GetPortalConstControl().Get(i) ==
                       expCellIds.GetPortalConstControl().Get(i),
                     "Incorrect cell ids");
    SVTKM_TEST_ASSERT(test_equal(pcoords.GetPortalConstControl().Get(i),
                                expPCoords.GetPortalConstControl().Get(i),
                                1e-3),
                     "Incorrect parameteric coordinates");
  }
}

} // anonymous

template <typename DeviceAdapter>
void TestingCellLocatorUniformBins()
{
  svtkm::cont::GetRuntimeDeviceTracker().ForceDevice(DeviceAdapter());

  svtkm::UInt32 seed = static_cast<svtkm::UInt32>(std::time(nullptr));
  std::cout << "Seed: " << seed << std::endl;
  RandomGenerator.seed(seed);

  TestCellLocator(svtkm::Id3(8), 512);  // 3D dataset
  TestCellLocator(svtkm::Id2(18), 512); // 2D dataset
}

#endif // svtk_m_cont_testing_TestingCellLocatorUniformBins_h
