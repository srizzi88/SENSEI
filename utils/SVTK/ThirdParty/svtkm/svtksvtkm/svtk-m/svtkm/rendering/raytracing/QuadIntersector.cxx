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
#include <svtkm/rendering/raytracing/QuadIntersector.h>
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

#define QUAD_AABB_EPSILON 1.0e-4f
class FindQuadAABBs : public svtkm::worklet::WorkletMapField
{
public:
  SVTKM_CONT
  FindQuadAABBs() {}
  typedef void ControlSignature(FieldIn,
                                FieldOut,
                                FieldOut,
                                FieldOut,
                                FieldOut,
                                FieldOut,
                                FieldOut,
                                WholeArrayIn);
  typedef void ExecutionSignature(_1, _2, _3, _4, _5, _6, _7, _8);
  template <typename PointPortalType>
  SVTKM_EXEC void operator()(const svtkm::Vec<svtkm::Id, 5> quadId,
                            svtkm::Float32& xmin,
                            svtkm::Float32& ymin,
                            svtkm::Float32& zmin,
                            svtkm::Float32& xmax,
                            svtkm::Float32& ymax,
                            svtkm::Float32& zmax,
                            const PointPortalType& points) const
  {
    // cast to Float32
    svtkm::Vec3f_32 q, r, s, t;

    q = static_cast<svtkm::Vec3f_32>(points.Get(quadId[1]));
    r = static_cast<svtkm::Vec3f_32>(points.Get(quadId[2]));
    s = static_cast<svtkm::Vec3f_32>(points.Get(quadId[3]));
    t = static_cast<svtkm::Vec3f_32>(points.Get(quadId[4]));

    xmin = q[0];
    ymin = q[1];
    zmin = q[2];
    xmax = xmin;
    ymax = ymin;
    zmax = zmin;
    xmin = svtkm::Min(xmin, r[0]);
    ymin = svtkm::Min(ymin, r[1]);
    zmin = svtkm::Min(zmin, r[2]);
    xmax = svtkm::Max(xmax, r[0]);
    ymax = svtkm::Max(ymax, r[1]);
    zmax = svtkm::Max(zmax, r[2]);
    xmin = svtkm::Min(xmin, s[0]);
    ymin = svtkm::Min(ymin, s[1]);
    zmin = svtkm::Min(zmin, s[2]);
    xmax = svtkm::Max(xmax, s[0]);
    ymax = svtkm::Max(ymax, s[1]);
    zmax = svtkm::Max(zmax, s[2]);
    xmin = svtkm::Min(xmin, t[0]);
    ymin = svtkm::Min(ymin, t[1]);
    zmin = svtkm::Min(zmin, t[2]);
    xmax = svtkm::Max(xmax, t[0]);
    ymax = svtkm::Max(ymax, t[1]);
    zmax = svtkm::Max(zmax, t[2]);

    svtkm::Float32 xEpsilon, yEpsilon, zEpsilon;
    const svtkm::Float32 minEpsilon = 1e-6f;
    xEpsilon = svtkm::Max(minEpsilon, QUAD_AABB_EPSILON * (xmax - xmin));
    yEpsilon = svtkm::Max(minEpsilon, QUAD_AABB_EPSILON * (ymax - ymin));
    zEpsilon = svtkm::Max(minEpsilon, QUAD_AABB_EPSILON * (zmax - zmin));

    xmin -= xEpsilon;
    ymin -= yEpsilon;
    zmin -= zEpsilon;
    xmax += xEpsilon;
    ymax += yEpsilon;
    zmax += zEpsilon;
  }

}; //class FindAABBs

template <typename Device>
class QuadLeafIntersector
{
public:
  using IdType = svtkm::Vec<svtkm::Id, 5>;
  using IdHandle = svtkm::cont::ArrayHandle<svtkm::Vec<svtkm::Id, 5>>;
  using IdArrayPortal = typename IdHandle::ExecutionTypes<Device>::PortalConst;
  IdArrayPortal QuadIds;

