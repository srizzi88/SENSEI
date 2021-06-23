//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_worklet_WorkletMapField_h
#define svtk_m_worklet_WorkletMapField_h

#include <svtkm/worklet/internal/WorkletBase.h>

#include <svtkm/cont/arg/ControlSignatureTagBase.h>
#include <svtkm/cont/arg/TransportTagArrayIn.h>
#include <svtkm/cont/arg/TransportTagArrayInOut.h>
#include <svtkm/cont/arg/TransportTagArrayOut.h>
#include <svtkm/cont/arg/TypeCheckTagArray.h>

#include <svtkm/exec/arg/FetchTagArrayDirectIn.h>
#include <svtkm/exec/arg/FetchTagArrayDirectInOut.h>
#include <svtkm/exec/arg/FetchTagArrayDirectOut.h>

namespace svtkm
{
namespace worklet
{

template <typename WorkletType>
class DispatcherMapField;

/// Base class for worklets that do a simple mapping of field arrays. All
/// inputs and outputs are on the same domain. That is, all the arrays are the
/// same size.
///
class WorkletMapField : public svtkm::worklet::internal::WorkletBase
{
public:
  template <typename Worklet>
  using Dispatcher = svtkm::worklet::DispatcherMapField<Worklet>;


  /// \brief A control signature tag for input fields.
  ///
  /// This tag means that the field is read only.
  ///
  struct FieldIn : svtkm::cont::arg::ControlSignatureTagBase
  {
    using TypeCheckTag = svtkm::cont::arg::TypeCheckTagArray;
    using TransportTag = svtkm::cont::arg::TransportTagArrayIn;
    using FetchTag = svtkm::exec::arg::FetchTagArrayDirectIn;
  };

  /// \brief A control signature tag for output fields.
  ///
  /// This tag means that the field is write only.
  ///
  struct FieldOut : svtkm::cont::arg::ControlSignatureTagBase
  {
    using TypeCheckTag = svtkm::cont::arg::TypeCheckTagArray;
    using TransportTag = svtkm::cont::arg::TransportTagArrayOut;
    using FetchTag = svtkm::exec::arg::FetchTagArrayDirectOut;
  };

  /// \brief A control signature tag for input-output (in-place) fields.
  ///
  /// This tag means that the field is read and write.
  ///
  struct FieldInOut : svtkm::cont::arg::ControlSignatureTagBase
  {
    using TypeCheckTag = svtkm::cont::arg::TypeCheckTagArray;
    using TransportTag = svtkm::cont::arg::TransportTagArrayInOut;
    using FetchTag = svtkm::exec::arg::FetchTagArrayDirectInOut;
  };
};
}
} // namespace svtkm::worklet

#endif //svtk_m_worklet_WorkletMapField_h
