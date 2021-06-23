//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <svtkm/CellShape.h>

#include <svtkm/testing/Testing.h>

namespace
{

template <typename T>
void CheckTypeSame(T, T)
{
  std::cout << "  Success" << std::endl;
}

template <typename T1, typename T2>
void CheckTypeSame(T1, T2)
{
  SVTKM_TEST_FAIL("Got unexpected types.");
}

struct CellShapeTestFunctor
{
  template <typename ShapeTag>
  void operator()(ShapeTag) const
  {
    SVTKM_IS_CELL_SHAPE_TAG(ShapeTag);

    const svtkm::IdComponent cellShapeId = ShapeTag::Id;
    std::cout << "Cell shape id: " << cellShapeId << std::endl;

    std::cout << "Check conversion between id and tag is consistent." << std::endl;
    CheckTypeSame(ShapeTag(), typename svtkm::CellShapeIdToTag<cellShapeId>::Tag());

    std::cout << "Check svtkmGenericCellShapeMacro." << std::endl;
    switch (cellShapeId)
    {
      svtkmGenericCellShapeMacro(CheckTypeSame(ShapeTag(), CellShapeTag()));
      default:
        SVTKM_TEST_FAIL("Generic shape switch not working.");
    }
  }
};

void CellShapeTest()
{
  svtkm::testing::Testing::TryAllCellShapes(CellShapeTestFunctor());
}

} // anonymous namespace

int UnitTestCellShape(int argc, char* argv[])
{
  return svtkm::testing::Testing::Run(CellShapeTest, argc, argv);
}
