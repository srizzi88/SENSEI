//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <svtkm/VectorAnalysis.h>
#include <svtkm/cont/Algorithm.h>
#include <svtkm/rendering/raytracing/BVHTraverser.h>
#include <svtkm/rendering/raytracing/RayOperations.h>
#include <svtkm/rendering/raytracing/SphereIntersector.h>
#include <svtkm/worklet/DispatcherMapField.h>
#include <svtkm/worklet/DispatcherMapTopology.h>

namespace svtkm
{
namespace rendering
{
namespace raytracing
{

namespace detail
{

class FindSphereAABBs : public svtkm::worklet::WorkletMapField
{
public:
  SVTKM_CONT
  FindSphereAABBs() {}
  typedef void ControlSignature(FieldIn,
                                FieldIn,
                                FieldOut,
                                FieldOut,
                                FieldOut,
                                FieldOut,
                                FieldOut,
                                FieldOut,
                                WholeArrayIn);
  typedef void ExecutionSignature(_1, _2, _3, _4, _5, _6, _7, _8, _9);
  template <typename PointPortalType>
  SVTKM_EXEC void operator()(const svtkm::Id pointId,
                            const svtkm::Float32& radius,
                            svtkm::Float32& xmin,
                            svtkm::Float32& ymin,
                            svtkm::Float32& zmin,
                            svtkm::Float32& xmax,
                            svtkm::Float32& ymax,
                            svtkm::Float32& zmax,
                            const PointPortalType& points) const
  {
    // cast to Float32
    svtkm::Vec3f_32 point;
    svtkm::Vec3f_32 temp;
    point = static_cast<svtkm::Vec3f_32>(points.Get(pointId));

    temp[0] = radius;
    temp[1] = 0.f;
    temp[2] = 0.f;

    svtkm::Vec3f_32 p = point + temp;
    //set first point to max and min
    xmin = p[0];
    xmax = p[0];
    ymin = p[1];
    ymax = p[1];
    zmin = p[2];
    zmax = p[2];

    p = point - temp;
    xmin = svtkm::Min(xmin, p[0]);
    xmax = svtkm::Max(xmax, p[0]);
    ymin = svtkm::Min(ymin, p[1]);
    ymax = svtkm::Max(ymax, p[1]);
    zmin = svtkm::Min(zmin, p[2]);
    zmax = svtkm::Max(zmax, p[2]);

    temp[0] = 0.f;
    temp[1] = radius;
    temp[2] = 0.f;

    p = point + temp;
    xmin = svtkm::Min(xmin, p[0]);
    xmax = svtkm::Max(xmax, p[0]);
    ymin = svtkm::Min(ymin, p[1]);
    ymax = svtkm::Max(ymax, p[1]);
    zmin = svtkm::Min(zmin, p[2]);
    zmax = svtkm::Max(zmax, p[2]);

    p = point - temp;
    xmin = svtkm::Min(xmin, p[0]);
    xmax = svtkm::Max(xmax, p[0]);
    ymin = svtkm::Min(ymin, p[1]);
    ymax = svtkm::Max(ymax, p[1]);
    zmin = svtkm::Min(zmin, p[2]);
    zmax = svtkm::Max(zmax, p[2]);

    temp[0] = 0.f;
    temp[1] = 0.f;
    temp[2] = radius;

    p = point + temp;
    xmin = svtkm::Min(xmin, p[0]);
    xmax = svtkm::Max(xmax, p[0]);
    ymin = svtkm::Min(ymin, p[1]);
    ymax = svtkm::Max(ymax, p[1]);
    zmin = svtkm::Min(zmin, p[2]);
    zmax = svtkm::Max(zmax, p[2]);

    p = point - temp;
    xmin = svtkm::Min(xmin, p[0]);
    xmax = svtkm::Max(xmax, p[0]);
    ymin = svtkm::Min(ymin, p[1]);
    ymax = svtkm::Max(ymax, p[1]);
    zmin = svtkm::Min(zmin, p[2]);
    zmax = svtkm::Max(zmax, p[2]);
  }
}; //class FindAABBs

template <typename Device>
class SphereLeafIntersector
{
public:
  using IdHandle = svtkm::cont::ArrayHandle<svtkm::Id>;
  using IdArrayPortal = typename IdHandle::ExecutionTypes<Device>::PortalConst;
  using FloatHandle = svtkm::cont::ArrayHandle<svtkm::Float32>;
  using FloatPortal = typename FloatHandle::ExecutionTypes<Device>::PortalConst;
  IdArrayPortal PointIds;
  FloatPortal Radii;

