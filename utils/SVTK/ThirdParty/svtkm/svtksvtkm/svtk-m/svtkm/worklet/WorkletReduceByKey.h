//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_worklet_WorkletReduceByKey_h
#define svtk_m_worklet_WorkletReduceByKey_h

#include <svtkm/worklet/internal/WorkletBase.h>

#include <svtkm/cont/arg/TransportTagArrayIn.h>
#include <svtkm/cont/arg/TransportTagArrayInOut.h>
#include <svtkm/cont/arg/TransportTagArrayOut.h>
#include <svtkm/cont/arg/TransportTagKeyedValuesIn.h>
#include <svtkm/cont/arg/TransportTagKeyedValuesInOut.h>
#include <svtkm/cont/arg/TransportTagKeyedValuesOut.h>
#include <svtkm/cont/arg/TransportTagKeysIn.h>
#include <svtkm/cont/arg/TypeCheckTagArray.h>
#include <svtkm/cont/arg/TypeCheckTagKeys.h>

#include <svtkm/exec/internal/ReduceByKeyLookup.h>

#include <svtkm/exec/arg/FetchTagArrayDirectIn.h>
#include <svtkm/exec/arg/FetchTagArrayDirectInOut.h>
#include <svtkm/exec/arg/FetchTagArrayDirectOut.h>
#include <svtkm/exec/arg/FetchTagKeysIn.h>
#include <svtkm/exec/arg/ThreadIndicesReduceByKey.h>
#include <svtkm/exec/arg/ValueCount.h>

namespace svtkm
{
namespace worklet
{

template <typename WorkletType>
class DispatcherReduceByKey;

class WorkletReduceByKey : public svtkm::worklet::internal::WorkletBase
{
public:
  template <typename Worklet>
  using Dispatcher = svtkm::worklet::DispatcherReduceByKey<Worklet>;
  /// \brief A control signature tag for input keys.
  ///
  /// A \c WorkletReduceByKey operates by collecting all identical keys and
  /// then executing the worklet on each unique key. This tag specifies a
  /// \c Keys object that defines and manages these keys.
  ///
  /// A \c WorkletReduceByKey should have exactly one \c KeysIn tag in its \c
  /// ControlSignature, and the \c InputDomain should point to it.
  ///
  struct KeysIn : svtkm::cont::arg::ControlSignatureTagBase
  {
    using TypeCheckTag = svtkm::cont::arg::TypeCheckTagKeys;
    using TransportTag = svtkm::cont::arg::TransportTagKeysIn;
    using FetchTag = svtkm::exec::arg::FetchTagKeysIn;
  };

  /// \brief A control signature tag for input values.
  ///
  /// A \c WorkletReduceByKey operates by collecting all values associated with
  /// identical keys and then giving the worklet a Vec-like object containing
  /// all values with a matching key. This tag specifies an \c ArrayHandle
  /// object that holds the values.
  ///
  struct ValuesIn : svtkm::cont::arg::ControlSignatureTagBase
  {
    using TypeCheckTag = svtkm::cont::arg::TypeCheckTagArray;
    using TransportTag = svtkm::cont::arg::TransportTagKeyedValuesIn;
    using FetchTag = svtkm::exec::arg::FetchTagArrayDirectIn;
  };

  /// \brief A control signature tag for input/output values.
  ///
  /// A \c WorkletReduceByKey operates by collecting all values associated with
  /// identical keys and then giving the worklet a Vec-like object containing
  /// all values with a matching key. This tag specifies an \c ArrayHandle
  /// object that holds the values.
  ///
  /// This tag might not work with scatter operations.
  ///
  struct ValuesInOut : svtkm::cont::arg::ControlSignatureTagBase
  {
    using TypeCheckTag = svtkm::cont::arg::TypeCheckTagArray;
    using TransportTag = svtkm::cont::arg::TransportTagKeyedValuesInOut;
    using FetchTag = svtkm::exec::arg::FetchTagArrayDirectIn;
  };

