//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <svtkm/cont/Initialize.h>
#include <svtkm/cont/testing/MakeTestDataSet.h>

#include <svtkm/rendering/Actor.h>
#include <svtkm/rendering/CanvasRayTracer.h>
#include <svtkm/rendering/MapperRayTracer.h>
#include <svtkm/rendering/Scene.h>
#include <svtkm/rendering/View3D.h>

#include <svtkm/filter/Contour.h>

// This example creates a simple data set and uses SVTK-m's rendering engine to render an image and
// write that image to a file. It then computes an isosurface on the input data set and renders
// this output data set in a separate image file

int main(int argc, char* argv[])
{
  svtkm::cont::Initialize(argc, argv, svtkm::cont::InitializeOptions::Strict);

  // Input variable declarations
  svtkm::cont::testing::MakeTestDataSet maker;
  svtkm::cont::DataSet inputData = maker.Make3DUniformDataSet0();
  svtkm::Float32 isovalue = 100.0f;
  std::string fieldName = "pointvar";

  using Mapper = svtkm::rendering::MapperRayTracer;
  using Canvas = svtkm::rendering::CanvasRayTracer;

  // Set up a camera for rendering the input data
  const svtkm::cont::CoordinateSystem coords = inputData.GetCoordinateSystem();
  Mapper mapper;
  svtkm::rendering::Camera camera;

  //Set3DView
  svtkm::Bounds coordsBounds = coords.GetBounds();

  camera.ResetToBounds(coordsBounds);

  svtkm::Vec3f_32 totalExtent;
  totalExtent[0] = svtkm::Float32(coordsBounds.X.Length());
  totalExtent[1] = svtkm::Float32(coordsBounds.Y.Length());
  totalExtent[2] = svtkm::Float32(coordsBounds.Z.Length());
  svtkm::Float32 mag = svtkm::Magnitude(totalExtent);
  svtkm::Normalize(totalExtent);
  camera.SetLookAt(totalExtent * (mag * .5f));
  camera.SetViewUp(svtkm::make_Vec(0.f, 1.f, 0.f));
  camera.SetClippingRange(1.f, 100.f);
  camera.SetFieldOfView(60.f);
  camera.SetPosition(totalExtent * (mag * 2.f));
  svtkm::cont::ColorTable colorTable("inferno");

  // Create a scene for rendering the input data
  svtkm::rendering::Scene scene;
  svtkm::rendering::Color bg(0.2f, 0.2f, 0.2f, 1.0f);
  Canvas canvas(512, 512);

  svtkm::rendering::Actor actor(inputData.GetCellSet(),
                               inputData.GetCoordinateSystem(),
                               inputData.GetField(fieldName),
                               colorTable);
  scene.AddActor(actor);
  // Create a view and use it to render the input data using OS Mesa
  svtkm::rendering::View3D view(scene, mapper, canvas, camera, bg);
  view.Initialize();
  view.Paint();
  view.SaveAs("demo_input.pnm");

  // Create an isosurface filter
  svtkm::filter::Contour filter;
  filter.SetGenerateNormals(false);
  filter.SetMergeDuplicatePoints(false);
  filter.SetIsoValue(0, isovalue);
  filter.SetActiveField(fieldName);
  svtkm::cont::DataSet outputData = filter.Execute(inputData);
  // Render a separate image with the output isosurface
  std::cout << "about to render the results of the Contour filter" << std::endl;
  svtkm::rendering::Scene scene2;
  svtkm::rendering::Actor actor2(outputData.GetCellSet(),
                                outputData.GetCoordinateSystem(),
                                outputData.GetField(fieldName),
                                colorTable);
  // By default, the actor will automatically scale the scalar range of the color table to match
  // that of the data. However, we are coloring by the scalar that we just extracted a contour
  // from, so we want the scalar range to match that of the previous image.
  actor2.SetScalarRange(actor.GetScalarRange());
  scene2.AddActor(actor2);

  svtkm::rendering::View3D view2(scene2, mapper, canvas, camera, bg);
  view2.Initialize();
  view2.Paint();
  view2.SaveAs("demo_output.pnm");

  return 0;
}