  QuadLeafIntersector() {}

  QuadLeafIntersector(const IdHandle& quadIds)
    : QuadIds(quadIds.PrepareForInput(Device()))
  {
  }

  template <typename vec3, typename Precision>
  SVTKM_EXEC bool quad(const vec3& ray_origin,
                      const vec3& ray_direction,
                      const vec3& v00,
                      const vec3& v10,
                      const vec3& v11,
                      const vec3& v01,
                      Precision& u,
                      Precision& v,
                      Precision& t) const
  {

    /* An Eﬃcient Ray-Quadrilateral Intersection Test
         Ares Lagae Philip Dutr´e
         http://graphics.cs.kuleuven.be/publications/LD05ERQIT/index.html

      v01 *------------ * v11
          |\           |
          |  \         |
          |    \       |
          |      \     |
          |        \   |
          |          \ |
      v00 *------------* v10
      */
    // Rejects rays that are parallel to Q, and rays that intersect the plane of
    // Q either on the left of the line V00V01 or on the right of the line V00V10.

    vec3 E03 = v01 - v00;
    vec3 P = svtkm::Cross(ray_direction, E03);
    vec3 E01 = v10 - v00;
    Precision det = svtkm::dot(E01, P);

    if (svtkm::Abs(det) < svtkm::Epsilon<Precision>())
      return false;
    Precision inv_det = 1.0f / det;
    vec3 T = ray_origin - v00;
    Precision alpha = svtkm::dot(T, P) * inv_det;
    if (alpha < 0.0)
      return false;
    vec3 Q = svtkm::Cross(T, E01);
    Precision beta = svtkm::dot(ray_direction, Q) * inv_det;
    if (beta < 0.0)
      return false;

    if ((alpha + beta) > 1.0f)
    {

      // Rejects rays that intersect the plane of Q either on the
      // left of the line V11V10 or on the right of the line V11V01.

      vec3 E23 = v01 - v11;
      vec3 E21 = v10 - v11;
      vec3 P_prime = svtkm::Cross(ray_direction, E21);
      Precision det_prime = svtkm::dot(E23, P_prime);
      if (svtkm::Abs(det_prime) < svtkm::Epsilon<Precision>())
        return false;
      Precision inv_det_prime = 1.0f / det_prime;
      vec3 T_prime = ray_origin - v11;
      Precision alpha_prime = svtkm::dot(T_prime, P_prime) * inv_det_prime;
      if (alpha_prime < 0.0f)
        return false;
      vec3 Q_prime = svtkm::Cross(T_prime, E23);
      Precision beta_prime = svtkm::dot(ray_direction, Q_prime) * inv_det_prime;
      if (beta_prime < 0.0f)
        return false;
    }

    // Compute the ray parameter of the intersection point, and
    // reject the ray if it does not hit Q.

    t = svtkm::dot(E03, Q) * inv_det;
    if (t < 0.0)
      return false;


    // Compute the barycentric coordinates of V11
    Precision alpha_11, beta_11;
    vec3 E02 = v11 - v00;
    vec3 n = svtkm::Cross(E01, E02);

    if ((svtkm::Abs(n[0]) >= svtkm::Abs(n[1])) && (svtkm::Abs(n[0]) >= svtkm::Abs(n[2])))
    {

      alpha_11 = ((E02[1] * E03[2]) - (E02[2] * E03[1])) / n[0];
      beta_11 = ((E01[1] * E02[2]) - (E01[2] * E02[1])) / n[0];
    }
    else if ((svtkm::Abs(n[1]) >= svtkm::Abs(n[0])) && (svtkm::Abs(n[1]) >= svtkm::Abs(n[2])))
    {

      alpha_11 = ((E02[2] * E03[0]) - (E02[0] * E03[2])) / n[1];
      beta_11 = ((E01[2] * E02[0]) - (E01[0] * E02[2])) / n[1];
    }
    else
    {

      alpha_11 = ((E02[0] * E03[1]) - (E02[1] * E03[0])) / n[2];
      beta_11 = ((E01[0] * E02[1]) - (E01[1] * E02[0])) / n[2];
    }

    // Compute the bilinear coordinates of the intersection point.
    if (svtkm::Abs(alpha_11 - 1.0f) < svtkm::Epsilon<Precision>())
    {

      u = alpha;
      if (svtkm::Abs(beta_11 - 1.0f) < svtkm::Epsilon<Precision>())
        v = beta;
      else
        v = beta / ((u * (beta_11 - 1.0f)) + 1.0f);
    }
    else if (svtkm::Abs(beta_11 - 1.0) < svtkm::Epsilon<Precision>())
    {

      v = beta;
      u = alpha / ((v * (alpha_11 - 1.0f)) + 1.0f);
    }
    else
    {

      Precision A = 1.0f - beta_11;
      Precision B = (alpha * (beta_11 - 1.0f)) - (beta * (alpha_11 - 1.0f)) - 1.0f;
      Precision C = alpha;
      Precision D = (B * B) - (4.0f * A * C);
      Precision QQ = -0.5f * (B + ((B < 0.0f ? -1.0f : 1.0f) * svtkm::Sqrt(D)));
      u = QQ / A;
      if ((u < 0.0f) || (u > 1.0f))
        u = C / QQ;
      v = beta / ((u * (beta_11 - 1.0f)) + 1.0f);
    }

    return true;
  }

