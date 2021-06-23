//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <svtkm/cont/ArrayHandle.h>
#include <svtkm/cont/ArrayHandleConstant.h>
#include <svtkm/cont/ArrayHandleIndex.h>
#include <svtkm/cont/CellSetPermutation.h>
#include <svtkm/cont/DataSet.h>
#include <svtkm/cont/DynamicCellSet.h>

#include <svtkm/worklet/DispatcherMapField.h>
#include <svtkm/worklet/WorkletMapField.h>

#include <svtkm/cont/testing/MakeTestDataSet.h>
#include <svtkm/cont/testing/Testing.h>

struct TestWholeCellSetIn
{
  template <typename VisitTopology, typename IncidentTopology>
  struct WholeCellSetWorklet : public svtkm::worklet::WorkletMapField
  {
    using ControlSignature = void(FieldIn indices,
                                  WholeCellSetIn<VisitTopology, IncidentTopology>,
                                  FieldOut numberOfElements,
                                  FieldOut shapes,
                                  FieldOut numberOfindices,
                                  FieldOut connectionSum);
    using ExecutionSignature = void(_1, _2, _3, _4, _5, _6);
    using InputDomain = _1;

    template <typename ConnectivityType>
    SVTKM_EXEC void operator()(svtkm::Id index,
                              const ConnectivityType& connectivity,
                              svtkm::Id& numberOfElements,
                              svtkm::UInt8& shape,
                              svtkm::IdComponent& numberOfIndices,
                              svtkm::Id& connectionSum) const
    {
      numberOfElements = connectivity.GetNumberOfElements();
      shape = connectivity.GetCellShape(index).Id;
      numberOfIndices = connectivity.GetNumberOfIndices(index);

      typename ConnectivityType::IndicesType indices = connectivity.GetIndices(index);
      if (numberOfIndices != indices.GetNumberOfComponents())
      {
        this->RaiseError("Got wrong number of connections.");
      }

      connectionSum = 0;
      for (svtkm::IdComponent componentIndex = 0; componentIndex < indices.GetNumberOfComponents();
           componentIndex++)
      {
        connectionSum += indices[componentIndex];
      }
    }
  };

  template <typename CellSetType>
  SVTKM_CONT static void RunCells(const CellSetType& cellSet,
                                 svtkm::cont::ArrayHandle<svtkm::Id> numberOfElements,
                                 svtkm::cont::ArrayHandle<svtkm::UInt8> shapeIds,
                                 svtkm::cont::ArrayHandle<svtkm::IdComponent> numberOfIndices,
                                 svtkm::cont::ArrayHandle<svtkm::Id> connectionSum)
  {
    using WorkletType =
      WholeCellSetWorklet<svtkm::TopologyElementTagCell, svtkm::TopologyElementTagPoint>;
    svtkm::worklet::DispatcherMapField<WorkletType> dispatcher;
    dispatcher.Invoke(svtkm::cont::ArrayHandleIndex(cellSet.GetNumberOfCells()),
                      cellSet,
                      numberOfElements,
                      shapeIds,
                      numberOfIndices,
                      &connectionSum);
  }

  template <typename CellSetType>
  SVTKM_CONT static void RunPoints(const CellSetType* cellSet,
                                  svtkm::cont::ArrayHandle<svtkm::Id> numberOfElements,
                                  svtkm::cont::ArrayHandle<svtkm::UInt8> shapeIds,
                                  svtkm::cont::ArrayHandle<svtkm::IdComponent> numberOfIndices,
                                  svtkm::cont::ArrayHandle<svtkm::Id> connectionSum)
  {
    using WorkletType =
      WholeCellSetWorklet<svtkm::TopologyElementTagPoint, svtkm::TopologyElementTagCell>;
    svtkm::worklet::DispatcherMapField<WorkletType> dispatcher;
    dispatcher.Invoke(svtkm::cont::ArrayHandleIndex(cellSet->GetNumberOfPoints()),
                      cellSet,
                      numberOfElements,
                      &shapeIds,
                      numberOfIndices,
                      connectionSum);
  }
};

template <typename CellSetType,
          typename ShapeArrayType,
          typename NumIndicesArrayType,
          typename ConnectionSumArrayType>
