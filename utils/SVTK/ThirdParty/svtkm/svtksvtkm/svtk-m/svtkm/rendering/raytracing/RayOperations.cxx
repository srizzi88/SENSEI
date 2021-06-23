//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#include <svtkm/rendering/raytracing/RayOperations.h>
namespace svtkm
{
namespace rendering
{
namespace raytracing
{

void RayOperations::MapCanvasToRays(Ray<svtkm::Float32>& rays,
                                    const svtkm::rendering::Camera& camera,
                                    const svtkm::rendering::CanvasRayTracer& canvas)
{
  svtkm::Id width = canvas.GetWidth();
  svtkm::Id height = canvas.GetHeight();
  svtkm::Matrix<svtkm::Float32, 4, 4> projview =
    svtkm::MatrixMultiply(camera.CreateProjectionMatrix(width, height), camera.CreateViewMatrix());
  bool valid;
  svtkm::Matrix<svtkm::Float32, 4, 4> inverse = svtkm::MatrixInverse(projview, valid);
  (void)valid; // this can be a false negative for really tiny spatial domains.
  svtkm::worklet::DispatcherMapField<detail::RayMapCanvas>(
    detail::RayMapCanvas(inverse, width, height, camera.GetPosition()))
    .Invoke(rays.PixelIdx, rays.MaxDistance, canvas.GetDepthBuffer());
}
}
}
} // svtkm::rendering::raytacing
