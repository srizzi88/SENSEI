//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <svtkm/rendering/raytracing/TriangleIntersector.h>

#include <cstring>
#include <svtkm/cont/ArrayHandle.h>
#include <svtkm/cont/ArrayHandleCompositeVector.h>
#include <svtkm/rendering/raytracing/BVHTraverser.h>
#include <svtkm/rendering/raytracing/BoundingVolumeHierarchy.h>
#include <svtkm/rendering/raytracing/RayOperations.h>
#include <svtkm/rendering/raytracing/TriangleIntersections.h>

#include <svtkm/worklet/WorkletMapField.h>

namespace svtkm
{
namespace rendering
{
namespace raytracing
{
namespace detail
{

template <typename Device>
class WaterTightLeafIntersector
{
public:
  using Id4Handle = svtkm::cont::ArrayHandle<svtkm::Id4>;
  using Id4ArrayPortal = typename Id4Handle::ExecutionTypes<Device>::PortalConst;
  Id4ArrayPortal Triangles;

public:
  WaterTightLeafIntersector() = default;

  WaterTightLeafIntersector(const Id4Handle& triangles)
    : Triangles(triangles.PrepareForInput(Device()))
  {
  }

  template <typename PointPortalType, typename LeafPortalType, typename Precision>
  SVTKM_EXEC inline void IntersectLeaf(const svtkm::Int32& currentNode,
                                      const svtkm::Vec<Precision, 3>& origin,
                                      const svtkm::Vec<Precision, 3>& dir,
                                      const PointPortalType& points,
                                      svtkm::Id& hitIndex,
                                      Precision& closestDistance,
                                      Precision& minU,
                                      Precision& minV,
                                      LeafPortalType leafs,
                                      const Precision& minDistance) const
  {
    const svtkm::Id triangleCount = leafs.Get(currentNode);
    WaterTight intersector;
    for (svtkm::Id i = 1; i <= triangleCount; ++i)
    {
      const svtkm::Id triIndex = leafs.Get(currentNode + i);
      svtkm::Vec<Id, 4> triangle = Triangles.Get(triIndex);
      svtkm::Vec<Precision, 3> a = svtkm::Vec<Precision, 3>(points.Get(triangle[1]));
      svtkm::Vec<Precision, 3> b = svtkm::Vec<Precision, 3>(points.Get(triangle[2]));
      svtkm::Vec<Precision, 3> c = svtkm::Vec<Precision, 3>(points.Get(triangle[3]));
      Precision distance = -1.;
      Precision u, v;

      intersector.IntersectTri(a, b, c, dir, distance, u, v, origin);
      if (distance != -1. && distance < closestDistance && distance > minDistance)
      {
        closestDistance = distance;
        minU = u;
        minV = v;
        hitIndex = triIndex;
      }
    } // for
  }
};

template <typename Device>
class MollerTriLeafIntersector
{
  //protected:
public:
  using Id4Handle = svtkm::cont::ArrayHandle<svtkm::Id4>;
  using Id4ArrayPortal = typename Id4Handle::ExecutionTypes<Device>::PortalConst;
  Id4ArrayPortal Triangles;

public:
  MollerTriLeafIntersector() {}

  MollerTriLeafIntersector(const Id4Handle& triangles)
    : Triangles(triangles.PrepareForInput(Device()))
  {
  }

  template <typename PointPortalType, typename LeafPortalType, typename Precision>
  SVTKM_EXEC inline void IntersectLeaf(const svtkm::Int32& currentNode,
                                      const svtkm::Vec<Precision, 3>& origin,
                                      const svtkm::Vec<Precision, 3>& dir,
                                      const PointPortalType& points,
                                      svtkm::Id& hitIndex,
                                      Precision& closestDistance,
                                      Precision& minU,
                                      Precision& minV,
                                      LeafPortalType leafs,
                                      const Precision& minDistance) const
  {
    const svtkm::Id triangleCount = leafs.Get(currentNode);
    Moller intersector;
    for (svtkm::Id i = 1; i <= triangleCount; ++i)
    {
      const svtkm::Id triIndex = leafs.Get(currentNode + i);
      svtkm::Vec<Id, 4> triangle = Triangles.Get(triIndex);
      svtkm::Vec<Precision, 3> a = svtkm::Vec<Precision, 3>(points.Get(triangle[1]));
      svtkm::Vec<Precision, 3> b = svtkm::Vec<Precision, 3>(points.Get(triangle[2]));
      svtkm::Vec<Precision, 3> c = svtkm::Vec<Precision, 3>(points.Get(triangle[3]));
      Precision distance = -1.;
      Precision u, v;

      intersector.IntersectTri(a, b, c, dir, distance, u, v, origin);

      if (distance != -1. && distance < closestDistance && distance > minDistance)
      {
        closestDistance = distance;
        minU = u;
        minV = v;
        hitIndex = triIndex;
      }
    } // for
  }
};

class MollerExecWrapper : public svtkm::cont::ExecutionObjectBase
{
protected:
  using Id4Handle = svtkm::cont::ArrayHandle<svtkm::Id4>;
  Id4Handle Triangles;

public:
  MollerExecWrapper(Id4Handle& triangles)
    : Triangles(triangles)
  {
  }

