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
#include <svtkm/rendering/CanvasGL.h>
#include <svtkm/rendering/Color.h>
#include <svtkm/rendering/MapperGL.h>
#include <svtkm/rendering/Scene.h>
#include <svtkm/rendering/View1D.h>
#include <svtkm/rendering/View2D.h>
#include <svtkm/rendering/View3D.h>
#include <svtkm/rendering/testing/RenderTest.h>

// this needs to be included after the svtk-m headers so that we include
// the gl headers in the correct order
#include <GLFW/glfw3.h>

#include <cstring>
#include <string>

namespace
{
static constexpr svtkm::Id WIDTH = 512, HEIGHT = 512;
static svtkm::Id which = 0, NUM_DATASETS = 5;
static bool done = false;
static bool batch = false;

static void keyCallback(GLFWwindow* svtkmNotUsed(window),
                        int key,
                        int svtkmNotUsed(scancode),
                        int action,
                        int svtkmNotUsed(mods))
{
  if (key == GLFW_KEY_ESCAPE)
    done = true;
  if (action == 1)
    which = (which + 1) % NUM_DATASETS;
}

void RenderTests()
{
  std::cout << "Press any key to cycle through datasets. ESC to quit." << std::endl;

  using MapperType = svtkm::rendering::MapperGL;
  using CanvasType = svtkm::rendering::CanvasGL;
  using View3DType = svtkm::rendering::View3D;
  using View2DType = svtkm::rendering::View2D;
  using View1DType = svtkm::rendering::View1D;

  svtkm::cont::DataSetFieldAdd dsf;
  svtkm::cont::testing::MakeTestDataSet maker;
  svtkm::cont::ColorTable colorTable("inferno");

  glfwInit();
  GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "GLFW Test", nullptr, nullptr);
  glfwMakeContextCurrent(window);
  glfwSetKeyCallback(window, keyCallback);

  CanvasType canvas[5] = { CanvasType(512, 512),
                           CanvasType(512, 512),
                           CanvasType(512, 512),
                           CanvasType(512, 512),
                           CanvasType(512, 512) };
  svtkm::rendering::Scene scene[5];
  svtkm::cont::DataSet ds[5];
  MapperType mapper[5];
  svtkm::rendering::Camera camera[5];

  ds[0] = maker.Make3DRegularDataSet0();
  ds[1] = maker.Make3DRectilinearDataSet0();
  ds[2] = maker.Make3DExplicitDataSet4();
  ds[3] = maker.Make2DRectilinearDataSet0();
  //create 1D uniform DS with tiny Y axis
  svtkm::cont::DataSet tinyDS = maker.Make1DUniformDataSet0();
  const std::size_t nVerts = static_cast<std::size_t>(tinyDS.GetField(0).GetNumberOfValues());
  std::vector<svtkm::Float32> vars(nVerts);
  float smallVal = 1.000;
  for (std::size_t i = 0; i < nVerts; i++)
  {
    vars[i] = smallVal;
    smallVal += .01f;
  }
  dsf.AddPointField(tinyDS, "smallScaledXAxis", vars);
  ds[4] = tinyDS;
  tinyDS.PrintSummary(std::cerr);

  std::string fldNames[5];
  fldNames[0] = "pointvar";
  fldNames[1] = "pointvar";
  fldNames[2] = "pointvar";
  fldNames[3] = "pointvar";
  fldNames[4] = "smallScaledXAxis";

  for (int i = 0; i < NUM_DATASETS; i++)
  {
    if (i < 3)
    {
      scene[i].AddActor(svtkm::rendering::Actor(ds[i].GetCellSet(),
                                               ds[i].GetCoordinateSystem(),
                                               ds[i].GetField(fldNames[i].c_str()),
                                               colorTable));
      svtkm::rendering::testing::SetCamera<View3DType>(camera[i],
                                                      ds[i].GetCoordinateSystem().GetBounds());
    }
    else if (i == 3)
    {
      scene[i].AddActor(svtkm::rendering::Actor(ds[i].GetCellSet(),
                                               ds[i].GetCoordinateSystem(),
                                               ds[i].GetField(fldNames[i].c_str()),
                                               colorTable));
      svtkm::rendering::testing::SetCamera<View2DType>(camera[i],
                                                      ds[i].GetCoordinateSystem().GetBounds());
    }
    else
    {
      scene[i].AddActor(svtkm::rendering::Actor(ds[i].GetCellSet(),
                                               ds[i].GetCoordinateSystem(),
                                               ds[i].GetField(fldNames[i].c_str()),
                                               svtkm::rendering::Color::white));
      svtkm::rendering::testing::SetCamera<View1DType>(
        camera[i], ds[i].GetCoordinateSystem().GetBounds(), ds[i].GetField(fldNames[i].c_str()));
    }
  }

  View3DType view3d0(
    scene[0], mapper[0], canvas[0], camera[0], svtkm::rendering::Color(0.2f, 0.2f, 0.2f, 1.0f));
  View3DType view3d1(
    scene[1], mapper[1], canvas[1], camera[1], svtkm::rendering::Color(0.2f, 0.2f, 0.2f, 1.0f));
  View3DType view3d2(
    scene[2], mapper[2], canvas[2], camera[2], svtkm::rendering::Color(0.2f, 0.2f, 0.2f, 1.0f));
  View2DType view2d0(
    scene[3], mapper[3], canvas[3], camera[3], svtkm::rendering::Color(0.2f, 0.2f, 0.2f, 1.0f));
  View1DType view1d0(
    scene[4], mapper[4], canvas[4], camera[4], svtkm::rendering::Color(0.2f, 0.2f, 0.2f, 1.0f));

  while (!glfwWindowShouldClose(window) && !done)
  {
    glfwPollEvents();

    if (which == 0)
      svtkm::rendering::testing::Render<MapperType, CanvasType, View3DType>(view3d0, "reg3D.pnm");
    else if (which == 1)
      svtkm::rendering::testing::Render<MapperType, CanvasType, View3DType>(view3d1, "rect3D.pnm");
    else if (which == 2)
      svtkm::rendering::testing::Render<MapperType, CanvasType, View3DType>(view3d2, "expl3D.pnm");
    else if (which == 3)
      svtkm::rendering::testing::Render<MapperType, CanvasType, View2DType>(view2d0, "rect2D.pnm");
    else if (which == 4)
      svtkm::rendering::testing::Render<MapperType, CanvasType, View1DType>(
        view1d0, "uniform1DSmallScaledXAxis.pnm");
    glfwSwapBuffers(window);

    if (batch)
    {
      which++;
      if (which >= NUM_DATASETS)
      {
        break;
      }
    }
  }

  glfwDestroyWindow(window);
}
} //namespace

int UnitTestMapperGLFW(int argc, char* argv[])
{
  if (argc > 1)
  {
    if (strcmp(argv[1], "-B") == 0)
    {
      batch = true;
    }
  }
  return svtkm::cont::testing::Testing::Run(RenderTests);
}
