//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#include <svtkm/rendering/CanvasRayTracer.h>

#include <svtkm/rendering/ConnectivityProxy.h>
#include <svtkm/rendering/Mapper.h>
#include <svtkm/rendering/MapperConnectivity.h>
#include <svtkm/rendering/View.h>

#include <cstdlib>
#include <svtkm/rendering/raytracing/Camera.h>

namespace svtkm
{
namespace rendering
{

SVTKM_CONT
MapperConnectivity::MapperConnectivity()
{
  CanvasRT = nullptr;
  SampleDistance = -1;
}

SVTKM_CONT
MapperConnectivity::~MapperConnectivity()
{
}

SVTKM_CONT
void MapperConnectivity::SetSampleDistance(const svtkm::Float32& distance)
{
  SampleDistance = distance;
}

SVTKM_CONT
void MapperConnectivity::SetCanvas(Canvas* canvas)
{
  if (canvas != nullptr)
  {

    CanvasRT = dynamic_cast<CanvasRayTracer*>(canvas);
    if (CanvasRT == nullptr)
    {
      throw svtkm::cont::ErrorBadValue("Volume Render: bad canvas type. Must be CanvasRayTracer");
    }
  }
}

svtkm::rendering::Canvas* MapperConnectivity::GetCanvas() const
{
  return CanvasRT;
}


SVTKM_CONT
void MapperConnectivity::RenderCells(const svtkm::cont::DynamicCellSet& cellset,
                                     const svtkm::cont::CoordinateSystem& coords,
                                     const svtkm::cont::Field& scalarField,
                                     const svtkm::cont::ColorTable& svtkmNotUsed(colorTable),
                                     const svtkm::rendering::Camera& camera,
                                     const svtkm::Range& svtkmNotUsed(scalarRange))
{
  svtkm::rendering::ConnectivityProxy tracerProxy(cellset, coords, scalarField);
  if (SampleDistance == -1.f)
  {
    // set a default distance
    svtkm::Bounds bounds = coords.GetBounds();
    svtkm::Float64 x2 = bounds.X.Length() * bounds.X.Length();
    svtkm::Float64 y2 = bounds.Y.Length() * bounds.Y.Length();
    svtkm::Float64 z2 = bounds.Z.Length() * bounds.Z.Length();
    svtkm::Float64 length = svtkm::Sqrt(x2 + y2 + z2);
    constexpr svtkm::Float64 defaultSamples = 200.;
    SampleDistance = static_cast<svtkm::Float32>(length / defaultSamples);
  }
  tracerProxy.SetSampleDistance(SampleDistance);
  tracerProxy.SetColorMap(ColorMap);
  tracerProxy.Trace(camera, CanvasRT);
}

void MapperConnectivity::StartScene()
{
  // Nothing needs to be done.
}

void MapperConnectivity::EndScene()
{
  // Nothing needs to be done.
}

svtkm::rendering::Mapper* MapperConnectivity::NewCopy() const
{
  return new svtkm::rendering::MapperConnectivity(*this);
}
}
} // namespace svtkm::rendering
