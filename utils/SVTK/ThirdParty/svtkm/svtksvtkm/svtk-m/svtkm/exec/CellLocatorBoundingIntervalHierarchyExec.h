//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_cont_CellLocatorBoundingIntervalHierarchyExec_h
#define svtk_m_cont_CellLocatorBoundingIntervalHierarchyExec_h

#include <svtkm/TopologyElementTag.h>
#include <svtkm/VecFromPortalPermute.h>
#include <svtkm/cont/ArrayHandle.h>
#include <svtkm/cont/ArrayHandleVirtualCoordinates.h>
#include <svtkm/exec/CellInside.h>
#include <svtkm/exec/CellLocator.h>
#include <svtkm/exec/ParametricCoordinates.h>

namespace svtkm
{
namespace exec
{




struct CellLocatorBoundingIntervalHierarchyNode
{
#if defined(SVTKM_CLANG)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wnested-anon-types"
#endif // gcc || clang
  svtkm::IdComponent Dimension;
  svtkm::Id ParentIndex;
  svtkm::Id ChildIndex;
  union {
    struct
    {
      svtkm::FloatDefault LMax;
      svtkm::FloatDefault RMin;
    } Node;
    struct
    {
      svtkm::Id Start;
      svtkm::Id Size;
    } Leaf;
  };
#if defined(SVTKM_CLANG)
#pragma GCC diagnostic pop
#endif // gcc || clang

