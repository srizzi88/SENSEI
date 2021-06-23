//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_worklet_Probe_h
#define svtk_m_worklet_Probe_h

#include <svtkm/cont/ArrayCopy.h>
#include <svtkm/cont/ArrayHandle.h>
#include <svtkm/cont/CellLocatorGeneral.h>
#include <svtkm/exec/CellInside.h>
#include <svtkm/exec/CellInterpolate.h>
#include <svtkm/exec/ParametricCoordinates.h>

#include <svtkm/worklet/DispatcherMapField.h>
#include <svtkm/worklet/DispatcherMapTopology.h>
#include <svtkm/worklet/WorkletMapField.h>
#include <svtkm/worklet/WorkletMapTopology.h>

#include <svtkm/VecFromPortalPermute.h>

namespace svtkm
{
namespace worklet
{

class Probe
{
  //============================================================================
public:
  class FindCellWorklet : public svtkm::worklet::WorkletMapField
  {
  public:
    using ControlSignature = void(FieldIn points,
                                  ExecObject locator,
                                  FieldOut cellIds,
                                  FieldOut pcoords);
    using ExecutionSignature = void(_1, _2, _3, _4);

    template <typename LocatorType>
    SVTKM_EXEC void operator()(const svtkm::Vec3f& point,
                              const LocatorType& locator,
                              svtkm::Id& cellId,
                              svtkm::Vec3f& pcoords) const
    {
      locator->FindCell(point, cellId, pcoords, *this);
    }
  };

private:
  template <typename CellSetType, typename PointsType, typename PointsStorage>
  void RunImpl(const CellSetType& cells,
               const svtkm::cont::CoordinateSystem& coords,
               const svtkm::cont::ArrayHandle<PointsType, PointsStorage>& points)
  {
    this->InputCellSet = svtkm::cont::DynamicCellSet(cells);

    svtkm::cont::CellLocatorGeneral locator;
    locator.SetCellSet(this->InputCellSet);
    locator.SetCoordinates(coords);
    locator.Update();

    svtkm::worklet::DispatcherMapField<FindCellWorklet> dispatcher;
    // CellLocatorGeneral is non-copyable. Pass it via a pointer.
    dispatcher.Invoke(points, &locator, this->CellIds, this->ParametricCoordinates);
  }

  //============================================================================
public:
  class ProbeUniformPoints : public svtkm::worklet::WorkletVisitCellsWithPoints
  {
  public:
    using ControlSignature = void(CellSetIn cellset,
                                  FieldInPoint coords,
                                  WholeArrayIn points,
                                  WholeArrayOut cellIds,
                                  WholeArrayOut parametricCoords);
    using ExecutionSignature = void(InputIndex, CellShape, _2, _3, _4, _5);
    using InputDomain = _1;