  /// \brief A control signature tag for output values.
  ///
  /// A \c WorkletReduceByKey operates by collecting all values associated with
  /// identical keys and then giving the worklet a Vec-like object containing
  /// all values with a matching key. This tag specifies an \c ArrayHandle
  /// object that holds the values.
  ///
  /// This tag might not work with scatter operations.
  ///
  struct ValuesOut : svtkm::cont::arg::ControlSignatureTagBase
  {
    using TypeCheckTag = svtkm::cont::arg::TypeCheckTagArray;
    using TransportTag = svtkm::cont::arg::TransportTagKeyedValuesOut;
    using FetchTag = svtkm::exec::arg::FetchTagArrayDirectIn;
  };

  /// \brief A control signature tag for reduced output values.
  ///
  /// A \c WorkletReduceByKey operates by collecting all identical keys and
  /// calling one instance of the worklet for those identical keys. The worklet
  /// then produces a "reduced" value per key.
  ///
  /// This tag specifies an \c ArrayHandle object that holds the values. It is
  /// an input array with entries for each reduced value. This could be useful
  /// to access values from a previous run of WorkletReduceByKey.
  ///
  struct ReducedValuesIn : svtkm::cont::arg::ControlSignatureTagBase
  {
    using TypeCheckTag = svtkm::cont::arg::TypeCheckTagArray;
    using TransportTag = svtkm::cont::arg::TransportTagArrayIn;
    using FetchTag = svtkm::exec::arg::FetchTagArrayDirectIn;
  };

  /// \brief A control signature tag for reduced output values.
  ///
  /// A \c WorkletReduceByKey operates by collecting all identical keys and
  /// calling one instance of the worklet for those identical keys. The worklet
  /// then produces a "reduced" value per key.
  ///
  /// This tag specifies an \c ArrayHandle object that holds the values. It is
  /// an input/output array with entries for each reduced value. This could be
  /// useful to access values from a previous run of WorkletReduceByKey.
  ///
  struct ReducedValuesInOut : svtkm::cont::arg::ControlSignatureTagBase
  {
    using TypeCheckTag = svtkm::cont::arg::TypeCheckTagArray;
    using TransportTag = svtkm::cont::arg::TransportTagArrayInOut;
    using FetchTag = svtkm::exec::arg::FetchTagArrayDirectInOut;
  };

  /// \brief A control signature tag for reduced output values.
  ///
  /// A \c WorkletReduceByKey operates by collecting all identical keys and
  /// calling one instance of the worklet for those identical keys. The worklet
  /// then produces a "reduced" value per key. This tag specifies an \c
  /// ArrayHandle object that holds the values.
  ///
  struct ReducedValuesOut : svtkm::cont::arg::ControlSignatureTagBase
  {
    using TypeCheckTag = svtkm::cont::arg::TypeCheckTagArray;
    using TransportTag = svtkm::cont::arg::TransportTagArrayOut;
    using FetchTag = svtkm::exec::arg::FetchTagArrayDirectOut;
  };

  /// \brief The \c ExecutionSignature tag to get the number of values.
  ///
  /// A \c WorkletReduceByKey operates by collecting all values associated with
  /// identical keys and then giving the worklet a Vec-like object containing all
  /// values with a matching key. This \c ExecutionSignature tag provides the
  /// number of values associated with the key and given in the Vec-like objects.
  ///
  struct ValueCount : svtkm::exec::arg::ValueCount
  {
  };

  /// Reduce by key worklets use the related thread indices class.
  ///
  SVTKM_SUPPRESS_EXEC_WARNINGS
  template <typename OutToInArrayType,
            typename VisitArrayType,
            typename ThreadToOutArrayType,
            typename InputDomainType>
  SVTKM_EXEC svtkm::exec::arg::ThreadIndicesReduceByKey GetThreadIndices(
    svtkm::Id threadIndex,
    const OutToInArrayType& outToIn,
    const VisitArrayType& visit,
    const ThreadToOutArrayType& threadToOut,
    const InputDomainType& inputDomain,
    svtkm::Id globalThreadIndexOffset = 0) const
  {
    const svtkm::Id outIndex = threadToOut.Get(threadIndex);
    return svtkm::exec::arg::ThreadIndicesReduceByKey(threadIndex,
                                                     outToIn.Get(outIndex),
                                                     visit.Get(outIndex),
                                                     outIndex,
                                                     inputDomain,
                                                     globalThreadIndexOffset);
  }
};
}
} // namespace svtkm::worklet

#endif //svtk_m_worklet_WorkletReduceByKey_h
