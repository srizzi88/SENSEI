//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_rendering_raytracing_MeshConnectivityBase
#define svtk_m_rendering_raytracing_MeshConnectivityBase

#include <sstream>
#include <svtkm/CellShape.h>
#include <svtkm/VirtualObjectBase.h>
#include <svtkm/cont/ErrorBadValue.h>
#include <svtkm/cont/VirtualObjectHandle.h>
#include <svtkm/rendering/raytracing/BoundingVolumeHierarchy.h>
#include <svtkm/rendering/raytracing/CellTables.h>
#include <svtkm/rendering/raytracing/Logger.h>
#include <svtkm/rendering/raytracing/Ray.h>
#include <svtkm/rendering/raytracing/TriangleIntersector.h>

namespace svtkm
{
namespace rendering
{
namespace raytracing
{
//
// Base class for different types of face-to-connecting-cell
// and other mesh information
//
class SVTKM_ALWAYS_EXPORT MeshConnectivityBase : public VirtualObjectBase
{
public:
  SVTKM_EXEC_CONT
  virtual svtkm::Id GetConnectingCell(const svtkm::Id& cellId, const svtkm::Id& face) const = 0;

  SVTKM_EXEC_CONT
  virtual svtkm::Int32 GetCellIndices(svtkm::Id cellIndices[8], const svtkm::Id& cellId) const = 0;

  SVTKM_EXEC_CONT
  virtual svtkm::UInt8 GetCellShape(const svtkm::Id& cellId) const = 0;
};

// A simple concrete type to wrap MeshConnectivityBase so we can
// pass an ExeObject to worklets.
class MeshWrapper
{
private:
  MeshConnectivityBase* MeshConn;

public:
  MeshWrapper() {}

  MeshWrapper(MeshConnectivityBase* meshConn)
    : MeshConn(meshConn){};

  SVTKM_EXEC_CONT
  svtkm::Id GetConnectingCell(const svtkm::Id& cellId, const svtkm::Id& face) const
  {
    return MeshConn->GetConnectingCell(cellId, face);
  }

  SVTKM_EXEC_CONT
  svtkm::Int32 GetCellIndices(svtkm::Id cellIndices[8], const svtkm::Id& cellId) const
  {
    return MeshConn->GetCellIndices(cellIndices, cellId);
  }

  SVTKM_EXEC_CONT
  svtkm::UInt8 GetCellShape(const svtkm::Id& cellId) const { return MeshConn->GetCellShape(cellId); }
};

class SVTKM_ALWAYS_EXPORT MeshConnStructured : public MeshConnectivityBase
{
protected:
  typedef typename svtkm::cont::ArrayHandle<svtkm::Id4> Id4Handle;
  svtkm::Id3 CellDims;
  svtkm::Id3 PointDims;

  SVTKM_CONT MeshConnStructured() = default;

public:
  SVTKM_CONT
  MeshConnStructured(const svtkm::Id3& cellDims, const svtkm::Id3& pointDims)
    : CellDims(cellDims)
    , PointDims(pointDims)
  {
  }

  SVTKM_EXEC_CONT
  svtkm::Id GetConnectingCell(const svtkm::Id& cellId, const svtkm::Id& face) const override
  {
    //TODO: there is probably a better way to do this.
    svtkm::Id3 logicalCellId;
    logicalCellId[0] = cellId % CellDims[0];
    logicalCellId[1] = (cellId / CellDims[0]) % CellDims[1];
    logicalCellId[2] = cellId / (CellDims[0] * CellDims[1]);
    if (face == 0)
      logicalCellId[1] -= 1;
    if (face == 2)
      logicalCellId[1] += 1;
    if (face == 1)
      logicalCellId[0] += 1;
    if (face == 3)
      logicalCellId[0] -= 1;
    if (face == 4)
      logicalCellId[2] -= 1;
    if (face == 5)
      logicalCellId[2] += 1;
    svtkm::Id nextCell =
      (logicalCellId[2] * CellDims[1] + logicalCellId[1]) * CellDims[0] + logicalCellId[0];
    bool validCell = true;
    if (logicalCellId[0] >= CellDims[0])
      validCell = false;
    if (logicalCellId[1] >= CellDims[1])
      validCell = false;
    if (logicalCellId[2] >= CellDims[2])
      validCell = false;
    svtkm::Id minId = svtkm::Min(logicalCellId[0], svtkm::Min(logicalCellId[1], logicalCellId[2]));
    if (minId < 0)
      validCell = false;
    if (!validCell)
      nextCell = -1;
    return nextCell;
  }