  template <typename Device>
  SVTKM_CONT MollerTriLeafIntersector<Device> PrepareForExecution(Device) const
  {
    return MollerTriLeafIntersector<Device>(Triangles);
  }
};

class WaterTightExecWrapper : public svtkm::cont::ExecutionObjectBase
{
protected:
  using Id4Handle = svtkm::cont::ArrayHandle<svtkm::Id4>;
  Id4Handle Triangles;

public:
  WaterTightExecWrapper(Id4Handle& triangles)
    : Triangles(triangles)
  {
  }

  template <typename Device>
  SVTKM_CONT WaterTightLeafIntersector<Device> PrepareForExecution(Device) const
  {
    return WaterTightLeafIntersector<Device>(Triangles);
  }
};

class CellIndexFilter : public svtkm::worklet::WorkletMapField
{
public:
  SVTKM_CONT
  CellIndexFilter() {}
  typedef void ControlSignature(FieldInOut, WholeArrayIn);
  typedef void ExecutionSignature(_1, _2);
  template <typename TrianglePortalType>
  SVTKM_EXEC void operator()(svtkm::Id& hitIndex, TrianglePortalType& triangles) const
  {
    svtkm::Id cellIndex = -1;
    if (hitIndex != -1)
    {
      cellIndex = triangles.Get(hitIndex)[0];
    }

    hitIndex = cellIndex;
  }
}; //class CellIndexFilter

class TriangleIntersectionData
{
public:
  // Worklet to calutate the normals of a triagle if
  // none are stored in the data set
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

      svtkm::Vec<Id, 4> indices = indicesPortal.Get(hitIndex);
      svtkm::Vec<Precision, 3> a = points.Get(indices[1]);
      svtkm::Vec<Precision, 3> b = points.Get(indices[2]);
      svtkm::Vec<Precision, 3> c = points.Get(indices[3]);

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
  class LerpScalar : public svtkm::worklet::WorkletMapField
  {
  private:
    Precision MinScalar;
    Precision invDeltaScalar;

  public:
    SVTKM_CONT
    LerpScalar(const svtkm::Float32& minScalar, const svtkm::Float32& maxScalar)
      : MinScalar(minScalar)
    {
      //Make sure the we don't divide by zero on
      //something like an iso-surface
      if (maxScalar - MinScalar != 0.f)
        invDeltaScalar = 1.f / (maxScalar - MinScalar);
      else
        invDeltaScalar = 0.f;
    }
    typedef void ControlSignature(FieldIn,
                                  FieldIn,
                                  FieldIn,
                                  FieldInOut,
                                  WholeArrayIn,
                                  WholeArrayIn);
    typedef void ExecutionSignature(_1, _2, _3, _4, _5, _6);
    template <typename ScalarPortalType, typename IndicesPortalType>
    SVTKM_EXEC void operator()(const svtkm::Id& hitIndex,
                              const Precision& u,
                              const Precision& v,
                              Precision& lerpedScalar,
                              const ScalarPortalType& scalars,
                              const IndicesPortalType& indicesPortal) const
    {
      if (hitIndex < 0)
        return;

      svtkm::Vec<Id, 4> indices = indicesPortal.Get(hitIndex);

      Precision n = 1.f - u - v;
      Precision aScalar = Precision(scalars.Get(indices[1]));
      Precision bScalar = Precision(scalars.Get(indices[2]));
      Precision cScalar = Precision(scalars.Get(indices[3]));
      lerpedScalar = aScalar * n + bScalar * u + cScalar * v;
      //normalize
      lerpedScalar = (lerpedScalar - MinScalar) * invDeltaScalar;
    }
  }; //class LerpScalar

