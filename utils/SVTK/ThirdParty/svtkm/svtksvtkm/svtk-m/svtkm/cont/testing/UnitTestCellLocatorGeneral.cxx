//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#include <svtkm/cont/CellLocatorGeneral.h>

#include <svtkm/cont/ArrayHandle.h>
#include <svtkm/cont/DataSetBuilderRectilinear.h>
#include <svtkm/cont/DataSetBuilderUniform.h>
#include <svtkm/cont/testing/Testing.h>
#include <svtkm/exec/CellInterpolate.h>
#include <svtkm/worklet/DispatcherMapField.h>
#include <svtkm/worklet/DispatcherMapTopology.h>
#include <svtkm/worklet/ScatterPermutation.h>
#include <svtkm/worklet/WorkletMapField.h>
#include <svtkm/worklet/WorkletMapTopology.h>

#include <ctime>
#include <random>

namespace
{

std::default_random_engine RandomGenerator;

using PointType = svtkm::Vec3f;

//-----------------------------------------------------------------------------
svtkm::cont::DataSet MakeTestDataSetUniform()
{
  return svtkm::cont::DataSetBuilderUniform::Create(
    svtkm::Id3{ 64 }, PointType{ -32.0f }, PointType{ 1.0f / 64.0f });
}

svtkm::cont::DataSet MakeTestDataSetRectilinear()
{
  std::uniform_real_distribution<svtkm::FloatDefault> coordGen(1.0f / 128.0f, 1.0f / 32.0f);

  svtkm::cont::ArrayHandle<svtkm::FloatDefault> coords[3];
  for (int i = 0; i < 3; ++i)
  {
    coords[i].Allocate(64);
    auto portal = coords[i].GetPortalControl();

    svtkm::FloatDefault cur = 0.0f;
    for (svtkm::Id j = 0; j < portal.GetNumberOfValues(); ++j)
    {
      cur += coordGen(RandomGenerator);
      portal.Set(j, cur);
    }
  }

  return svtkm::cont::DataSetBuilderRectilinear::Create(coords[0], coords[1], coords[2]);
}

svtkm::cont::DataSet MakeTestDataSetCurvilinear()
{
  auto recti = MakeTestDataSetRectilinear();
  auto coords = recti.GetCoordinateSystem().GetData();

  svtkm::cont::ArrayHandle<PointType> sheared;
  sheared.Allocate(coords.GetNumberOfValues());

  auto inPortal = coords.GetPortalConstControl();
  auto outPortal = sheared.GetPortalControl();
  for (svtkm::Id i = 0; i < inPortal.GetNumberOfValues(); ++i)
  {
    auto val = inPortal.Get(i);
    outPortal.Set(i, val + svtkm::make_Vec(val[1], val[2], val[0]));
  }

  svtkm::cont::DataSet curvi;
  curvi.SetCellSet(recti.GetCellSet());
  curvi.AddCoordinateSystem(svtkm::cont::CoordinateSystem("coords", sheared));

  return curvi;
}

//-----------------------------------------------------------------------------
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
    wc = svtkm::exec::CellInterpolate(points, pc, cellShape, *this);
  }
};

void GenerateRandomInput(const svtkm::cont::DataSet& ds,
                         svtkm::Id count,
                         svtkm::cont::ArrayHandle<svtkm::Id>& cellIds,
                         svtkm::cont::ArrayHandle<PointType>& pcoords,
                         svtkm::cont::ArrayHandle<PointType>& wcoords)
{
  svtkm::Id numberOfCells = ds.GetNumberOfCells();

  std::uniform_int_distribution<svtkm::Id> cellIdGen(0, numberOfCells - 1);
  std::uniform_real_distribution<svtkm::FloatDefault> pcoordGen(0.0f, 1.0f);

  cellIds.Allocate(count);
  pcoords.Allocate(count);
  wcoords.Allocate(count);

  for (svtkm::Id i = 0; i < count; ++i)
  {
    cellIds.GetPortalControl().Set(i, cellIdGen(RandomGenerator));

    PointType pc{ pcoordGen(RandomGenerator),
                  pcoordGen(RandomGenerator),
                  pcoordGen(RandomGenerator) };
    pcoords.GetPortalControl().Set(i, pc);
  }

  svtkm::worklet::DispatcherMapTopology<ParametricToWorldCoordinates> dispatcher(
    ParametricToWorldCoordinates::MakeScatter(cellIds));
  dispatcher.Invoke(ds.GetCellSet(), ds.GetCoordinateSystem().GetData(), pcoords, wcoords);
}

//-----------------------------------------------------------------------------
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

void TestWithDataSet(svtkm::cont::CellLocatorGeneral& locator, const svtkm::cont::DataSet& dataset)
{
  locator.SetCellSet(dataset.GetCellSet());
  locator.SetCoordinates(dataset.GetCoordinateSystem());
  locator.Update();

  const svtkm::cont::CellLocator& curLoc = *locator.GetCurrentLocator();
  std::cout << "using locator: " << typeid(curLoc).name() << "\n";

  svtkm::cont::ArrayHandle<svtkm::Id> expCellIds;
  svtkm::cont::ArrayHandle<PointType> expPCoords;
  svtkm::cont::ArrayHandle<PointType> points;
  GenerateRandomInput(dataset, 128, expCellIds, expPCoords, points);

  svtkm::cont::ArrayHandle<svtkm::Id> cellIds;
  svtkm::cont::ArrayHandle<PointType> pcoords;

  svtkm::worklet::DispatcherMapField<FindCellWorklet> dispatcher;
  // CellLocatorGeneral is non-copyable. Pass it via a pointer.
  dispatcher.Invoke(points, &locator, cellIds, pcoords);

  for (svtkm::Id i = 0; i < 128; ++i)
  {
    SVTKM_TEST_ASSERT(cellIds.GetPortalConstControl().Get(i) ==
                       expCellIds.GetPortalConstControl().Get(i),
                     "Incorrect cell ids");
    SVTKM_TEST_ASSERT(test_equal(pcoords.GetPortalConstControl().Get(i),
                                expPCoords.GetPortalConstControl().Get(i),
                                1e-3),
                     "Incorrect parameteric coordinates");
  }

  std::cout << "Passed.\n";
}

void TestCellLocatorGeneral()
{
  svtkm::cont::CellLocatorGeneral locator;

  std::cout << "Test UniformGrid dataset\n";
  TestWithDataSet(locator, MakeTestDataSetUniform());

  std::cout << "Test Rectilinear dataset\n";
  TestWithDataSet(locator, MakeTestDataSetRectilinear());

  std::cout << "Test Curvilinear dataset\n";
  TestWithDataSet(locator, MakeTestDataSetCurvilinear());
}

} // anonymous namespace

int UnitTestCellLocatorGeneral(int argc, char* argv[])
{
  svtkm::cont::GetRuntimeDeviceTracker().ForceDevice(svtkm::cont::DeviceAdapterTagSerial{});
  return svtkm::cont::testing::Testing::Run(TestCellLocatorGeneral, argc, argv);
}
