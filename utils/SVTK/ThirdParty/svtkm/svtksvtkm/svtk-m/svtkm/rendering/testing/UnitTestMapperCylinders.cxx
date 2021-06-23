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
#include <svtkm/rendering/MapperCylinder.h>
#include <svtkm/rendering/Scene.h>
#include <svtkm/rendering/View3D.h>
#include <svtkm/rendering/testing/RenderTest.h>

namespace
{

void RenderTests()
{
  typedef svtkm::rendering::MapperCylinder M;
  typedef svtkm::rendering::CanvasRayTracer C;
  typedef svtkm::rendering::View3D V3;
  typedef svtkm::rendering::View2D V2;

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
    maker.Make3DExplicitDataSet8(), "cellvar", colorTable, "cylinder.pnm");

  //hexahedron, wedge, pyramid, tetrahedron
  svtkm::rendering::testing::Render<M, C, V3>(
    maker.Make3DExplicitDataSet5(), "cellvar", colorTable, "rt_hex3d.pnm");

  M mapper;

  mapper.SetRadius(0.1f);
  svtkm::rendering::testing::Render<M, C, V3>(
    mapper, maker.Make3DExplicitDataSet8(), "cellvar", colorTable, "cyl_static_radius.pnm");

  mapper.UseVariableRadius(true);
  mapper.SetRadiusDelta(2.0f);
  svtkm::rendering::testing::Render<M, C, V3>(
    mapper, maker.Make3DExplicitDataSet8(), "cellvar", colorTable, "cyl_var_radius.pnm");

  //test to make sure can reset
  mapper.UseVariableRadius(false);
  svtkm::rendering::testing::Render<M, C, V3>(
    maker.Make3DExplicitDataSet8(), "cellvar", colorTable, "cylinder2.pnm");
}

} //namespace

int UnitTestMapperCylinders(int argc, char* argv[])
{
  return svtkm::cont::testing::Testing::Run(RenderTests, argc, argv);
}