SVTKM_CONT void TryCellConnectivity(const CellSetType& cellSet,
                                   const ShapeArrayType& expectedShapeIds,
                                   const NumIndicesArrayType& expectedNumberOfIndices,
                                   const ConnectionSumArrayType& expectedSum)
{
  std::cout << "  trying point to cell connectivity" << std::endl;
  svtkm::cont::ArrayHandle<svtkm::Id> numberOfElements;
  svtkm::cont::ArrayHandle<svtkm::UInt8> shapeIds;
  svtkm::cont::ArrayHandle<svtkm::IdComponent> numberOfIndices;
  svtkm::cont::ArrayHandle<svtkm::Id> connectionSum;

  TestWholeCellSetIn::RunCells(cellSet, numberOfElements, shapeIds, numberOfIndices, connectionSum);

  std::cout << "    Number of elements: " << numberOfElements.GetPortalConstControl().Get(0)
            << std::endl;
  SVTKM_TEST_ASSERT(test_equal_portals(numberOfElements.GetPortalConstControl(),
                                      svtkm::cont::make_ArrayHandleConstant(
                                        cellSet.GetNumberOfCells(), cellSet.GetNumberOfCells())
                                        .GetPortalConstControl()),
                   "Incorrect number of elements.");

  std::cout << "    Shape Ids: ";
  svtkm::cont::printSummary_ArrayHandle(shapeIds, std::cout, true);
  SVTKM_TEST_ASSERT(
    test_equal_portals(shapeIds.GetPortalConstControl(), expectedShapeIds.GetPortalConstControl()),
    "Incorrect shape Ids.");

  std::cout << "    Number of indices: ";
  svtkm::cont::printSummary_ArrayHandle(numberOfIndices, std::cout, true);
  SVTKM_TEST_ASSERT(test_equal_portals(numberOfIndices.GetPortalConstControl(),
                                      expectedNumberOfIndices.GetPortalConstControl()),
                   "Incorrect number of indices.");

  std::cout << "    Sum of indices: ";
  svtkm::cont::printSummary_ArrayHandle(connectionSum, std::cout, true);
  SVTKM_TEST_ASSERT(
    test_equal_portals(connectionSum.GetPortalConstControl(), expectedSum.GetPortalConstControl()),
    "Incorrect sum of indices.");
}

template <typename CellSetType,
          typename ShapeArrayType,
          typename NumIndicesArrayType,
          typename ConnectionSumArrayType>
SVTKM_CONT void TryPointConnectivity(const CellSetType& cellSet,
                                    const ShapeArrayType& expectedShapeIds,
                                    const NumIndicesArrayType& expectedNumberOfIndices,
                                    const ConnectionSumArrayType& expectedSum)
{
  std::cout << "  trying cell to point connectivity" << std::endl;
  svtkm::cont::ArrayHandle<svtkm::Id> numberOfElements;
  svtkm::cont::ArrayHandle<svtkm::UInt8> shapeIds;
  svtkm::cont::ArrayHandle<svtkm::IdComponent> numberOfIndices;
  svtkm::cont::ArrayHandle<svtkm::Id> connectionSum;

  TestWholeCellSetIn::RunPoints(
    &cellSet, numberOfElements, shapeIds, numberOfIndices, connectionSum);

  std::cout << "    Number of elements: " << numberOfElements.GetPortalConstControl().Get(0)
            << std::endl;
  SVTKM_TEST_ASSERT(test_equal_portals(numberOfElements.GetPortalConstControl(),
                                      svtkm::cont::make_ArrayHandleConstant(
                                        cellSet.GetNumberOfPoints(), cellSet.GetNumberOfPoints())
                                        .GetPortalConstControl()),
                   "Incorrect number of elements.");

  std::cout << "    Shape Ids: ";
  svtkm::cont::printSummary_ArrayHandle(shapeIds, std::cout, true);
  SVTKM_TEST_ASSERT(
    test_equal_portals(shapeIds.GetPortalConstControl(), expectedShapeIds.GetPortalConstControl()),
    "Incorrect shape Ids.");

  std::cout << "    Number of indices: ";
  svtkm::cont::printSummary_ArrayHandle(numberOfIndices, std::cout, true);
  SVTKM_TEST_ASSERT(test_equal_portals(numberOfIndices.GetPortalConstControl(),
                                      expectedNumberOfIndices.GetPortalConstControl()),
                   "Incorrect number of indices.");

  std::cout << "    Sum of indices: ";
  svtkm::cont::printSummary_ArrayHandle(connectionSum, std::cout, true);
  SVTKM_TEST_ASSERT(
    test_equal_portals(connectionSum.GetPortalConstControl(), expectedSum.GetPortalConstControl()),
    "Incorrect sum of indices.");
}