  template <typename PointPortalType, typename LeafPortalType, typename Precision>
  SVTKM_EXEC inline void IntersectLeaf(
    const svtkm::Int32& currentNode,
    const svtkm::Vec<Precision, 3>& origin,
    const svtkm::Vec<Precision, 3>& dir,
    const PointPortalType& points,
    svtkm::Id& hitIndex,
    Precision& closestDistance, // closest distance in this set of primitives
    Precision& minU,
    Precision& minV,
    LeafPortalType leafs,
    const Precision& minDistance) const // report intesections past this distance
  {
    const svtkm::Id quadCount = leafs.Get(currentNode);
    for (svtkm::Id i = 1; i <= quadCount; ++i)
    {
      const svtkm::Id quadIndex = leafs.Get(currentNode + i);
      if (quadIndex < QuadIds.GetNumberOfValues())
      {
        IdType pointIndex = QuadIds.Get(quadIndex);
        Precision dist;
        svtkm::Vec<Precision, 3> q, r, s, t;
        q = svtkm::Vec<Precision, 3>(points.Get(pointIndex[1]));
        r = svtkm::Vec<Precision, 3>(points.Get(pointIndex[2]));
        s = svtkm::Vec<Precision, 3>(points.Get(pointIndex[3]));
        t = svtkm::Vec<Precision, 3>(points.Get(pointIndex[4]));
        Precision u, v;

        bool ret = quad(origin, dir, q, r, s, t, u, v, dist);
        if (ret)
        {
          if (dist < closestDistance && dist > minDistance)
          {
            //matid = svtkm::Vec<, 3>(points.Get(cur_offset + 2))[0];
            closestDistance = dist;
            hitIndex = quadIndex;
            minU = u;
            minV = v;
          }
        }
      }
    } // for
  }
};

class QuadExecWrapper : public svtkm::cont::ExecutionObjectBase
{
protected:
  using IdType = svtkm::Vec<svtkm::Id, 5>;
  using IdHandle = svtkm::cont::ArrayHandle<svtkm::Vec<svtkm::Id, 5>>;
  IdHandle QuadIds;

public:
  QuadExecWrapper(IdHandle& quadIds)
    : QuadIds(quadIds)
  {
  }

