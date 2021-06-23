//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_worklet_internal_TriangulateTables_h
#define svtk_m_worklet_internal_TriangulateTables_h

#include <svtkm/CellShape.h>
#include <svtkm/Types.h>

#include <svtkm/cont/ExecutionObjectBase.h>

#include <svtkm/cont/ArrayHandle.h>
#include <svtkm/cont/StorageBasic.h>

namespace svtkm
{
namespace worklet
{
namespace internal
{

using TriangulateArrayHandle =
  svtkm::cont::ArrayHandle<svtkm::IdComponent, svtkm::cont::StorageTagBasic>;

static svtkm::IdComponent TriangleCountData[svtkm::NUMBER_OF_CELL_SHAPES] = {
  0,  //  0 = svtkm::CELL_SHAPE_EMPTY_CELL
  0,  //  1 = svtkm::CELL_SHAPE_VERTEX
  0,  //  2 = svtkm::CELL_SHAPE_POLY_VERTEX
  0,  //  3 = svtkm::CELL_SHAPE_LINE
  0,  //  4 = svtkm::CELL_SHAPE_POLY_LINE
  1,  //  5 = svtkm::CELL_SHAPE_TRIANGLE
  0,  //  6 = svtkm::CELL_SHAPE_TRIANGLE_STRIP
  -1, //  7 = svtkm::CELL_SHAPE_POLYGON
  0,  //  8 = svtkm::CELL_SHAPE_PIXEL
  2,  //  9 = svtkm::CELL_SHAPE_QUAD
  0,  // 10 = svtkm::CELL_SHAPE_TETRA
  0,  // 11 = svtkm::CELL_SHAPE_VOXEL
  0,  // 12 = svtkm::CELL_SHAPE_HEXAHEDRON
  0,  // 13 = svtkm::CELL_SHAPE_WEDGE
  0   // 14 = svtkm::CELL_SHAPE_PYRAMID
};

static svtkm::IdComponent TriangleOffsetData[svtkm::NUMBER_OF_CELL_SHAPES] = {
  -1, //  0 = svtkm::CELL_SHAPE_EMPTY_CELL
  -1, //  1 = svtkm::CELL_SHAPE_VERTEX
  -1, //  2 = svtkm::CELL_SHAPE_POLY_VERTEX
  -1, //  3 = svtkm::CELL_SHAPE_LINE
  -1, //  4 = svtkm::CELL_SHAPE_POLY_LINE
  0,  //  5 = svtkm::CELL_SHAPE_TRIANGLE
  -1, //  6 = svtkm::CELL_SHAPE_TRIANGLE_STRIP
  -1, //  7 = svtkm::CELL_SHAPE_POLYGON
  -1, //  8 = svtkm::CELL_SHAPE_PIXEL
  1,  //  9 = svtkm::CELL_SHAPE_QUAD
  -1, // 10 = svtkm::CELL_SHAPE_TETRA
  -1, // 11 = svtkm::CELL_SHAPE_VOXEL
  -1, // 12 = svtkm::CELL_SHAPE_HEXAHEDRON
  -1, // 13 = svtkm::CELL_SHAPE_WEDGE
  -1  // 14 = svtkm::CELL_SHAPE_PYRAMID
};

static svtkm::IdComponent TriangleIndexData[] = {
  // svtkm::CELL_SHAPE_TRIANGLE
  0,
  1,
  2,
  // svtkm::CELL_SHAPE_QUAD
  0,
  1,
  2,
  0,
  2,
  3
};

template <typename DeviceAdapter>
class TriangulateTablesExecutionObject
{
public:
  using PortalType = typename TriangulateArrayHandle::ExecutionTypes<DeviceAdapter>::PortalConst;
  SVTKM_SUPPRESS_EXEC_WARNINGS
  SVTKM_EXEC
  TriangulateTablesExecutionObject() {}

  SVTKM_CONT
  TriangulateTablesExecutionObject(const TriangulateArrayHandle& counts,
                                   const TriangulateArrayHandle& offsets,
                                   const TriangulateArrayHandle& indices)
    : Counts(counts.PrepareForInput(DeviceAdapter()))
    , Offsets(offsets.PrepareForInput(DeviceAdapter()))
    , Indices(indices.PrepareForInput(DeviceAdapter()))
  {
  }

  template <typename CellShape>
  SVTKM_EXEC svtkm::IdComponent GetCount(CellShape shape, svtkm::IdComponent numPoints) const
  {
    if (shape.Id == svtkm::CELL_SHAPE_POLYGON)
    {
      return numPoints - 2;
    }
    else
    {
      return this->Counts.Get(shape.Id);
    }
  }