SVTKM_CONT
void TryExplicitGrid()
{
  std::cout << "Testing explicit grid." << std::endl;
  svtkm::cont::DataSet dataSet = svtkm::cont::testing::MakeTestDataSet().Make3DExplicitDataSet5();
  svtkm::cont::CellSetExplicit<> cellSet;
  dataSet.GetCellSet().CopyTo(cellSet);

  svtkm::UInt8 expectedCellShapes[] = { svtkm::CELL_SHAPE_HEXAHEDRON,
                                       svtkm::CELL_SHAPE_PYRAMID,
                                       svtkm::CELL_SHAPE_TETRA,
                                       svtkm::CELL_SHAPE_WEDGE };

  svtkm::IdComponent expectedCellNumIndices[] = { 8, 5, 4, 6 };

  svtkm::Id expectedCellIndexSum[] = { 28, 22, 29, 41 };

  svtkm::Id numCells = cellSet.GetNumberOfCells();
  TryCellConnectivity(cellSet,
                      svtkm::cont::make_ArrayHandle(expectedCellShapes, numCells),
                      svtkm::cont::make_ArrayHandle(expectedCellNumIndices, numCells),
                      svtkm::cont::make_ArrayHandle(expectedCellIndexSum, numCells));

  svtkm::IdComponent expectedPointNumIndices[] = { 1, 2, 2, 1, 2, 4, 4, 2, 2, 1, 2 };

  svtkm::Id expectedPointIndexSum[] = { 0, 1, 1, 0, 3, 6, 6, 3, 3, 3, 5 };

  svtkm::Id numPoints = cellSet.GetNumberOfPoints();
  TryPointConnectivity(
    cellSet,
    svtkm::cont::make_ArrayHandleConstant(svtkm::CellShapeTagVertex::Id, numPoints),
    svtkm::cont::make_ArrayHandle(expectedPointNumIndices, numPoints),
    svtkm::cont::make_ArrayHandle(expectedPointIndexSum, numPoints));
}

SVTKM_CONT
void TryCellSetPermutation()
{
  std::cout << "Testing permutation grid." << std::endl;
  svtkm::cont::DataSet dataSet = svtkm::cont::testing::MakeTestDataSet().Make3DExplicitDataSet5();
  svtkm::cont::CellSetExplicit<> originalCellSet;
  dataSet.GetCellSet().CopyTo(originalCellSet);

  svtkm::Id permutationArray[] = { 2, 0, 1 };

  svtkm::cont::CellSetPermutation<svtkm::cont::CellSetExplicit<>, svtkm::cont::ArrayHandle<svtkm::Id>>
    cellSet(svtkm::cont::make_ArrayHandle(permutationArray, 3), originalCellSet);

  svtkm::UInt8 expectedCellShapes[] = { svtkm::CELL_SHAPE_TETRA,
                                       svtkm::CELL_SHAPE_HEXAHEDRON,
                                       svtkm::CELL_SHAPE_PYRAMID };

  svtkm::IdComponent expectedCellNumIndices[] = { 4, 8, 5 };

  svtkm::Id expectedCellIndexSum[] = { 29, 28, 22 };

  svtkm::Id numCells = cellSet.GetNumberOfCells();
  TryCellConnectivity(cellSet,
                      svtkm::cont::make_ArrayHandle(expectedCellShapes, numCells),
                      svtkm::cont::make_ArrayHandle(expectedCellNumIndices, numCells),
                      svtkm::cont::make_ArrayHandle(expectedCellIndexSum, numCells));

  // Permutation cell set does not support cell to point connectivity.
}

SVTKM_CONT
void TryStructuredGrid3D()
{
  std::cout << "Testing 3D structured grid." << std::endl;
  svtkm::cont::DataSet dataSet = svtkm::cont::testing::MakeTestDataSet().Make3DUniformDataSet0();
  svtkm::cont::CellSetStructured<3> cellSet;
  dataSet.GetCellSet().CopyTo(cellSet);

  svtkm::Id expectedCellIndexSum[4] = { 40, 48, 88, 96 };

  svtkm::Id numCells = cellSet.GetNumberOfCells();
  TryCellConnectivity(
    cellSet,
    svtkm::cont::ArrayHandleConstant<svtkm::UInt8>(svtkm::CELL_SHAPE_HEXAHEDRON, numCells),
    svtkm::cont::ArrayHandleConstant<svtkm::IdComponent>(8, numCells),
    svtkm::cont::make_ArrayHandle(expectedCellIndexSum, numCells));

  svtkm::IdComponent expectedPointNumIndices[18] = { 1, 2, 1, 1, 2, 1, 2, 4, 2,
                                                    2, 4, 2, 1, 2, 1, 1, 2, 1 };

  svtkm::Id expectedPointIndexSum[18] = { 0, 1, 1, 0, 1, 1, 2, 6, 4, 2, 6, 4, 2, 5, 3, 2, 5, 3 };

  svtkm::Id numPoints = cellSet.GetNumberOfPoints();
  TryPointConnectivity(
    cellSet,
    svtkm::cont::ArrayHandleConstant<svtkm::UInt8>(svtkm::CELL_SHAPE_VERTEX, numPoints),
    svtkm::cont::make_ArrayHandle(expectedPointNumIndices, numPoints),
    svtkm::cont::make_ArrayHandle(expectedPointIndexSum, numPoints));
}