    template <typename CellShapeTag,
              typename CoordsVecType,
              typename UniformPoints,
              typename CellIdsType,
              typename ParametricCoordsType>
    SVTKM_EXEC void operator()(svtkm::Id cellId,
                              CellShapeTag cellShape,
                              const CoordsVecType& cellPoints,
                              const UniformPoints& points,
                              CellIdsType& cellIds,
                              ParametricCoordsType& pcoords) const
    {
      // Compute cell bounds
      using CoordsType = typename svtkm::VecTraits<CoordsVecType>::ComponentType;
      auto numPoints = svtkm::VecTraits<CoordsVecType>::GetNumberOfComponents(cellPoints);

      CoordsType cbmin = cellPoints[0], cbmax = cellPoints[0];
      for (svtkm::IdComponent i = 1; i < numPoints; ++i)
      {
        cbmin = svtkm::Min(cbmin, cellPoints[i]);
        cbmax = svtkm::Max(cbmax, cellPoints[i]);
      }

      // Compute points inside cell bounds
      auto portal = points.GetPortal();
      auto minp =
        static_cast<svtkm::Id3>(svtkm::Ceil((cbmin - portal.GetOrigin()) / portal.GetSpacing()));
      auto maxp =
        static_cast<svtkm::Id3>(svtkm::Floor((cbmax - portal.GetOrigin()) / portal.GetSpacing()));

      // clamp
      minp = svtkm::Max(minp, svtkm::Id3(0));
      maxp = svtkm::Min(maxp, portal.GetDimensions() - svtkm::Id3(1));

      for (svtkm::Id k = minp[2]; k <= maxp[2]; ++k)
      {
        for (svtkm::Id j = minp[1]; j <= maxp[1]; ++j)
        {
          for (svtkm::Id i = minp[0]; i <= maxp[0]; ++i)
          {
            auto pt = portal.Get(svtkm::Id3(i, j, k));
            bool success = false;
            auto pc = svtkm::exec::WorldCoordinatesToParametricCoordinates(
              cellPoints, pt, cellShape, success, *this);
            if (success && svtkm::exec::CellInside(pc, cellShape))
            {
              auto pointId = i + portal.GetDimensions()[0] * (j + portal.GetDimensions()[1] * k);
              cellIds.Set(pointId, cellId);
              pcoords.Set(pointId, pc);
            }
          }
        }
      }
    }
  };

private:
  template <typename CellSetType>
  void RunImpl(const CellSetType& cells,
               const svtkm::cont::CoordinateSystem& coords,
               const svtkm::cont::ArrayHandleUniformPointCoordinates::Superclass& points)
  {
    this->InputCellSet = svtkm::cont::DynamicCellSet(cells);
    svtkm::cont::ArrayCopy(
      svtkm::cont::make_ArrayHandleConstant(svtkm::Id(-1), points.GetNumberOfValues()),
      this->CellIds);
    this->ParametricCoordinates.Allocate(points.GetNumberOfValues());

    svtkm::worklet::DispatcherMapTopology<ProbeUniformPoints> dispatcher;
    dispatcher.Invoke(cells, coords, points, this->CellIds, this->ParametricCoordinates);
  }

  //============================================================================
  struct RunImplCaller
  {
    template <typename PointsArrayType, typename CellSetType>
    void operator()(const PointsArrayType& points,
                    Probe& worklet,
                    const CellSetType& cells,
                    const svtkm::cont::CoordinateSystem& coords) const
    {
      worklet.RunImpl(cells, coords, points);
    }
  };

public:
  template <typename CellSetType, typename PointsArrayType>
  void Run(const CellSetType& cells,
           const svtkm::cont::CoordinateSystem& coords,
           const PointsArrayType& points)
  {
    svtkm::cont::CastAndCall(points, RunImplCaller(), *this, cells, coords);
  }

  //============================================================================
  class InterpolatePointField : public svtkm::worklet::WorkletMapField
  {
  public:
    using ControlSignature = void(FieldIn cellIds,
                                  FieldIn parametricCoords,
                                  WholeCellSetIn<> inputCells,
                                  WholeArrayIn inputField,
                                  FieldOut result);
    using ExecutionSignature = void(_1, _2, _3, _4, _5);

    template <typename ParametricCoordType, typename CellSetType, typename InputFieldPortalType>
    SVTKM_EXEC void operator()(svtkm::Id cellId,
                              const ParametricCoordType& pc,
                              const CellSetType& cells,
                              const InputFieldPortalType& in,
                              typename InputFieldPortalType::ValueType& out) const
    {
      if (cellId != -1)
      {
        auto indices = cells.GetIndices(cellId);
        auto pointVals = svtkm::make_VecFromPortalPermute(&indices, in);
        out = svtkm::exec::CellInterpolate(pointVals, pc, cells.GetCellShape(cellId), *this);
      }
    }
  };

  /// Intepolate the input point field data at the points of the geometry
  template <typename T,
            typename Storage,
            typename InputCellSetTypeList = SVTKM_DEFAULT_CELL_SET_LIST>
  svtkm::cont::ArrayHandle<T> ProcessPointField(
    const svtkm::cont::ArrayHandle<T, Storage>& field,
    InputCellSetTypeList icsTypes = InputCellSetTypeList()) const
  {
    svtkm::cont::ArrayHandle<T> result;
    svtkm::worklet::DispatcherMapField<InterpolatePointField> dispatcher;
    dispatcher.Invoke(this->CellIds,
                      this->ParametricCoordinates,
                      this->InputCellSet.ResetCellSetList(icsTypes),
                      field,
                      result);

    return result;
  }