  template <typename Device>
  SVTKM_CONT QuadLeafIntersector<Device> PrepareForExecution(Device) const
  {
    return QuadLeafIntersector<Device>(QuadIds);
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
                                   const svtkm::Vec<Precision, 3>& rayDir,
                                   Precision& normalX,
                                   Precision& normalY,
                                   Precision& normalZ,
                                   const PointPortalType& points,
                                   const IndicesPortalType& indicesPortal) const
  {
    if (hitIndex < 0)
      return;

    svtkm::Vec<svtkm::Id, 5> quadId = indicesPortal.Get(hitIndex);

    svtkm::Vec<Precision, 3> a, b, c;
    a = points.Get(quadId[1]);
    b = points.Get(quadId[2]);
    c = points.Get(quadId[3]);

    svtkm::Vec<Precision, 3> normal = svtkm::TriangleNormal(a, b, c);
    svtkm::Normalize(normal);

    //flip the normal if its pointing the wrong way
    if (svtkm::dot(normal, rayDir) > 0.f)
      normal = -normal;

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

    //TODO: this should be interpolated?
    svtkm::Vec<svtkm::Id, 5> pointId = indicesPortal.Get(hitIndex);

    scalar = Precision(scalars.Get(pointId[0]));
    //normalize
    scalar = (scalar - MinScalar) * invDeltaScalar;
  }
}; //class GetScalar

} // namespace detail

QuadIntersector::QuadIntersector()
  : ShapeIntersector()
{
}

QuadIntersector::~QuadIntersector()
{
}


void QuadIntersector::IntersectRays(Ray<svtkm::Float32>& rays, bool returnCellIndex)
{
  IntersectRaysImp(rays, returnCellIndex);
}

void QuadIntersector::IntersectRays(Ray<svtkm::Float64>& rays, bool returnCellIndex)
{
  IntersectRaysImp(rays, returnCellIndex);
}

template <typename Precision>
void QuadIntersector::IntersectRaysImp(Ray<Precision>& rays, bool svtkmNotUsed(returnCellIndex))
{

  detail::QuadExecWrapper leafIntersector(this->QuadIds);

  BVHTraverser traverser;
  traverser.IntersectRays(rays, this->BVH, leafIntersector, this->CoordsHandle);

  RayOperations::UpdateRayStatus(rays);
}

template <typename Precision>
void QuadIntersector::IntersectionDataImp(Ray<Precision>& rays,
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
    .Invoke(rays.HitIdx, rays.Dir, rays.NormalX, rays.NormalY, rays.NormalZ, CoordsHandle, QuadIds);

  svtkm::worklet::DispatcherMapField<detail::GetScalar<Precision>>(
    detail::GetScalar<Precision>(svtkm::Float32(scalarRange.Min), svtkm::Float32(scalarRange.Max)))
    .Invoke(rays.HitIdx,
            rays.Scalar,
            scalarField.GetData().ResetTypes(svtkm::TypeListFieldScalar()),
            QuadIds);
}

void QuadIntersector::IntersectionData(Ray<svtkm::Float32>& rays,
                                       const svtkm::cont::Field scalarField,
                                       const svtkm::Range& scalarRange)
{
  IntersectionDataImp(rays, scalarField, scalarRange);
}

void QuadIntersector::IntersectionData(Ray<svtkm::Float64>& rays,
                                       const svtkm::cont::Field scalarField,
                                       const svtkm::Range& scalarRange)
{
  IntersectionDataImp(rays, scalarField, scalarRange);
}

void QuadIntersector::SetData(const svtkm::cont::CoordinateSystem& coords,
                              svtkm::cont::ArrayHandle<svtkm::Vec<svtkm::Id, 5>> quadIds)
{

  this->QuadIds = quadIds;
  this->CoordsHandle = coords;
  AABBs AABB;

  svtkm::worklet::DispatcherMapField<detail::FindQuadAABBs> faabbsInvoker;
  faabbsInvoker.Invoke(this->QuadIds,
                       AABB.xmins,
                       AABB.ymins,
                       AABB.zmins,
                       AABB.xmaxs,
                       AABB.ymaxs,
                       AABB.zmaxs,
                       CoordsHandle);

  this->SetAABBs(AABB);
}

svtkm::Id QuadIntersector::GetNumberOfShapes() const
{
  return QuadIds.GetNumberOfValues();
}
}
}
} //namespace svtkm::rendering::raytracing