  SVTKM_EXEC_CONT
  svtkm::Int32 GetCellIndices(svtkm::Id cellIndices[8], const svtkm::Id& cellIndex) const override
  {
    svtkm::Id3 cellId;
    cellId[0] = cellIndex % CellDims[0];
    cellId[1] = (cellIndex / CellDims[0]) % CellDims[1];
    cellId[2] = cellIndex / (CellDims[0] * CellDims[1]);
    cellIndices[0] = (cellId[2] * PointDims[1] + cellId[1]) * PointDims[0] + cellId[0];
    cellIndices[1] = cellIndices[0] + 1;
    cellIndices[2] = cellIndices[1] + PointDims[0];
    cellIndices[3] = cellIndices[2] - 1;
    cellIndices[4] = cellIndices[0] + PointDims[0] * PointDims[1];
    cellIndices[5] = cellIndices[4] + 1;
    cellIndices[6] = cellIndices[5] + PointDims[0];
    cellIndices[7] = cellIndices[6] - 1;
    return 8;
  }

  SVTKM_EXEC
  svtkm::UInt8 GetCellShape(const svtkm::Id& svtkmNotUsed(cellId)) const override
  {
    return svtkm::UInt8(CELL_SHAPE_HEXAHEDRON);
  }
}; // MeshConnStructured

template <typename Device>
class SVTKM_ALWAYS_EXPORT MeshConnUnstructured : public MeshConnectivityBase
{
protected:
  using IdHandle = typename svtkm::cont::ArrayHandle<svtkm::Id>;
  using UCharHandle = typename svtkm::cont::ArrayHandle<svtkm::UInt8>;
  using IdConstPortal = typename IdHandle::ExecutionTypes<Device>::PortalConst;
  using UCharConstPortal = typename UCharHandle::ExecutionTypes<Device>::PortalConst;

  // Constant Portals for the execution Environment
  //FaceConn
  IdConstPortal FaceConnPortal;
  IdConstPortal FaceOffsetsPortal;
  //Cell Set
  IdConstPortal CellConnPortal;
  IdConstPortal CellOffsetsPortal;
  UCharConstPortal ShapesPortal;

  SVTKM_CONT MeshConnUnstructured() = default;

public:
  SVTKM_CONT
  MeshConnUnstructured(const IdHandle& faceConnectivity,
                       const IdHandle& faceOffsets,
                       const IdHandle& cellConn,
                       const IdHandle& cellOffsets,
                       const UCharHandle& shapes)
    : FaceConnPortal(faceConnectivity.PrepareForInput(Device()))
    , FaceOffsetsPortal(faceOffsets.PrepareForInput(Device()))
    , CellConnPortal(cellConn.PrepareForInput(Device()))
    , CellOffsetsPortal(cellOffsets.PrepareForInput(Device()))
    , ShapesPortal(shapes.PrepareForInput(Device()))
  {
  }

  SVTKM_EXEC_CONT
  svtkm::Id GetConnectingCell(const svtkm::Id& cellId, const svtkm::Id& face) const override
  {
    BOUNDS_CHECK(FaceOffsetsPortal, cellId);
    svtkm::Id cellStartIndex = FaceOffsetsPortal.Get(cellId);
    BOUNDS_CHECK(FaceConnPortal, cellStartIndex + face);
    return FaceConnPortal.Get(cellStartIndex + face);
  }

  //----------------------------------------------------------------------------
  SVTKM_EXEC
  svtkm::Int32 GetCellIndices(svtkm::Id cellIndices[8], const svtkm::Id& cellId) const override
  {
    const svtkm::Int32 shapeId = static_cast<svtkm::Int32>(ShapesPortal.Get(cellId));
    CellTables tables;
    const svtkm::Int32 numIndices = tables.FaceLookUp(tables.CellTypeLookUp(shapeId), 2);
    BOUNDS_CHECK(CellOffsetsPortal, cellId);
    const svtkm::Id cellOffset = CellOffsetsPortal.Get(cellId);

    for (svtkm::Int32 i = 0; i < numIndices; ++i)
    {
      BOUNDS_CHECK(CellConnPortal, cellOffset + i);
      cellIndices[i] = CellConnPortal.Get(cellOffset + i);
    }
    return numIndices;
  }

  //----------------------------------------------------------------------------
  SVTKM_EXEC
  svtkm::UInt8 GetCellShape(const svtkm::Id& cellId) const override
  {
    BOUNDS_CHECK(ShapesPortal, cellId)
    return ShapesPortal.Get(cellId);
  }

}; // MeshConnUnstructured

template <typename Device>
class MeshConnSingleType : public MeshConnectivityBase
{
protected:
  using IdHandle = typename svtkm::cont::ArrayHandle<svtkm::Id>;
  using IdConstPortal = typename IdHandle::ExecutionTypes<Device>::PortalConst;

