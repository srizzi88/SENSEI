//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_worklet_internal_WorkletBase_h
#define svtk_m_worklet_internal_WorkletBase_h

#include <svtkm/TopologyElementTag.h>

#include <svtkm/exec/FunctorBase.h>
#include <svtkm/exec/arg/BasicArg.h>
#include <svtkm/exec/arg/FetchTagExecObject.h>
#include <svtkm/exec/arg/FetchTagWholeCellSetIn.h>
#include <svtkm/exec/arg/InputIndex.h>
#include <svtkm/exec/arg/OutputIndex.h>
#include <svtkm/exec/arg/ThreadIndices.h>
#include <svtkm/exec/arg/ThreadIndicesBasic.h>
#include <svtkm/exec/arg/VisitIndex.h>
#include <svtkm/exec/arg/WorkIndex.h>

#include <svtkm/cont/arg/ControlSignatureTagBase.h>
#include <svtkm/cont/arg/TransportTagAtomicArray.h>
#include <svtkm/cont/arg/TransportTagBitField.h>
#include <svtkm/cont/arg/TransportTagCellSetIn.h>
#include <svtkm/cont/arg/TransportTagExecObject.h>
#include <svtkm/cont/arg/TransportTagWholeArrayIn.h>
#include <svtkm/cont/arg/TransportTagWholeArrayInOut.h>
#include <svtkm/cont/arg/TransportTagWholeArrayOut.h>
#include <svtkm/cont/arg/TypeCheckTagArray.h>
#include <svtkm/cont/arg/TypeCheckTagAtomicArray.h>
#include <svtkm/cont/arg/TypeCheckTagBitField.h>
#include <svtkm/cont/arg/TypeCheckTagCellSet.h>
#include <svtkm/cont/arg/TypeCheckTagExecObject.h>

#include <svtkm/worklet/MaskNone.h>
#include <svtkm/worklet/ScatterIdentity.h>
#include <svtkm/worklet/internal/Placeholders.h>

namespace svtkm
{
namespace worklet
{
namespace internal
{

/// Base class for all worklet classes. Worklet classes are subclasses and a
/// operator() const is added to implement an algorithm in SVTK-m. Different
/// worklets have different calling semantics.
///
class SVTKM_ALWAYS_EXPORT WorkletBase : public svtkm::exec::FunctorBase
{
public:
  using _1 = svtkm::placeholders::Arg<1>;
  using _2 = svtkm::placeholders::Arg<2>;
  using _3 = svtkm::placeholders::Arg<3>;
  using _4 = svtkm::placeholders::Arg<4>;
  using _5 = svtkm::placeholders::Arg<5>;
  using _6 = svtkm::placeholders::Arg<6>;
  using _7 = svtkm::placeholders::Arg<7>;
  using _8 = svtkm::placeholders::Arg<8>;
  using _9 = svtkm::placeholders::Arg<9>;
  using _10 = svtkm::placeholders::Arg<10>;
  using _11 = svtkm::placeholders::Arg<11>;
  using _12 = svtkm::placeholders::Arg<12>;
  using _13 = svtkm::placeholders::Arg<13>;
  using _14 = svtkm::placeholders::Arg<14>;
  using _15 = svtkm::placeholders::Arg<15>;
  using _16 = svtkm::placeholders::Arg<16>;
  using _17 = svtkm::placeholders::Arg<17>;
  using _18 = svtkm::placeholders::Arg<18>;
  using _19 = svtkm::placeholders::Arg<19>;
  using _20 = svtkm::placeholders::Arg<20>;

  /// \c ExecutionSignature tag for getting the work index.
  ///
  using WorkIndex = svtkm::exec::arg::WorkIndex;

  /// \c ExecutionSignature tag for getting the input index.
  ///
  using InputIndex = svtkm::exec::arg::InputIndex;

  /// \c ExecutionSignature tag for getting the output index.
  ///
  using OutputIndex = svtkm::exec::arg::OutputIndex;

  /// \c ExecutionSignature tag for getting the thread indices.
  ///
  using ThreadIndices = svtkm::exec::arg::ThreadIndices;

  /// \c ExecutionSignature tag for getting the visit index.
  ///
  using VisitIndex = svtkm::exec::arg::VisitIndex;

