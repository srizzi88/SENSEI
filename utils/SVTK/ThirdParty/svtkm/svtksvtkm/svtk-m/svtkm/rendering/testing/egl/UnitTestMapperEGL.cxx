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
#include <svtkm/cont/DataSetFieldAdd.h>
#include <svtkm/cont/DeviceAdapter.h>
#include <svtkm/cont/testing/MakeTestDataSet.h>
#include <svtkm/cont/testing/Testing.h>
#include <svtkm/rendering/Actor.h>
#include <svtkm/rendering/CanvasEGL.h>
#include <svtkm/rendering/Color.h>
#include <svtkm/rendering/MapperGL.h>
#include <svtkm/rendering/Scene.h>
#include <svtkm/rendering/View1D.h>
#include <svtkm/rendering/View2D.h>
#include <svtkm/rendering/View3D.h>
#include <svtkm/rendering/testing/RenderTest.h>

namespace
{

void RenderTests()
{
  using M = svtkm::rendering::MapperGL;
  using C = svtkm::rendering::CanvasEGL;
  using V3 = svtkm::rendering::View3D;
  using V2 = svtkm::rendering::View2D;
  using V1 = svtkm::rendering::View1D;

  svtkm::cont::DataSetFieldAdd dsf;
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
    maker.Make1DUniformDataSet0(), "pointvar", svtkm::rendering::Color::white, "uniform1D.pnm");
  svtkm::rendering::testing::Render<M, C, V1>(
    maker.Make1DExplicitDataSet0(), "pointvar", svtkm::rendering::Color::white, "expl1D.pnm");

  svtkm::cont::DataSet ds = maker.Make1DUniformDataSet0();
  svtkm::Int32 nVerts = ds.GetField(0).GetNumberOfValues();
  svtkm::Float32 vars[nVerts];
  svtkm::Float32 smallVal = 1.000;
  for (svtkm::Int32 i = 0; i <= nVerts; i++)
  {
    vars[i] = smallVal;
    smallVal += .01;
  }
  dsf.AddPointField(ds, "smallScaledYAxis", vars, nVerts);
  svtkm::rendering::testing::Render<M, C, V1>(
    ds, "smallScaledYAxis", svtkm::rendering::Color::white, "uniform1DSmallScaledYAxis.pnm");

  // Test to demonstrate that straight horizontal lines can be drawn
  ds = maker.Make1DUniformDataSet0();
  nVerts = ds.GetField(0).GetNumberOfValues();
  svtkm::Float32 largeVal = 1e-16;
  for (svtkm::Int32 i = 0; i <= nVerts; i++)
  {
    vars[i] = largeVal;
  }
  dsf.AddPointField(ds, "straightLine", vars, nVerts);
  svtkm::rendering::testing::Render<M, C, V1>(
    ds, "straightLine", svtkm::rendering::Color::white, "uniform1DStraightLine.pnm");


  ds = maker.Make1DUniformDataSet0();
  nVerts = ds.GetField(0).GetNumberOfValues();
  largeVal = 1;
  for (svtkm::Int32 i = 0; i <= nVerts; i++)
  {
    vars[i] = largeVal;
    if (i < 2)
    {
      largeVal *= 100;
    }
    else
    {
      largeVal /= 2.25;
    }
  }
  dsf.AddPointField(ds, "logScaledYAxis", vars, nVerts);
  svtkm::rendering::testing::Render<M, C, V1>(
    ds, "logScaledYAxis", svtkm::rendering::Color::white, "uniform1DLogScaledYAxis.pnm", true);
}
} //namespace

int UnitTestMapperEGL(int argc, char* argv[])
{
  return svtkm::cont::testing::Testing::Run(RenderTests, argc, argv);
}