  //============================================================================
  class MapCellField : public svtkm::worklet::WorkletMapField
  {
  public:
    using ControlSignature = void(FieldIn cellIds, WholeArrayIn inputField, FieldOut result);
    using ExecutionSignature = void(_1, _2, _3);

    template <typename InputFieldPortalType>
    SVTKM_EXEC void operator()(svtkm::Id cellId,
                              const InputFieldPortalType& in,
                              typename InputFieldPortalType::ValueType& out) const
    {
      if (cellId != -1)
      {
        out = in.Get(cellId);
      }
    }
  };

  /// Map the input cell field data to the points of the geometry. Each point gets the value
  /// associated with its containing cell. For points that fall on cell edges, the containing
  /// cell is chosen arbitrarily.
  ///
  template <typename T, typename Storage>
  svtkm::cont::ArrayHandle<T> ProcessCellField(
    const svtkm::cont::ArrayHandle<T, Storage>& field) const
  {
    svtkm::cont::ArrayHandle<T> result;
    svtkm::worklet::DispatcherMapField<MapCellField> dispatcher;
    dispatcher.Invoke(this->CellIds, field, result);

    return result;
  }

  //============================================================================
  struct HiddenPointsWorklet : public WorkletMapField
  {
    using ControlSignature = void(FieldIn cellids, FieldOut hfield);
    using ExecutionSignature = _2(_1);

    SVTKM_EXEC svtkm::UInt8 operator()(svtkm::Id cellId) const { return (cellId == -1) ? HIDDEN : 0; }
  };

  /// Get an array of flags marking the invalid points (points that do not fall inside any of
  /// the cells of the input). The flag value is the same as the HIDDEN flag in SVTK and VISIT.
  ///
  svtkm::cont::ArrayHandle<svtkm::UInt8> GetHiddenPointsField() const
  {
    svtkm::cont::ArrayHandle<svtkm::UInt8> field;
    svtkm::worklet::DispatcherMapField<HiddenPointsWorklet> dispatcher;
    dispatcher.Invoke(this->CellIds, field);
    return field;
  }

  //============================================================================
  struct HiddenCellsWorklet : public WorkletVisitCellsWithPoints
  {
    using ControlSignature = void(CellSetIn cellset, FieldInPoint cellids, FieldOutCell);
    using ExecutionSignature = _3(_2, PointCount);

    template <typename CellIdsVecType>
    SVTKM_EXEC svtkm::UInt8 operator()(const CellIdsVecType& cellIds,
                                     svtkm::IdComponent numPoints) const
    {
      for (svtkm::IdComponent i = 0; i < numPoints; ++i)
      {
        if (cellIds[i] == -1)
        {
          return HIDDEN;
        }
      }
      return 0;
    }
  };

  /// Get an array of flags marking the invalid cells. Invalid cells are the cells with at least
  /// one invalid point. The flag value is the same as the HIDDEN flag in SVTK and VISIT.
  ///
  template <typename CellSetType>
  svtkm::cont::ArrayHandle<svtkm::UInt8> GetHiddenCellsField(CellSetType cellset) const
  {
    svtkm::cont::ArrayHandle<svtkm::UInt8> field;
    svtkm::worklet::DispatcherMapTopology<HiddenCellsWorklet> dispatcher;
    dispatcher.Invoke(cellset, this->CellIds, field);
    return field;
  }

  //============================================================================
private:
  static constexpr svtkm::UInt8 HIDDEN = 2; // from svtk

  svtkm::cont::ArrayHandle<svtkm::Id> CellIds;
  svtkm::cont::ArrayHandle<svtkm::Vec3f> ParametricCoordinates;
  svtkm::cont::DynamicCellSet InputCellSet;
};
}
} // svtkm::worklet

#endif // svtk_m_worklet_Probe_h