  /// \c ExecutionSignature tag for getting the device adapter tag.
  ///
  struct Device : svtkm::exec::arg::ExecutionSignatureTagBase
  {
    // INDEX 0 (which is an invalid parameter index) is reserved to mean the device adapter tag.
    static constexpr svtkm::IdComponent INDEX = 0;
    using AspectTag = svtkm::exec::arg::AspectTagDefault;
  };

  /// \c ControlSignature tag for execution object inputs.
  struct ExecObject : svtkm::cont::arg::ControlSignatureTagBase
  {
    using TypeCheckTag = svtkm::cont::arg::TypeCheckTagExecObject;
    using TransportTag = svtkm::cont::arg::TransportTagExecObject;
    using FetchTag = svtkm::exec::arg::FetchTagExecObject;
  };

  /// Default input domain is the first argument. Worklet subclasses can
  /// override this by redefining this type.
  using InputDomain = _1;

  /// All worklets must define their scatter operation. The scatter defines
  /// what output each input contributes to. The default scatter is the
  /// identity scatter (1-to-1 input to output).
  using ScatterType = svtkm::worklet::ScatterIdentity;

  /// All worklets must define their mask operation. The mask defines which
  /// outputs are generated. The default mask is the none mask, which generates
  /// everything in the output domain.
  using MaskType = svtkm::worklet::MaskNone;

  /// \c ControlSignature tag for whole input arrays.
  ///
  /// The \c WholeArrayIn control signature tag specifies an \c ArrayHandle
  /// passed to the \c Invoke operation of the dispatcher. This is converted
  /// to an \c ArrayPortal object and passed to the appropriate worklet
  /// operator argument with one of the default args.
  ///
  /// The template operator specifies all the potential value types of the
  /// array. The default value type is all types.
  ///
  struct WholeArrayIn : svtkm::cont::arg::ControlSignatureTagBase
  {
    using TypeCheckTag = svtkm::cont::arg::TypeCheckTagArray;
    using TransportTag = svtkm::cont::arg::TransportTagWholeArrayIn;
    using FetchTag = svtkm::exec::arg::FetchTagExecObject;
  };

  /// \c ControlSignature tag for whole output arrays.
  ///
  /// The \c WholeArrayOut control signature tag specifies an \c ArrayHandle
  /// passed to the \c Invoke operation of the dispatcher. This is converted to
  /// an \c ArrayPortal object and passed to the appropriate worklet operator
  /// argument with one of the default args. Care should be taken to not write
  /// a value in one instance that will be overridden by another entry.
  ///
  /// The template operator specifies all the potential value types of the
  /// array. The default value type is all types.
  ///
  struct WholeArrayOut : svtkm::cont::arg::ControlSignatureTagBase
  {
    using TypeCheckTag = svtkm::cont::arg::TypeCheckTagArray;
    using TransportTag = svtkm::cont::arg::TransportTagWholeArrayOut;
    using FetchTag = svtkm::exec::arg::FetchTagExecObject;
  };

  /// \c ControlSignature tag for whole input/output arrays.
  ///
  /// The \c WholeArrayOut control signature tag specifies an \c ArrayHandle
  /// passed to the \c Invoke operation of the dispatcher. This is converted to
  /// an \c ArrayPortal object and passed to the appropriate worklet operator
  /// argument with one of the default args. Care should be taken to not write
  /// a value in one instance that will be read by or overridden by another
  /// entry.
  ///
  /// The template operator specifies all the potential value types of the
  /// array. The default value type is all types.
  ///
  struct WholeArrayInOut : svtkm::cont::arg::ControlSignatureTagBase
  {
    using TypeCheckTag = svtkm::cont::arg::TypeCheckTagArray;
    using TransportTag = svtkm::cont::arg::TransportTagWholeArrayInOut;
    using FetchTag = svtkm::exec::arg::FetchTagExecObject;
  };