  SVTKM_EXEC_CONT
  CellLocatorBoundingIntervalHierarchyNode()
    : Dimension()
    , ParentIndex()
    , ChildIndex()
    , Node{ 0, 0 }
  {
  }
}; // struct CellLocatorBoundingIntervalHierarchyNode

template <typename DeviceAdapter, typename CellSetType>
class SVTKM_ALWAYS_EXPORT CellLocatorBoundingIntervalHierarchyExec final
  : public svtkm::exec::CellLocator
{
  using NodeArrayHandle =
    svtkm::cont::ArrayHandle<svtkm::exec::CellLocatorBoundingIntervalHierarchyNode>;
  using CellIdArrayHandle = svtkm::cont::ArrayHandle<svtkm::Id>;

public:
  SVTKM_CONT
  CellLocatorBoundingIntervalHierarchyExec() {}

  SVTKM_CONT
  CellLocatorBoundingIntervalHierarchyExec(const NodeArrayHandle& nodes,
                                           const CellIdArrayHandle& cellIds,
                                           const CellSetType& cellSet,
                                           const svtkm::cont::ArrayHandleVirtualCoordinates& coords,
                                           DeviceAdapter)
    : Nodes(nodes.PrepareForInput(DeviceAdapter()))
    , CellIds(cellIds.PrepareForInput(DeviceAdapter()))
    , CellSet(cellSet.PrepareForInput(DeviceAdapter(), VisitType(), IncidentType()))
    , Coords(coords.PrepareForInput(DeviceAdapter()))
  {
  }


  SVTKM_EXEC_CONT virtual ~CellLocatorBoundingIntervalHierarchyExec() noexcept override
  {
    // This must not be defaulted, since defaulted virtual destructors are
    // troublesome with CUDA __host__ __device__ markup.
  }

  SVTKM_EXEC
  void FindCell(const svtkm::Vec3f& point,
                svtkm::Id& cellId,
                svtkm::Vec3f& parametric,
                const svtkm::exec::FunctorBase& worklet) const override
  {
    cellId = -1;
    svtkm::Id nodeIndex = 0;
    FindCellState state = FindCellState::EnterNode;

    while ((cellId < 0) && !((nodeIndex == 0) && (state == FindCellState::AscendFromNode)))
    {
      switch (state)
      {
        case FindCellState::EnterNode:
          this->EnterNode(state, point, cellId, nodeIndex, parametric, worklet);
          break;
        case FindCellState::AscendFromNode:
          this->AscendFromNode(state, nodeIndex);
          break;
        case FindCellState::DescendLeftChild:
          this->DescendLeftChild(state, point, nodeIndex);
          break;
        case FindCellState::DescendRightChild:
          this->DescendRightChild(state, point, nodeIndex);
          break;
      }
    }
  }

private:
  enum struct FindCellState
  {
    EnterNode,
    AscendFromNode,
    DescendLeftChild,
    DescendRightChild
  };

  SVTKM_EXEC
  void EnterNode(FindCellState& state,
                 const svtkm::Vec3f& point,
                 svtkm::Id& cellId,
                 svtkm::Id nodeIndex,
                 svtkm::Vec3f& parametric,
                 const svtkm::exec::FunctorBase& worklet) const
  {
    SVTKM_ASSERT(state == FindCellState::EnterNode);

    const svtkm::exec::CellLocatorBoundingIntervalHierarchyNode& node = this->Nodes.Get(nodeIndex);

    if (node.ChildIndex < 0)
    {
      // In a leaf node. Look for a containing cell.
      cellId = this->FindInLeaf(point, parametric, node, worklet);
      state = FindCellState::AscendFromNode;
    }
    else
    {
      state = FindCellState::DescendLeftChild;
    }
  }

  SVTKM_EXEC
  void AscendFromNode(FindCellState& state, svtkm::Id& nodeIndex) const
  {
    SVTKM_ASSERT(state == FindCellState::AscendFromNode);

    svtkm::Id childNodeIndex = nodeIndex;
    const svtkm::exec::CellLocatorBoundingIntervalHierarchyNode& childNode =
      this->Nodes.Get(childNodeIndex);
    nodeIndex = childNode.ParentIndex;
    const svtkm::exec::CellLocatorBoundingIntervalHierarchyNode& parentNode =
      this->Nodes.Get(nodeIndex);

    if (parentNode.ChildIndex == childNodeIndex)
    {
      // Ascending from left child. Descend into the right child.
      state = FindCellState::DescendRightChild;
    }
    else
    {
      SVTKM_ASSERT(parentNode.ChildIndex + 1 == childNodeIndex);
      // Ascending from right child. Ascend again. (Don't need to change state.)
    }
  }

  SVTKM_EXEC
  void DescendLeftChild(FindCellState& state, const svtkm::Vec3f& point, svtkm::Id& nodeIndex) const
  {
    SVTKM_ASSERT(state == FindCellState::DescendLeftChild);

    const svtkm::exec::CellLocatorBoundingIntervalHierarchyNode& node = this->Nodes.Get(nodeIndex);
    const svtkm::FloatDefault& coordinate = point[node.Dimension];
    if (coordinate <= node.Node.LMax)
    {
      // Left child does contain the point. Do the actual descent.
      nodeIndex = node.ChildIndex;
      state = FindCellState::EnterNode;
    }
    else
    {
      // Left child does not contain the point. Skip to the right child.
      state = FindCellState::DescendRightChild;
    }
  }

  SVTKM_EXEC
  void DescendRightChild(FindCellState& state, const svtkm::Vec3f& point, svtkm::Id& nodeIndex) const
  {
    SVTKM_ASSERT(state == FindCellState::DescendRightChild);

    const svtkm::exec::CellLocatorBoundingIntervalHierarchyNode& node = this->Nodes.Get(nodeIndex);
    const svtkm::FloatDefault& coordinate = point[node.Dimension];
    if (coordinate >= node.Node.RMin)
    {
      // Right child does contain the point. Do the actual descent.
      nodeIndex = node.ChildIndex + 1;
      state = FindCellState::EnterNode;
    }
    else
    {
      // Right child does not contain the point. Skip to ascent
      state = FindCellState::AscendFromNode;
    }
  }

  SVTKM_EXEC svtkm::Id FindInLeaf(const svtkm::Vec3f& point,
                                svtkm::Vec3f& parametric,
                                const svtkm::exec::CellLocatorBoundingIntervalHierarchyNode& node,
                                const svtkm::exec::FunctorBase& worklet) const
  {
    using IndicesType = typename CellSetPortal::IndicesType;
    for (svtkm::Id i = node.Leaf.Start; i < node.Leaf.Start + node.Leaf.Size; ++i)
    {
      svtkm::Id cellId = this->CellIds.Get(i);
      IndicesType cellPointIndices = this->CellSet.GetIndices(cellId);
      svtkm::VecFromPortalPermute<IndicesType, CoordsPortal> cellPoints(&cellPointIndices,
                                                                       this->Coords);
      if (IsPointInCell(point, parametric, this->CellSet.GetCellShape(cellId), cellPoints, worklet))
      {
        return cellId;
      }
    }
    return -1;
  }

  template <typename CoordsType, typename CellShapeTag>
  SVTKM_EXEC static bool IsPointInCell(const svtkm::Vec3f& point,
                                      svtkm::Vec3f& parametric,
                                      CellShapeTag cellShape,
                                      const CoordsType& cellPoints,
                                      const svtkm::exec::FunctorBase& worklet)
  {
    bool success = false;
    parametric = svtkm::exec::WorldCoordinatesToParametricCoordinates(
      cellPoints, point, cellShape, success, worklet);
    return success && svtkm::exec::CellInside(parametric, cellShape);
  }

  using VisitType = svtkm::TopologyElementTagCell;
  using IncidentType = svtkm::TopologyElementTagPoint;
  using NodePortal = typename NodeArrayHandle::template ExecutionTypes<DeviceAdapter>::PortalConst;
  using CellIdPortal =
    typename CellIdArrayHandle::template ExecutionTypes<DeviceAdapter>::PortalConst;
  using CellSetPortal = typename CellSetType::template ExecutionTypes<DeviceAdapter,
                                                                      VisitType,
                                                                      IncidentType>::ExecObjectType;
  using CoordsPortal = typename svtkm::cont::ArrayHandleVirtualCoordinates::template ExecutionTypes<
    DeviceAdapter>::PortalConst;

  NodePortal Nodes;
  CellIdPortal CellIds;
  CellSetPortal CellSet;
  CoordsPortal Coords;
}; // class CellLocatorBoundingIntervalHierarchyExec

} // namespace exec

} // namespace svtkm

#endif //svtk_m_cont_CellLocatorBoundingIntervalHierarchyExec_h
