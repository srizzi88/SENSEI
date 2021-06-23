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
#include <svtkm/rendering/raytracing/CylinderIntersector.h>
#include <svtkm/rendering/raytracing/RayOperations.h>
#include <svtkm/worklet/DispatcherMapTopology.h>

namespace svtkm
{
namespace rendering
{
namespace raytracing
{
namespace detail
{

class FindCylinderAABBs : public svtkm::worklet::WorkletMapField
{
public:
  SVTKM_CONT
  FindCylinderAABBs() {}
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
  SVTKM_EXEC void operator()(const svtkm::Id3 cylId,
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
    svtkm::Vec3f_32 point1, point2;
    svtkm::Vec3f_32 temp;

    point1 = static_cast<svtkm::Vec3f_32>(points.Get(cylId[1]));
    point2 = static_cast<svtkm::Vec3f_32>(points.Get(cylId[2]));

    temp[0] = radius;
    temp[1] = 0.0f;
    temp[2] = 0.0f;
    xmin = ymin = zmin = svtkm::Infinity32();
    xmax = ymax = zmax = svtkm::NegativeInfinity32();


    //set first point to max and min
    Bounds(point1, radius, xmin, ymin, zmin, xmax, ymax, zmax);

    Bounds(point2, radius, xmin, ymin, zmin, xmax, ymax, zmax);
  }