  template <typename CellShape>
  SVTKM_EXEC svtkm::IdComponent3 GetIndices(CellShape shape, svtkm::IdComponent triangleIndex) const
  {
    svtkm::IdComponent3 triIndices;
    if (shape.Id == svtkm::CELL_SHAPE_POLYGON)
    {
      triIndices[0] = 0;
      triIndices[1] = triangleIndex + 1;
      triIndices[2] = triangleIndex + 2;
    }
    else
    {
      svtkm::IdComponent offset = 3 * (this->Offsets.Get(shape.Id) + triangleIndex);
      triIndices[0] = this->Indices.Get(offset + 0);
      triIndices[1] = this->Indices.Get(offset + 1);
      triIndices[2] = this->Indices.Get(offset + 2);
    }
    return triIndices;
  }

private:
  PortalType Counts;
  PortalType Offsets;
  PortalType Indices;
};

class TriangulateTablesExecutionObjectFactory : public svtkm::cont::ExecutionObjectBase
{
public:
  template <typename Device>
  SVTKM_CONT TriangulateTablesExecutionObject<Device> PrepareForExecution(Device) const
  {
    if (BasicImpl)
    {
      return TriangulateTablesExecutionObject<Device>();
    }
    return TriangulateTablesExecutionObject<Device>(this->Counts, this->Offsets, this->Indices);
  }
  SVTKM_CONT
  TriangulateTablesExecutionObjectFactory()
    : BasicImpl(true)
  {
  }

  SVTKM_CONT
  TriangulateTablesExecutionObjectFactory(const TriangulateArrayHandle& counts,
                                          const TriangulateArrayHandle& offsets,
                                          const TriangulateArrayHandle& indices)
    : BasicImpl(false)
    , Counts(counts)
    , Offsets(offsets)
    , Indices(indices)
  {
  }

private:
  bool BasicImpl;
  TriangulateArrayHandle Counts;
  TriangulateArrayHandle Offsets;
  TriangulateArrayHandle Indices;
};

class TriangulateTables
{
public:
  SVTKM_CONT
  TriangulateTables()
    : Counts(svtkm::cont::make_ArrayHandle(svtkm::worklet::internal::TriangleCountData,
                                          svtkm::NUMBER_OF_CELL_SHAPES))
    , Offsets(svtkm::cont::make_ArrayHandle(svtkm::worklet::internal::TriangleOffsetData,
                                           svtkm::NUMBER_OF_CELL_SHAPES))
    , Indices(svtkm::cont::make_ArrayHandle(svtkm::worklet::internal::TriangleIndexData, svtkm::Id(9)))
  {
  }

  svtkm::worklet::internal::TriangulateTablesExecutionObjectFactory PrepareForInput() const
  {
    return svtkm::worklet::internal::TriangulateTablesExecutionObjectFactory(
      this->Counts, this->Offsets, this->Indices);
  }

private:
  TriangulateArrayHandle Counts;
  TriangulateArrayHandle Offsets;
  TriangulateArrayHandle Indices;
};

static svtkm::IdComponent TetrahedronCountData[svtkm::NUMBER_OF_CELL_SHAPES] = {
  0, //  0 = svtkm::CELL_SHAPE_EMPTY_CELL
  0, //  1 = svtkm::CELL_SHAPE_VERTEX
  0, //  2 = svtkm::CELL_SHAPE_POLY_VERTEX
  0, //  3 = svtkm::CELL_SHAPE_LINE
  0, //  4 = svtkm::CELL_SHAPE_POLY_LINE
  0, //  5 = svtkm::CELL_SHAPE_TRIANGLE
  0, //  6 = svtkm::CELL_SHAPE_TRIANGLE_STRIP
  0, //  7 = svtkm::CELL_SHAPE_POLYGON
  0, //  8 = svtkm::CELL_SHAPE_PIXEL
  0, //  9 = svtkm::CELL_SHAPE_QUAD
  1, // 10 = svtkm::CELL_SHAPE_TETRA
  0, // 11 = svtkm::CELL_SHAPE_VOXEL
  5, // 12 = svtkm::CELL_SHAPE_HEXAHEDRON
  3, // 13 = svtkm::CELL_SHAPE_WEDGE
  2  // 14 = svtkm::CELL_SHAPE_PYRAMID
};

static svtkm::IdComponent TetrahedronOffsetData[svtkm::NUMBER_OF_CELL_SHAPES] = {
  -1, //  0 = svtkm::CELL_SHAPE_EMPTY_CELL
  -1, //  1 = svtkm::CELL_SHAPE_VERTEX
  -1, //  2 = svtkm::CELL_SHAPE_POLY_VERTEX
  -1, //  3 = svtkm::CELL_SHAPE_LINE
  -1, //  4 = svtkm::CELL_SHAPE_POLY_LINE
  -1, //  5 = svtkm::CELL_SHAPE_TRIANGLE
  -1, //  6 = svtkm::CELL_SHAPE_TRIANGLE_STRIP
  -1, //  7 = svtkm::CELL_SHAPE_POLYGON
  -1, //  8 = svtkm::CELL_SHAPE_PIXEL
  -1, //  9 = svtkm::CELL_SHAPE_QUAD
  0,  // 10 = svtkm::CELL_SHAPE_TETRA
  -1, // 11 = svtkm::CELL_SHAPE_VOXEL
  1,  // 12 = svtkm::CELL_SHAPE_HEXAHEDRON
  6,  // 13 = svtkm::CELL_SHAPE_WEDGE
  9   // 14 = svtkm::CELL_SHAPE_PYRAMID
};

static svtkm::IdComponent TetrahedronIndexData[] = {
  // svtkm::CELL_SHAPE_TETRA
  0,
  1,
  2,
  3,
  // svtkm::CELL_SHAPE_HEXAHEDRON
  0,
  1,
  3,
  4,
  1,
  4,
  5,
  6,
  1,
  4,
  6,
  3,
  1,
  3,
  6,
  2,
  3,
  6,
  7,
  4,
  // svtkm::CELL_SHAPE_WEDGE
  0,
  1,
  2,
  4,
  3,
  4,
  5,
  2,
  0,
  2,
  3,
  4,
  // svtkm::CELL_SHAPE_PYRAMID
  0,
  1,
  2,
  4,
  0,
  2,
  3,
  4
};

template <typename DeviceAdapter>
class TetrahedralizeTablesExecutionObject
{
public:
  using PortalType = typename TriangulateArrayHandle::ExecutionTypes<DeviceAdapter>::PortalConst;
  template <typename Device>
  SVTKM_CONT TetrahedralizeTablesExecutionObject PrepareForExecution(Device) const
  {
    return *this;
  }
  SVTKM_SUPPRESS_EXEC_WARNINGS
  SVTKM_EXEC
  TetrahedralizeTablesExecutionObject() {}

