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
#include <svtkm/cont/ArrayHandle.h>
#include <svtkm/cont/ArrayHandleConstant.h>
#include <svtkm/cont/ArrayHandleCounting.h>
#include <svtkm/cont/ArrayHandleImplicit.h>
#include <svtkm/cont/CellSet.h>
#include <svtkm/cont/CellSetExplicit.h>
#include <svtkm/cont/CellSetPermutation.h>
#include <svtkm/cont/CellSetSingleType.h>
#include <svtkm/cont/CellSetStructured.h>
#include <svtkm/cont/testing/Testing.h>

namespace
{

constexpr svtkm::Id xdim = 3, ydim = 5, zdim = 7;
constexpr svtkm::Id3 BaseLinePointDimensions{ xdim, ydim, zdim };
constexpr svtkm::Id BaseLineNumberOfPoints = xdim * ydim * zdim;
constexpr svtkm::Id BaseLineNumberOfCells = (xdim - 1) * (ydim - 1) * (zdim - 1);

svtkm::cont::CellSetStructured<3> BaseLine;

void InitializeBaseLine()
{
  BaseLine.SetPointDimensions(BaseLinePointDimensions);
}

class BaseLineConnectivityFunctor
{
public:
  explicit BaseLineConnectivityFunctor()
  {
    this->Structure.SetPointDimensions(BaseLinePointDimensions);
  }

  SVTKM_EXEC_CONT
  svtkm::Id operator()(svtkm::Id idx) const
  {
    auto i = idx / this->Structure.NUM_POINTS_IN_CELL;
    auto c = static_cast<svtkm::IdComponent>(idx % this->Structure.NUM_POINTS_IN_CELL);
    return this->Structure.GetPointsOfCell(i)[c];
  }

private:
  svtkm::internal::ConnectivityStructuredInternals<3> Structure;
};

using BaseLineConnectivityType = svtkm::cont::ArrayHandleImplicit<BaseLineConnectivityFunctor>;
BaseLineConnectivityType BaseLineConnectivity(BaseLineConnectivityFunctor{},
                                              BaseLineNumberOfCells * 8);

auto PermutationArray = svtkm::cont::ArrayHandleCounting<svtkm::Id>(0, 2, BaseLineNumberOfCells / 2);

//-----------------------------------------------------------------------------
svtkm::cont::CellSetExplicit<> MakeCellSetExplicit()
{
  svtkm::cont::ArrayHandle<svtkm::UInt8> shapes;
  svtkm::cont::ArrayCopy(svtkm::cont::ArrayHandleConstant<svtkm::UInt8>{ svtkm::CELL_SHAPE_HEXAHEDRON,
                                                                      BaseLineNumberOfCells },
                        shapes);

  svtkm::cont::ArrayHandle<svtkm::IdComponent> numIndices;
  svtkm::cont::ArrayCopy(
    svtkm::cont::ArrayHandleConstant<svtkm::IdComponent>{ 8, BaseLineNumberOfCells }, numIndices);

  svtkm::cont::ArrayHandle<svtkm::Id> connectivity;
  svtkm::cont::ArrayCopy(BaseLineConnectivity, connectivity);

  auto offsets = svtkm::cont::ConvertNumIndicesToOffsets(numIndices);

  svtkm::cont::CellSetExplicit<> cellset;
  cellset.Fill(BaseLineNumberOfPoints, shapes, connectivity, offsets);
  return cellset;
}

svtkm::cont::CellSetSingleType<typename BaseLineConnectivityType::StorageTag> MakeCellSetSingleType()
{
  svtkm::cont::CellSetSingleType<typename BaseLineConnectivityType::StorageTag> cellset;
  cellset.Fill(BaseLineNumberOfPoints, svtkm::CELL_SHAPE_HEXAHEDRON, 8, BaseLineConnectivity);
  return cellset;
}

svtkm::cont::CellSetStructured<3> MakeCellSetStructured()
{
  svtkm::cont::CellSetStructured<3> cellset;
  cellset.SetPointDimensions(BaseLinePointDimensions);
  return cellset;
}

//-----------------------------------------------------------------------------
enum class IsPermutationCellSet
{
  NO = 0,
  YES = 1
};

void TestAgainstBaseLine(const svtkm::cont::CellSet& cellset,
                         IsPermutationCellSet flag = IsPermutationCellSet::NO)
{
  svtkm::internal::ConnectivityStructuredInternals<3> baseLineStructure;
  baseLineStructure.SetPointDimensions(BaseLinePointDimensions);

  SVTKM_TEST_ASSERT(cellset.GetNumberOfPoints() == BaseLineNumberOfPoints, "Wrong number of points");

  svtkm::Id numCells = cellset.GetNumberOfCells();
  svtkm::Id expectedNumCell = (flag == IsPermutationCellSet::NO)
    ? BaseLineNumberOfCells
    : PermutationArray.GetNumberOfValues();
  SVTKM_TEST_ASSERT(numCells == expectedNumCell, "Wrong number of cells");

  for (svtkm::Id i = 0; i < numCells; ++i)
  {
    SVTKM_TEST_ASSERT(cellset.GetCellShape(i) == svtkm::CELL_SHAPE_HEXAHEDRON, "Wrong shape");
    SVTKM_TEST_ASSERT(cellset.GetNumberOfPointsInCell(i) == 8, "Wrong number of points-of-cell");

    svtkm::Id baseLineCellId =
      (flag == IsPermutationCellSet::YES) ? PermutationArray.GetPortalConstControl().Get(i) : i;
    auto baseLinePointIds = baseLineStructure.GetPointsOfCell(baseLineCellId);

    svtkm::Id pointIds[8];
    cellset.GetCellPointIds(i, pointIds);
    for (int j = 0; j < 8; ++j)
    {
      SVTKM_TEST_ASSERT(pointIds[j] == baseLinePointIds[j], "Wrong points-of-cell point id");
    }
  }
}

void RunTests(const svtkm::cont::CellSet& cellset,
              IsPermutationCellSet flag = IsPermutationCellSet::NO)
{
  TestAgainstBaseLine(cellset, flag);
  auto deepcopy = cellset.NewInstance();
  deepcopy->DeepCopy(&cellset);
  TestAgainstBaseLine(*deepcopy, flag);
}

void TestCellSet()
{
  InitializeBaseLine();

  std::cout << "Testing CellSetExplicit\n";
  auto csExplicit = MakeCellSetExplicit();
  RunTests(csExplicit);
  std::cout << "Testing CellSetPermutation of CellSetExplicit\n";
  RunTests(svtkm::cont::make_CellSetPermutation(PermutationArray, csExplicit),
           IsPermutationCellSet::YES);

  std::cout << "Testing CellSetSingleType\n";
  auto csSingle = MakeCellSetSingleType();
  RunTests(csSingle);
  std::cout << "Testing CellSetPermutation of CellSetSingleType\n";
  RunTests(svtkm::cont::make_CellSetPermutation(PermutationArray, csSingle),
           IsPermutationCellSet::YES);

  std::cout << "Testing CellSetStructured\n";
  auto csStructured = MakeCellSetStructured();
  RunTests(csStructured);
  std::cout << "Testing CellSetPermutation of CellSetStructured\n";
  RunTests(svtkm::cont::make_CellSetPermutation(PermutationArray, csStructured),
           IsPermutationCellSet::YES);
}

} // anonymous namespace

//-----------------------------------------------------------------------------
int UnitTestCellSet(int argc, char* argv[])
{
  return svtkm::cont::testing::Testing::Run(TestCellSet, argc, argv);
}
