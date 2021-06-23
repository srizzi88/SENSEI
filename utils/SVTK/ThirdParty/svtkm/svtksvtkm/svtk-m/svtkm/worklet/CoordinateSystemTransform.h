//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef svtk_m_worklet_CoordinateSystemTransform_h
#define svtk_m_worklet_CoordinateSystemTransform_h

#include <svtkm/Math.h>
#include <svtkm/Matrix.h>
#include <svtkm/Transform3D.h>
#include <svtkm/VectorAnalysis.h>
#include <svtkm/worklet/DispatcherMapField.h>
#include <svtkm/worklet/WorkletMapField.h>

namespace svtkm
{
namespace worklet
{
namespace detail
{
template <typename T>
struct CylToCar : public svtkm::worklet::WorkletMapField
{
  using ControlSignature = void(FieldIn, FieldOut);
  using ExecutionSignature = _2(_1);

  //Functor
  SVTKM_EXEC svtkm::Vec<T, 3> operator()(const svtkm::Vec<T, 3>& vec) const
  {
    svtkm::Vec<T, 3> res(vec[0] * static_cast<T>(svtkm::Cos(vec[1])),
                        vec[0] * static_cast<T>(svtkm::Sin(vec[1])),
                        vec[2]);
    return res;
  }
};

template <typename T>
struct CarToCyl : public svtkm::worklet::WorkletMapField
{
  using ControlSignature = void(FieldIn, FieldOut);
  using ExecutionSignature = _2(_1);

  //Functor
  SVTKM_EXEC svtkm::Vec<T, 3> operator()(const svtkm::Vec<T, 3>& vec) const
  {
    T R = svtkm::Sqrt(vec[0] * vec[0] + vec[1] * vec[1]);
    T Theta = 0;

    if (vec[0] == 0 && vec[1] == 0)
      Theta = 0;
    else if (vec[0] < 0)
      Theta = -svtkm::ASin(vec[1] / R) + static_cast<T>(svtkm::Pi());
    else
      Theta = svtkm::ASin(vec[1] / R);

    svtkm::Vec<T, 3> res(R, Theta, vec[2]);
    return res;
  }
};

template <typename T>
struct SphereToCar : public svtkm::worklet::WorkletMapField
{
  using ControlSignature = void(FieldIn, FieldOut);
  using ExecutionSignature = _2(_1);

  //Functor
  SVTKM_EXEC svtkm::Vec<T, 3> operator()(const svtkm::Vec<T, 3>& vec) const
  {
    T R = vec[0];
    T Theta = vec[1];
    T Phi = vec[2];

    T sinTheta = static_cast<T>(svtkm::Sin(Theta));
    T cosTheta = static_cast<T>(svtkm::Cos(Theta));
    T sinPhi = static_cast<T>(svtkm::Sin(Phi));
    T cosPhi = static_cast<T>(svtkm::Cos(Phi));

    T x = R * sinTheta * cosPhi;
    T y = R * sinTheta * sinPhi;
    T z = R * cosTheta;

    svtkm::Vec<T, 3> r(x, y, z);
    return r;
  }
};

template <typename T>
struct CarToSphere : public svtkm::worklet::WorkletMapField
{
  using ControlSignature = void(FieldIn, FieldOut);
  using ExecutionSignature = _2(_1);

  //Functor
  SVTKM_EXEC svtkm::Vec<T, 3> operator()(const svtkm::Vec<T, 3>& vec) const
  {
    T R = svtkm::Sqrt(svtkm::Dot(vec, vec));
    T Theta = 0;
    if (R > 0)
      Theta = svtkm::ACos(vec[2] / R);
    T Phi = svtkm::ATan2(vec[1], vec[0]);
    if (Phi < 0)
      Phi += static_cast<T>(svtkm::TwoPi());

    return svtkm::Vec<T, 3>(R, Theta, Phi);
  }
};
};

class CylindricalCoordinateTransform
{
public:
  SVTKM_CONT
  CylindricalCoordinateTransform()
    : cartesianToCylindrical(true)
  {
  }

  SVTKM_CONT void SetCartesianToCylindrical() { cartesianToCylindrical = true; }
  SVTKM_CONT void SetCylindricalToCartesian() { cartesianToCylindrical = false; }

  template <typename T, typename InStorageType, typename OutStorageType>
  void Run(const svtkm::cont::ArrayHandle<svtkm::Vec<T, 3>, InStorageType>& inPoints,
           svtkm::cont::ArrayHandle<svtkm::Vec<T, 3>, OutStorageType>& outPoints) const
  {
    if (cartesianToCylindrical)
    {
      svtkm::worklet::DispatcherMapField<detail::CarToCyl<T>> dispatcher;
      dispatcher.Invoke(inPoints, outPoints);
    }
    else
    {
      svtkm::worklet::DispatcherMapField<detail::CylToCar<T>> dispatcher;
      dispatcher.Invoke(inPoints, outPoints);
    }
  }

  template <typename T, typename CoordsStorageType>
  void Run(const svtkm::cont::CoordinateSystem& inPoints,
           svtkm::cont::ArrayHandle<svtkm::Vec<T, 3>, CoordsStorageType>& outPoints) const
  {
    if (cartesianToCylindrical)
    {
      svtkm::worklet::DispatcherMapField<detail::CarToCyl<T>> dispatcher;
      dispatcher.Invoke(inPoints, outPoints);
    }
    else
    {
      svtkm::worklet::DispatcherMapField<detail::CylToCar<T>> dispatcher;
      dispatcher.Invoke(inPoints, outPoints);
    }
  }

private:
  bool cartesianToCylindrical;
};

class SphericalCoordinateTransform
{
public:
  SVTKM_CONT
  SphericalCoordinateTransform()
    : CartesianToSpherical(true)
  {
  }

  SVTKM_CONT void SetCartesianToSpherical() { CartesianToSpherical = true; }
  SVTKM_CONT void SetSphericalToCartesian() { CartesianToSpherical = false; }

  template <typename T, typename InStorageType, typename OutStorageType>
  void Run(const svtkm::cont::ArrayHandle<svtkm::Vec<T, 3>, InStorageType>& inPoints,
           svtkm::cont::ArrayHandle<svtkm::Vec<T, 3>, OutStorageType>& outPoints) const
  {
    if (CartesianToSpherical)
    {
      svtkm::worklet::DispatcherMapField<detail::CarToSphere<T>> dispatcher;
      dispatcher.Invoke(inPoints, outPoints);
    }
    else
    {
      svtkm::worklet::DispatcherMapField<detail::SphereToCar<T>> dispatcher;
      dispatcher.Invoke(inPoints, outPoints);
    }
  }

  template <typename T, typename CoordsStorageType>
  void Run(const svtkm::cont::CoordinateSystem& inPoints,
           svtkm::cont::ArrayHandle<svtkm::Vec<T, 3>, CoordsStorageType>& outPoints) const
  {
    if (CartesianToSpherical)
    {
      svtkm::worklet::DispatcherMapField<detail::CarToSphere<T>> dispatcher;
      dispatcher.Invoke(inPoints, outPoints);
    }
    else
    {
      svtkm::worklet::DispatcherMapField<detail::SphereToCar<T>> dispatcher;
      dispatcher.Invoke(inPoints, outPoints);
    }
  }

private:
  bool CartesianToSpherical;
};
}
} // namespace svtkm::worklet

#endif // svtk_m_worklet_CoordinateSystemTransform_h
