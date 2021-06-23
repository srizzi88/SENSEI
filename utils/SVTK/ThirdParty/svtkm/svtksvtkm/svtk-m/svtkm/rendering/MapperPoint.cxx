//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <svtkm/rendering/MapperPoint.h>

#include <svtkm/cont/Timer.h>

#include <svtkm/rendering/CanvasRayTracer.h>
#include <svtkm/rendering/internal/RunTriangulator.h>
#include <svtkm/rendering/raytracing/Camera.h>
#include <svtkm/rendering/raytracing/Logger.h>
#include <svtkm/rendering/raytracing/RayOperations.h>
#include <svtkm/rendering/raytracing/RayTracer.h>
#include <svtkm/rendering/raytracing/SphereExtractor.h>
#include <svtkm/rendering/raytracing/SphereIntersector.h>

namespace svtkm
{
namespace rendering
{

struct MapperPoint::InternalsType
{
  svtkm::rendering::CanvasRayTracer* Canvas;
  svtkm::rendering::raytracing::RayTracer Tracer;
  svtkm::rendering::raytracing::Camera RayCamera;
  svtkm::rendering::raytracing::Ray<svtkm::Float32> Rays;
  bool CompositeBackground;
  svtkm::Float32 PointRadius;
  bool UseNodes;
  svtkm::Float32 PointDelta;
  bool UseVariableRadius;

  SVTKM_CONT
  InternalsType()
    : Canvas(nullptr)
    , CompositeBackground(true)
    , PointRadius(-1.f)
    , UseNodes(true)
    , PointDelta(0.5f)
    , UseVariableRadius(false)
  {
  }
};

MapperPoint::MapperPoint()
  : Internals(new InternalsType)
{
}

MapperPoint::~MapperPoint()
{
}

void MapperPoint::SetCanvas(svtkm::rendering::Canvas* canvas)
{
  if (canvas != nullptr)
  {
    this->Internals->Canvas = dynamic_cast<CanvasRayTracer*>(canvas);
    if (this->Internals->Canvas == nullptr)
    {
      throw svtkm::cont::ErrorBadValue("MapperPoint: bad canvas type. Must be CanvasRayTracer");
    }
  }
  else
  {
    this->Internals->Canvas = nullptr;
  }
}

svtkm::rendering::Canvas* MapperPoint::GetCanvas() const
{
  return this->Internals->Canvas;
}

void MapperPoint::UseCells()
{
  this->Internals->UseNodes = false;
}
void MapperPoint::UseNodes()
{
  this->Internals->UseNodes = true;
}

void MapperPoint::SetRadius(const svtkm::Float32& radius)
{
  if (radius <= 0.f)
  {
    throw svtkm::cont::ErrorBadValue("MapperPoint: point radius must be positive");
  }
  this->Internals->PointRadius = radius;
}

void MapperPoint::SetRadiusDelta(const svtkm::Float32& delta)
{
  this->Internals->PointDelta = delta;
}

void MapperPoint::UseVariableRadius(bool useVariableRadius)
{
  this->Internals->UseVariableRadius = useVariableRadius;
}

void MapperPoint::RenderCells(const svtkm::cont::DynamicCellSet& cellset,
                              const svtkm::cont::CoordinateSystem& coords,
                              const svtkm::cont::Field& scalarField,
                              const svtkm::cont::ColorTable& svtkmNotUsed(colorTable),
                              const svtkm::rendering::Camera& camera,
                              const svtkm::Range& scalarRange)
{
  raytracing::Logger* logger = raytracing::Logger::GetInstance();

  // make sure we start fresh
  this->Internals->Tracer.Clear();

  logger->OpenLogEntry("mapper_ray_tracer");
  svtkm::cont::Timer tot_timer;
  tot_timer.Start();
  svtkm::cont::Timer timer;

  svtkm::Bounds coordBounds = coords.GetBounds();
  svtkm::Float32 baseRadius = this->Internals->PointRadius;
  if (baseRadius == -1.f)
  {
    // set a default radius
    svtkm::Float64 lx = coordBounds.X.Length();
    svtkm::Float64 ly = coordBounds.Y.Length();
    svtkm::Float64 lz = coordBounds.Z.Length();
    svtkm::Float64 mag = svtkm::Sqrt(lx * lx + ly * ly + lz * lz);
    // same as used in svtk ospray
    constexpr svtkm::Float64 heuristic = 500.;
    baseRadius = static_cast<svtkm::Float32>(mag / heuristic);
  }

  svtkm::Bounds shapeBounds;

  raytracing::SphereExtractor sphereExtractor;

  if (this->Internals->UseVariableRadius)
  {
    svtkm::Float32 minRadius = baseRadius - baseRadius * this->Internals->PointDelta;
    svtkm::Float32 maxRadius = baseRadius + baseRadius * this->Internals->PointDelta;
    if (this->Internals->UseNodes)
    {

      sphereExtractor.ExtractCoordinates(coords, scalarField, minRadius, maxRadius);
    }
    else
    {
      sphereExtractor.ExtractCells(cellset, scalarField, minRadius, maxRadius);
    }
  }
  else
  {
    if (this->Internals->UseNodes)
    {

      sphereExtractor.ExtractCoordinates(coords, baseRadius);
    }
    else
    {
      sphereExtractor.ExtractCells(cellset, baseRadius);
    }
  }

  if (sphereExtractor.GetNumberOfSpheres() > 0)
  {
    auto sphereIntersector = std::make_shared<raytracing::SphereIntersector>();
    sphereIntersector->SetData(coords, sphereExtractor.GetPointIds(), sphereExtractor.GetRadii());
    this->Internals->Tracer.AddShapeIntersector(sphereIntersector);
    shapeBounds.Include(sphereIntersector->GetShapeBounds());
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

void MapperPoint::SetCompositeBackground(bool on)
{
  this->Internals->CompositeBackground = on;
}

void MapperPoint::StartScene()
{
  // Nothing needs to be done.
}

void MapperPoint::EndScene()
{
  // Nothing needs to be done.
}

svtkm::rendering::Mapper* MapperPoint::NewCopy() const
{
  return new svtkm::rendering::MapperPoint(*this);
}
}
} // svtkm::rendering
