//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef svtk_m_worklet_gradient_CellGradient_h
#define svtk_m_worklet_gradient_CellGradient_h

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
using CellGradientInType = svtkm::List<T>;

template <typename T>
struct CellGradient : svtkm::worklet::WorkletVisitCellsWithPoints
{
  using ControlSignature = void(CellSetIn,
                                FieldInPoint pointCoordinates,
                                FieldInPoint inputField,
                                GradientOutputs outputFields);

  using ExecutionSignature = void(CellShape, PointCount, _2, _3, _4);
  using InputDomain = _1;

  template <typename CellTagType,
            typename PointCoordVecType,
            typename FieldInVecType,
            typename GradientOutType>
  SVTKM_EXEC void operator()(CellTagType shape,
                            svtkm::IdComponent pointCount,
                            const PointCoordVecType& wCoords,
                            const FieldInVecType& field,
                            GradientOutType& outputGradient) const
  {
    svtkm::Vec3f center = svtkm::exec::ParametricCoordinatesCenter(pointCount, shape, *this);

    outputGradient = svtkm::exec::CellDerivative(field, wCoords, center, shape, *this);
  }
};
}
}
}

#endif
