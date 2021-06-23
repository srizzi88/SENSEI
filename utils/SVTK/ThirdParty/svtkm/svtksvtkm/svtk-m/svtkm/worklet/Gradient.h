//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef svtk_m_worklet_Gradient_h
#define svtk_m_worklet_Gradient_h

#include <svtkm/worklet/DispatcherMapTopology.h>
#include <svtkm/worklet/DispatcherPointNeighborhood.h>

#include <svtkm/worklet/gradient/CellGradient.h>
#include <svtkm/worklet/gradient/Divergence.h>
#include <svtkm/worklet/gradient/GradientOutput.h>
#include <svtkm/worklet/gradient/PointGradient.h>
#include <svtkm/worklet/gradient/QCriterion.h>
#include <svtkm/worklet/gradient/StructuredPointGradient.h>
#include <svtkm/worklet/gradient/Transpose.h>
#include <svtkm/worklet/gradient/Vorticity.h>

namespace svtkm
{
namespace worklet
{

template <typename T>
struct GradientOutputFields;

namespace gradient
{

//-----------------------------------------------------------------------------
template <typename CoordinateSystem, typename T, typename S>
struct DeducedPointGrad
{
  DeducedPointGrad(const CoordinateSystem& coords,
                   const svtkm::cont::ArrayHandle<T, S>& field,
                   GradientOutputFields<T>* result)
    : Points(&coords)
    , Field(&field)
    , Result(result)
  {
  }

  template <typename CellSetType>
  void operator()(const CellSetType& cellset) const
  {
    svtkm::worklet::DispatcherMapTopology<PointGradient<T>> dispatcher;
    dispatcher.Invoke(cellset, //topology to iterate on a per point basis
                      cellset, //whole cellset in
                      *this->Points,
                      *this->Field,
                      *this->Result);
  }

  void operator()(const svtkm::cont::CellSetStructured<3>& cellset) const
  {
    svtkm::worklet::DispatcherPointNeighborhood<StructuredPointGradient<T>> dispatcher;
    dispatcher.Invoke(cellset, //topology to iterate on a per point basis
                      *this->Points,
                      *this->Field,
                      *this->Result);
  }

  template <typename PermIterType>
  void operator()(const svtkm::cont::CellSetPermutation<svtkm::cont::CellSetStructured<3>,
                                                       PermIterType>& cellset) const
  {
    svtkm::worklet::DispatcherPointNeighborhood<StructuredPointGradient<T>> dispatcher;
    dispatcher.Invoke(cellset, //topology to iterate on a per point basis
                      *this->Points,
                      *this->Field,
                      *this->Result);
  }

  void operator()(const svtkm::cont::CellSetStructured<2>& cellset) const
  {
    svtkm::worklet::DispatcherPointNeighborhood<StructuredPointGradient<T>> dispatcher;
    dispatcher.Invoke(cellset, //topology to iterate on a per point basis
                      *this->Points,
                      *this->Field,
                      *this->Result);
  }

  template <typename PermIterType>
  void operator()(const svtkm::cont::CellSetPermutation<svtkm::cont::CellSetStructured<2>,
                                                       PermIterType>& cellset) const
  {
    svtkm::worklet::DispatcherPointNeighborhood<StructuredPointGradient<T>> dispatcher;
    dispatcher.Invoke(cellset, //topology to iterate on a per point basis
                      *this->Points,
                      *this->Field,
                      *this->Result);
  }


  const CoordinateSystem* const Points;
  const svtkm::cont::ArrayHandle<T, S>* const Field;
  GradientOutputFields<T>* Result;

private:
  void operator=(const DeducedPointGrad<CoordinateSystem, T, S>&) = delete;
};

} //namespace gradient

template <typename T>
struct GradientOutputFields : public svtkm::cont::ExecutionObjectBase
{

  using ValueType = T;
  using BaseTType = typename svtkm::VecTraits<T>::BaseComponentType;

  template <typename DeviceAdapter>
  struct ExecutionTypes
  {
    using Portal = svtkm::exec::GradientOutput<T>;
  };

  GradientOutputFields()
    : Gradient()
    , Divergence()
    , Vorticity()
    , QCriterion()
    , StoreGradient(true)
    , ComputeDivergence(false)
    , ComputeVorticity(false)
    , ComputeQCriterion(false)
  {
  }

  GradientOutputFields(bool store, bool divergence, bool vorticity, bool qc)
    : Gradient()
    , Divergence()
    , Vorticity()
    , QCriterion()
    , StoreGradient(store)
    , ComputeDivergence(divergence)
    , ComputeVorticity(vorticity)
    , ComputeQCriterion(qc)
  {
  }

