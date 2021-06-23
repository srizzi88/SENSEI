//============================================================================
//
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//=======================================================================
#ifndef svtk_m_worklet_moments_ComputeMoments_h
#define svtk_m_worklet_moments_ComputeMoments_h

#include <svtkm/Math.h>
#include <svtkm/worklet/WorkletPointNeighborhood.h>

#include <svtkm/cont/Field.h>

#include <svtkm/exec/BoundaryState.h>

#include <cassert>
#include <string>

namespace svtkm
{
namespace worklet
{
namespace moments
{

struct ComputeMoments2D : public svtkm::worklet::WorkletPointNeighborhood
{
public:
  ComputeMoments2D(const svtkm::Vec3f& _spacing, svtkm::FloatDefault _radius, int _p, int _q)
    : Radius(_radius)
    , RadiusDiscrete(svtkm::IdComponent(_radius / (_spacing[0] - 1e-10)),
                     svtkm::IdComponent(_radius / (_spacing[1] - 1e-10)),
                     svtkm::IdComponent(_radius / (_spacing[2] - 1e-10)))
    , SpacingProduct(_spacing[0] * _spacing[1])
    , p(_p)
    , q(_q)
  {
    assert(_spacing[0] > 1e-10);
    assert(_spacing[1] > 1e-10);
    assert(_spacing[2] > 1e-10);

    assert(_p >= 0);
    assert(_q >= 0);
  }

  using ControlSignature = void(CellSetIn, FieldInNeighborhood, FieldOut);

  using ExecutionSignature = void(_2, Boundary, _3);

  template <typename NeighIn, typename T>
  SVTKM_EXEC void operator()(const NeighIn& image,
                            const svtkm::exec::BoundaryState& boundary,
                            T& moment) const
  {
    // TODO: type safety and numerical precision
    auto sum = svtkm::TypeTraits<T>::ZeroInitialization();

    // Clamp the radius to the dataset bounds (discard out-of-bounds points).
    const auto minRadius = boundary.ClampNeighborIndex(-this->RadiusDiscrete);
    const auto maxRadius = boundary.ClampNeighborIndex(this->RadiusDiscrete);

    svtkm::Vec2f_64 radius;
    for (svtkm::IdComponent j = minRadius[1]; j <= maxRadius[1]; ++j)
    {
      if (j > -this->RadiusDiscrete[1] && boundary.IJK[1] + j == 0)
      { // Don't double count samples that exist on other nodes:
        continue;
      }
      radius[1] = j * 1. / this->RadiusDiscrete[1];

      for (svtkm::IdComponent i = minRadius[0]; i <= maxRadius[0]; ++i)
      {
        if (i > -this->RadiusDiscrete[0] && boundary.IJK[0] + i == 0)
        { // Don't double count samples that exist on other nodes:
          continue;
        }
        radius[0] = i * 1. / this->RadiusDiscrete[0];

        if (svtkm::Dot(radius, radius) <= 1)
        {
          sum += pow(radius[0], p) * pow(radius[1], q) * image.Get(i, j, 0);
        }
      }
    }

    moment = T(sum * this->SpacingProduct);
  }

private:
  const svtkm::FloatDefault Radius;
  svtkm::Vec3i_32 RadiusDiscrete;
  const svtkm::FloatDefault SpacingProduct;
  const int p;
  const int q;
};

struct ComputeMoments3D : public svtkm::worklet::WorkletPointNeighborhood
{
public:
  ComputeMoments3D(const svtkm::Vec3f& _spacing, svtkm::FloatDefault _radius, int _p, int _q, int _r)
    : Radius(_radius)
    , RadiusDiscrete(svtkm::IdComponent(_radius / (_spacing[0] - 1e-10)),
                     svtkm::IdComponent(_radius / (_spacing[1] - 1e-10)),
                     svtkm::IdComponent(_radius / (_spacing[2] - 1e-10)))
    , SpacingProduct(svtkm::ReduceProduct(_spacing))
    , p(_p)
    , q(_q)
    , r(_r)
  {
    assert(_spacing[0] > 1e-10);
    assert(_spacing[1] > 1e-10);
    assert(_spacing[2] > 1e-10);

    assert(_p >= 0);
    assert(_q >= 0);
    assert(_r >= 0);
  }

  using ControlSignature = void(CellSetIn, FieldInNeighborhood, FieldOut);

  using ExecutionSignature = void(_2, Boundary, _3);

