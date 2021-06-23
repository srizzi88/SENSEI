//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef svtk_m_worklet_CellMeasure_h
#define svtk_m_worklet_CellMeasure_h

#include <svtkm/worklet/WorkletMapTopology.h>

#include <svtkm/exec/CellMeasure.h>

namespace svtkm
{

// Tags used to choose which types of integration are performed on cells:
struct IntegrateOver
{
};
struct IntegrateOverCurve : IntegrateOver
{
};
struct IntegrateOverSurface : IntegrateOver
{
};
struct IntegrateOverSolid : IntegrateOver
{
};

// Lists of acceptable types of integration
using ArcLength = svtkm::List<IntegrateOverCurve>;
using Area = svtkm::List<IntegrateOverSurface>;
using Volume = svtkm::List<IntegrateOverSolid>;
using AllMeasures = svtkm::List<IntegrateOverSolid, IntegrateOverSurface, IntegrateOverCurve>;

namespace worklet
{

/**\brief Simple functor that returns the spatial integral of each cell as a cell field.
  *
  * The integration is done over the spatial extent of the cell and thus units
  * are either null, arc length, area, or volume depending on whether the parametric
  * dimension of the cell is 0 (vertices), 1 (curves), 2 (surfaces), or 3 (volumes).
  * The template parameter of this class configures which types of cells (based on their
  * parametric dimensions) should be integrated. Other cells will report 0.
  *
  * Note that the integrals are signed; inverted cells will report negative values.
  */
template <typename IntegrationTypeList>
class CellMeasure : public svtkm::worklet::WorkletVisitCellsWithPoints
{
public:
  using ControlSignature = void(CellSetIn cellset,
                                FieldInPoint pointCoords,
                                FieldOutCell volumesOut);
  using ExecutionSignature = void(CellShape, PointCount, _2, _3);
  using InputDomain = _1;

  template <typename CellShape, typename PointCoordVecType, typename OutType>
  SVTKM_EXEC void operator()(CellShape shape,
                            const svtkm::IdComponent& numPoints,
                            const PointCoordVecType& pts,
                            OutType& volume) const
  {
    switch (shape.Id)
    {
      svtkmGenericCellShapeMacro(volume =
                                  this->ComputeMeasure<OutType>(numPoints, pts, CellShapeTag()));
      default:
        this->RaiseError("Asked for volume of unknown cell shape.");
        volume = OutType(0.0);
    }
  }

protected:
  template <typename OutType, typename PointCoordVecType, typename CellShapeType>
  SVTKM_EXEC OutType ComputeMeasure(const svtkm::IdComponent& numPts,
                                   const PointCoordVecType& pts,
                                   CellShapeType) const
  {
#if defined(SVTKM_MSVC)
#pragma warning(push)
#pragma warning(disable : 4068) //unknown pragma
#endif
#ifdef __NVCC__
#pragma push
#pragma diag_suppress = code_is_unreachable
#endif
    switch (svtkm::CellTraits<CellShapeType>::TOPOLOGICAL_DIMENSIONS)
    {
      case 0:
        // Fall through to return 0 measure.
        break;
      case 1:
        if (svtkm::ListHas<IntegrationTypeList, IntegrateOverCurve>::value)
        {
          return svtkm::exec::CellMeasure<OutType>(numPts, pts, CellShapeType(), *this);
        }
        break;
      case 2:
        if (svtkm::ListHas<IntegrationTypeList, IntegrateOverSurface>::value)
        {
          return svtkm::exec::CellMeasure<OutType>(numPts, pts, CellShapeType(), *this);
        }
        break;
      case 3:
        if (svtkm::ListHas<IntegrationTypeList, IntegrateOverSolid>::value)
        {
          return svtkm::exec::CellMeasure<OutType>(numPts, pts, CellShapeType(), *this);
        }
        break;
      default:
        // Fall through to return 0 measure.
        break;
    }
    return OutType(0.0);
#ifdef __NVCC__
#pragma pop
#endif
#if defined(SVTKM_MSVC)
#pragma warning(pop)
#endif
  }
};
}
} // namespace svtkm::worklet

#endif // svtk_m_worklet_CellMeasure_h