SVTKM_CONT
void TryStructuredGrid2D()
{
  std::cout << "Testing 2D structured grid." << std::endl;
  svtkm::cont::DataSet dataSet = svtkm::cont::testing::MakeTestDataSet().Make2DUniformDataSet0();
  svtkm::cont::CellSetStructured<2> cellSet;
  dataSet.GetCellSet().CopyTo(cellSet);

  svtkm::Id expectedCellIndexSum[2] = { 8, 12 };

  svtkm::Id numCells = cellSet.GetNumberOfCells();
  TryCellConnectivity(cellSet,
                      svtkm::cont::ArrayHandleConstant<svtkm::UInt8>(svtkm::CELL_SHAPE_QUAD, numCells),
                      svtkm::cont::ArrayHandleConstant<svtkm::IdComponent>(4, numCells),
                      svtkm::cont::make_ArrayHandle(expectedCellIndexSum, numCells));

  svtkm::IdComponent expectedPointNumIndices[6] = { 1, 2, 1, 1, 2, 1 };

  svtkm::Id expectedPointIndexSum[6] = { 0, 1, 1, 0, 1, 1 };

  svtkm::Id numPoints = cellSet.GetNumberOfPoints();
  TryPointConnectivity(
    cellSet,
    svtkm::cont::ArrayHandleConstant<svtkm::UInt8>(svtkm::CELL_SHAPE_VERTEX, numPoints),
    svtkm::cont::make_ArrayHandle(expectedPointNumIndices, numPoints),
    svtkm::cont::make_ArrayHandle(expectedPointIndexSum, numPoints));
}

SVTKM_CONT
void TryStructuredGrid1D()
{
  std::cout << "Testing 1D structured grid." << std::endl;
  svtkm::cont::DataSet dataSet = svtkm::cont::testing::MakeTestDataSet().Make1DUniformDataSet0();
  svtkm::cont::CellSetStructured<1> cellSet;
  dataSet.GetCellSet().CopyTo(cellSet);

  svtkm::Id expectedCellIndexSum[5] = { 1, 3, 5, 7, 9 };

  svtkm::Id numCells = cellSet.GetNumberOfCells();
  TryCellConnectivity(cellSet,
                      svtkm::cont::ArrayHandleConstant<svtkm::UInt8>(svtkm::CELL_SHAPE_LINE, numCells),
                      svtkm::cont::ArrayHandleConstant<svtkm::IdComponent>(2, numCells),
                      svtkm::cont::make_ArrayHandle(expectedCellIndexSum, numCells));

  svtkm::IdComponent expectedPointNumIndices[6] = { 1, 2, 2, 2, 2, 1 };

  svtkm::Id expectedPointIndexSum[6] = { 0, 1, 3, 5, 7, 4 };

  svtkm::Id numPoints = cellSet.GetNumberOfPoints();
  TryPointConnectivity(
    cellSet,
    svtkm::cont::ArrayHandleConstant<svtkm::UInt8>(svtkm::CELL_SHAPE_VERTEX, numPoints),
    svtkm::cont::make_ArrayHandle(expectedPointNumIndices, numPoints),
    svtkm::cont::make_ArrayHandle(expectedPointIndexSum, numPoints));
}

SVTKM_CONT
void RunWholeCellSetInTests()
{
  TryExplicitGrid();
  TryCellSetPermutation();
  TryStructuredGrid3D();
  TryStructuredGrid2D();
  TryStructuredGrid1D();
}

int UnitTestWholeCellSetIn(int argc, char* argv[])
{
  return svtkm::cont::testing::Testing::Run(RunWholeCellSetInTests, argc, argv);
}