  SVTKM_EXEC void Bounds(const svtkm::Vec3f_32& point,
                        const svtkm::Float32& radius,
                        svtkm::Float32& xmin,
                        svtkm::Float32& ymin,
                        svtkm::Float32& zmin,
                        svtkm::Float32& xmax,
                        svtkm::Float32& ymax,
                        svtkm::Float32& zmax) const
  {
    svtkm::Vec3f_32 temp, p;
    temp[0] = radius;
    temp[1] = 0.0f;
    temp[2] = 0.0f;
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
class CylinderLeafIntersector
{
public:
  using IdHandle = svtkm::cont::ArrayHandle<svtkm::Id3>;
  using IdArrayPortal = typename IdHandle::ExecutionTypes<Device>::PortalConst;
  using FloatHandle = svtkm::cont::ArrayHandle<svtkm::Float32>;
  using FloatPortal = typename FloatHandle::ExecutionTypes<Device>::PortalConst;
  IdArrayPortal CylIds;
  FloatPortal Radii;

  CylinderLeafIntersector() {}

  CylinderLeafIntersector(const IdHandle& cylIds, const FloatHandle& radii)
    : CylIds(cylIds.PrepareForInput(Device()))
    , Radii(radii.PrepareForInput(Device()))
  {
  }

  template <typename vec3>
  SVTKM_EXEC vec3 cylinder(const vec3& ray_start,
                          const vec3& ray_direction,
                          const vec3& p,
                          const vec3& q,
                          float r) const
  {
    float t = 0;
    vec3 d = q - p;
    vec3 m = ray_start - p;

    vec3 s = ray_start - q;

    svtkm::Float32 mdotm = svtkm::Float32(svtkm::dot(m, m));
    vec3 n = ray_direction * (svtkm::Max(mdotm, static_cast<svtkm::Float32>(svtkm::dot(s, s))) + r);

    svtkm::Float32 mdotd = svtkm::Float32(svtkm::dot(m, d));
    svtkm::Float32 ndotd = svtkm::Float32(svtkm::dot(n, d));
    svtkm::Float32 ddotd = svtkm::Float32(svtkm::dot(d, d));
    if ((mdotd < 0.0f) && (mdotd + ndotd < 0.0f))
    {
      return vec3(0.f, 0.f, 0.f);
    }
    if ((mdotd > ddotd) && (mdotd + ndotd > ddotd))
    {
      return vec3(0.f, 0.f, 0.f);
    }
    svtkm::Float32 ndotn = svtkm::Float32(svtkm::dot(n, n));
    svtkm::Float32 nlen = svtkm::Float32(sqrt(ndotn));
    svtkm::Float32 mdotn = svtkm::Float32(svtkm::dot(m, n));
    svtkm::Float32 a = ddotd * ndotn - ndotd * ndotd;
    svtkm::Float32 k = mdotm - r * r;
    svtkm::Float32 c = ddotd * k - mdotd * mdotd;

    if (fabs(a) < 1e-6)
    {
      if (c > 0.0)
      {
        return vec3(0, 0, 0);
      }
      if (mdotd < 0.0f)
      {
        t = -mdotn / ndotn;
      }
      else if (mdotd > ddotd)
      {
        t = (ndotd - mdotn) / ndotn;
      }
      else
        t = 0;

      return vec3(1, t * nlen, 0);
    }
    svtkm::Float32 b = ddotd * mdotn - ndotd * mdotd;
    svtkm::Float32 discr = b * b - a * c;
    if (discr < 0.0f)
    {
      return vec3(0, 0, 0);
    }
    t = (-b - svtkm::Sqrt(discr)) / a;
    if (t < 0.0f || t > 1.0f)
    {
      return vec3(0, 0, 0);
    }

    svtkm::Float32 u = mdotd + t * ndotd;

    if (u > ddotd)
    {
      if (ndotd >= 0.0f)
      {
        return vec3(0, 0, 0);
      }
      t = (ddotd - mdotd) / ndotd;

      return vec3(
        k + ddotd - 2 * mdotd + t * (2 * (mdotn - ndotd) + t * ndotn) <= 0.0f, t * nlen, 0);
    }
    else if (u < 0.0f)
    {
      if (ndotd <= 0.0f)
      {
        return vec3(0.0, 0.0, 0);
      }
      t = -mdotd / ndotd;

      return vec3(k + 2 * t * (mdotn + t * ndotn) <= 0.0f, t * nlen, 0);
    }
    return vec3(1, t * nlen, 0);
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
    const svtkm::Id cylCount = leafs.Get(currentNode);
    for (svtkm::Id i = 1; i <= cylCount; ++i)
    {
      const svtkm::Id cylIndex = leafs.Get(currentNode + i);
      if (cylIndex < CylIds.GetNumberOfValues())
      {
        svtkm::Id3 pointIndex = CylIds.Get(cylIndex);
        svtkm::Float32 radius = Radii.Get(cylIndex);
        svtkm::Vec<Precision, 3> bottom, top;
        bottom = svtkm::Vec<Precision, 3>(points.Get(pointIndex[1]));
        top = svtkm::Vec<Precision, 3>(points.Get(pointIndex[2]));

        svtkm::Vec3f_32 ret;
        ret = cylinder(origin, dir, bottom, top, radius);
        if (ret[0] > 0)
        {
          if (ret[1] < closestDistance && ret[1] > minDistance)
          {
            //matid = svtkm::Vec<, 3>(points.Get(cur_offset + 2))[0];
            closestDistance = ret[1];
            hitIndex = cylIndex;
          }
        }
      }
    } // for
  }
};

class CylinderLeafWrapper : public svtkm::cont::ExecutionObjectBase
{
protected:
  using IdHandle = svtkm::cont::ArrayHandle<svtkm::Id3>;
  using FloatHandle = svtkm::cont::ArrayHandle<svtkm::Float32>;
  IdHandle CylIds;
  FloatHandle Radii;

public:
  CylinderLeafWrapper(IdHandle& cylIds, FloatHandle radii)
    : CylIds(cylIds)
    , Radii(radii)
  {
  }

  template <typename Device>
  SVTKM_CONT CylinderLeafIntersector<Device> PrepareForExecution(Device) const
  {
    return CylinderLeafIntersector<Device>(CylIds, Radii);
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

    svtkm::Id3 cylId = indicesPortal.Get(hitIndex);

    svtkm::Vec<Precision, 3> a, b;
    a = points.Get(cylId[1]);
    b = points.Get(cylId[2]);

    svtkm::Vec<Precision, 3> ap, ab;
    ap = intersection - a;
    ab = b - a;

    Precision mag2 = svtkm::Magnitude(ab);
    Precision len = svtkm::dot(ab, ap);
    Precision t = len / mag2;

    svtkm::Vec<Precision, 3> center;
    center = a + t * ab;

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
  typedef void ControlSignature(FieldIn, FieldInOut, WholeArrayIn, WholeArrayIn);
  typedef void ExecutionSignature(_1, _2, _3, _4);
  template <typename ScalarPortalType, typename IndicesPortalType>
  SVTKM_EXEC void operator()(const svtkm::Id& hitIndex,
                            Precision& scalar,
                            const ScalarPortalType& scalars,
                            const IndicesPortalType& indicesPortal) const
  {
    if (hitIndex < 0)
      return;

    //TODO: this should be interpolated?
    svtkm::Id3 pointId = indicesPortal.Get(hitIndex);

    scalar = Precision(scalars.Get(pointId[0]));
    //normalize
    scalar = (scalar - MinScalar) * invDeltaScalar;
  }
}; //class GetScalar

} // namespace detail

CylinderIntersector::CylinderIntersector()
  : ShapeIntersector()
{
}

CylinderIntersector::~CylinderIntersector()
{
}

void CylinderIntersector::SetData(const svtkm::cont::CoordinateSystem& coords,
                                  svtkm::cont::ArrayHandle<svtkm::Id3> cylIds,
                                  svtkm::cont::ArrayHandle<svtkm::Float32> radii)
{
  this->Radii = radii;
  this->CylIds = cylIds;
  this->CoordsHandle = coords;
  AABBs AABB;

  svtkm::worklet::DispatcherMapField<detail::FindCylinderAABBs>(detail::FindCylinderAABBs())
    .Invoke(this->CylIds,
            this->Radii,
            AABB.xmins,
            AABB.ymins,
            AABB.zmins,
            AABB.xmaxs,
            AABB.ymaxs,
            AABB.zmaxs,
            CoordsHandle);

  this->SetAABBs(AABB);
}

void CylinderIntersector::IntersectRays(Ray<svtkm::Float32>& rays, bool returnCellIndex)
{
  IntersectRaysImp(rays, returnCellIndex);
}

void CylinderIntersector::IntersectRays(Ray<svtkm::Float64>& rays, bool returnCellIndex)
{
  IntersectRaysImp(rays, returnCellIndex);
}

template <typename Precision>
void CylinderIntersector::IntersectRaysImp(Ray<Precision>& rays, bool svtkmNotUsed(returnCellIndex))
{

  detail::CylinderLeafWrapper leafIntersector(this->CylIds, Radii);

  BVHTraverser traverser;
  traverser.IntersectRays(rays, this->BVH, leafIntersector, this->CoordsHandle);

  RayOperations::UpdateRayStatus(rays);
}

template <typename Precision>
void CylinderIntersector::IntersectionDataImp(Ray<Precision>& rays,
                                              const svtkm::cont::Field scalarField,
                                              const svtkm::Range& scalarRange)
{
  ShapeIntersector::IntersectionPoint(rays);

  // TODO: if this is nodes of a mesh, support points
  const bool isSupportedField = scalarField.IsFieldCell() || scalarField.IsFieldPoint();
  if (!isSupportedField)
  {
    throw svtkm::cont::ErrorBadValue("Field not accociated with a cell set");
  }

  svtkm::worklet::DispatcherMapField<detail::CalculateNormals>(detail::CalculateNormals())
    .Invoke(rays.HitIdx,
            rays.Intersection,
            rays.NormalX,
            rays.NormalY,
            rays.NormalZ,
            CoordsHandle,
            CylIds);

  svtkm::worklet::DispatcherMapField<detail::GetScalar<Precision>>(
    detail::GetScalar<Precision>(svtkm::Float32(scalarRange.Min), svtkm::Float32(scalarRange.Max)))
    .Invoke(
      rays.HitIdx, rays.Scalar, scalarField.GetData().ResetTypes(ScalarRenderingTypes()), CylIds);
}

void CylinderIntersector::IntersectionData(Ray<svtkm::Float32>& rays,
                                           const svtkm::cont::Field scalarField,
                                           const svtkm::Range& scalarRange)
{
  IntersectionDataImp(rays, scalarField, scalarRange);
}

void CylinderIntersector::IntersectionData(Ray<svtkm::Float64>& rays,
                                           const svtkm::cont::Field scalarField,
                                           const svtkm::Range& scalarRange)
{
  IntersectionDataImp(rays, scalarField, scalarRange);
}

svtkm::Id CylinderIntersector::GetNumberOfShapes() const
{
  return CylIds.GetNumberOfValues();
}
}
}
} //namespace svtkm::rendering::raytracing