  using CountingHandle = typename svtkm::cont::ArrayHandleCounting<svtkm::Id>;
  using CountingPortal = typename CountingHandle::ExecutionTypes<Device>::PortalConst;
  // Constant Portals for the execution Environment
  IdConstPortal FaceConnPortal;
  IdConstPortal CellConnectivityPortal;
  CountingPortal CellOffsetsPortal;

  svtkm::Int32 ShapeId;
  svtkm::Int32 NumIndices;
  svtkm::Int32 NumFaces;

private:
  SVTKM_CONT
  MeshConnSingleType() {}

public:
  SVTKM_CONT
  MeshConnSingleType(IdHandle& faceConn,
                     IdHandle& cellConn,
                     CountingHandle& cellOffsets,
                     svtkm::Int32 shapeId,
                     svtkm::Int32 numIndices,
                     svtkm::Int32 numFaces)
    : FaceConnPortal(faceConn.PrepareForInput(Device()))
    , CellConnectivityPortal(cellConn.PrepareForInput(Device()))
    , CellOffsetsPortal(cellOffsets.PrepareForInput(Device()))
    , ShapeId(shapeId)
    , NumIndices(numIndices)
    , NumFaces(numFaces)
  {
  }

  //----------------------------------------------------------------------------
  //                       Execution Environment Methods
  //----------------------------------------------------------------------------
  SVTKM_EXEC
  svtkm::Id GetConnectingCell(const svtkm::Id& cellId, const svtkm::Id& face) const override
  {
    BOUNDS_CHECK(CellOffsetsPortal, cellId);
    svtkm::Id cellStartIndex = cellId * NumFaces;
    BOUNDS_CHECK(FaceConnPortal, cellStartIndex + face);
    return FaceConnPortal.Get(cellStartIndex + face);
  }

  SVTKM_EXEC
  svtkm::Int32 GetCellIndices(svtkm::Id cellIndices[8], const svtkm::Id& cellId) const override
  {
    BOUNDS_CHECK(CellOffsetsPortal, cellId);
    const svtkm::Id cellOffset = CellOffsetsPortal.Get(cellId);

    for (svtkm::Int32 i = 0; i < NumIndices; ++i)
    {
      BOUNDS_CHECK(CellConnectivityPortal, cellOffset + i);
      cellIndices[i] = CellConnectivityPortal.Get(cellOffset + i);
    }

    return NumIndices;
  }

  //----------------------------------------------------------------------------
  SVTKM_EXEC
  svtkm::UInt8 GetCellShape(const svtkm::Id& svtkmNotUsed(cellId)) const override
  {
    return svtkm::UInt8(ShapeId);
  }

}; //MeshConn Single type specialization

class SVTKM_ALWAYS_EXPORT MeshConnHandle
  : public svtkm::cont::VirtualObjectHandle<MeshConnectivityBase>
{
private:
  using Superclass = svtkm::cont::VirtualObjectHandle<MeshConnectivityBase>;

public:
  MeshConnHandle() = default;

  template <typename MeshConnType, typename DeviceAdapterList = SVTKM_DEFAULT_DEVICE_ADAPTER_LIST>
  explicit MeshConnHandle(MeshConnType* meshConn,
                          bool aquireOwnership = true,
                          DeviceAdapterList devices = DeviceAdapterList())
    : Superclass(meshConn, aquireOwnership, devices)
  {
  }
};

template <typename MeshConnType, typename DeviceAdapterList = SVTKM_DEFAULT_DEVICE_ADAPTER_LIST>
SVTKM_CONT MeshConnHandle make_MeshConnHandle(MeshConnType&& func,
                                             DeviceAdapterList devices = DeviceAdapterList())
{
  using IFType = typename std::remove_reference<MeshConnType>::type;
  return MeshConnHandle(new IFType(std::forward<MeshConnType>(func)), true, devices);
}
}
}
} //namespace svtkm::rendering::raytracing

#ifdef SVTKM_CUDA

// Cuda seems to have a bug where it expects the template class VirtualObjectTransfer
// to be instantiated in a consistent order among all the translation units of an
// executable. Failing to do so results in random crashes and incorrect results.
// We workaroud this issue by explicitly instantiating VirtualObjectTransfer for
// all the implicit functions here.

#include <svtkm/cont/cuda/internal/VirtualObjectTransferCuda.h>
SVTKM_EXPLICITLY_INSTANTIATE_TRANSFER(svtkm::rendering::raytracing::MeshConnStructured);
SVTKM_EXPLICITLY_INSTANTIATE_TRANSFER(
  svtkm::rendering::raytracing::MeshConnUnstructured<svtkm::cont::DeviceAdapterTagCuda>);

#endif

#endif // MeshConnectivityBase
