//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <svtkm/cont/Algorithm.h>
#include <svtkm/cont/ArrayHandleConcatenate.h>
#include <svtkm/cont/CellLocatorBoundingIntervalHierarchy.h>
#include <svtkm/cont/DataSetBuilderUniform.h>
#include <svtkm/cont/Invoker.h>
#include <svtkm/cont/Timer.h>
#include <svtkm/cont/testing/Testing.h>
#include <svtkm/exec/CellInterpolate.h>
#include <svtkm/exec/ParametricCoordinates.h>
#include <svtkm/io/reader/SVTKDataSetReader.h>

namespace
{
struct CellCentroidCalculator : public svtkm::worklet::WorkletVisitCellsWithPoints
{
  typedef void ControlSignature(CellSetIn, FieldInPoint, FieldOut);
  typedef _3 ExecutionSignature(_1, PointCount, _2);

  template <typename CellShape, typename InputPointField>
  SVTKM_EXEC typename InputPointField::ComponentType operator()(
    CellShape shape,
    svtkm::IdComponent numPoints,
    const InputPointField& inputPointField) const
  {
    svtkm::Vec3f parametricCenter = svtkm::exec::ParametricCoordinatesCenter(numPoints, shape, *this);
    return svtkm::exec::CellInterpolate(inputPointField, parametricCenter, shape, *this);
  }
}; // struct CellCentroidCalculator

struct BoundingIntervalHierarchyTester : public svtkm::worklet::WorkletMapField
{
  typedef void ControlSignature(FieldIn, ExecObject, FieldIn, FieldOut);
  typedef _4 ExecutionSignature(_1, _2, _3);

  template <typename Point, typename BoundingIntervalHierarchyExecObject>
  SVTKM_EXEC svtkm::IdComponent operator()(const Point& point,
                                         const BoundingIntervalHierarchyExecObject& bih,
                                         const svtkm::Id expectedId) const
  {
    svtkm::Vec3f parametric;
    svtkm::Id cellId;
    bih->FindCell(point, cellId, parametric, *this);
    return (1 - static_cast<svtkm::IdComponent>(expectedId == cellId));
  }
}; // struct BoundingIntervalHierarchyTester

svtkm::cont::DataSet ConstructDataSet(svtkm::Id size)
{
  return svtkm::cont::DataSetBuilderUniform().Create(svtkm::Id3(size, size, size));
}

void TestBoundingIntervalHierarchy(svtkm::cont::DataSet dataSet, svtkm::IdComponent numPlanes)
{
  using Timer = svtkm::cont::Timer;

  svtkm::cont::DynamicCellSet cellSet = dataSet.GetCellSet();
  svtkm::cont::ArrayHandleVirtualCoordinates vertices = dataSet.GetCoordinateSystem().GetData();

  std::cout << "Using numPlanes: " << numPlanes << "\n";
  std::cout << "Building Bounding Interval Hierarchy Tree" << std::endl;
  svtkm::cont::CellLocatorBoundingIntervalHierarchy bih =
    svtkm::cont::CellLocatorBoundingIntervalHierarchy(numPlanes, 5);
  bih.SetCellSet(cellSet);
  bih.SetCoordinates(dataSet.GetCoordinateSystem());
  bih.Update();
  std::cout << "Built Bounding Interval Hierarchy Tree" << std::endl;

  Timer centroidsTimer;
  centroidsTimer.Start();
  svtkm::cont::ArrayHandle<svtkm::Vec3f> centroids;
  svtkm::worklet::DispatcherMapTopology<CellCentroidCalculator>().Invoke(
    cellSet, vertices, centroids);
  centroidsTimer.Stop();
  std::cout << "Centroids calculation time: " << centroidsTimer.GetElapsedTime() << "\n";

  svtkm::cont::ArrayHandleCounting<svtkm::Id> expectedCellIds(0, 1, cellSet.GetNumberOfCells());

  Timer interpolationTimer;
  interpolationTimer.Start();
  svtkm::cont::ArrayHandle<svtkm::IdComponent> results;

  svtkm::worklet::DispatcherMapField<BoundingIntervalHierarchyTester>().Invoke(
    centroids, bih, expectedCellIds, results);

  svtkm::Id numDiffs = svtkm::cont::Algorithm::Reduce(results, 0, svtkm::Add());
  interpolationTimer.Stop();
  svtkm::Float64 timeDiff = interpolationTimer.GetElapsedTime();
  std::cout << "No of interpolations: " << results.GetNumberOfValues() << "\n";
  std::cout << "Interpolation time: " << timeDiff << "\n";
  std::cout << "Average interpolation rate: "
            << (static_cast<svtkm::Float64>(results.GetNumberOfValues()) / timeDiff) << "\n";
  std::cout << "No of diffs: " << numDiffs << "\n";
  SVTKM_TEST_ASSERT(numDiffs == 0, "Calculated cell Ids not the same as expected cell Ids");
}

void RunTest()
{
//If this test is run on a machine that already has heavy
//cpu usage it will fail, so we limit the number of threads
//to avoid the test timing out
#ifdef SVTKM_ENABLE_OPENMP
  omp_set_num_threads(std::min(4, omp_get_max_threads()));
#endif

  TestBoundingIntervalHierarchy(ConstructDataSet(16), 3);
  TestBoundingIntervalHierarchy(ConstructDataSet(16), 4);
  TestBoundingIntervalHierarchy(ConstructDataSet(16), 6);
  TestBoundingIntervalHierarchy(ConstructDataSet(16), 9);
}

} // anonymous namespace

int UnitTestBoundingIntervalHierarchy(int argc, char* argv[])
{
  return svtkm::cont::testing::Testing::Run(RunTest, argc, argv);
}