  SphereLeafIntersector() {}

  SphereLeafIntersector(const IdHandle& pointIds, const FloatHandle& radii)
    : PointIds(pointIds.PrepareForInput(Device()))
    , Radii(radii.PrepareForInput(Device()))
  {
  }

  template <typename PointPortalType, typename LeafPortalType, typename Precision>
  SVTKM_EXEC inline void IntersectLeaf(
    const svtkm::Int32& currentNode,
    const svtkm::Vec<Precision, 3>& origin,
    const svtkm::Vec<Precision, 3>& dir,
    const PointPortalType& points,
    svtkm::Id& hitIndex,
    Precision& closestDistance, // closest distance in this set of primitives
    Precision& svtkmNotUsed(minU),
    Precision& svtkmNotUsed(minV),
    LeafPortalType leafs,
    const Precision& minDistance) const // report intesections past this distance
  {
    const svtkm::Id sphereCount = leafs.Get(currentNode);
    for (svtkm::Id i = 1; i <= sphereCount; ++i)
    {
      const svtkm::Id sphereIndex = leafs.Get(currentNode + i);
      svtkm::Id pointIndex = PointIds.Get(sphereIndex);
      svtkm::Float32 radius = Radii.Get(sphereIndex);
      svtkm::Vec<Precision, 3> center = svtkm::Vec<Precision, 3>(points.Get(pointIndex));

      svtkm::Vec<Precision, 3> l = center - origin;

      Precision dot1 = svtkm::dot(l, dir);

      if (dot1 >= 0)
      {
        Precision d = svtkm::dot(l, l) - dot1 * dot1;
        Precision r2 = radius * radius;
        if (d <= r2)
        {
          Precision tch = svtkm::Sqrt(r2 - d);
          Precision t0 = dot1 - tch;
          //float t1 = dot1+tch; /* if t1 is > 0 and t0<0 then the ray is inside the sphere.

          if (t0 < closestDistance && t0 > minDistance)
          {
            hitIndex = pointIndex;
            closestDistance = t0;
          }
        }
      }
    } // for
  }
};

class SphereLeafWrapper : public svtkm::cont::ExecutionObjectBase
{
protected:
  using IdHandle = svtkm::cont::ArrayHandle<svtkm::Id>;
  using FloatHandle = svtkm::cont::ArrayHandle<svtkm::Float32>;
  IdHandle PointIds;
  FloatHandle Radii;

public:
  SphereLeafWrapper(IdHandle& pointIds, FloatHandle radii)
    : PointIds(pointIds)
    , Radii(radii)
  {
  }