  template <typename Precision>
  class NodalScalar : public svtkm::worklet::WorkletMapField
  {
  private:
    Precision MinScalar;
    Precision invDeltaScalar;

  public:
    SVTKM_CONT
    NodalScalar(const svtkm::Float32& minScalar, const svtkm::Float32& maxScalar)
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

      svtkm::Vec<Id, 4> indices = indicesPortal.Get(hitIndex);

      //Todo: one normalization
      scalar = Precision(scalars.Get(indices[0]));

      //normalize
      scalar = (scalar - MinScalar) * invDeltaScalar;
    }
  }; //class LerpScalar

  template <typename Precision>
  SVTKM_CONT void Run(Ray<Precision>& rays,
                     svtkm::cont::ArrayHandle<svtkm::Id4> triangles,
                     svtkm::cont::CoordinateSystem coordsHandle,
                     const svtkm::cont::Field scalarField,
                     const svtkm::Range& scalarRange)
  {
    const bool isSupportedField = scalarField.IsFieldCell() || scalarField.IsFieldPoint();
    if (!isSupportedField)
    {
      throw svtkm::cont::ErrorBadValue("Field not accociated with cell set or points");
    }
    const bool isAssocPoints = scalarField.IsFieldPoint();

    // Find the triangle normal
    svtkm::worklet::DispatcherMapField<CalculateNormals>(CalculateNormals())
      .Invoke(
        rays.HitIdx, rays.Dir, rays.NormalX, rays.NormalY, rays.NormalZ, coordsHandle, triangles);

    // Calculate scalar value at intersection point
    if (isAssocPoints)
    {
      svtkm::worklet::DispatcherMapField<LerpScalar<Precision>>(
        LerpScalar<Precision>(svtkm::Float32(scalarRange.Min), svtkm::Float32(scalarRange.Max)))
        .Invoke(rays.HitIdx,
                rays.U,
                rays.V,
                rays.Scalar,
                scalarField.GetData().ResetTypes(ScalarRenderingTypes()),
                triangles);
    }
    else
    {
      svtkm::worklet::DispatcherMapField<NodalScalar<Precision>>(
        NodalScalar<Precision>(svtkm::Float32(scalarRange.Min), svtkm::Float32(scalarRange.Max)))
        .Invoke(rays.HitIdx,
                rays.Scalar,
                scalarField.GetData().ResetTypes(ScalarRenderingTypes()),
                triangles);
    }
  } // Run

}; // Class IntersectionData

#define AABB_EPSILON 0.00001f
class FindTriangleAABBs : public svtkm::worklet::WorkletMapField
{
public:
  SVTKM_CONT
  FindTriangleAABBs() {}
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
  SVTKM_EXEC void operator()(const svtkm::Id4 indices,
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
    point = static_cast<svtkm::Vec3f_32>(points.Get(indices[1]));
    xmin = point[0];
    ymin = point[1];
    zmin = point[2];
    xmax = xmin;
    ymax = ymin;
    zmax = zmin;
    point = static_cast<svtkm::Vec3f_32>(points.Get(indices[2]));
    xmin = svtkm::Min(xmin, point[0]);
    ymin = svtkm::Min(ymin, point[1]);
    zmin = svtkm::Min(zmin, point[2]);
    xmax = svtkm::Max(xmax, point[0]);
    ymax = svtkm::Max(ymax, point[1]);
    zmax = svtkm::Max(zmax, point[2]);
    point = static_cast<svtkm::Vec3f_32>(points.Get(indices[3]));
    xmin = svtkm::Min(xmin, point[0]);
    ymin = svtkm::Min(ymin, point[1]);
    zmin = svtkm::Min(zmin, point[2]);
    xmax = svtkm::Max(xmax, point[0]);
    ymax = svtkm::Max(ymax, point[1]);
    zmax = svtkm::Max(zmax, point[2]);


    svtkm::Float32 xEpsilon, yEpsilon, zEpsilon;
    const svtkm::Float32 minEpsilon = 1e-6f;
    xEpsilon = svtkm::Max(minEpsilon, AABB_EPSILON * (xmax - xmin));
    yEpsilon = svtkm::Max(minEpsilon, AABB_EPSILON * (ymax - ymin));
    zEpsilon = svtkm::Max(minEpsilon, AABB_EPSILON * (zmax - zmin));

    xmin -= xEpsilon;
    ymin -= yEpsilon;
    zmin -= zEpsilon;
    xmax += xEpsilon;
    ymax += yEpsilon;
    zmax += zEpsilon;
  }
}; //class FindAABBs
#undef AABB_EPSILON

} // namespace detail

TriangleIntersector::TriangleIntersector()
  : UseWaterTight(false)
{
}

void TriangleIntersector::SetUseWaterTight(bool useIt)
{
  UseWaterTight = useIt;
}

void TriangleIntersector::SetData(const svtkm::cont::CoordinateSystem& coords,
                                  svtkm::cont::ArrayHandle<svtkm::Id4> triangles)
{

  CoordsHandle = coords;
  Triangles = triangles;

  svtkm::rendering::raytracing::AABBs AABB;
  svtkm::worklet::DispatcherMapField<detail::FindTriangleAABBs>(detail::FindTriangleAABBs())
    .Invoke(Triangles,
            AABB.xmins,
            AABB.ymins,
            AABB.zmins,
            AABB.xmaxs,
            AABB.ymaxs,
            AABB.zmaxs,
            CoordsHandle);

  this->SetAABBs(AABB);
}

svtkm::cont::ArrayHandle<svtkm::Id4> TriangleIntersector::GetTriangles()
{
  return Triangles;
}



SVTKM_CONT void TriangleIntersector::IntersectRays(Ray<svtkm::Float32>& rays, bool returnCellIndex)
{
  IntersectRaysImp(rays, returnCellIndex);
}


SVTKM_CONT void TriangleIntersector::IntersectRays(Ray<svtkm::Float64>& rays, bool returnCellIndex)
{
  IntersectRaysImp(rays, returnCellIndex);
}

template <typename Precision>
SVTKM_CONT void TriangleIntersector::IntersectRaysImp(Ray<Precision>& rays, bool returnCellIndex)
{
  if (UseWaterTight)
  {
    detail::WaterTightExecWrapper leafIntersector(this->Triangles);
    BVHTraverser traverser;
    traverser.IntersectRays(rays, this->BVH, leafIntersector, this->CoordsHandle);
  }
  else
  {
    detail::MollerExecWrapper leafIntersector(this->Triangles);

    BVHTraverser traverser;
    traverser.IntersectRays(rays, this->BVH, leafIntersector, this->CoordsHandle);
  }
  // Normally we return the index of the triangle hit,
  // but in some cases we are only interested in the cell
  if (returnCellIndex)
  {
    svtkm::worklet::DispatcherMapField<detail::CellIndexFilter> cellIndexFilterDispatcher;
    cellIndexFilterDispatcher.Invoke(rays.HitIdx, Triangles);
  }
  // Update ray status
  RayOperations::UpdateRayStatus(rays);
}

SVTKM_CONT void TriangleIntersector::IntersectionData(Ray<svtkm::Float32>& rays,
                                                     const svtkm::cont::Field scalarField,
                                                     const svtkm::Range& scalarRange)
{
  IntersectionDataImp(rays, scalarField, scalarRange);
}

SVTKM_CONT void TriangleIntersector::IntersectionData(Ray<svtkm::Float64>& rays,
                                                     const svtkm::cont::Field scalarField,
                                                     const svtkm::Range& scalarRange)
{
  IntersectionDataImp(rays, scalarField, scalarRange);
}

template <typename Precision>
SVTKM_CONT void TriangleIntersector::IntersectionDataImp(Ray<Precision>& rays,
                                                        const svtkm::cont::Field scalarField,
                                                        const svtkm::Range& scalarRange)
{
  ShapeIntersector::IntersectionPoint(rays);
  detail::TriangleIntersectionData intData;
  intData.Run(rays, this->Triangles, this->CoordsHandle, scalarField, scalarRange);
}

svtkm::Id TriangleIntersector::GetNumberOfShapes() const
{
  return Triangles.GetNumberOfValues();
}
}
}
} //namespace svtkm::rendering::raytracing