  SVTKM_CONT
  TetrahedralizeTablesExecutionObject(const TriangulateArrayHandle& counts,
                                      const TriangulateArrayHandle& offsets,
                                      const TriangulateArrayHandle& indices)
    : Counts(counts.PrepareForInput(DeviceAdapter()))
    , Offsets(offsets.PrepareForInput(DeviceAdapter()))
    , Indices(indices.PrepareForInput(DeviceAdapter()))
  {
  }

  template <typename CellShape>
  SVTKM_EXEC svtkm::IdComponent GetCount(CellShape shape) const
  {
    return this->Counts.Get(shape.Id);
  }

  template <typename CellShape>
  SVTKM_EXEC svtkm::IdComponent4 GetIndices(CellShape shape, svtkm::IdComponent tetrahedronIndex) const
  {
    svtkm::IdComponent4 tetIndices;
    svtkm::IdComponent offset = 4 * (this->Offsets.Get(shape.Id) + tetrahedronIndex);
    tetIndices[0] = this->Indices.Get(offset + 0);
    tetIndices[1] = this->Indices.Get(offset + 1);
    tetIndices[2] = this->Indices.Get(offset + 2);
    tetIndices[3] = this->Indices.Get(offset + 3);
    return tetIndices;
  }

private:
  PortalType Counts;
  PortalType Offsets;
  PortalType Indices;
};

class TetrahedralizeTablesExecutionObjectFactory : public svtkm::cont::ExecutionObjectBase
{
public:
  template <typename Device>
  SVTKM_CONT TetrahedralizeTablesExecutionObject<Device> PrepareForExecution(Device) const
  {
    if (BasicImpl)
    {
      return TetrahedralizeTablesExecutionObject<Device>();
    }
    return TetrahedralizeTablesExecutionObject<Device>(this->Counts, this->Offsets, this->Indices);
  }

  SVTKM_CONT
  TetrahedralizeTablesExecutionObjectFactory()
    : BasicImpl(true)
  {
  }

  SVTKM_CONT
  TetrahedralizeTablesExecutionObjectFactory(const TriangulateArrayHandle& counts,
                                             const TriangulateArrayHandle& offsets,
                                             const TriangulateArrayHandle& indices)
    : BasicImpl(false)
    , Counts(counts)
    , Offsets(offsets)
    , Indices(indices)
  {
  }

private:
  bool BasicImpl;
  TriangulateArrayHandle Counts;
  TriangulateArrayHandle Offsets;
  TriangulateArrayHandle Indices;
};

class TetrahedralizeTables
{
public:
  SVTKM_CONT
  TetrahedralizeTables()
    : Counts(svtkm::cont::make_ArrayHandle(svtkm::worklet::internal::TetrahedronCountData,
                                          svtkm::NUMBER_OF_CELL_SHAPES))
    , Offsets(svtkm::cont::make_ArrayHandle(svtkm::worklet::internal::TetrahedronOffsetData,
                                           svtkm::NUMBER_OF_CELL_SHAPES))
    , Indices(
        svtkm::cont::make_ArrayHandle(svtkm::worklet::internal::TetrahedronIndexData, svtkm::Id(44)))
  {
  }

  svtkm::worklet::internal::TetrahedralizeTablesExecutionObjectFactory PrepareForInput() const
  {
    return svtkm::worklet::internal::TetrahedralizeTablesExecutionObjectFactory(
      this->Counts, this->Offsets, this->Indices);
  }

private:
  TriangulateArrayHandle Counts;
  TriangulateArrayHandle Offsets;
  TriangulateArrayHandle Indices;
};
}
}
}

#endif //svtk_m_worklet_internal_TriangulateTables_h
