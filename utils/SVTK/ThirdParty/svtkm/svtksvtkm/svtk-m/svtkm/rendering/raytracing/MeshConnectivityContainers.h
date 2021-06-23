//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_rendering_raytracing_MeshConnectivityContainer_h
#define svtk_m_rendering_raytracing_MeshConnectivityContainer_h

#include <svtkm/cont/DataSet.h>
#include <svtkm/rendering/raytracing/MeshConnectivityBase.h>
#include <svtkm/rendering/raytracing/TriangleIntersector.h>

namespace svtkm
{
namespace rendering
{
namespace raytracing
{

class MeshConnContainer : svtkm::cont::ExecutionObjectBase
{
public:
  MeshConnContainer();
  virtual ~MeshConnContainer();

  virtual const MeshConnectivityBase* Construct(const svtkm::cont::DeviceAdapterId deviceId) = 0;

  MeshWrapper PrepareForExecution(const svtkm::cont::DeviceAdapterId deviceId);

  template <typename T>
  SVTKM_CONT void FindEntryImpl(Ray<T>& rays);

  void FindEntry(Ray<svtkm::Float32>& rays);

  void FindEntry(Ray<svtkm::Float64>& rays);

protected:
  using Id4Handle = typename svtkm::cont::ArrayHandle<svtkm::Id4>;
  // Mesh Boundary
  Id4Handle Triangles;
  TriangleIntersector Intersector;
  MeshConnHandle Handle;
};

class UnstructuredContainer : public MeshConnContainer
{
public:
  typedef svtkm::cont::ArrayHandle<svtkm::Id> IdHandle;
  typedef svtkm::cont::ArrayHandle<svtkm::Id4> Id4Handle;
  typedef svtkm::cont::ArrayHandle<svtkm::UInt8> UCharHandle;
  // Control Environment Handles
  // FaceConn
  IdHandle FaceConnectivity;
  IdHandle FaceOffsets;
  //Cell Set
  IdHandle CellConn;
  IdHandle CellOffsets;
  UCharHandle Shapes;

  svtkm::Bounds CoordinateBounds;
  svtkm::cont::CellSetExplicit<> Cellset;
  svtkm::cont::CoordinateSystem Coords;

private:
  SVTKM_CONT
  UnstructuredContainer(){};

public:
  SVTKM_CONT
  UnstructuredContainer(const svtkm::cont::CellSetExplicit<>& cellset,
                        const svtkm::cont::CoordinateSystem& coords,
                        IdHandle& faceConn,
                        IdHandle& faceOffsets,
                        Id4Handle& triangles);

  virtual ~UnstructuredContainer();

  const MeshConnectivityBase* Construct(const svtkm::cont::DeviceAdapterId deviceId);
};

class StructuredContainer : public MeshConnContainer
{
protected:
  typedef svtkm::cont::ArrayHandle<svtkm::Id4> Id4Handle;
  svtkm::Id3 CellDims;
  svtkm::Id3 PointDims;
  svtkm::Bounds CoordinateBounds;
  svtkm::cont::CoordinateSystem Coords;
  svtkm::cont::CellSetStructured<3> Cellset;

private:
  SVTKM_CONT
  StructuredContainer() {}

public:
  SVTKM_CONT
  StructuredContainer(const svtkm::cont::CellSetStructured<3>& cellset,
                      const svtkm::cont::CoordinateSystem& coords,
                      Id4Handle& triangles);

  const MeshConnectivityBase* Construct(const svtkm::cont::DeviceAdapterId deviceId) override;

}; //structure mesh conn

class UnstructuredSingleContainer : public MeshConnContainer
{
public:
  typedef svtkm::cont::ArrayHandle<svtkm::Id> IdHandle;
  typedef svtkm::cont::ArrayHandle<svtkm::Id4> Id4Handle;
  typedef svtkm::cont::ArrayHandleCounting<svtkm::Id> CountingHandle;
  typedef svtkm::cont::ArrayHandleConstant<svtkm::UInt8> ShapesHandle;
  typedef svtkm::cont::ArrayHandleConstant<svtkm::IdComponent> NumIndicesHandle;
  // Control Environment Handles
  IdHandle FaceConnectivity;
  CountingHandle CellOffsets;
  IdHandle CellConnectivity;

  svtkm::Bounds CoordinateBounds;
  svtkm::cont::CoordinateSystem Coords;
  svtkm::cont::CellSetSingleType<> Cellset;

  svtkm::Int32 ShapeId;
  svtkm::Int32 NumIndices;
  svtkm::Int32 NumFaces;

private:
  SVTKM_CONT
  UnstructuredSingleContainer();

public:
  SVTKM_CONT
  UnstructuredSingleContainer(const svtkm::cont::CellSetSingleType<>& cellset,
                              const svtkm::cont::CoordinateSystem& coords,
                              IdHandle& faceConn,
                              Id4Handle& externalFaces);

  const MeshConnectivityBase* Construct(const svtkm::cont::DeviceAdapterId deviceId) override;

}; //UnstructuredSingleContainer
}
}
} //namespace svtkm::rendering::raytracing
#endif
