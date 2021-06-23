//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <svtkm/rendering/MapperCylinder.h>

#include <svtkm/cont/Timer.h>
#include <svtkm/cont/TryExecute.h>

#include <svtkm/rendering/CanvasRayTracer.h>
#include <svtkm/rendering/Cylinderizer.h>
#include <svtkm/rendering/raytracing/Camera.h>
#include <svtkm/rendering/raytracing/CylinderExtractor.h>
#include <svtkm/rendering/raytracing/CylinderIntersector.h>
#include <svtkm/rendering/raytracing/Logger.h>
#include <svtkm/rendering/raytracing/RayOperations.h>
#include <svtkm/rendering/raytracing/RayTracer.h>
#include <svtkm/rendering/raytracing/Worklets.h>

namespace svtkm
{
namespace rendering
{

class CalcDistance : public svtkm::worklet::WorkletMapField
{
public:
  SVTKM_CONT
  CalcDistance(const svtkm::Vec3f_32& _eye_pos)
    : eye_pos(_eye_pos)
  {
  }
  typedef void ControlSignature(FieldIn, FieldOut);
  typedef void ExecutionSignature(_1, _2);
  template <typename VecType, typename OutType>
  SVTKM_EXEC inline void operator()(const VecType& pos, OutType& out) const
  {
    VecType tmp = eye_pos - pos;
    out = static_cast<OutType>(svtkm::Sqrt(svtkm::dot(tmp, tmp)));
  }

  const svtkm::Vec3f_32 eye_pos;
}; //class CalcDistance

struct MapperCylinder::InternalsType
{
  svtkm::rendering::CanvasRayTracer* Canvas;
  svtkm::rendering::raytracing::RayTracer Tracer;
  svtkm::rendering::raytracing::Camera RayCamera;
  svtkm::rendering::raytracing::Ray<svtkm::Float32> Rays;
  bool CompositeBackground;
  svtkm::Float32 Radius;
  svtkm::Float32 Delta;
  bool UseVariableRadius;
  SVTKM_CONT
  InternalsType()
    : Canvas(nullptr)
    , CompositeBackground(true)
    , Radius(-1.0f)
    , Delta(0.5)
    , UseVariableRadius(false)
  {
  }
};

MapperCylinder::MapperCylinder()
  : Internals(new InternalsType)
{
}

MapperCylinder::~MapperCylinder()
{
}

void MapperCylinder::SetCanvas(svtkm::rendering::Canvas* canvas)
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

svtkm::rendering::Canvas* MapperCylinder::GetCanvas() const
{
  return this->Internals->Canvas;
}

void MapperCylinder::UseVariableRadius(bool useVariableRadius)
{
  this->Internals->UseVariableRadius = useVariableRadius;
}

void MapperCylinder::SetRadius(const svtkm::Float32& radius)
{
  if (radius <= 0.f)
  {
    throw svtkm::cont::ErrorBadValue("MapperCylinder: radius must be positive");
  }
  this->Internals->Radius = radius;
}
void MapperCylinder::SetRadiusDelta(const svtkm::Float32& delta)
{
  this->Internals->Delta = delta;
}

void MapperCylinder::RenderCells(const svtkm::cont::DynamicCellSet& cellset,
                                 const svtkm::cont::CoordinateSystem& coords,
                                 const svtkm::cont::Field& scalarField,
                                 const svtkm::cont::ColorTable& svtkmNotUsed(colorTable),
                                 const svtkm::rendering::Camera& camera,
                                 const svtkm::Range& scalarRange)
{
  raytracing::Logger* logger = raytracing::Logger::GetInstance();
  logger->OpenLogEntry("mapper_cylinder");
  svtkm::cont::Timer tot_timer;
  tot_timer.Start();
  svtkm::cont::Timer timer;


  svtkm::Bounds shapeBounds;
  raytracing::CylinderExtractor cylExtractor;

  svtkm::Float32 baseRadius = this->Internals->Radius;
  if (baseRadius == -1.f)
  {
    // set a default radius
    svtkm::cont::ArrayHandle<svtkm::Float32> dist;
    svtkm::worklet::DispatcherMapField<CalcDistance>(CalcDistance(camera.GetPosition()))
      .Invoke(coords, dist);


    svtkm::Float32 min_dist =
      svtkm::cont::Algorithm::Reduce(dist, svtkm::Infinity<svtkm::Float32>(), svtkm::Minimum());

    baseRadius = 0.576769694f * min_dist - 0.603522029f * svtkm::Pow(svtkm::Float32(min_dist), 2.f) +
      0.232171175f * svtkm::Pow(svtkm::Float32(min_dist), 3.f) -
      0.038697244f * svtkm::Pow(svtkm::Float32(min_dist), 4.f) +
      0.002366979f * svtkm::Pow(svtkm::Float32(min_dist), 5.f);
    baseRadius /= min_dist;
    svtkm::worklet::DispatcherMapField<svtkm::rendering::raytracing::MemSet<svtkm::Float32>>(
      svtkm::rendering::raytracing::MemSet<svtkm::Float32>(baseRadius))
      .Invoke(cylExtractor.GetRadii());
  }

  if (this->Internals->UseVariableRadius)
  {
    svtkm::Float32 minRadius = baseRadius - baseRadius * this->Internals->Delta;
    svtkm::Float32 maxRadius = baseRadius + baseRadius * this->Internals->Delta;

    cylExtractor.ExtractCells(cellset, scalarField, minRadius, maxRadius);
  }
  else
  {
    cylExtractor.ExtractCells(cellset, baseRadius);
  }

  //
  // Add supported shapes
  //

  if (cylExtractor.GetNumberOfCylinders() > 0)
  {
    auto cylIntersector = std::make_shared<raytracing::CylinderIntersector>();
    cylIntersector->SetData(coords, cylExtractor.GetCylIds(), cylExtractor.GetRadii());
    this->Internals->Tracer.AddShapeIntersector(cylIntersector);
    shapeBounds.Include(cylIntersector->GetShapeBounds());
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

void MapperCylinder::SetCompositeBackground(bool on)
{
  this->Internals->CompositeBackground = on;
}

void MapperCylinder::StartScene()
{
  // Nothing needs to be done.
}

void MapperCylinder::EndScene()
{
  // Nothing needs to be done.
}

svtkm::rendering::Mapper* MapperCylinder::NewCopy() const
{
  return new svtkm::rendering::MapperCylinder(*this);
}
}
} // svtkm::rendering
