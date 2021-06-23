//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <svtkm/cont/DeviceAdapter.h>
#include <svtkm/cont/testing/MakeTestDataSet.h>
#include <svtkm/cont/testing/Testing.h>
#include <svtkm/rendering/Actor.h>
#include <svtkm/rendering/CanvasRayTracer.h>
#include <svtkm/rendering/MapperRayTracer.h>
#include <svtkm/rendering/Scene.h>
#include <svtkm/rendering/View3D.h>
#include <svtkm/rendering/testing/RenderTest.h>

namespace
{

void RenderTests()
{
  using M = svtkm::rendering::MapperRayTracer;
  using C = svtkm::rendering::CanvasRayTracer;
  using V3 = svtkm::rendering::View3D;
  using V2 = svtkm::rendering::View2D;

  svtkm::cont::testing::MakeTestDataSet maker;
  svtkm::cont::ColorTable colorTable("inferno");

  svtkm::rendering::testing::Render<M, C, V3>(
    maker.Make3DRegularDataSet0(), "pointvar", colorTable, "rt_reg3D.pnm");
  svtkm::rendering::testing::Render<M, C, V3>(
    maker.Make3DRectilinearDataSet0(), "pointvar", colorTable, "rt_rect3D.pnm");
  svtkm::rendering::testing::Render<M, C, V3>(
    maker.Make3DExplicitDataSet4(), "pointvar", colorTable, "rt_expl3D.pnm");

  svtkm::rendering::testing::Render<M, C, V2>(
    maker.Make2DUniformDataSet1(), "pointvar", colorTable, "uni2D.pnm");

  svtkm::rendering::testing::Render<M, C, V3>(
    maker.Make3DExplicitDataSet7(), "cellvar", colorTable, "spheres.pnm");
}

} //namespace

int UnitTestMapperRayTracer(int argc, char* argv[])
{
  return svtkm::cont::testing::Testing::Run(RenderTests, argc, argv);
}
