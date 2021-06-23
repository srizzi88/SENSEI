//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#include <svtkm/Bounds.h>
#include <svtkm/cont/DeviceAdapter.h>
#include <svtkm/cont/testing/MakeTestDataSet.h>
#include <svtkm/cont/testing/Testing.h>
#include <svtkm/rendering/Actor.h>
#include <svtkm/rendering/CanvasOSMesa.h>
#include <svtkm/rendering/Color.h>
#include <svtkm/rendering/MapperGL.h>
#include <svtkm/rendering/Scene.h>
#include <svtkm/rendering/View2D.h>
#include <svtkm/rendering/View3D.h>
#include <svtkm/rendering/testing/RenderTest.h>

namespace
{

void RenderTests()
{
  using M = svtkm::rendering::MapperGL;
  using C = svtkm::rendering::CanvasOSMesa;
  using V3 = svtkm::rendering::View3D;
  using V2 = svtkm::rendering::View2D;
  using V1 = svtkm::rendering::View1D;

  svtkm::cont::testing::MakeTestDataSet maker;
  svtkm::cont::ColorTable colorTable("inferno");

  svtkm::rendering::testing::Render<M, C, V3>(
    maker.Make3DRegularDataSet0(), "pointvar", colorTable, "reg3D.pnm");
  svtkm::rendering::testing::Render<M, C, V3>(
    maker.Make3DRectilinearDataSet0(), "pointvar", colorTable, "rect3D.pnm");
  svtkm::rendering::testing::Render<M, C, V3>(
    maker.Make3DExplicitDataSet4(), "pointvar", colorTable, "expl3D.pnm");
  svtkm::rendering::testing::Render<M, C, V2>(
    maker.Make2DRectilinearDataSet0(), "pointvar", colorTable, "rect2D.pnm");
  svtkm::rendering::testing::Render<M, C, V1>(
    maker.Make1DUniformDataSet0(), "pointvar", svtkm::rendering::Color(1, 1, 1, 1), "uniform1D.pnm");
  svtkm::rendering::testing::Render<M, C, V1>(
    maker.Make1DExplicitDataSet0(), "pointvar", svtkm::rendering::Color(1, 1, 1, 1), "expl1D.pnm");
}

} //namespace

int UnitTestMapperOSMesa(int argc, char* argv[])
{
  return svtkm::cont::testing::Testing::Run(RenderTests, argc, argv);
}