  /// Add divergence field to the output data.
  /// The input array must have 3 components in order to compute this.
  /// The default is off.
  void SetComputeDivergence(bool enable) { ComputeDivergence = enable; }
  bool GetComputeDivergence() const { return ComputeDivergence; }

  /// Add voriticity/curl field to the output data.
  /// The input array must have 3 components in order to compute this.
  /// The default is off.
  void SetComputeVorticity(bool enable) { ComputeVorticity = enable; }
  bool GetComputeVorticity() const { return ComputeVorticity; }

  /// Add Q-criterion field to the output data.
  /// The input array must have 3 components in order to compute this.
  /// The default is off.
  void SetComputeQCriterion(bool enable) { ComputeQCriterion = enable; }
  bool GetComputeQCriterion() const { return ComputeQCriterion; }

  /// Add gradient field to the output data.
  /// The input array must have 3 components in order to disable this.
  /// The default is on.
  void SetComputeGradient(bool enable) { StoreGradient = enable; }
  bool GetComputeGradient() const { return StoreGradient; }

  //todo fix this for scalar
  svtkm::exec::GradientOutput<T> PrepareForOutput(svtkm::Id size)
  {
    svtkm::exec::GradientOutput<T> portal(this->StoreGradient,
                                         this->ComputeDivergence,
                                         this->ComputeVorticity,
                                         this->ComputeQCriterion,
                                         this->Gradient,
                                         this->Divergence,
                                         this->Vorticity,
                                         this->QCriterion,
                                         size);
    return portal;
  }

  svtkm::cont::ArrayHandle<svtkm::Vec<T, 3>> Gradient;
  svtkm::cont::ArrayHandle<BaseTType> Divergence;
  svtkm::cont::ArrayHandle<svtkm::Vec<BaseTType, 3>> Vorticity;
  svtkm::cont::ArrayHandle<BaseTType> QCriterion;

private:
  bool StoreGradient;
  bool ComputeDivergence;
  bool ComputeVorticity;
  bool ComputeQCriterion;
};
class PointGradient
{
public:
  template <typename CellSetType, typename CoordinateSystem, typename T, typename S>
  svtkm::cont::ArrayHandle<svtkm::Vec<T, 3>> Run(const CellSetType& cells,
                                               const CoordinateSystem& coords,
                                               const svtkm::cont::ArrayHandle<T, S>& field)
  {
    svtkm::worklet::GradientOutputFields<T> extraOutput(true, false, false, false);
    return this->Run(cells, coords, field, extraOutput);
  }

  template <typename CellSetType, typename CoordinateSystem, typename T, typename S>
  svtkm::cont::ArrayHandle<svtkm::Vec<T, 3>> Run(const CellSetType& cells,
                                               const CoordinateSystem& coords,
                                               const svtkm::cont::ArrayHandle<T, S>& field,
                                               GradientOutputFields<T>& extraOutput)
  {
    //we are using cast and call here as we pass the cells twice to the invoke
    //and want the type resolved once before hand instead of twice
    //by the dispatcher ( that will cost more in time and binary size )
    gradient::DeducedPointGrad<CoordinateSystem, T, S> func(coords, field, &extraOutput);
    svtkm::cont::CastAndCall(cells, func);
    return extraOutput.Gradient;
  }
};

class CellGradient
{
public:
  template <typename CellSetType, typename CoordinateSystem, typename T, typename S>
  svtkm::cont::ArrayHandle<svtkm::Vec<T, 3>> Run(const CellSetType& cells,
                                               const CoordinateSystem& coords,
                                               const svtkm::cont::ArrayHandle<T, S>& field)
  {
    svtkm::worklet::GradientOutputFields<T> extra(true, false, false, false);
    return this->Run(cells, coords, field, extra);
  }

  template <typename CellSetType, typename CoordinateSystem, typename T, typename S>
  svtkm::cont::ArrayHandle<svtkm::Vec<T, 3>> Run(const CellSetType& cells,
                                               const CoordinateSystem& coords,
                                               const svtkm::cont::ArrayHandle<T, S>& field,
                                               GradientOutputFields<T>& extraOutput)
  {
    using DispatcherType =
      svtkm::worklet::DispatcherMapTopology<svtkm::worklet::gradient::CellGradient<T>>;

    svtkm::worklet::gradient::CellGradient<T> worklet;
    DispatcherType dispatcher(worklet);

    dispatcher.Invoke(cells, coords, field, extraOutput);
    return extraOutput.Gradient;
  }
};
}
} // namespace svtkm::worklet
#endif
