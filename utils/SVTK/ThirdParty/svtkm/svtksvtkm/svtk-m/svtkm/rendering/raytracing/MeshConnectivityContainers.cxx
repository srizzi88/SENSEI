//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#include <sstream>
#include <svtkm/CellShape.h>
#include <svtkm/cont/ErrorBadValue.h>
#include <svtkm/cont/Timer.h>
#include <svtkm/cont/internal/DeviceAdapterListHelpers.h>
#include <svtkm/rendering/raytracing/BoundingVolumeHierarchy.h>
#include <svtkm/rendering/raytracing/Logger.h>
#include <svtkm/rendering/raytracing/MeshConnectivityBase.h>
#include <svtkm/rendering/raytracing/MeshConnectivityContainers.h>
#include <svtkm/rendering/raytracing/Ray.h>
#include <svtkm/rendering/raytracing/TriangleIntersector.h>
namespace svtkm
{
namespace rendering
{
namespace raytracing
{

MeshConnContainer::MeshConnContainer(){};
MeshConnContainer::~MeshConnContainer(){};

template <typename T>
SVTKM_CONT void MeshConnContainer::FindEntryImpl(Ray<T>& rays)
{
  bool getCellIndex = true;

  Intersector.SetUseWaterTight(true);

  Intersector.IntersectRays(rays, getCellIndex);
}

MeshWrapper MeshConnContainer::PrepareForExecution(const svtkm::cont::DeviceAdapterId deviceId)
{
  return MeshWrapper(const_cast<MeshConnectivityBase*>(this->Construct(deviceId)));
}

void MeshConnContainer::FindEntry(Ray<svtkm::Float32>& rays)
{
  this->FindEntryImpl(rays);
}

void MeshConnContainer::FindEntry(Ray<svtkm::Float64>& rays)
{
  this->FindEntryImpl(rays);
}

SVTKM_CONT
UnstructuredContainer::UnstructuredContainer(const svtkm::cont::CellSetExplicit<>& cellset,
                                             const svtkm::cont::CoordinateSystem& coords,
                                             IdHandle& faceConn,
                                             IdHandle& faceOffsets,
                                             Id4Handle& triangles)
  : FaceConnectivity(faceConn)
  , FaceOffsets(faceOffsets)
  , Cellset(cellset)
  , Coords(coords)
{
  this->Triangles = triangles;
  //
  // Grab the cell arrays
  //
  CellConn =
    Cellset.GetConnectivityArray(svtkm::TopologyElementTagCell(), svtkm::TopologyElementTagPoint());
  CellOffsets =
    Cellset.GetOffsetsArray(svtkm::TopologyElementTagCell(), svtkm::TopologyElementTagPoint());
  Shapes = Cellset.GetShapesArray(svtkm::TopologyElementTagCell(), svtkm::TopologyElementTagPoint());

  Intersector.SetData(Coords, Triangles);
}

UnstructuredContainer::~UnstructuredContainer(){};

const MeshConnectivityBase* UnstructuredContainer::Construct(
  const svtkm::cont::DeviceAdapterId deviceId)
{
  switch (deviceId.GetValue())
  {
#ifdef SVTKM_ENABLE_OPENMP
    case SVTKM_DEVICE_ADAPTER_OPENMP:
      using OMP = svtkm::cont::DeviceAdapterTagOpenMP;
      {
        MeshConnUnstructured<OMP> conn(this->FaceConnectivity,
                                       this->FaceOffsets,
                                       this->CellConn,
                                       this->CellOffsets,
                                       this->Shapes);
        Handle = make_MeshConnHandle(conn);
      }
      return Handle.PrepareForExecution(OMP());
#endif
#ifdef SVTKM_ENABLE_TBB
    case SVTKM_DEVICE_ADAPTER_TBB:
      using TBB = svtkm::cont::DeviceAdapterTagTBB;
      {
        MeshConnUnstructured<TBB> conn(this->FaceConnectivity,
                                       this->FaceOffsets,
                                       this->CellConn,
                                       this->CellOffsets,
                                       this->Shapes);
        Handle = make_MeshConnHandle(conn);
      }
      return Handle.PrepareForExecution(TBB());
#endif
#ifdef SVTKM_ENABLE_CUDA
    case SVTKM_DEVICE_ADAPTER_CUDA:
      using CUDA = svtkm::cont::DeviceAdapterTagCuda;
      {
        MeshConnUnstructured<CUDA> conn(this->FaceConnectivity,
                                        this->FaceOffsets,
                                        this->CellConn,
                                        this->CellOffsets,
                                        this->Shapes);
        Handle = make_MeshConnHandle(conn);
      }
      return Handle.PrepareForExecution(CUDA());
#endif
    case SVTKM_DEVICE_ADAPTER_SERIAL:
      SVTKM_FALLTHROUGH;
    default:
      using SERIAL = svtkm::cont::DeviceAdapterTagSerial;
      {
        MeshConnUnstructured<SERIAL> conn(this->FaceConnectivity,
                                          this->FaceOffsets,
                                          this->CellConn,
                                          this->CellOffsets,
                                          this->Shapes);
        Handle = make_MeshConnHandle(conn);
      }
      return Handle.PrepareForExecution(SERIAL());
  }
}

SVTKM_CONT
UnstructuredSingleContainer::UnstructuredSingleContainer()
{
}

SVTKM_CONT
UnstructuredSingleContainer::UnstructuredSingleContainer(
  const svtkm::cont::CellSetSingleType<>& cellset,
  const svtkm::cont::CoordinateSystem& coords,
  IdHandle& faceConn,
  Id4Handle& triangles)
  : FaceConnectivity(faceConn)
  , Coords(coords)
  , Cellset(cellset)
{

  this->Triangles = triangles;

  this->Intersector.SetUseWaterTight(true);

  CellConnectivity =
    Cellset.GetConnectivityArray(svtkm::TopologyElementTagCell(), svtkm::TopologyElementTagPoint());
  svtkm::cont::ArrayHandleConstant<svtkm::UInt8> shapes =
    Cellset.GetShapesArray(svtkm::TopologyElementTagCell(), svtkm::TopologyElementTagPoint());

  ShapeId = shapes.GetPortalConstControl().Get(0);
  CellTables tables;
  NumIndices = tables.FaceLookUp(tables.CellTypeLookUp(ShapeId), 2);

  if (NumIndices == 0)
  {
    std::stringstream message;
    message << "Unstructured Mesh Connecitity Single type Error: unsupported cell type: ";
    message << ShapeId;
    throw svtkm::cont::ErrorBadValue(message.str());
  }
  svtkm::Id start = 0;
  NumFaces = tables.FaceLookUp(tables.CellTypeLookUp(ShapeId), 1);
  svtkm::Id numCells = CellConnectivity.GetPortalConstControl().GetNumberOfValues();
  CellOffsets = svtkm::cont::make_ArrayHandleCounting<svtkm::Id>(start, NumIndices, numCells);

  Logger* logger = Logger::GetInstance();
  logger->OpenLogEntry("mesh_conn_construction");

  Intersector.SetData(Coords, Triangles);
}

const MeshConnectivityBase* UnstructuredSingleContainer::Construct(
  const svtkm::cont::DeviceAdapterId deviceId)
{
  switch (deviceId.GetValue())
  {
#ifdef SVTKM_ENABLE_OPENMP
    case SVTKM_DEVICE_ADAPTER_OPENMP:
      using OMP = svtkm::cont::DeviceAdapterTagOpenMP;
      {
        MeshConnSingleType<OMP> conn(this->FaceConnectivity,
                                     this->CellConnectivity,
                                     this->CellOffsets,
                                     this->ShapeId,
                                     this->NumIndices,
                                     this->NumFaces);
        Handle = make_MeshConnHandle(conn);
      }
      return Handle.PrepareForExecution(OMP());
#endif
#ifdef SVTKM_ENABLE_TBB
    case SVTKM_DEVICE_ADAPTER_TBB:
      using TBB = svtkm::cont::DeviceAdapterTagTBB;
      {
        MeshConnSingleType<TBB> conn(this->FaceConnectivity,
                                     this->CellConnectivity,
                                     this->CellOffsets,
                                     this->ShapeId,
                                     this->NumIndices,
                                     this->NumFaces);
        Handle = make_MeshConnHandle(conn);
      }
      return Handle.PrepareForExecution(TBB());
#endif
#ifdef SVTKM_ENABLE_CUDA
    case SVTKM_DEVICE_ADAPTER_CUDA:
      using CUDA = svtkm::cont::DeviceAdapterTagCuda;
      {
        MeshConnSingleType<CUDA> conn(this->FaceConnectivity,
                                      this->CellConnectivity,
                                      this->CellOffsets,
                                      this->ShapeId,
                                      this->NumIndices,
                                      this->NumFaces);
        Handle = make_MeshConnHandle(conn);
      }
      return Handle.PrepareForExecution(CUDA());
#endif
    case SVTKM_DEVICE_ADAPTER_SERIAL:
      SVTKM_FALLTHROUGH;
    default:
      using SERIAL = svtkm::cont::DeviceAdapterTagSerial;
      {
        MeshConnSingleType<SERIAL> conn(this->FaceConnectivity,
                                        this->CellConnectivity,
                                        this->CellOffsets,
                                        this->ShapeId,
                                        this->NumIndices,
                                        this->NumFaces);
        Handle = make_MeshConnHandle(conn);
      }
      return Handle.PrepareForExecution(SERIAL());
  }
}

StructuredContainer::StructuredContainer(const svtkm::cont::CellSetStructured<3>& cellset,
                                         const svtkm::cont::CoordinateSystem& coords,
                                         Id4Handle& triangles)
  : Coords(coords)
  , Cellset(cellset)
{

  Triangles = triangles;
  Intersector.SetUseWaterTight(true);

  PointDims = Cellset.GetPointDimensions();
  CellDims = Cellset.GetCellDimensions();

  this->Intersector.SetData(Coords, Triangles);
}

const MeshConnectivityBase* StructuredContainer::Construct(
  const svtkm::cont::DeviceAdapterId deviceId)
{

  MeshConnStructured conn(CellDims, PointDims);
  Handle = make_MeshConnHandle(conn);

  switch (deviceId.GetValue())
  {
#ifdef SVTKM_ENABLE_OPENMP
    case SVTKM_DEVICE_ADAPTER_OPENMP:
      return Handle.PrepareForExecution(svtkm::cont::DeviceAdapterTagOpenMP());
#endif
#ifdef SVTKM_ENABLE_TBB
    case SVTKM_DEVICE_ADAPTER_TBB:
      return Handle.PrepareForExecution(svtkm::cont::DeviceAdapterTagTBB());
#endif
#ifdef SVTKM_ENABLE_CUDA
    case SVTKM_DEVICE_ADAPTER_CUDA:
      return Handle.PrepareForExecution(svtkm::cont::DeviceAdapterTagCuda());
#endif
    case SVTKM_DEVICE_ADAPTER_SERIAL:
      SVTKM_FALLTHROUGH;
    default:
      return Handle.PrepareForExecution(svtkm::cont::DeviceAdapterTagSerial());
  }
}
}
}
} //namespace svtkm::rendering::raytracing