  template <typename Device>
  SVTKM_CONT SphereLeafIntersector<Device> PrepareForExecution(Device) const
  {
    return SphereLeafIntersector<Device>(PointIds, Radii);
  }
};

class CalculateNormals : public svtkm::worklet::WorkletMapField
{
public:
  SVTKM_CONT
  CalculateNormals() {}
  typedef void
    ControlSignature(FieldIn, FieldIn, FieldOut, FieldOut, FieldOut, WholeArrayIn, WholeArrayIn);
  typedef void ExecutionSignature(_1, _2, _3, _4, _5, _6, _7);
  template <typename Precision, typename PointPortalType, typename IndicesPortalType>
  SVTKM_EXEC inline void operator()(const svtkm::Id& hitIndex,
                                   const svtkm::Vec<Precision, 3>& intersection,
                                   Precision& normalX,
                                   Precision& normalY,
                                   Precision& normalZ,
                                   const PointPortalType& points,
                                   const IndicesPortalType& indicesPortal) const
  {
    if (hitIndex < 0)
      return;

    svtkm::Id pointId = indicesPortal.Get(hitIndex);
    svtkm::Vec<Precision, 3> center = points.Get(pointId);

    svtkm::Vec<Precision, 3> normal = intersection - center;
    svtkm::Normalize(normal);

    //flip the normal if its pointing the wrong way
    normalX = normal[0];
    normalY = normal[1];
    normalZ = normal[2];
  }
}; //class CalculateNormals

template <typename Precision>
class GetScalar : public svtkm::worklet::WorkletMapField
{
private:
  Precision MinScalar;
  Precision invDeltaScalar;

public:
  SVTKM_CONT
  GetScalar(const svtkm::Float32& minScalar, const svtkm::Float32& maxScalar)
    : MinScalar(minScalar)
  {
    //Make sure the we don't divide by zero on
    //something like an iso-surface
    if (maxScalar - MinScalar != 0.f)
      invDeltaScalar = 1.f / (maxScalar - MinScalar);
    else
      invDeltaScalar = 1.f / minScalar;
  }
  typedef void ControlSignature(FieldIn, FieldOut, WholeArrayIn, WholeArrayIn);
  typedef void ExecutionSignature(_1, _2, _3, _4);
  template <typename ScalarPortalType, typename IndicesPortalType>
  SVTKM_EXEC void operator()(const svtkm::Id& hitIndex,
                            Precision& scalar,
                            const ScalarPortalType& scalars,
                            const IndicesPortalType& indicesPortal) const
  {
    if (hitIndex < 0)
      return;

    svtkm::Id pointId = indicesPortal.Get(hitIndex);

    scalar = Precision(scalars.Get(pointId));
    //normalize
    scalar = (scalar - MinScalar) * invDeltaScalar;
  }
}; //class GetScalar

} // namespace detail

SphereIntersector::SphereIntersector()
  : ShapeIntersector()
{
}

SphereIntersector::~SphereIntersector()
{
}

void SphereIntersector::SetData(const svtkm::cont::CoordinateSystem& coords,
                                svtkm::cont::ArrayHandle<svtkm::Id> pointIds,
                                svtkm::cont::ArrayHandle<svtkm::Float32> radii)
{
  this->PointIds = pointIds;
  this->Radii = radii;
  this->CoordsHandle = coords;
  AABBs AABB;
  svtkm::worklet::DispatcherMapField<detail::FindSphereAABBs>(detail::FindSphereAABBs())
    .Invoke(PointIds,
            Radii,
            AABB.xmins,
            AABB.ymins,
            AABB.zmins,
            AABB.xmaxs,
            AABB.ymaxs,
            AABB.zmaxs,
            CoordsHandle);

  this->SetAABBs(AABB);
}

void SphereIntersector::IntersectRays(Ray<svtkm::Float32>& rays, bool returnCellIndex)
{
  IntersectRaysImp(rays, returnCellIndex);
}

void SphereIntersector::IntersectRays(Ray<svtkm::Float64>& rays, bool returnCellIndex)
{
  IntersectRaysImp(rays, returnCellIndex);
}

template <typename Precision>
void SphereIntersector::IntersectRaysImp(Ray<Precision>& rays, bool svtkmNotUsed(returnCellIndex))
{

  detail::SphereLeafWrapper leafIntersector(this->PointIds, Radii);

  BVHTraverser traverser;
  traverser.IntersectRays(rays, this->BVH, leafIntersector, this->CoordsHandle);

  RayOperations::UpdateRayStatus(rays);
}

template <typename Precision>
void SphereIntersector::IntersectionDataImp(Ray<Precision>& rays,
                                            const svtkm::cont::Field scalarField,
                                            const svtkm::Range& scalarRange)
{
  ShapeIntersector::IntersectionPoint(rays);

  const bool isSupportedField = scalarField.IsFieldCell() || scalarField.IsFieldPoint();
  if (!isSupportedField)
  {
    throw svtkm::cont::ErrorBadValue(
      "SphereIntersector: Field not accociated with a cell set or field");
  }

  svtkm::worklet::DispatcherMapField<detail::CalculateNormals>(detail::CalculateNormals())
    .Invoke(rays.HitIdx,
            rays.Intersection,
            rays.NormalX,
            rays.NormalY,
            rays.NormalZ,
            CoordsHandle,
            PointIds);

  svtkm::worklet::DispatcherMapField<detail::GetScalar<Precision>>(
    detail::GetScalar<Precision>(svtkm::Float32(scalarRange.Min), svtkm::Float32(scalarRange.Max)))
    .Invoke(rays.HitIdx,
            rays.Scalar,
            scalarField.GetData().ResetTypes(svtkm::TypeListFieldScalar()),
            PointIds);
}

void SphereIntersector::IntersectionData(Ray<svtkm::Float32>& rays,
                                         const svtkm::cont::Field scalarField,
                                         const svtkm::Range& scalarRange)
{
  IntersectionDataImp(rays, scalarField, scalarRange);
}

void SphereIntersector::IntersectionData(Ray<svtkm::Float64>& rays,
                                         const svtkm::cont::Field scalarField,
                                         const svtkm::Range& scalarRange)
{
  IntersectionDataImp(rays, scalarField, scalarRange);
}

svtkm::Id SphereIntersector::GetNumberOfShapes() const
{
  return PointIds.GetNumberOfValues();
}
}
}
} //namespace svtkm::rendering::raytracing