  template <typename NeighIn, typename T>
  SVTKM_EXEC void operator()(const NeighIn& image,
                            const svtkm::exec::BoundaryState& boundary,
                            T& moment) const
  {
    // TODO: type safety and numerical precision
    auto sum = svtkm::TypeTraits<T>::ZeroInitialization();

    // Clamp the radius to the dataset bounds (discard out-of-bounds points).
    const auto minRadius = boundary.ClampNeighborIndex(-this->RadiusDiscrete);
    const auto maxRadius = boundary.ClampNeighborIndex(this->RadiusDiscrete);

    svtkm::Vec3f_64 radius;
    for (svtkm::IdComponent k = minRadius[2]; k <= maxRadius[2]; ++k)
    {
      if (k > -this->RadiusDiscrete[2] && boundary.IJK[2] + k == 0)
      { // Don't double count samples that exist on other nodes:
        continue;
      }
      radius[2] = k * 1. / this->RadiusDiscrete[2];

      for (svtkm::IdComponent j = minRadius[1]; j <= maxRadius[1]; ++j)
      {
        if (j > -this->RadiusDiscrete[1] && boundary.IJK[1] + j == 0)
        { // Don't double count samples that exist on other nodes:
          continue;
        }
        radius[1] = j * 1. / this->RadiusDiscrete[1];

        for (svtkm::IdComponent i = minRadius[0]; i <= maxRadius[0]; ++i)
        {
          if (i > -this->RadiusDiscrete[0] && boundary.IJK[0] + i == 0)
          { // Don't double count samples that exist on other nodes:
            continue;
          }
          radius[0] = i * 1. / this->RadiusDiscrete[0];

          if (svtkm::Dot(radius, radius) <= 1)
          {
            sum += pow(radius[0], p) * pow(radius[1], q) * pow(radius[2], r) * image.Get(i, j, k);
          }
        }
      }
    }

    moment = T(sum * this->SpacingProduct);
  }

private:
  const svtkm::FloatDefault Radius;
  svtkm::Vec3i_32 RadiusDiscrete;
  const svtkm::FloatDefault SpacingProduct;
  const int p;
  const int q;
  const int r;
};

class ComputeMoments
{
public:
  ComputeMoments(const svtkm::Vec3f& _spacing, const double _radius)
    : Spacing(_spacing)
    , Radius(_radius)
  {
  }

  class ResolveDynamicCellSet
  {
  public:
    template <typename T, typename S>
    void operator()(const svtkm::cont::CellSetStructured<2>& input,
                    const svtkm::cont::ArrayHandle<T, S>& pixels,
                    svtkm::Vec3f Spacing,
                    svtkm::FloatDefault Radius,
                    int maxOrder,
                    svtkm::cont::DataSet& output) const
    {
      using WorkletType = svtkm::worklet::moments::ComputeMoments2D;
      using DispatcherType = svtkm::worklet::DispatcherPointNeighborhood<WorkletType>;

      for (int order = 0; order <= maxOrder; ++order)
      {
        for (int p = 0; p <= order; ++p)
        {
          const int q = order - p;

          svtkm::cont::ArrayHandle<T> moments;

          DispatcherType dispatcher(WorkletType{ Spacing, Radius, p, q });
          dispatcher.Invoke(input, pixels, moments);

          std::string fieldName = std::string("index") + std::string(p, '0') + std::string(q, '1');

          svtkm::cont::Field momentsField(
            fieldName, svtkm::cont::Field::Association::POINTS, moments);
          output.AddField(momentsField);
        }
      }
    }

    template <typename T, typename S>
    void operator()(const svtkm::cont::CellSetStructured<3>& input,
                    const svtkm::cont::ArrayHandle<T, S>& pixels,
                    svtkm::Vec3f Spacing,
                    svtkm::FloatDefault Radius,
                    int maxOrder,
                    svtkm::cont::DataSet& output) const
    {
      using WorkletType = svtkm::worklet::moments::ComputeMoments3D;
      using DispatcherType = svtkm::worklet::DispatcherPointNeighborhood<WorkletType>;

      for (int order = 0; order <= maxOrder; ++order)
      {
        for (int r = 0; r <= order; ++r)
        {
          const int qMax = order - r;
          for (int q = 0; q <= qMax; ++q)
          {
            const int p = order - r - q;

            svtkm::cont::ArrayHandle<T> moments;

            DispatcherType dispatcher(WorkletType{ Spacing, Radius, p, q, r });
            dispatcher.Invoke(input, pixels, moments);

            std::string fieldName = std::string("index") + std::string(p, '0') +
              std::string(q, '1') + std::string(r, '2');

            svtkm::cont::Field momentsField(
              fieldName, svtkm::cont::Field::Association::POINTS, moments);
            output.AddField(momentsField);
          }
        }
      }
    }
  };

  template <typename T, typename S>
  void Run(const svtkm::cont::DynamicCellSet& input,
           const svtkm::cont::ArrayHandle<T, S>& pixels,
           int maxOrder,
           svtkm::cont::DataSet& output) const
  {
    input.ResetCellSetList(svtkm::cont::CellSetListStructured())
      .CastAndCall(ResolveDynamicCellSet(), pixels, this->Spacing, this->Radius, maxOrder, output);
  }

private:
  const svtkm::FloatDefault Radius = 1;
  const svtkm::Vec3f Spacing = { 1, 1, 1 };
};
}
}
}

#endif // svtk_m_worklet_moments_ComputeMoments_h
