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
#include <svtkm/rendering/MapperRayTracer.h>
#include <svtkm/rendering/MapperVolume.h>
#include <svtkm/rendering/Scene.h>
#include <svtkm/rendering/View3D.h>
#include <svtkm/rendering/testing/RenderTest.h>

#include <svtkm/rendering/raytracing/RayOperations.h>

namespace
{

void RenderTests()
{
  using M1 = svtkm::rendering::MapperVolume;
  using M2 = svtkm::rendering::MapperConnectivity;
  using R = svtkm::rendering::MapperRayTracer;
  using C = svtkm::rendering::CanvasRayTracer;
  using V3 = svtkm::rendering::View3D;

  svtkm::cont::testing::MakeTestDataSet maker;
  svtkm::cont::ColorTable colorTable("inferno");


  svtkm::cont::ColorTable colorTable2("cool to warm");
  colorTable2.AddPointAlpha(0.0, .02f);
  colorTable2.AddPointAlpha(1.0, .02f);

  svtkm::rendering::testing::MultiMapperRender<R, M2, C, V3>(maker.Make3DExplicitDataSetPolygonal(),
                                                            maker.Make3DRectilinearDataSet0(),
                                                            "pointvar",
                                                            colorTable,
                                                            colorTable2,
                                                            "multi1.pnm");

  svtkm::rendering::testing::MultiMapperRender<R, M1, C, V3>(maker.Make3DExplicitDataSet4(),
                                                            maker.Make3DRectilinearDataSet0(),
                                                            "pointvar",
                                                            colorTable,
                                                            colorTable2,
                                                            "multi2.pnm");
}

} //namespace

int UnitTestMultiMapper(int argc, char* argv[])
{
  return svtkm::cont::testing::Testing::Run(RenderTests, argc, argv);
}
