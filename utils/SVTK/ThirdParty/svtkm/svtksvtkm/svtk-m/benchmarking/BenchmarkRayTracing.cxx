//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include "Benchmarker.h"

#include <svtkm/TypeTraits.h>

#include <svtkm/cont/ArrayHandle.h>
#include <svtkm/cont/DeviceAdapterAlgorithm.h>
#include <svtkm/cont/Initialize.h>
#include <svtkm/cont/Timer.h>
#include <svtkm/cont/testing/MakeTestDataSet.h>

#include <svtkm/rendering/Camera.h>
#include <svtkm/rendering/raytracing/Ray.h>
#include <svtkm/rendering/raytracing/RayTracer.h>
#include <svtkm/rendering/raytracing/SphereIntersector.h>
#include <svtkm/rendering/raytracing/TriangleExtractor.h>

#include <svtkm/exec/FunctorBase.h>

#include <svtkm/cont/ColorTable.hxx>

#include <sstream>
#include <string>
#include <vector>

namespace
{

// Hold configuration state (e.g. active device)
svtkm::cont::InitializeResult Config;

void BenchRayTracing(::benchmark::State& state)
{
  const svtkm::Id3 dims(128, 128, 128);

  svtkm::cont::testing::MakeTestDataSet maker;
  auto dataset = maker.Make3DUniformDataSet3(dims);
  auto coords = dataset.GetCoordinateSystem();

  svtkm::rendering::Camera camera;
  svtkm::Bounds bounds = dataset.GetCoordinateSystem().GetBounds();
  camera.ResetToBounds(bounds);

  svtkm::cont::DynamicCellSet cellset = dataset.GetCellSet();

  svtkm::rendering::raytracing::TriangleExtractor triExtractor;
  triExtractor.ExtractCells(cellset);

  auto triIntersector = std::make_shared<svtkm::rendering::raytracing::TriangleIntersector>(
    svtkm::rendering::raytracing::TriangleIntersector());

  svtkm::rendering::raytracing::RayTracer tracer;
  triIntersector->SetData(coords, triExtractor.GetTriangles());
  tracer.AddShapeIntersector(triIntersector);

  svtkm::rendering::CanvasRayTracer canvas(1920, 1080);
  svtkm::rendering::raytracing::Camera rayCamera;
  rayCamera.SetParameters(camera, canvas);
  svtkm::rendering::raytracing::Ray<svtkm::Float32> rays;
  rayCamera.CreateRays(rays, coords.GetBounds());

  rays.Buffers.at(0).InitConst(0.f);

  svtkm::cont::Field field = dataset.GetField("pointvar");
  svtkm::Range range = field.GetRange().GetPortalConstControl().Get(0);

  tracer.SetField(field, range);

  svtkm::cont::ArrayHandle<svtkm::Vec4ui_8> temp;
  svtkm::cont::ColorTable table("cool to warm");
  table.Sample(100, temp);

  svtkm::cont::ArrayHandle<svtkm::Vec4f_32> colors;
  colors.Allocate(100);
  auto portal = colors.GetPortalControl();
  auto colorPortal = temp.GetPortalConstControl();
  constexpr svtkm::Float32 conversionToFloatSpace = (1.0f / 255.0f);
  for (svtkm::Id i = 0; i < 100; ++i)
  {
    auto color = colorPortal.Get(i);
    svtkm::Vec4f_32 t(color[0] * conversionToFloatSpace,
                     color[1] * conversionToFloatSpace,
                     color[2] * conversionToFloatSpace,
                     color[3] * conversionToFloatSpace);
    portal.Set(i, t);
  }

  tracer.SetColorMap(colors);
  tracer.Render(rays);

  svtkm::cont::Timer timer{ Config.Device };
  for (auto _ : state)
  {
    (void)_;
    timer.Start();
    rayCamera.CreateRays(rays, coords.GetBounds());
    tracer.Render(rays);
    timer.Stop();

    state.SetIterationTime(timer.GetElapsedTime());
  }
}

SVTKM_BENCHMARK(BenchRayTracing);

} // end namespace svtkm::benchmarking

int main(int argc, char* argv[])
{
  // Parse SVTK-m options:
  auto opts = svtkm::cont::InitializeOptions::RequireDevice | svtkm::cont::InitializeOptions::AddHelp;
  Config = svtkm::cont::Initialize(argc, argv, opts);

  // Setup device:
  svtkm::cont::GetRuntimeDeviceTracker().ForceDevice(Config.Device);

  // handle benchmarking related args and run benchmarks:
  SVTKM_EXECUTE_BENCHMARKS(argc, argv);
}
