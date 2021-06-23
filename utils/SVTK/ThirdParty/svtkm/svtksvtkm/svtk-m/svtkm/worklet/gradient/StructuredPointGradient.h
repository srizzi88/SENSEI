//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef svtk_m_worklet_gradient_StructuredPointGradient_h
#define svtk_m_worklet_gradient_StructuredPointGradient_h

#include <svtkm/worklet/WorkletPointNeighborhood.h>
#include <svtkm/worklet/gradient/GradientOutput.h>


namespace svtkm
{
namespace worklet
{
namespace gradient
{

template <typename T>
using StructuredPointGradientInType = svtkm::List<T>;

template <typename T>
struct StructuredPointGradient : public svtkm::worklet::WorkletPointNeighborhood
{

  using ControlSignature = void(CellSetIn,
                                FieldInNeighborhood points,
                                FieldInNeighborhood,
                                GradientOutputs outputFields);

  using ExecutionSignature = void(Boundary, _2, _3, _4);

  using InputDomain = _1;

  template <typename PointsIn, typename FieldIn, typename GradientOutType>
  SVTKM_EXEC void operator()(const svtkm::exec::BoundaryState& boundary,
                            const PointsIn& inputPoints,
                            const FieldIn& inputField,
                            GradientOutType& outputGradient) const
  {
    using CoordType = typename PointsIn::ValueType;
    using CT = typename svtkm::VecTraits<CoordType>::BaseComponentType;
    using OT = typename GradientOutType::ComponentType;

    svtkm::Vec<CT, 3> xi, eta, zeta;
    this->Jacobian(inputPoints, boundary, xi, eta, zeta); //store the metrics in xi,eta,zeta

    T dxi = inputField.Get(1, 0, 0) - inputField.Get(-1, 0, 0);
    T deta = inputField.Get(0, 1, 0) - inputField.Get(0, -1, 0);
    T dzeta = inputField.Get(0, 0, 1) - inputField.Get(0, 0, -1);

    dxi = (boundary.IsRadiusInXBoundary(1) ? dxi * 0.5f : dxi);
    deta = (boundary.IsRadiusInYBoundary(1) ? deta * 0.5f : deta);
    dzeta = (boundary.IsRadiusInZBoundary(1) ? dzeta * 0.5f : dzeta);

    outputGradient[0] = static_cast<OT>(xi[0] * dxi + eta[0] * deta + zeta[0] * dzeta);
    outputGradient[1] = static_cast<OT>(xi[1] * dxi + eta[1] * deta + zeta[1] * dzeta);
    outputGradient[2] = static_cast<OT>(xi[2] * dxi + eta[2] * deta + zeta[2] * dzeta);
  }

  template <typename FieldIn, typename GradientOutType>
  SVTKM_EXEC void operator()(const svtkm::exec::BoundaryState& boundary,
                            const svtkm::exec::FieldNeighborhood<
                              svtkm::internal::ArrayPortalUniformPointCoordinates>& inputPoints,
                            const FieldIn& inputField,
                            GradientOutType& outputGradient) const
  {
    //When the points and cells are both structured we can achieve even better
    //performance by not doing the Jacobian, but instead do an image gradient
    //using central differences
    using PointsIn =
      svtkm::exec::FieldNeighborhood<svtkm::internal::ArrayPortalUniformPointCoordinates>;
    using CoordType = typename PointsIn::ValueType;
    using OT = typename GradientOutType::ComponentType;


    CoordType r = inputPoints.Portal.GetSpacing();

    r[0] = (boundary.IsRadiusInXBoundary(1) ? r[0] * 0.5f : r[0]);
    r[1] = (boundary.IsRadiusInYBoundary(1) ? r[1] * 0.5f : r[1]);
    r[2] = (boundary.IsRadiusInZBoundary(1) ? r[2] * 0.5f : r[2]);

    const T dx = inputField.Get(1, 0, 0) - inputField.Get(-1, 0, 0);
    const T dy = inputField.Get(0, 1, 0) - inputField.Get(0, -1, 0);
    const T dz = inputField.Get(0, 0, 1) - inputField.Get(0, 0, -1);

    outputGradient[0] = static_cast<OT>(dx * r[0]);
    outputGradient[1] = static_cast<OT>(dy * r[1]);
    outputGradient[2] = static_cast<OT>(dz * r[2]);
  }

  //we need to pass the coordinates into this function, and instead
  //of the input being Vec<coordtype,3> it needs to be Vec<float,3> as the metrics
  //will be float,3 even when T is a 3 component field
  template <typename PointsIn, typename CT>
  SVTKM_EXEC void Jacobian(const PointsIn& inputPoints,
                          const svtkm::exec::BoundaryState& boundary,
                          svtkm::Vec<CT, 3>& m_xi,
                          svtkm::Vec<CT, 3>& m_eta,
                          svtkm::Vec<CT, 3>& m_zeta) const
  {
    using CoordType = typename PointsIn::ValueType;

    CoordType xi = inputPoints.Get(1, 0, 0) - inputPoints.Get(-1, 0, 0);
    CoordType eta = inputPoints.Get(0, 1, 0) - inputPoints.Get(0, -1, 0);
    CoordType zeta = inputPoints.Get(0, 0, 1) - inputPoints.Get(0, 0, -1);

    xi = (boundary.IsRadiusInXBoundary(1) ? xi * 0.5f : xi);
    eta = (boundary.IsRadiusInYBoundary(1) ? eta * 0.5f : eta);
    zeta = (boundary.IsRadiusInZBoundary(1) ? zeta * 0.5f : zeta);

    CT aj = xi[0] * eta[1] * zeta[2] + xi[1] * eta[2] * zeta[0] + xi[2] * eta[0] * zeta[1] -
      xi[2] * eta[1] * zeta[0] - xi[1] * eta[0] * zeta[2] - xi[0] * eta[2] * zeta[1];

    aj = (aj != 0.0) ? 1.f / aj : aj;

    //  Xi metrics.
    m_xi[0] = aj * (eta[1] * zeta[2] - eta[2] * zeta[1]);
    m_xi[1] = -aj * (eta[0] * zeta[2] - eta[2] * zeta[0]);
    m_xi[2] = aj * (eta[0] * zeta[1] - eta[1] * zeta[0]);

    //  Eta metrics.
    m_eta[0] = -aj * (xi[1] * zeta[2] - xi[2] * zeta[1]);
    m_eta[1] = aj * (xi[0] * zeta[2] - xi[2] * zeta[0]);
    m_eta[2] = -aj * (xi[0] * zeta[1] - xi[1] * zeta[0]);

    //  Zeta metrics.
    m_zeta[0] = aj * (xi[1] * eta[2] - xi[2] * eta[1]);
    m_zeta[1] = -aj * (xi[0] * eta[2] - xi[2] * eta[0]);
    m_zeta[2] = aj * (xi[0] * eta[1] - xi[1] * eta[0]);
  }
};
}
}
}

#endif
