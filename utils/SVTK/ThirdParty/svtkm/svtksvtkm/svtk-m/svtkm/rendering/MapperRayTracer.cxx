//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <svtkm/rendering/MapperRayTracer.h>

#include <svtkm/cont/Timer.h>
#include <svtkm/cont/TryExecute.h>

#include <svtkm/rendering/CanvasRayTracer.h>
#include <svtkm/rendering/internal/RunTriangulator.h>
#include <svtkm/rendering/raytracing/Camera.h>
#include <svtkm/rendering/raytracing/Logger.h>
#include <svtkm/rendering/raytracing/RayOperations.h>
#include <svtkm/rendering/raytracing/RayTracer.h>
#include <svtkm/rendering/raytracing/SphereExtractor.h>
#include <svtkm/rendering/raytracing/SphereIntersector.h>
#include <svtkm/rendering/raytracing/TriangleExtractor.h>

namespace svtkm
{
namespace rendering
{

struct MapperRayTracer::InternalsType
{
  svtkm::rendering::CanvasRayTracer* Canvas;
  svtkm::rendering::raytracing::RayTracer Tracer;
  svtkm::rendering::raytracing::Camera RayCamera;
  svtkm::rendering::raytracing::Ray<svtkm::Float32> Rays;
  bool CompositeBackground;
  bool Shade;
  SVTKM_CONT
  InternalsType()
    : Canvas(nullptr)
    , CompositeBackground(true)
    , Shade(true)
  {
  }
};

MapperRayTracer::MapperRayTracer()
  : Internals(new InternalsType)
{
}

MapperRayTracer::~MapperRayTracer()
{
}

void MapperRayTracer::SetCanvas(svtkm::rendering::Canvas* canvas)
{
  if (canvas != nullptr)
  {
    this->Internals->Canvas = dynamic_cast<CanvasRayTracer*>(canvas);
    if (this->Internals->Canvas == nullptr)
    {
      throw svtkm::cont::ErrorBadValue("Ray Tracer: bad canvas type. Must be CanvasRayTracer");
    }
  }
  else
  {
    this->Internals->Canvas = nullptr;
  }
}

svtkm::rendering::Canvas* MapperRayTracer::GetCanvas() const
{
  return this->Internals->Canvas;
}

void MapperRayTracer::RenderCells(const svtkm::cont::DynamicCellSet& cellset,
                                  const svtkm::cont::CoordinateSystem& coords,
                                  const svtkm::cont::Field& scalarField,
                                  const svtkm::cont::ColorTable& svtkmNotUsed(colorTable),
                                  const svtkm::rendering::Camera& camera,
                                  const svtkm::Range& scalarRange)
{
  raytracing::Logger* logger = raytracing::Logger::GetInstance();
  logger->OpenLogEntry("mapper_ray_tracer");
  svtkm::cont::Timer tot_timer;
  tot_timer.Start();
  svtkm::cont::Timer timer;

  // make sure we start fresh
  this->Internals->Tracer.Clear();
  //
  // Add supported shapes
  //
  svtkm::Bounds shapeBounds;
  raytracing::TriangleExtractor triExtractor;
  triExtractor.ExtractCells(cellset);
  if (triExtractor.GetNumberOfTriangles() > 0)
  {
    auto triIntersector = std::make_shared<raytracing::TriangleIntersector>();
    triIntersector->SetData(coords, triExtractor.GetTriangles());
    this->Internals->Tracer.AddShapeIntersector(triIntersector);
    shapeBounds.Include(triIntersector->GetShapeBounds());
  }

  //
  // Create rays
  //
  svtkm::rendering::raytracing::Camera& cam = this->Internals->Tracer.GetCamera();
  cam.SetParameters(camera, *this->Internals->Canvas);
  this->Internals->RayCamera.SetParameters(camera, *this->Internals->Canvas);

  this->Internals->RayCamera.CreateRays(this->Internals->Rays, shapeBounds);
  this->Internals->Rays.Buffers.at(0).InitConst(0.f);
  raytracing::RayOperations::MapCanvasToRays(
    this->Internals->Rays, camera, *this->Internals->Canvas);



  this->Internals->Tracer.SetField(scalarField, scalarRange);

  this->Internals->Tracer.SetColorMap(this->ColorMap);
  this->Internals->Tracer.SetShadingOn(this->Internals->Shade);
  this->Internals->Tracer.Render(this->Internals->Rays);

  timer.Start();
  this->Internals->Canvas->WriteToCanvas(
    this->Internals->Rays, this->Internals->Rays.Buffers.at(0).Buffer, camera);

  if (this->Internals->CompositeBackground)
  {
    this->Internals->Canvas->BlendBackground();
  }

  svtkm::Float64 time = timer.GetElapsedTime();
  logger->AddLogData("write_to_canvas", time);
  time = tot_timer.GetElapsedTime();
  logger->CloseLogEntry(time);
}

void MapperRayTracer::SetCompositeBackground(bool on)
{
  this->Internals->CompositeBackground = on;
}

void MapperRayTracer::SetShadingOn(bool on)
{
  this->Internals->Shade = on;
}

void MapperRayTracer::StartScene()
{
  // Nothing needs to be done.
}

void MapperRayTracer::EndScene()
{
  // Nothing needs to be done.
}

svtkm::rendering::Mapper* MapperRayTracer::NewCopy() const
{
  return new svtkm::rendering::MapperRayTracer(*this);
}
}
} // svtkm::rendering
