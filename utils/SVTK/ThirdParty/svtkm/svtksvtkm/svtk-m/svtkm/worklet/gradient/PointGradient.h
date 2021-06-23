//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef svtk_m_worklet_gradient_PointGradient_h
#define svtk_m_worklet_gradient_PointGradient_h

#include <svtkm/exec/CellDerivative.h>
#include <svtkm/exec/ParametricCoordinates.h>
#include <svtkm/worklet/WorkletMapTopology.h>

#include <svtkm/worklet/gradient/GradientOutput.h>


namespace svtkm
{
namespace worklet
{
namespace gradient
{

template <typename T>
using PointGradientInType = svtkm::List<T>;

template <typename T>
struct PointGradient : public svtkm::worklet::WorkletVisitPointsWithCells
{
  using ControlSignature = void(CellSetIn,
                                WholeCellSetIn<Cell, Point>,
                                WholeArrayIn pointCoordinates,
                                WholeArrayIn inputField,
                                GradientOutputs outputFields);

  using ExecutionSignature = void(CellCount, CellIndices, WorkIndex, _2, _3, _4, _5);
  using InputDomain = _1;

  template <typename FromIndexType,
            typename CellSetInType,
            typename WholeCoordinatesIn,
            typename WholeFieldIn,
            typename GradientOutType>
  SVTKM_EXEC void operator()(const svtkm::IdComponent& numCells,
                            const FromIndexType& cellIds,
                            const svtkm::Id& pointId,
                            const CellSetInType& geometry,
                            const WholeCoordinatesIn& pointCoordinates,
                            const WholeFieldIn& inputField,
                            GradientOutType& outputGradient) const
  {
    using CellThreadIndices = svtkm::exec::arg::ThreadIndicesTopologyMap<CellSetInType>;
    using ValueType = typename WholeFieldIn::ValueType;
    using CellShapeTag = typename CellSetInType::CellShapeTag;

    svtkm::Vec<ValueType, 3> gradient(ValueType(0.0));
    for (svtkm::IdComponent i = 0; i < numCells; ++i)
    {
      const svtkm::Id cellId = cellIds[i];
      CellThreadIndices cellIndices(cellId, cellId, 0, cellId, geometry);

      const CellShapeTag cellShape = cellIndices.GetCellShape();

      // compute the parametric coordinates for the current point
      const auto wCoords = this->GetValues(cellIndices, pointCoordinates);
      const auto field = this->GetValues(cellIndices, inputField);

      const svtkm::IdComponent pointIndexForCell = this->GetPointIndexForCell(cellIndices, pointId);

      this->ComputeGradient(cellShape, pointIndexForCell, wCoords, field, gradient);
    }

    if (numCells != 0)
    {
      using BaseGradientType = typename svtkm::VecTraits<ValueType>::BaseComponentType;
      const BaseGradientType invNumCells =
        static_cast<BaseGradientType>(1.) / static_cast<BaseGradientType>(numCells);

      gradient[0] = gradient[0] * invNumCells;
      gradient[1] = gradient[1] * invNumCells;
      gradient[2] = gradient[2] * invNumCells;
    }
    outputGradient = gradient;
  }

private:
  template <typename CellShapeTag,
            typename PointCoordVecType,
            typename FieldInVecType,
            typename OutValueType>
  inline SVTKM_EXEC void ComputeGradient(CellShapeTag cellShape,
                                        const svtkm::IdComponent& pointIndexForCell,
                                        const PointCoordVecType& wCoords,
                                        const FieldInVecType& field,
                                        svtkm::Vec<OutValueType, 3>& gradient) const
  {
    svtkm::Vec3f pCoords;
    svtkm::exec::ParametricCoordinatesPoint(
      wCoords.GetNumberOfComponents(), pointIndexForCell, pCoords, cellShape, *this);

    //we need to add this to a return value
    gradient += svtkm::exec::CellDerivative(field, wCoords, pCoords, cellShape, *this);
  }

  template <typename CellSetInType>
  SVTKM_EXEC svtkm::IdComponent GetPointIndexForCell(
    const svtkm::exec::arg::ThreadIndicesTopologyMap<CellSetInType>& indices,
    svtkm::Id pointId) const
  {
    svtkm::IdComponent result = 0;
    const auto& topo = indices.GetIndicesIncident();
    for (svtkm::IdComponent i = 0; i < topo.GetNumberOfComponents(); ++i)
    {
      if (topo[i] == pointId)
      {
        result = i;
      }
    }
    return result;
  }

  //This is fairly complex so that we can trigger code to extract
  //VecRectilinearPointCoordinates when using structured connectivity, and
  //uniform point coordinates.
  //c++14 would make the return type simply auto
  template <typename CellSetInType, typename WholeFieldIn>
  SVTKM_EXEC
    typename svtkm::exec::arg::Fetch<svtkm::exec::arg::FetchTagArrayTopologyMapIn,
                                    svtkm::exec::arg::AspectTagDefault,
                                    svtkm::exec::arg::ThreadIndicesTopologyMap<CellSetInType>,
                                    typename WholeFieldIn::PortalType>::ValueType
    GetValues(const svtkm::exec::arg::ThreadIndicesTopologyMap<CellSetInType>& indices,
              const WholeFieldIn& in) const
  {
    //the current problem is that when the topology is structured
    //we are passing in an svtkm::Id when it wants a Id2 or an Id3 that
    //represents the flat index of the topology
    using ExecObjectType = typename WholeFieldIn::PortalType;
    using Fetch = svtkm::exec::arg::Fetch<svtkm::exec::arg::FetchTagArrayTopologyMapIn,
                                         svtkm::exec::arg::AspectTagDefault,
                                         svtkm::exec::arg::ThreadIndicesTopologyMap<CellSetInType>,
                                         ExecObjectType>;
    Fetch fetch;
    return fetch.Load(indices, in.GetPortal());
  }
};
}
}
}

#endif