  /// \c ControlSignature tag for whole input/output arrays.
  ///
  /// The \c AtomicArrayInOut control signature tag specifies an \c ArrayHandle
  /// passed to the \c Invoke operation of the dispatcher. This is converted to
  /// a \c svtkm::exec::AtomicArray object and passed to the appropriate worklet
  /// operator argument with one of the default args. The provided atomic
  /// operations can be used to resolve concurrency hazards, but have the
  /// potential to slow the program quite a bit.
  ///
  /// The template operator specifies all the potential value types of the
  /// array. The default value type is all types.
  ///
  struct AtomicArrayInOut : svtkm::cont::arg::ControlSignatureTagBase
  {
    using TypeCheckTag = svtkm::cont::arg::TypeCheckTagAtomicArray;
    using TransportTag = svtkm::cont::arg::TransportTagAtomicArray;
    using FetchTag = svtkm::exec::arg::FetchTagExecObject;
  };

  /// \c ControlSignature tags for whole BitFields.
  ///
  /// When a BitField is passed in to a worklet expecting this ControlSignature
  /// type, the appropriate BitPortal is generated and given to the worklet's
  /// execution.
  ///
  /// Be aware that this data structure is especially prone to race conditions,
  /// so be sure to use the appropriate atomic methods when necessary.
  /// @{
  ///
  struct BitFieldIn : svtkm::cont::arg::ControlSignatureTagBase
  {
    using TypeCheckTag = svtkm::cont::arg::TypeCheckTagBitField;
    using TransportTag = svtkm::cont::arg::TransportTagBitFieldIn;
    using FetchTag = svtkm::exec::arg::FetchTagExecObject;
  };
  struct BitFieldOut : svtkm::cont::arg::ControlSignatureTagBase
  {
    using TypeCheckTag = svtkm::cont::arg::TypeCheckTagBitField;
    using TransportTag = svtkm::cont::arg::TransportTagBitFieldOut;
    using FetchTag = svtkm::exec::arg::FetchTagExecObject;
  };
  struct BitFieldInOut : svtkm::cont::arg::ControlSignatureTagBase
  {
    using TypeCheckTag = svtkm::cont::arg::TypeCheckTagBitField;
    using TransportTag = svtkm::cont::arg::TransportTagBitFieldInOut;
    using FetchTag = svtkm::exec::arg::FetchTagExecObject;
  };
  /// @}

  /// \c ControlSignature tag for whole input topology.
  ///
  /// The \c WholeCellSetIn control signature tag specifies an \c CellSet
  /// passed to the \c Invoke operation of the dispatcher. This is converted to
  /// a \c svtkm::exec::Connectivity* object and passed to the appropriate worklet
  /// operator argument with one of the default args. This can be used to
  /// global lookup for arbitrary topology information

  using Point = svtkm::TopologyElementTagPoint;
  using Cell = svtkm::TopologyElementTagCell;
  using Edge = svtkm::TopologyElementTagEdge;
  using Face = svtkm::TopologyElementTagFace;
  template <typename VisitTopology = Cell, typename IncidentTopology = Point>
  struct WholeCellSetIn : svtkm::cont::arg::ControlSignatureTagBase
  {
    using TypeCheckTag = svtkm::cont::arg::TypeCheckTagCellSet;
    using TransportTag = svtkm::cont::arg::TransportTagCellSetIn<VisitTopology, IncidentTopology>;
    using FetchTag = svtkm::exec::arg::FetchTagWholeCellSetIn;
  };

  /// \brief Creates a \c ThreadIndices object.
  ///
  /// Worklet types can add additional indices by returning different object
  /// types.
  ///
  SVTKM_SUPPRESS_EXEC_WARNINGS
  template <typename T,
            typename OutToInArrayType,
            typename VisitArrayType,
            typename ThreadToOutArrayType,
            typename InputDomainType>
  SVTKM_EXEC svtkm::exec::arg::ThreadIndicesBasic GetThreadIndices(
    const T& threadIndex,
    const OutToInArrayType& outToIn,
    const VisitArrayType& visit,
    const ThreadToOutArrayType& threadToOut,
    const InputDomainType&,
    const T& globalThreadIndexOffset = 0) const
  {
    svtkm::Id outIndex = threadToOut.Get(threadIndex);
    return svtkm::exec::arg::ThreadIndicesBasic(
      threadIndex, outToIn.Get(outIndex), visit.Get(outIndex), outIndex, globalThreadIndexOffset);
  }
};
}
}
} // namespace svtkm::worklet::internal

#endif //svtk_m_worklet_internal_WorkletBase_h
