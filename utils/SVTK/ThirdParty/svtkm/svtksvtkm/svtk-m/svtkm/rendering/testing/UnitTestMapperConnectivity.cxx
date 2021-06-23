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
#include <svtkm/rendering/Canvas.h>
#include <svtkm/rendering/CanvasRayTracer.h>
#include <svtkm/rendering/MapperConnectivity.h>
#include <svtkm/rendering/Scene.h>
#include <svtkm/rendering/View3D.h>
#include <svtkm/rendering/raytracing/Logger.h>
#include <svtkm/rendering/testing/RenderTest.h>

namespace
{

void RenderTests()
{
  try
  {
    svtkm::cont::testing::MakeTestDataSet maker;
    svtkm::cont::ColorTable colorTable("inferno");
    using M = svtkm::rendering::MapperConnectivity;
    using C = svtkm::rendering::CanvasRayTracer;
    using V3 = svtkm::rendering::View3D;

    svtkm::rendering::testing::Render<M, C, V3>(
      maker.Make3DRegularDataSet0(), "pointvar", colorTable, "reg3D.pnm");
    svtkm::rendering::testing::Render<M, C, V3>(
      maker.Make3DRectilinearDataSet0(), "pointvar", colorTable, "rect3D.pnm");
    svtkm::rendering::testing::Render<M, C, V3>(
      maker.Make3DExplicitDataSetZoo(), "pointvar", colorTable, "explicit3D.pnm");
  }
  catch (const std::exception& e)
  {
    std::cout << svtkm::rendering::raytracing::Logger::GetInstance()->GetStream().str() << "\n";
    std::cout << e.what() << "\n";
  }
}

} //namespace

int UnitTestMapperConnectivity(int argc, char* argv[])
{
  return svtkm::cont::testing::Testing::Run(RenderTests, argc, argv);
}
