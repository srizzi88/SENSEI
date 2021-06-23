//=============================================================================
//
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2019 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2019 UT-Battelle, LLC.
//  Copyright 2019 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//
//=============================================================================

#include <svtkm/worklet/TriangleWinding.h>

#include <svtkm/Types.h>

#include <svtkm/cont/Algorithm.h>
#include <svtkm/cont/ArrayHandle.h>
#include <svtkm/cont/ArrayHandleGroupVecVariable.h>
#include <svtkm/cont/DataSet.h>
#include <svtkm/cont/Field.h>

#include <svtkm/cont/testing/MakeTestDataSet.h>
#include <svtkm/cont/testing/Testing.h>

using MyNormalT = svtkm::Vec<svtkm::Float32, 3>;

namespace
{

svtkm::cont::DataSet GenerateDataSet()
{
  auto ds = svtkm::cont::testing::MakeTestDataSet{}.Make3DExplicitDataSetPolygonal();
  const auto numCells = ds.GetNumberOfCells();

  svtkm::cont::ArrayHandle<MyNormalT> cellNormals;
  svtkm::cont::Algorithm::Fill(cellNormals, MyNormalT{ 1., 0., 0. }, numCells);

  ds.AddField(svtkm::cont::make_FieldCell("normals", cellNormals));
  return ds;
}

void Validate(svtkm::cont::DataSet dataSet)
{
  const auto cellSet = dataSet.GetCellSet().Cast<svtkm::cont::CellSetExplicit<>>();
  const auto coordsArray = dataSet.GetCoordinateSystem().GetData();
  const auto conn =
    cellSet.GetConnectivityArray(svtkm::TopologyElementTagCell{}, svtkm::TopologyElementTagPoint{});
  const auto offsets =
    cellSet.GetOffsetsArray(svtkm::TopologyElementTagCell{}, svtkm::TopologyElementTagPoint{});
  const auto offsetsTrim =
    svtkm::cont::make_ArrayHandleView(offsets, 0, offsets.GetNumberOfValues() - 1);
  const auto cellArray = svtkm::cont::make_ArrayHandleGroupVecVariable(conn, offsetsTrim);
  const auto cellNormalsVar = dataSet.GetCellField("normals").GetData();
  const auto cellNormalsArray = cellNormalsVar.Cast<svtkm::cont::ArrayHandle<MyNormalT>>();

  const auto cellPortal = cellArray.GetPortalConstControl();
  const auto cellNormals = cellNormalsArray.GetPortalConstControl();
  const auto coords = coordsArray.GetPortalConstControl();

  const auto numCells = cellPortal.GetNumberOfValues();
  SVTKM_TEST_ASSERT(numCells == cellNormals.GetNumberOfValues());

  for (svtkm::Id cellId = 0; cellId < numCells; ++cellId)
  {
    const auto cell = cellPortal.Get(cellId);
    if (cell.GetNumberOfComponents() != 3)
    { // Triangles only!
      continue;
    }

    const MyNormalT cellNormal = cellNormals.Get(cellId);
    const MyNormalT p0 = coords.Get(cell[0]);
    const MyNormalT p1 = coords.Get(cell[1]);
    const MyNormalT p2 = coords.Get(cell[2]);
    const MyNormalT v01 = p1 - p0;
    const MyNormalT v02 = p2 - p0;
    const MyNormalT triangleNormal = svtkm::Cross(v01, v02);
    SVTKM_TEST_ASSERT(svtkm::Dot(triangleNormal, cellNormal) > 0,
                     "Triangle at index ",
                     cellId,
                     " incorrectly wound.");
  }
}

void DoTest()
{
  auto ds = GenerateDataSet();

  // Ensure that the test dataset needs to be rewound:
  bool threw = false;
  try
  {
    std::cerr << "Expecting an exception...\n";
    Validate(ds);
  }
  catch (...)
  {
    threw = true;
  }

  SVTKM_TEST_ASSERT(threw, "Test dataset is already wound consistently wrt normals.");

  auto cellSet = ds.GetCellSet().Cast<svtkm::cont::CellSetExplicit<>>();
  const auto coords = ds.GetCoordinateSystem().GetData();
  const auto cellNormalsVar = ds.GetCellField("normals").GetData();
  const auto cellNormals = cellNormalsVar.Cast<svtkm::cont::ArrayHandle<MyNormalT>>();


  auto newCells = svtkm::worklet::TriangleWinding::Run(cellSet, coords, cellNormals);

  svtkm::cont::DataSet result;
  result.AddCoordinateSystem(ds.GetCoordinateSystem());
  result.SetCellSet(newCells);
  for (svtkm::Id i = 0; i < ds.GetNumberOfFields(); ++i)
  {
    result.AddField(ds.GetField(i));
  }

  Validate(result);
}

} // end anon namespace

int UnitTestTriangleWinding(int argc, char* argv[])
{
  return svtkm::cont::testing::Testing::Run(DoTest, argc, argv);
}
