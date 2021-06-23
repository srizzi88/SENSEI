//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <svtkm/rendering/MapperVolume.h>

#include <svtkm/cont/Timer.h>
#include <svtkm/cont/TryExecute.h>

#include <svtkm/rendering/CanvasRayTracer.h>

#include <svtkm/rendering/raytracing/Camera.h>
#include <svtkm/rendering/raytracing/Logger.h>
#include <svtkm/rendering/raytracing/RayOperations.h>
#include <svtkm/rendering/raytracing/VolumeRendererStructured.h>

#include <sstream>

#define DEFAULT_SAMPLE_DISTANCE -1.f

namespace svtkm
{
namespace rendering
{

struct MapperVolume::InternalsType
{
  svtkm::rendering::CanvasRayTracer* Canvas;
  svtkm::Float32 SampleDistance;
  bool CompositeBackground;

  SVTKM_CONT
  InternalsType()
    : Canvas(nullptr)
    , SampleDistance(DEFAULT_SAMPLE_DISTANCE)
    , CompositeBackground(true)
  {
  }
};

MapperVolume::MapperVolume()
  : Internals(new InternalsType)
{
}

MapperVolume::~MapperVolume()
{
}

void MapperVolume::SetCanvas(svtkm::rendering::Canvas* canvas)
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

svtkm::rendering::Canvas* MapperVolume::GetCanvas() const
{
  return this->Internals->Canvas;
}

void MapperVolume::RenderCells(const svtkm::cont::DynamicCellSet& cellset,
                               const svtkm::cont::CoordinateSystem& coords,
                               const svtkm::cont::Field& scalarField,
                               const svtkm::cont::ColorTable& svtkmNotUsed(colorTable),
                               const svtkm::rendering::Camera& camera,
                               const svtkm::Range& scalarRange)
{
  if (!cellset.IsSameType(svtkm::cont::CellSetStructured<3>()))
  {
    std::stringstream msg;
    std::string theType = typeid(cellset).name();
    msg << "Mapper volume: cell set type not currently supported\n";
    msg << "Type : " << theType << std::endl;
    throw svtkm::cont::ErrorBadValue(msg.str());
  }
  else
  {
    raytracing::Logger* logger = raytracing::Logger::GetInstance();
    logger->OpenLogEntry("mapper_volume");
    svtkm::cont::Timer tot_timer;
    tot_timer.Start();
    svtkm::cont::Timer timer;

    svtkm::rendering::raytracing::VolumeRendererStructured tracer;

    svtkm::rendering::raytracing::Camera rayCamera;
    svtkm::rendering::raytracing::Ray<svtkm::Float32> rays;

    rayCamera.SetParameters(camera, *this->Internals->Canvas);

    rayCamera.CreateRays(rays, coords.GetBounds());
    rays.Buffers.at(0).InitConst(0.f);
    raytracing::RayOperations::MapCanvasToRays(rays, camera, *this->Internals->Canvas);


    if (this->Internals->SampleDistance != DEFAULT_SAMPLE_DISTANCE)
    {
      tracer.SetSampleDistance(this->Internals->SampleDistance);
    }

    tracer.SetData(
      coords, scalarField, cellset.Cast<svtkm::cont::CellSetStructured<3>>(), scalarRange);
    tracer.SetColorMap(this->ColorMap);

    tracer.Render(rays);

    timer.Start();
    this->Internals->Canvas->WriteToCanvas(rays, rays.Buffers.at(0).Buffer, camera);

    if (this->Internals->CompositeBackground)
    {
      this->Internals->Canvas->BlendBackground();
    }
    svtkm::Float64 time = timer.GetElapsedTime();
    logger->AddLogData("write_to_canvas", time);
    time = tot_timer.GetElapsedTime();
    logger->CloseLogEntry(time);
  }
}

void MapperVolume::StartScene()
{
  // Nothing needs to be done.
}

void MapperVolume::EndScene()
{
  // Nothing needs to be done.
}

svtkm::rendering::Mapper* MapperVolume::NewCopy() const
{
  return new svtkm::rendering::MapperVolume(*this);
}

void MapperVolume::SetSampleDistance(const svtkm::Float32 sampleDistance)
{
  this->Internals->SampleDistance = sampleDistance;
}

void MapperVolume::SetCompositeBackground(const bool compositeBackground)
{
  this->Internals->CompositeBackground = compositeBackground;
}
}
} // namespace svtkm::rendering
