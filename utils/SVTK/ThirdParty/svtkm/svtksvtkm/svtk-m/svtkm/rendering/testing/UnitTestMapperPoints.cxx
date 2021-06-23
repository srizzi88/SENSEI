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
#include <svtkm/rendering/MapperPoint.h>
#include <svtkm/rendering/Scene.h>
#include <svtkm/rendering/View3D.h>
#include <svtkm/rendering/testing/RenderTest.h>

namespace
{

void RenderTests()
{
  using M = svtkm::rendering::MapperPoint;
  using C = svtkm::rendering::CanvasRayTracer;
  using V3 = svtkm::rendering::View3D;

  svtkm::cont::testing::MakeTestDataSet maker;
  svtkm::cont::ColorTable colorTable("inferno");

  M mapper;
  std::cout << "Testing uniform delta raduis\n";
  mapper.SetRadiusDelta(4.0f);
  svtkm::rendering::testing::Render<M, C, V3>(
    mapper, maker.Make3DUniformDataSet1(), "pointvar", colorTable, "points_vr_reg3D.pnm");

  // restore defaults
  mapper.SetRadiusDelta(0.5f);
  mapper.UseVariableRadius(false);

  mapper.SetRadius(0.2f);
  svtkm::rendering::testing::Render<M, C, V3>(
    mapper, maker.Make3DUniformDataSet1(), "pointvar", colorTable, "points_reg3D.pnm");

  mapper.UseCells();
  mapper.SetRadius(1.f);
  svtkm::rendering::testing::Render<M, C, V3>(
    mapper, maker.Make3DExplicitDataSet7(), "cellvar", colorTable, "spheres.pnm");
}

} //namespace

int UnitTestMapperPoints(int argc, char* argv[])
{
  return svtkm::cont::testing::Testing::Run(RenderTests, argc, argv);
}
